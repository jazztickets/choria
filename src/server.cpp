/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2015  Alan Witkowski
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#include <server.h>
#include <network/servernetwork.h>
#include <network/peer.h>
#include <objects/object.h>
#include <objects/inventory.h>
#include <objects/map.h>
#include <objects/battle.h>
#include <scripting.h>
#include <save.h>
#include <manager.h>
#include <database.h>
#include <buffer.h>
#include <packet.h>
#include <stats.h>
#include <constants.h>
#include <config.h>
#include <SDL_timer.h>
#include <algorithm>

// Function to run the server thread
void RunThread(void *Arguments) {

	// Get server object
	_Server *Server = (_Server *)Arguments;

	// Init timer
	Uint64 Timer = SDL_GetPerformanceCounter();
	double TimeStepAccumulator = 0.0;
	double TimeStep = DEFAULT_TIMESTEP;
	while(!Server->Done) {
		double FrameTime = (SDL_GetPerformanceCounter() - Timer) / (double)SDL_GetPerformanceFrequency();
		Timer = SDL_GetPerformanceCounter();

		// Run server
		TimeStepAccumulator += FrameTime;
		while(TimeStepAccumulator >= TimeStep) {
			Server->Update(TimeStep);
			TimeStepAccumulator -= TimeStep;
		}

		// Sleep thread
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

// Constructor
_Server::_Server(_Stats *Stats, uint16_t NetworkPort)
:	Done(false),
	StartShutdown(false),
	TimeSteps(0),
	Time(0.0),
	SaveTime(0.0),
	Clock(0.0),
	Stats(Stats),
	Network(new _ServerNetwork(Config.MaxClients, NetworkPort)),
	Thread(nullptr) {

	Log.Open((Config.ConfigPath + "server.log").c_str());

	if(!Network->HasConnection())
		throw std::runtime_error("Unable to bind address!");

	Network->SetFakeLag(Config.FakeLag);
	Network->SetUpdatePeriod(Config.NetworkRate);

	ObjectManager = new _Manager<_Object>();
	MapManager = new _Manager<_Map>();
	BattleManager = new _Manager<_Battle>();
	Save = new _Save();
	Clock = Save->GetClock();

	Scripting = new _Scripting();
	Scripting->Setup(Stats, SCRIPTS_PATH + SCRIPTS_GAME);

}

// Destructor
_Server::~_Server() {
	Done = true;
	JoinThread();

	Save->SaveClock(Clock);

	delete MapManager;
	delete BattleManager;
	delete ObjectManager;
	delete Scripting;
	delete Save;
	delete Thread;
}

// Start the server thread
void _Server::StartThread() {
	Thread = new std::thread(RunThread, this);
}

// Wait for thread to join
void _Server::JoinThread() {
	if(Thread)
		Thread->join();

	delete Thread;
	Thread = nullptr;
}

// Stop the server
void _Server::StopServer() {
	Network->DisconnectAll();
	StartShutdown = true;
}

// Update
void _Server::Update(double FrameTime) {
	//if(std::abs(std::fmod(Time, 1.0)) >= 0.99)
	//	std::cout << "Server: O=" << ObjectManager->Objects.size() << " B=" << BattleManager->Objects.size() << std::endl;

	// Update network
	Network->Update(FrameTime);

	// Get events
	_NetworkEvent NetworkEvent;
	while(Network->GetNetworkEvent(NetworkEvent)) {

		switch(NetworkEvent.Type) {
			case _NetworkEvent::CONNECT:
				HandleConnect(NetworkEvent);
			break;
			case _NetworkEvent::DISCONNECT:
				HandleDisconnect(NetworkEvent);
			break;
			case _NetworkEvent::PACKET:
				HandlePacket(*NetworkEvent.Data, NetworkEvent.Peer);
				delete NetworkEvent.Data;
			break;
		}
	}

	// Update objects
	ObjectManager->Update(FrameTime);

	// Update maps
	MapManager->Update(FrameTime);

	// Update battles
	BattleManager->Update(FrameTime);

	// Check if updates should be sent
	if(Network->NeedsUpdate()) {
		Network->ResetUpdateTimer();
		if(Network->GetPeers().size() > 0) {

			// Send object updates
			for(auto &Map : MapManager->Objects) {
				Map->SendObjectUpdates();
			}
		}
	}

	// Wait for peers to disconnect
	if(StartShutdown && Network->GetPeers().size() == 0) {
		Done = true;
	}

	TimeSteps++;
	Time += FrameTime;

	// Update scripting environment
	Scripting->InjectTime(Time);

	// Update clock
	Clock += FrameTime * MAP_CLOCK_SPEED;
	if(Clock >= MAP_DAY_LENGTH)
		Clock -= MAP_DAY_LENGTH;

	// Update autosave
	SaveTime += FrameTime;
	if(SaveTime >= DEFAULT_AUTOSAVE_PERIOD) {
		SaveTime = 0;

		// Save players
		for(auto &Object : ObjectManager->Objects) {
			Save->SavePlayer(Object);
		}
	}
}

// Handle client connect
void _Server::HandleConnect(_NetworkEvent &Event) {
	char Buffer[16];
	ENetAddress *Address = &Event.Peer->ENetPeer->address;
	enet_address_get_host_ip(Address, Buffer, 16);
	Log << "Connect: " << Buffer << ":" << Address->port << std::endl;

	// Send game version
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::VERSION);
	Packet.WriteString(GAME_VERSION);
	Network->SendPacket(Packet, Event.Peer);
}

// Handle client disconnect
void _Server::HandleDisconnect(_NetworkEvent &Event) {
	char Buffer[16];
	ENetAddress *Address = &Event.Peer->ENetPeer->address;
	enet_address_get_host_ip(Address, Buffer, 16);
	Log << "Disconnect: " << Buffer << ":" << Address->port << std::endl;

	// Get object
	_Object *Player = Event.Peer->Object;
	if(Player) {
		Player->Peer = nullptr;

		if(Player->Map) {

			// Broadcast message
			for(auto &ReceivePeer : Network->GetPeers()) {
				if(ReceivePeer != Event.Peer)
					SendMessage(ReceivePeer, Player->Name + " has left the server", COLOR_GRAY);
			}
		}

		// Leave trading screen
		_Object *TradePlayer = Player->TradePlayer;
		if(TradePlayer) {
			TradePlayer->TradePlayer = nullptr;

			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::TRADE_CANCEL);
			Network->SendPacket(Packet, TradePlayer->Peer);
		}

		// Save player
		Save->SavePlayer(Player);

		Player->Deleted = true;
	}

	// Delete peer from network
	Network->DeletePeer(Event.Peer);
}

