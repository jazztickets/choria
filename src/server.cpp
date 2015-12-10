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
_Server::_Server(uint16_t NetworkPort)
:	Done(false),
	StartShutdown(false),
	TimeSteps(0),
	Time(0.0),
	Clock(0.0),
	Stats(nullptr),
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
	Scripting->LoadScript(SCRIPTS_PATH + SCRIPTS_AI);
	Scripting->LoadScript(SCRIPTS_PATH + SCRIPTS_SKILLS);
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
		case PacketType::WORLD_ACTIONBAR_USE:
			HandleActionBarUse(Data, Peer);
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
		case PacketType::BATTLE_SETACTION:
			HandleBattleAction(Data, Peer);
		break;
		case PacketType::BATTLE_CHANGETARGET:
			HandleBattleChangeTarget(Data, Peer);
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
	int Slot = Data.Read<int>();
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
	int32_t Slot = Data.Read<int32_t>();
	int CharacterID = 0;

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
	int Slot = Data.Read<char>();
	int MapID = 0;
	std::string Name;

	// Get character info
	std::stringstream Query;
	Query << "SELECT id, map_id, name FROM character WHERE account_id = " << Peer->AccountID << " and slot = " << Slot;
	Save->Database->PrepareQuery(Query.str());
	if(Save->Database->FetchRow()) {
		Peer->CharacterID = Save->Database->GetInt("id");
		MapID = Save->Database->GetInt("map_id");
		Name = Save->Database->GetString("name");
	}
	Save->Database->CloseQuery();
	Query.str("");

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

// Handle player using action outside of battle
void _Server::HandleActionBarUse(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;

	uint8_t Slot = Data.Read<uint8_t>();
	if(Player->UseAction(Slot))
		SendHUD(Player->Peer);
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
	int CharacterCount = Save->Database->GetInt(0);
	Save->Database->CloseQuery();

	// Create the packet
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::CHARACTERS_LIST);
	Packet.Write<char>(CharacterCount);

	// Generate a list of characters
	Save->Database->PrepareQuery("SELECT slot, name, portrait_id, experience FROM character WHERE account_id = @account_id");
	Save->Database->BindInt(1, Peer->AccountID);
	while(Save->Database->FetchRow()) {
		Packet.Write<int32_t>(Save->Database->GetInt("slot"));
		Packet.WriteString(Save->Database->GetString("name"));
		Packet.Write<int32_t>(Save->Database->GetInt("portrait_id"));
		Packet.Write<int32_t>(Save->Database->GetInt("experience"));
	}
	Save->Database->CloseQuery();

	// Send list
	Network->SendPacket(Packet, Peer);
}

