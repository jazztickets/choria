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
#include <instances/map.h>
#include <instances/battle.h>
#include <scripting.h>
#include <save.h>
#include <database.h>
#include <buffer.h>
#include <packet.h>
#include <stats.h>
#include <constants.h>
#include <config.h>
#include <SDL_timer.h>

// Function to run the server thread
void RunThread(void *Arguments) {

	// Get server object
	_Server *Server = (_Server *)Arguments;

	// Init timer
	Uint64 Timer = SDL_GetPerformanceCounter();
	double TimeStepAccumulator = 0.0;
	double TimeStep = GAME_TIMESTEP;
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
	Clock(0.0),
	Stats(Stats),
	Network(new _ServerNetwork(Config.MaxClients, NetworkPort)),
	NextMapID(0),
	Thread(nullptr) {

	Log.Open((Config.ConfigPath + "server.log").c_str());

	if(!Network->HasConnection())
		throw std::runtime_error("Unable to bind address!");

	Network->SetFakeLag(Config.FakeLag);
	Network->SetUpdatePeriod(Config.NetworkRate);

	Save = new _Save();
	Clock = Save->GetClock();

	Scripting = new _Scripting();
	Scripting->InjectStats(Stats);
	Scripting->LoadScript(SCRIPTS_PATH + SCRIPTS_ITEMS);
	Scripting->LoadScript(SCRIPTS_PATH + SCRIPTS_AI);
	Scripting->LoadScript(SCRIPTS_PATH + SCRIPTS_SKILLS);
	Scripting->LoadScript(SCRIPTS_PATH + SCRIPTS_BUFFS);
}

// Destructor
_Server::~_Server() {
	Done = true;
	JoinThread();

	// Delete maps
	for(auto &Map : Maps) {
		delete Map;
	}
	Maps.clear();

	Save->SaveClock(Clock);

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

	// Update maps
	for(auto &Map : Maps) {
		Map->Clock = Clock;
		Map->Update(FrameTime);
	}

	// Update maps
	for(auto &Battle : Battles)
		Battle->Update(FrameTime);

	// Check if updates should be sent
	if(Network->NeedsUpdate()) {
		Network->ResetUpdateTimer();
		if(Network->GetPeers().size() > 0) {

			// Send object updates
			for(auto &Map : Maps) {
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

	// Update clock
	Clock += FrameTime * MAP_CLOCK_SPEED;
	if(Clock >= MAP_DAY_LENGTH)
		Clock -= MAP_DAY_LENGTH;

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
		if(Player->Map) {
			Player->Map->RemovePeer(Event.Peer);

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

		// Remove from battle
		RemovePlayerFromBattle(Player);

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
		case PacketType::PLAYER_USEACTION:
			HandlePlayerUseAction(Data, Peer);
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
		case PacketType::BATTLE_CLIENTDONE:
			HandleBattleFinished(Data, Peer);
		break;
	/*
		case PacketType::WORLD_ATTACKPLAYER:
			HandleAttackPlayer(Data, Peer);
		break;
	*/
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
	Save->CreateCharacter(Peer->AccountID, Slot, Name, PortraitID);

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
	uint32_t MapID = 0;
	std::string Name;

	// Get character info
	Save->Database->PrepareQuery("SELECT id, map_id, name FROM character WHERE account_id = @account_id and slot = @slot");
	Save->Database->BindInt(1, Peer->AccountID);
	Save->Database->BindInt(2, Slot);
	if(Save->Database->FetchRow()) {
		Peer->CharacterID = Save->Database->GetInt<uint32_t>("id");
		MapID = Save->Database->GetInt<uint32_t>("map_id");
		Name = Save->Database->GetString("name");
	}
	Save->Database->CloseQuery();

	// Check for valid character id
	if(!Peer->CharacterID) {
		Log << "Character slot " << Slot << " empty!";
		return;
	}

	// Send map and players to new player
	SpawnPlayer(Peer, MapID, _Map::EVENT_SPAWN);

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
	Player->InputState = Data.Read<char>();
}

// Handle a chat message
void _Server::HandleChatMessage(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;

	// Get message
	std::string Message = Data.ReadString();
	if(Message.length() > NETWORKING_CHAT_SIZE)
		Message.resize(NETWORKING_CHAT_SIZE);

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
void _Server::SpawnPlayer(_Peer *Peer, uint32_t MapID, uint32_t EventType) {
	if(!Peer->AccountID || !Peer->CharacterID)
		return;

	// Get player object
	_Object *Player = Peer->Object;

	// Get new map
	_Map *Map = GetMap(MapID);

	// Get old map
	_Map *OldMap = nullptr;
	if(Player)
		OldMap = Player->Map;

	// Remove old player if map has changed
	if(Map != OldMap) {
		int OldInvisPower = 0;

		// Delete old player
		if(Player) {
			Save->SavePlayer(Player);
			Player->Deleted = true;
			OldInvisPower = Player->InvisPower;
		}

		// Create new player
		Player = CreatePlayer(Peer);
		Player->NetworkID = Map->GenerateObjectID();
		Player->Map = Map;
		Player->InvisPower = OldInvisPower;

		// Find spawn point in map
		uint32_t SpawnPoint = Player->SpawnPoint;
		if(EventType == _Map::EVENT_MAPCHANGE)
			SpawnPoint = OldMap->ID;
		Map->FindEvent(_Event(EventType, SpawnPoint), Player->Position);

		// Add player to map
		Map->AddObject(Player);

		// Send new map id
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_CHANGEMAPS);
		Packet.Write<uint32_t>(MapID);
		Packet.Write<double>(Clock);
		Network->SendPacket(Packet, Peer);

		// Add peer to map
		Map->AddPeer(Peer);

		// Set player object list
		Map->SendObjectList(Peer);

		// Set full player data to peer
		SendPlayerInfo(Peer);
	}
	else {
		Map->FindEvent(_Event(EventType, Player->SpawnPoint), Player->Position);
		SendPlayerPosition(Peer);
		SendHUD(Peer);
	}
}

// Gets a map from the manager. Loads the level if it doesn't exist
_Map *_Server::GetMap(uint32_t MapID) {

	// Loop through loaded maps
	for(auto &Map : Maps) {

		// Check id
		if(Map->ID == MapID)
			return Map;
	}

	// Not found, so create it
	_Map *NewMap = new _Map();
	NewMap->ID = MapID;
	NewMap->Server = this;
	NewMap->Load(Stats->GetMap(MapID)->File);
	Maps.push_back(NewMap);

	return NewMap;
}

// Create player object and load stats from save
_Object *_Server::CreatePlayer(_Peer *Peer) {

	// Create object
	_Object *Player = new _Object();
	Peer->Object = Player;
	Player->CharacterID = Peer->CharacterID;
	Player->Peer = Peer;
	Player->Stats = Stats;

	Save->LoadPlayer(Player);

	// Get stats
	Player->GenerateNextBattle();
	Player->CalculateStats();
	Player->RestoreHealthMana();

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

	// Use an item
	//Player->UseInventory(Data.Read<uint8_t>());
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

	// Exchange items and notify client
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::INVENTORY_UPDATE);
	Player->AcceptTrader(Packet, RequiredItemSlots, RewardSlot);
	Network->SendPacket(Packet, Peer);
}

// Handles a skill adjust
void _Server::HandleSkillAdjust(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Process packet
	bool Spend = Data.ReadBit();
	uint32_t SkillID = Data.Read<uint32_t>();
	if(Spend) {
		Player->AdjustSkillLevel(SkillID, 1);
	}
	else {
		Player->AdjustSkillLevel(SkillID, -1);
	}

	Player->CalculateSkillPoints();
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

	// Read packet
	uint8_t Status = Data.Read<uint8_t>();
	switch(Status) {
		case _Object::STATUS_NONE:
			Player->InventoryOpen = false;
			Player->SkillsOpen = false;
			Player->Paused = false;
			Player->Vendor = nullptr;
			Player->Trader = nullptr;
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
			Player->TeleportTime = GAME_TELEPORT_TIME;
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::WORLD_TELEPORTSTART);
			Packet.Write<double>(Player->TeleportTime);
			Network->SendPacket(Packet, Peer);
		} break;
		default:
		break;
	}

}

// Handle player using action outside of battle
void _Server::HandlePlayerUseAction(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;

	if(Player->Battle) {
		Player->Battle->ServerHandleAction(Player, Data);
	}
	else {
		uint8_t Slot = Data.Read<uint8_t>();

		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::PLAYER_ACTIONRESULTS);
		if(Player->UseActionWorld(Packet, Scripting, Slot)) {
			Network->SendPacket(Packet, Peer);
		}
	}
}

// Handle a skill bar change
void _Server::HandleActionBarChanged(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Read skills
	for(size_t i = 0; i < Player->ActionBar.size(); i++)
		Player->ActionBar[i].Unserialize(Data, Stats);

	Player->CalculateStats();
}

// The client is done with the battle results screen
void _Server::HandleBattleFinished(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Check for the last player leaving the battle
	RemovePlayerFromBattle(Player);

	// Check for death
	if(Player->Health == 0) {
		Player->RestoreHealthMana();
		SpawnPlayer(Player->Peer, Player->SpawnMapID, _Map::EVENT_SPAWN);
	}
}

// Removes a player from a battle and deletes the battle if necessary
void _Server::RemovePlayerFromBattle(_Object *Player) {
	if(!Player->Battle)
		return;

	// Save pointer to battle
	_Battle *Battle = Player->Battle;

	// Delete instance
	Battle->RemoveFighter(Player);
	if(Battle->GetPeerCount() == 0) {

		// Loop through loaded battles
		for(auto BattleIterator = Battles.begin(); BattleIterator != Battles.end(); ++BattleIterator) {
			if(*BattleIterator == Battle) {
				Battles.erase(BattleIterator);
				delete Battle;
				break;
			}
		}
	}
}

/*
// Handles a player's request to attack another player
void _Server::HandleAttackPlayer(_Buffer &Data, _Peer *Peer) {
	_Object *Player = Peer->data;
	if(!Player || !Player->CanAttackPlayer())
		return;

	_Map *Map = Player->Map;
	if(!Map)
		return;

	// Check for a valid pvp tile
	if(Player->GetTile()->PVP) {

		// Reset timer
		Player->ResetAttackPlayerTime();

		// Get a list of players next to the player
		std::list<_Object *> Players;
		Map->GetClosePlayers(Player, 1.5f * 1.5f, Players);

		// Find a suitable player to attack
		for(std::list<_Object *>::iterator Iterator = Players.begin(); Iterator != Players.end(); ++Iterator) {
			_Object *VictimPlayer = *Iterator;
			if(VictimPlayer->State != _Object::STATE_BATTLE) {
				_ServerBattle *Battle = new _ServerBattle();
				Battles.push_back(Battle);

				Battle->AddFighter(Player, 1);
				Battle->AddFighter(VictimPlayer, 0);
				Battle->StartBattle();
				break;
			}
		}
	}
}
*/

// Updates the player's HUD
void _Server::SendHUD(_Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_HUD);
	Packet.Write<int32_t>(Player->Experience);
	Packet.Write<int32_t>(Player->Gold);
	Packet.Write<int32_t>(Player->Health);
	Packet.Write<int32_t>(Player->Mana);
	Packet.Write<float>(Player->HealthAccumulator);
	Packet.Write<float>(Player->ManaAccumulator);

	Network->SendPacket(Packet, Peer);
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
void _Server::StartBattle(_Object *Object, uint32_t Zone) {
	Zone = 4;
	if(!Zone)
		return;

	// Get monsters
	std::list<uint32_t> MonsterIDs;
	Stats->GenerateMonsterListFromZone(Zone, MonsterIDs);
	if(MonsterIDs.size()) {

		// Create a new battle instance
		_Battle *Battle = new _Battle();
		Battle->Stats = Stats;
		Battle->Server = this;
		Battle->Scripting = Scripting;
		Battles.push_back(Battle);

		/*
		for(int i = 0; i < 7; i++) {
			_Object *Monster = new _Object();
			Monster->DatabaseID = 1;
			Stats->GetMonsterStats(1, Monster);
			Battle->AddFighter(Monster, 0);
		}*/

		// Add players
		Battle->AddFighter(Object, 0);

		// Get a list of players
		std::list<_Object *> Players;
		Object->Map->GetClosePlayers(Object, 7*7, Players);

		// Add players to battle
		int PlayersAdded = 0;
		for(auto &PartyPlayer : Players) {
			if(PartyPlayer->CanBattle()) {
				Battle->AddFighter(PartyPlayer, 0);
				PlayersAdded++;
				if(PlayersAdded == BATTLE_MAXFIGHTERS_SIDE-1)
					break;
			}
		}

		// Add monsters
		for(auto &MonsterID : MonsterIDs) {
			_Object *Monster = new _Object();
			Monster->DatabaseID = MonsterID;
			Stats->GetMonsterStats(MonsterID, Monster);
			Battle->AddFighter(Monster, 1);
		}

		// Send messages out
		Battle->ServerStartBattle();
	}
}