// Handle packet data
void _Server::HandlePacket(_Buffer &Data, _Peer *Peer) {
	PacketType Type = Data.Read<PacketType>();

	switch(Type) {
		case PacketType::ACCOUNT_LOGININFO:
			HandleLoginInfo(Data, Peer);
		break;
		case PacketType::CHARACTERS_REQUEST:
			HandleCharacterListRequest(Data, Peer);
		break;
		case PacketType::CHARACTERS_PLAY:
			HandleCharacterPlay(Data, Peer);
		break;
		case PacketType::CREATECHARACTER_INFO:
			HandleCharacterCreate(Data, Peer);
		break;
		case PacketType::CHARACTERS_DELETE:
			HandleCharacterDelete(Data, Peer);
		break;
		case PacketType::WORLD_MOVECOMMAND:
			HandleMoveCommand(Data, Peer);
		break;
		case PacketType::WORLD_RESPAWN:
			HandleRespawn(Data, Peer);
		break;
		case PacketType::ACTION_USE:
			HandleActionUse(Data, Peer);
		break;
		case PacketType::CHAT_MESSAGE:
			HandleChatMessage(Data, Peer);
		break;
		case PacketType::INVENTORY_MOVE:
			HandleInventoryMove(Data, Peer);
		break;
		case PacketType::INVENTORY_USE:
			HandleInventoryUse(Data, Peer);
		break;
		case PacketType::INVENTORY_SPLIT:
			HandleInventorySplit(Data, Peer);
		break;
		case PacketType::VENDOR_EXCHANGE:
			HandleVendorExchange(Data, Peer);
		break;
		case PacketType::TRADER_ACCEPT:
			HandleTraderAccept(Data, Peer);
		break;
		case PacketType::ACTIONBAR_CHANGED:
			HandleActionBarChanged(Data, Peer);
		break;
		case PacketType::SKILLS_SKILLADJUST:
			HandleSkillAdjust(Data, Peer);
		break;
		case PacketType::TRADE_REQUEST:
			HandleTradeRequest(Data, Peer);
		break;
		case PacketType::TRADE_CANCEL:
			HandleTradeCancel(Data, Peer);
		break;
		case PacketType::TRADE_GOLD:
			HandleTradeGold(Data, Peer);
		break;
		case PacketType::TRADE_ACCEPT:
			HandleTradeAccept(Data, Peer);
		break;
		case PacketType::PLAYER_STATUS:
			HandlePlayerStatus(Data, Peer);
		break;
		default:
		break;
	}
}