// Spawns a player at a particular spawn point
void _Server::SpawnPlayer(_Peer *Peer, int MapID, int EventType) {
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
		int SpawnPoint = Player->SpawnPoint;
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

	// Send character packet
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_YOURCHARACTERINFO);
	Packet.WriteString(Player->Name.c_str());
	Packet.Write<uint32_t>(Player->PortraitID);
	Packet.Write<int32_t>(Player->Experience);
	Packet.Write<int32_t>(Player->Gold);
	Packet.Write<int32_t>(Player->PlayTime);
	Packet.Write<int32_t>(Player->Deaths);
	Packet.Write<int32_t>(Player->MonsterKills);
	Packet.Write<int32_t>(Player->PlayerKills);
	Packet.Write<int32_t>(Player->Bounty);
	Packet.Write<int32_t>(Player->InvisPower);

	// Get item count
	int ItemCount = 0;
	for(int i = 0; i < _Object::INVENTORY_COUNT; i++) {
		if(Player->Inventory[i].Item)
			ItemCount++;
	}

	// Write items
	Packet.Write<char>(ItemCount);
	for(int i = 0; i < _Object::INVENTORY_COUNT; i++) {
		if(Player->Inventory[i].Item) {
			Packet.Write<char>(i);
			Packet.Write<char>(Player->Inventory[i].Count);
			Packet.Write<int32_t>(Player->Inventory[i].Item->ID);
		}
	}

	// Get skill count
	int SkillCount = 0;
	for(const auto &SkillLevel : Player->SkillLevels) {
		if(SkillLevel.second > 0)
			SkillCount++;
	}

	// Write skills
	Packet.Write<char>(SkillCount);
	for(const auto &SkillLevel : Player->SkillLevels) {
		if(SkillLevel.second > 0) {
			Packet.Write<uint32_t>(SkillLevel.first);
			Packet.Write<int32_t>(SkillLevel.second);
		}
	}

	// Write skill bar
	Packet.Write<uint8_t>(Player->ActionBar.size());
	for(size_t i = 0; i < Player->ActionBar.size(); i++) {
		Player->ActionBar[i].Serialize(Packet);
	}

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

	int OldSlot = Data.Read<char>();
	int NewSlot = Data.Read<char>();

	// Move items
	Player->MoveInventory(OldSlot, NewSlot);
	Player->CalculateStats();

	// Check for trading players
	_Object *TradePlayer = Player->TradePlayer;
	if(Player->WaitingForTrade && TradePlayer && (_Object::IsSlotTrade(OldSlot) || _Object::IsSlotTrade(NewSlot))) {

		// Reset agreement
		Player->TradeAccepted = false;
		TradePlayer->TradeAccepted = false;

		// Send item updates to trading player
		_InventorySlot *OldSlotItem = &Player->Inventory[OldSlot];
		_InventorySlot *NewSlotItem = &Player->Inventory[NewSlot];

		// Get item information
		int OldItemID = 0, NewItemID = 0, OldItemCount = 0, NewItemCount = 0;
		if(OldSlotItem->Item) {
			OldItemID = OldSlotItem->Item->ID;
			OldItemCount = OldSlotItem->Count;
		}
		if(NewSlotItem->Item) {
			NewItemID = NewSlotItem->Item->ID;
			NewItemCount = NewSlotItem->Count;
		}

		// Build packet
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::TRADE_ITEM);
		Packet.Write<int32_t>(OldItemID);
		Packet.Write<char>(OldSlot);
		if(OldItemID > 0)
			Packet.Write<char>(OldItemCount);
		Packet.Write<int32_t>(NewItemID);
		Packet.Write<char>(NewSlot);
		if(NewItemID > 0)
			Packet.Write<char>(NewItemCount);

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
	Player->UseInventory(Data.Read<char>());
}

// Handle a player's inventory split stack request
void _Server::HandleInventorySplit(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	int Slot = Data.Read<char>();
	int Count = Data.Read<char>();

	// Inventory only
	if(!_Object::IsSlotInventory(Slot))
		return;

	Player->SplitStack(Slot, Count);
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
	int Amount = Data.Read<char>();
	int Slot = Data.Read<char>();
	if(Slot < 0)
		return;

	// Update player
	if(Buy) {
		if(Slot >= (int)Vendor->Items.size())
			return;

		// Get optional inventory slot
		int TargetSlot = Data.Read<char>();

		// Get item info
		const _Item *Item = Vendor->Items[Slot];
		int Price = Item->GetPrice(Vendor, Amount, Buy);

		// Update player
		Player->UpdateGold(-Price);
		Player->AddItem(Item, Amount, TargetSlot);
		Player->CalculateStats();
	}
	else {
		if(Slot >= _Object::INVENTORY_COUNT)
			return;

		// Get item info
		_InventorySlot *Item = &Player->Inventory[Slot];
		if(Item && Item->Item) {
			int Price = Item->Item->GetPrice(Vendor, Amount, Buy);
			Player->UpdateGold(Price);
			Player->UpdateInventory(Slot, -Amount);
		}
	}
}