// Login information
void _Server::HandleLoginInfo(_Buffer &Data, _Peer *Peer) {

	// Read packet
	bool CreateAccount = Data.ReadBit();
	std::string Username(Data.ReadString());
	std::string Password(Data.ReadString());
	uint64_t Secret = Data.Read<uint64_t>();

	Username.resize(std::min(ACCOUNT_MAX_USERNAME_SIZE, Username.length()));
	Password.resize(std::min(ACCOUNT_MAX_PASSWORD_SIZE, Password.length()));

	// Validate singleplayer
	if(!Username.length() && !Password.length() && Secret != Save->GetSecret())
		return;

	// Create account or login
	if(CreateAccount) {

		// Check for existing account
		if(Save->CheckUsername(Username)) {
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::ACCOUNT_EXISTS);
			Network->SendPacket(Packet, Peer);
			return;
		}
		else
			Save->CreateAccount(Username, Password);
	}

	// Get account id
	Peer->AccountID = Save->GetAccountID(Username, Password);

	// Make sure account exists
	_Buffer Packet;
	if(Peer->AccountID == 0) {
		Packet.Write<PacketType>(PacketType::ACCOUNT_NOTFOUND);
	}
	else {
		bool AccountInUse = false;
		for(auto &CheckPeer : Network->GetPeers()) {
			if(CheckPeer != Peer && CheckPeer->AccountID == Peer->AccountID) {
				AccountInUse = true;
				Peer->AccountID = 0;
				break;
			}
		}

		if(AccountInUse)
			Packet.Write<PacketType>(PacketType::ACCOUNT_INUSE);
		else
			Packet.Write<PacketType>(PacketType::ACCOUNT_SUCCESS);
	}

	Network->SendPacket(Packet, Peer);
}

// Sends a player his/her character list
void _Server::HandleCharacterListRequest(_Buffer &Data, _Peer *Peer) {
	if(!Peer->AccountID)
		return;

	SendCharacterList(Peer);
}

// Handles the character create request
void _Server::HandleCharacterCreate(_Buffer &Data, _Peer *Peer) {
	if(!Peer->AccountID)
		return;

	// Get character information
	std::string Name(Data.ReadString());
	uint32_t PortraitID = Data.Read<uint32_t>();
	uint32_t BuildID = Data.Read<uint32_t>();
	uint32_t Slot = Data.Read<uint8_t>();
	if(Name.size() > PLAYER_NAME_SIZE)
		return;

	// Check number of characters in account
	if(Save->GetCharacterCount(Peer->AccountID) >= ACCOUNT_MAX_CHARACTER_SLOTS)
		return;

	// Found an existing name
	if(Save->GetCharacterIDByName(Name) != 0) {
		_Buffer NewPacket;
		NewPacket.Write<PacketType>(PacketType::CREATECHARACTER_INUSE);
		Network->SendPacket(NewPacket, Peer);
		return;
	}

	// Create the character
	Save->CreateCharacter(Stats, Peer->AccountID, Slot, Name, PortraitID, BuildID);

	// Notify the client
	_Buffer NewPacket;
	NewPacket.Write<PacketType>(PacketType::CREATECHARACTER_SUCCESS);
	Network->SendPacket(NewPacket, Peer);
}

// Handle a character delete request
void _Server::HandleCharacterDelete(_Buffer &Data, _Peer *Peer) {
	if(!Peer->AccountID)
		return;

	// Get delete slot
	uint32_t Slot = Data.Read<uint8_t>();
	uint32_t CharacterID = 0;

	// Get character id
	CharacterID = Save->GetCharacterIDBySlot(Peer->AccountID, Slot);
	if(!CharacterID)
		return;

	// Delete character
	Save->DeleteCharacter(CharacterID);

	// Update the player
	SendCharacterList(Peer);
}

// Loads the player, updates the world, notifies clients
void _Server::HandleCharacterPlay(_Buffer &Data, _Peer *Peer) {

	// Read packet
	uint32_t Slot = Data.Read<uint8_t>();
	NetworkIDType MapID = 0;
	std::string Name;

	// Get character info
	Save->Database->PrepareQuery("SELECT id, map_id, name FROM character WHERE account_id = @account_id and slot = @slot");
	Save->Database->BindInt(1, Peer->AccountID);
	Save->Database->BindInt(2, Slot);
	if(Save->Database->FetchRow()) {
		Peer->CharacterID = Save->Database->GetInt<uint32_t>("id");
		MapID = (NetworkIDType)Save->Database->GetInt<uint32_t>("map_id");
		Name = Save->Database->GetString("name");
	}
	Save->Database->CloseQuery();

	// Check for valid character id
	if(!Peer->CharacterID) {
		Log << "Character slot " << Slot << " empty!";
		return;
	}

	// Send map and players to new player
	Peer->Object = CreatePlayer(Peer);
	SpawnPlayer(Peer->Object, MapID, _Map::EVENT_SPAWN);

	// Broadcast message
	for(auto &ReceivePeer : Network->GetPeers()) {
		if(ReceivePeer != Peer)
			SendMessage(ReceivePeer, Name + " has joined the server", COLOR_GRAY);
	}
}

// Handles move commands from a client
void _Server::HandleMoveCommand(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;
	if(!Player->IsAlive())
		return;

	Player->InputStates.push_back(Data.Read<char>());
}

// Handle respawn command from client
void _Server::HandleRespawn(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;

	// Check death
	if(!Player->IsAlive()) {

		// Wait for battle to finish
		if(Player->Battle)
			return;

		Player->Health = 0.5f * Player->MaxHealth;
		Player->Mana = 0.5f * Player->MaxMana;
		SpawnPlayer(Player, Player->SpawnMapID, _Map::EVENT_SPAWN);
	}
}

// Handle a chat message
void _Server::HandleChatMessage(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;

	// Get message
	std::string Message = Data.ReadString();
	if(Message.length() > HUD_CHAT_SIZE)
		Message.resize(HUD_CHAT_SIZE);

	// Append name
	Message = Player->Name + ": " + Message;

	// Send message to other players
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::CHAT_MESSAGE);
	Packet.Write<glm::vec4>(COLOR_WHITE);
	Packet.WriteString(Message.c_str());

	// Broadcast message
	for(auto &ReceivePeer : Network->GetPeers()) {
		Network->SendPacket(Packet, ReceivePeer);
	}
}

// Send position to player
void _Server::SendPlayerPosition(_Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;

	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_POSITION);
	Packet.Write<glm::ivec2>(Player->Position);

	Network->SendPacket(Packet, Player->Peer);
}

// Send character list
void _Server::SendCharacterList(_Peer *Peer) {

	// Get a count of the account's characters
	Save->Database->PrepareQuery("SELECT count(id) FROM character WHERE account_id = @account_id");
	Save->Database->BindInt(1, Peer->AccountID);
	Save->Database->FetchRow();
	uint8_t CharacterCount = Save->Database->GetInt<uint8_t>(0);
	Save->Database->CloseQuery();

	// Create the packet
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::CHARACTERS_LIST);
	Packet.Write<uint8_t>(CharacterCount);

	// Generate a list of characters
	Save->Database->PrepareQuery("SELECT slot, name, portrait_id, experience FROM character WHERE account_id = @account_id");
	Save->Database->BindInt(1, Peer->AccountID);
	while(Save->Database->FetchRow()) {
		Packet.Write<uint8_t>(Save->Database->GetInt<uint8_t>("slot"));
		Packet.WriteString(Save->Database->GetString("name"));
		Packet.Write<uint32_t>(Save->Database->GetInt<uint32_t>("portrait_id"));
		Packet.Write<int32_t>(Save->Database->GetInt<int>("experience"));
	}
	Save->Database->CloseQuery();

	// Send list
	Network->SendPacket(Packet, Peer);
}