// Handles a trader accept
void _Server::HandleTraderAccept(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Get trader information
	int RequiredItemSlots[8];
	int RewardSlot = Player->GetRequiredItemSlots(RequiredItemSlots);
	if(RewardSlot == -1)
		return;

	// Exchange items
	Player->AcceptTrader(RequiredItemSlots, RewardSlot);
	Player->Trader = nullptr;
	Player->CalculateStats();
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

// Handles a skill adjust
void _Server::HandleSkillAdjust(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;

	// Process packet
	bool Spend = Data.ReadBit();
	int SkillID = Data.Read<int32_t>();
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
			for(int i = 0; i < PLAYER_TRADEITEMS; i++) {
				int InventorySlot = i + _Object::INVENTORY_TRADE;
				TempItems[i] = Player->Inventory[InventorySlot];

				Player->SetInventory(InventorySlot, &TradePlayer->Inventory[InventorySlot]);
				TradePlayer->SetInventory(InventorySlot, &TempItems[i]);
			}

			// Exchange gold
			Player->UpdateGold(TradePlayer->TradeGold - Player->TradeGold);
			TradePlayer->UpdateGold(Player->TradeGold - TradePlayer->TradeGold);

			// Send packet to players
			{
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::TRADE_EXCHANGE);
				BuildTradeItemsPacket(Player, Packet, Player->Gold);
				Network->SendPacket(Packet, Player->Peer);
			}
			{
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::TRADE_EXCHANGE);
				BuildTradeItemsPacket(TradePlayer, Packet, TradePlayer->Gold);
				Network->SendPacket(Packet, TradePlayer->Peer);
			}

			Player->WaitingForTrade = false;
			Player->TradePlayer = nullptr;
			Player->TradeGold = 0;
			Player->MoveTradeToInventory();
			TradePlayer->WaitingForTrade = false;
			TradePlayer->TradePlayer = nullptr;
			TradePlayer->TradeGold = 0;
			TradePlayer->MoveTradeToInventory();
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
	int Status = Data.Read<char>();
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

// Handles battle commands from a client
void _Server::HandleBattleAction(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	_Object *Player = Peer->Object;
	if(!Player->Battle)
		return;

	Player->Battle->ServerHandleAction(Player, Data);
}

// Handles battle target changes
void _Server::HandleBattleChangeTarget(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
		return;

	//_Object *Player = Peer->Object;
	//if(!Player->Battle)
	//	return;

	//int BattleTargetID = Data.Read<char>();
	//Player->BattleTarget = Player->Battle->GetObjectByID(BattleTargetID);
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

// Adds trade item information to a packet
void _Server::BuildTradeItemsPacket(_Object *Player, _Buffer &Packet, int Gold) {
	Packet.Write<int32_t>(Gold);
	for(int i = _Object::INVENTORY_TRADE; i < _Object::INVENTORY_COUNT; i++) {
		if(Player->Inventory[i].Item) {
			Packet.Write<int32_t>(Player->Inventory[i].Item->ID);
			Packet.Write<char>(Player->Inventory[i].Count);
		}
		else
			Packet.Write<int32_t>(0);
	}
}

// Sends information to another player about items they're trading
void _Server::SendTradeInformation(_Object *Sender, _Object *Receiver) {

	// Send items to trader player
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::TRADE_REQUEST);
	Packet.Write<NetworkIDType>(Sender->NetworkID);
	BuildTradeItemsPacket(Sender, Packet, Sender->TradeGold);
	Network->SendPacket(Packet, Receiver->Peer);
}

// Start a battle event
void _Server::StartBattle(_Object *Object, int Zone) {
	Zone = 4;
	if(!Zone)
		return;

	// Get monsters
	std::list<int> MonsterIDs;
	Stats->GenerateMonsterListFromZone(Zone, MonsterIDs);
	if(MonsterIDs.size()) {

		// Create a new battle instance
		_Battle *Battle = new _Battle();
		Battle->Stats = Stats;
		Battle->Server = this;
		Battle->Scripting = Scripting;
		Battles.push_back(Battle);

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
				if(PlayersAdded == 2)
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