// Spawns a player at a particular spawn point
void _Server::SpawnPlayer(_Object *Player, NetworkIDType MapID, uint32_t EventType) {
	if(!ValidatePeer(Player->Peer) || !Player->Peer->CharacterID)
	   return;

	// Get map
	_Map *Map = MapManager->IDMap[MapID];

	// Load map
	if(!Map) {
		Map = MapManager->CreateWithID(MapID);
		Map->Clock = Clock;
		Map->Server = this;
		Map->Load(Stats->GetMap(MapID)->File);
	}

	// Get old map
	_Map *OldMap = Player->Map;

	// Place player in new map
	if(Map != OldMap) {
		if(OldMap) {
			OldMap->RemoveObject(Player);
		}

		Player->Map = Map;

		// Find spawn point in map
		uint32_t SpawnPoint = Player->SpawnPoint;
		if(EventType == _Map::EVENT_MAPENTRANCE)
			SpawnPoint = OldMap->NetworkID;

		// Default to mapchange event if entrance not found
		if(!Map->FindEvent(_Event(EventType, SpawnPoint), Player->Position))
			Map->FindEvent(_Event(_Map::EVENT_MAPCHANGE, SpawnPoint), Player->Position);

		// Add player to map
		Map->AddObject(Player);

		// Send new map id
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_CHANGEMAPS);
		Packet.Write<uint32_t>(MapID);
		Packet.Write<double>(Clock);
		Network->SendPacket(Packet, Player->Peer);

		// Set player object list
		Map->SendObjectList(Player->Peer);

		// Set full player data to peer
		SendPlayerInfo(Player->Peer);
	}
	else {
		Map->FindEvent(_Event(EventType, Player->SpawnPoint), Player->Position);
		SendPlayerPosition(Player->Peer);
		SendHUD(Player->Peer);
	}
}

// Create player object and load stats from save
_Object *_Server::CreatePlayer(_Peer *Peer) {

	// Create object
	_Object *Player = ObjectManager->Create();
	Player->Scripting = Scripting;
	Player->Server = this;
	Player->CharacterID = Peer->CharacterID;
	Player->Peer = Peer;
	Player->Stats = Stats;
	Peer->Object = Player;

	Save->LoadPlayer(Stats, Player);

	return Player;
}

// Send player stats to peer
void _Server::SendPlayerInfo(_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Build packet
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::OBJECT_STATS);
	Player->SerializeStats(Packet);

	Network->SendPacket(Packet, Peer);
}

// Validate a peer's attributes
bool _Server::ValidatePeer(_Peer *Peer) {
	if(!Peer->AccountID)
		return false;

	if(!Peer->Object)
		return false;

	return true;
}

// Handles a player's inventory move
void _Server::HandleInventoryMove(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	size_t OldSlot = Data.Read<uint8_t>();
	size_t NewSlot = Data.Read<uint8_t>();

	// Move items
	{
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::INVENTORY_SWAP);
		if(Player->Inventory->MoveInventory(Packet, OldSlot, NewSlot)) {
			Network->SendPacket(Packet, Peer);
			Player->CalculateStats();
		}
	}

	// Check for trading players
	_Object *TradePlayer = Player->TradePlayer;
	if(Player->WaitingForTrade && TradePlayer && (_Inventory::IsSlotTrade(OldSlot) || _Inventory::IsSlotTrade(NewSlot))) {

		// Reset agreement
		Player->TradeAccepted = false;
		TradePlayer->TradeAccepted = false;

		// Build packet
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::TRADE_ITEM);

		// Write slots
		Player->Inventory->SerializeSlot(Packet, NewSlot);
		Player->Inventory->SerializeSlot(Packet, OldSlot);

		// Send updates
		Network->SendPacket(Packet, TradePlayer->Peer);
	}
}

// Handle a player's inventory use request
void _Server::HandleInventoryUse(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get item slot index
	size_t Slot = Data.Read<uint8_t>();
	if(Slot >= Player->Inventory->Slots.size())
		return;

	// Check for existing action
	if(!Player->Action.IsSet()) {
		Player->Targets.clear();
		Player->Targets.push_back(Player);
		Player->Action.Item = Player->Inventory->Slots[Slot].Item;
		Player->Action.Level = Player->Action.Item->Level;
		Player->Action.InventorySlot = (int)Slot;
	}
}

// Handle a player's inventory split stack request
void _Server::HandleInventorySplit(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	uint8_t Slot = Data.Read<uint8_t>();
	uint8_t Count = Data.Read<uint8_t>();

	// Inventory only
	if(_Inventory::IsSlotTrade(Slot))
		return;

	// Split items
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::INVENTORY_UPDATE);
	if(Player->Inventory->SplitStack(Packet, Slot, Count))
		Network->SendPacket(Packet, Peer);
}

// Handles a vendor exchange message
void _Server::HandleVendorExchange(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get vendor
	const _Vendor *Vendor = Player->Vendor;
	if(!Vendor)
		return;

	// Get info
	bool Buy = Data.ReadBit();
	uint8_t Amount = Data.Read<uint8_t>();
	size_t Slot = Data.Read<uint8_t>();

	// Buy item
	if(Buy) {
		if(Slot >= Vendor->Items.size())
			return;

		// Get optional inventory slot
		size_t TargetSlot = Data.Read<uint8_t>();

		// Get item info
		const _Item *Item = Vendor->Items[Slot];
		int Price = Item->GetPrice(Vendor, Amount, Buy);

		// Not enough gold
		if(Price > Player->Gold)
			return;

		// Find open slot for new item
		if(TargetSlot >= Player->Inventory->Slots.size())
			TargetSlot = Player->Inventory->FindSlotForItem(Item, Amount);

		// No room
		if(TargetSlot >= Player->Inventory->Slots.size())
			return;

		// Attempt to add item
		if(!Player->Inventory->AddItem(Item, Amount, TargetSlot))
			return;

		// Update gold
		Player->UpdateGold(-Price);
		{
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::INVENTORY_GOLD);
			Packet.Write<int32_t>(Player->Gold);
			Network->SendPacket(Packet, Peer);
		}

		// Update items
		{
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::INVENTORY_UPDATE);
			Packet.Write<uint8_t>(1);
			Player->Inventory->SerializeSlot(Packet, TargetSlot);
			Network->SendPacket(Packet, Peer);
		}

		Player->CalculateStats();
	}
	// Sell item
	else {
		if(Slot >= Player->Inventory->Slots.size())
			return;

		// Get item info
		const _Item *Item = Player->Inventory->Slots[Slot].Item;
		if(Item) {
			int Price = Item->GetPrice(Vendor, Amount, Buy);

			// Update gold
			Player->UpdateGold(Price);
			{
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::INVENTORY_GOLD);
				Packet.Write<int32_t>(Player->Gold);
				Network->SendPacket(Packet, Peer);
			}

			// Update items
			Player->Inventory->DecrementItemCount(Slot, -Amount);
			{
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::INVENTORY_UPDATE);
				Packet.Write<uint8_t>(1);
				Player->Inventory->SerializeSlot(Packet, Slot);
				Network->SendPacket(Packet, Peer);
			}
		}
	}
}

// Handles a trader accept
void _Server::HandleTraderAccept(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get trader information
	std::vector<size_t> RequiredItemSlots(Player->Trader->TraderItems.size(), Player->Inventory->Slots.size());
	size_t RewardSlot = Player->Inventory->GetRequiredItemSlots(Player->Trader, RequiredItemSlots);
	if(RewardSlot >= Player->Inventory->Slots.size())
		return;

	// Update items
	Player->AcceptTrader(RequiredItemSlots);

	// Send new inventory
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::INVENTORY);
	Player->Inventory->Serialize(Packet);
	Network->SendPacket(Packet, Peer);
}

// Handles a skill adjust
void _Server::HandleSkillAdjust(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Process packet
	uint32_t SkillID = Data.Read<uint32_t>();
	int Amount = Data.Read<int>();
	Player->AdjustSkillLevel(SkillID, Amount);

	Player->CalculateStats();
}

// Handle a trade request
void _Server::HandleTradeRequest(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get map object
	_Map *Map = Player->Map;
	if(!Map)
		return;

	// Set status
	Player->WaitingForTrade = true;

	// Find the nearest player to trade with
	_Object *TradePlayer = Map->FindTradePlayer(Player, 2.0f * 2.0f);
	if(TradePlayer == nullptr) {

		// Set up trade post
		Player->TradeGold = 0;
		Player->TradeAccepted = false;
		Player->TradePlayer = nullptr;
	}
	else {

		// Set up trade screen for both players
		SendTradeInformation(Player, TradePlayer);
		SendTradeInformation(TradePlayer, Player);

		Player->TradePlayer = TradePlayer;
		Player->TradeAccepted = false;
		TradePlayer->TradePlayer = Player;
		TradePlayer->TradeAccepted = false;
		TradePlayer->WaitingForTrade = true;
	}
}

// Handles a trade cancel
void _Server::HandleTradeCancel(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Notify trading player
	_Object *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {
		TradePlayer->TradePlayer = nullptr;
		TradePlayer->TradeAccepted = false;

		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::TRADE_CANCEL);
		Network->SendPacket(Packet, TradePlayer->Peer);
	}

	// Set state back to normal
	Player->TradePlayer = nullptr;
	Player->WaitingForTrade = false;
}

// Handle a trade gold update
void _Server::HandleTradeGold(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Set gold amount
	int Gold = Data.Read<int32_t>();
	if(Gold < 0)
		Gold = 0;
	else if(Gold > Player->Gold)
		Gold = Player->Gold;
	Player->TradeGold = Gold;
	Player->TradeAccepted = false;

	// Notify player
	_Object *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {
		TradePlayer->TradeAccepted = false;

		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::TRADE_GOLD);
		Packet.Write<int32_t>(Gold);
		Network->SendPacket(Packet, TradePlayer->Peer);
	}
}

// Handles a trade accept from a player
void _Server::HandleTradeAccept(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get trading player
	_Object *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {

		// Set the player's state
		bool Accepted = !!Data.Read<char>();
		Player->TradeAccepted = Accepted;

		// Check if both player's agree
		if(Accepted && TradePlayer->TradeAccepted) {

			// Exchange items
			_InventorySlot TempItems[PLAYER_TRADEITEMS];
			for(size_t i = 0; i < PLAYER_TRADEITEMS; i++) {
				size_t InventorySlot = i + InventoryType::TRADE;
				TempItems[i] = Player->Inventory->Slots[InventorySlot];

				Player->Inventory->Slots[InventorySlot] = TradePlayer->Inventory->Slots[InventorySlot];
				TradePlayer->Inventory->Slots[InventorySlot] = TempItems[i];
			}

			// Exchange gold
			Player->UpdateGold(TradePlayer->TradeGold - Player->TradeGold);
			TradePlayer->UpdateGold(Player->TradeGold - TradePlayer->TradeGold);

			// Move items to inventory and reset
			Player->WaitingForTrade = false;
			Player->TradePlayer = nullptr;
			Player->TradeGold = 0;
			Player->Inventory->MoveTradeToInventory();
			TradePlayer->WaitingForTrade = false;
			TradePlayer->TradePlayer = nullptr;
			TradePlayer->TradeGold = 0;
			TradePlayer->Inventory->MoveTradeToInventory();

			// Send packet to players
			{
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::TRADE_EXCHANGE);
				Packet.Write<int32_t>(Player->Gold);
				Player->Inventory->Serialize(Packet);
				Network->SendPacket(Packet, Player->Peer);
			}
			{
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::TRADE_EXCHANGE);
				Packet.Write<int32_t>(TradePlayer->Gold);
				TradePlayer->Inventory->Serialize(Packet);
				Network->SendPacket(Packet, TradePlayer->Peer);
			}

		}
		else {

			// Notify trading player
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::TRADE_ACCEPT);
			Packet.Write<char>(Accepted);
			Network->SendPacket(Packet, TradePlayer->Peer);
		}
	}
}

// Handles player status change
void _Server::HandlePlayerStatus(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;
	if(Player->Battle)
		return;

	// Read packet
	uint8_t Status = Data.Read<uint8_t>();
	switch(Status) {
		case _Object::STATUS_NONE:
			Player->InventoryOpen = false;
			Player->SkillsOpen = false;
			Player->Paused = false;
			Player->Vendor = nullptr;
			Player->Trader = nullptr;
			Player->TeleportTime = -1.0;
		break;
		case _Object::STATUS_PAUSE:
			Player->Paused = true;
		break;
		case _Object::STATUS_INVENTORY:
			Player->InventoryOpen = true;
		break;
		case _Object::STATUS_SKILLS:
			Player->SkillsOpen = true;
		break;
		case _Object::STATUS_TELEPORT: {
			Player->TeleportTime = PLAYER_TELEPORT_TIME;
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::WORLD_TELEPORTSTART);
			Packet.Write<double>(Player->TeleportTime);
			Network->SendPacket(Packet, Peer);
		} break;
		default:
		break;
	}

}

// Handle action use by player
void _Server::HandleActionUse(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;
	if(!Player->IsAlive())
		return;

	// Set action used
	Player->SetActionUsing(Data, ObjectManager);

	// Check for battle
	if(Player->Battle) {

		// Notify other players of action
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::BATTLE_ACTION);
		Packet.Write<NetworkIDType>(Player->NetworkID);
		if(Player->Action.Item)
			Packet.Write<uint32_t>(Player->Action.Item->ID);
		else
			Packet.Write<uint32_t>(0);

		Player->Battle->BroadcastPacket(Packet);
	}
}

// Handle an action bar change
void _Server::HandleActionBarChanged(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Read skills
	for(size_t i = 0; i < Player->ActionBar.size(); i++)
		Player->ActionBar[i].Unserialize(Data, Stats);

	Player->CalculateStats();
}

// Updates the player's HUD
void _Server::SendHUD(_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_HUD);
	Packet.Write<float>(Player->Health);
	Packet.Write<float>(Player->Mana);
	Packet.Write<float>(Player->MaxHealth);
	Packet.Write<float>(Player->MaxMana);
	Packet.Write<int32_t>(Player->Experience);
	Packet.Write<int32_t>(Player->Gold);

	Network->SendPacket(Packet, Peer);
}

// Run event script from map
void _Server::RunEventScript(uint32_t ScriptID, _Object *Object) {
	if(!Object || !Object->Peer)
		return;

	// Find script
	auto Iterator = Stats->Scripts.find(ScriptID);
	if(Iterator != Stats->Scripts.end()) {
		_Script &Script = Iterator->second;

		_StatChange StatChange;
		StatChange.Object = Object;
		if(Scripting->StartMethodCall(Script.Name, "Activate")) {
			Scripting->PushInt(Script.Level);
			Scripting->PushObject(StatChange.Object);
			Scripting->PushStatChange(&StatChange);
			Scripting->MethodCall(3, 1);
			Scripting->GetStatChange(1, StatChange);
			Scripting->FinishMethodCall();

			StatChange.Object->UpdateStats(StatChange);

			// Build packet
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::STAT_CHANGE);
			StatChange.Serialize(Packet);

			// Send packet to player
			Network->SendPacket(Packet, Object->Peer);
		}
	}
}

// Send a message to the player
void _Server::SendMessage(_Peer *Peer, const std::string &Message, const glm::vec4 &Color) {
	if(!ValidatePeer(Peer))
		return;

	// Build message
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::CHAT_MESSAGE);
	Packet.Write<glm::vec4>(Color);
	Packet.WriteString(Message.c_str());

	// Send
	Network->SendPacket(Packet, Peer);
}

// Sends information to another player about items they're trading
void _Server::SendTradeInformation(_Object *Sender, _Object *Receiver) {

	// Send items to trader player
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::TRADE_REQUEST);
	Packet.Write<NetworkIDType>(Sender->NetworkID);
	Packet.Write<int32_t>(Sender->TradeGold);
	for(size_t i = InventoryType::TRADE; i < InventoryType::COUNT; i++)
		Sender->Inventory->SerializeSlot(Packet, i);

	Network->SendPacket(Packet, Receiver->Peer);
}

// Start a battle event
void _Server::StartBattle(_Object *Object, uint32_t Zone, bool Scripted) {
	//Zone = 1;
	if(!Zone)
		return;

	// Get a list of players
	std::list<_Object *> Players;
	Object->Map->GetClosePlayers(Object, 7*7, BATTLE_MAXFIGHTERS_SIDE-1, Players);
	int AdditionalCount = 0;
	if(!Scripted)
		AdditionalCount = (int)Players.size();

	// Get monsters
	std::list<uint32_t> MonsterIDs;
	bool Boss = false;
	Stats->GenerateMonsterListFromZone(AdditionalCount, Zone, MonsterIDs, Boss);
	//MonsterIDs.clear();
	//MonsterIDs.push_back(10);

	if(MonsterIDs.size()) {

		// Create a new battle instance
		_Battle *Battle = BattleManager->Create();
		Battle->Manager = ObjectManager;
		Battle->Stats = Stats;
		Battle->Server = this;
		Battle->Boss = Boss;
		Battle->Scripting = Scripting;

		/*
		for(int i = 0; i < 5; i++) {
			_Object *Monster = ObjectManager->Create();
			Monster->Server = this;
			Monster->Scripting = Scripting;
			Monster->DatabaseID = 3;
			Monster->Stats = Stats;
			Stats->GetMonsterStats(Monster->DatabaseID, Monster);
			Monster->CalculateStats();
			Battle->AddFighter(Monster, 0);
		}*/

		// Add player
		Players.push_back(Object);

		// Sort by network id
		Players.sort();

		// Add players to battle
		for(auto &PartyPlayer : Players) {
			Battle->AddFighter(PartyPlayer, 0);
		}

		// Add monsters
		for(auto &MonsterID : MonsterIDs) {
			_Object *Monster = ObjectManager->Create();
			Monster->Server = this;
			Monster->Scripting = Scripting;
			Monster->DatabaseID = MonsterID;
			Monster->Stats = Stats;
			Stats->GetMonsterStats(MonsterID, Monster);
			Monster->CalculateStats();
			Battle->AddFighter(Monster, 1);
		}

		// Send battle to players
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::BATTLE_START);
		Battle->Serialize(Packet);
		Battle->BroadcastPacket(Packet);
	}
}
