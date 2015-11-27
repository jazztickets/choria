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
#include <save.h>
#include <database.h>
#include <buffer.h>
#include <packet.h>
#include <stats.h>
#include <constants.h>
#include <config.h>
#include <SDL_timer.h>
#include <enet/enet.h>

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
	Stats(nullptr),
	Network(new _ServerNetwork(32, NetworkPort)),
	NextMapID(0),
	Thread(nullptr) {

	Log.Open((Config.ConfigPath + "server.log").c_str());

	if(!Network->HasConnection())
		throw std::runtime_error("Unable to bind address!");

	Network->SetFakeLag(Config.FakeLag);
	Network->SetUpdatePeriod(Config.NetworkRate);

	Save = new _Save();
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
	//Log << "ServerUpdate " << Time << std::endl;

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
	for(auto &Map : Maps)
		Map->Update(FrameTime);

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
}

// Handle client connect
void _Server::HandleConnect(_NetworkEvent &Event) {
	char Buffer[16];
	ENetAddress *Address = &Event.Peer->ENetPeer->address;
	enet_address_get_host_ip(Address, Buffer, 16);
	Log << "Connect: " << Buffer << ":" << Address->port << std::endl;

	// Send game version
	_Buffer Packet;
	Packet.Write<char>(Packet::VERSION);
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
		}

		// Save player
		Save->SavePlayer(Player);

		Player->Deleted = true;
	}

	// Leave trading screen
	/*_Object *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {
		TradePlayer->TradePlayer = nullptr;

		_Buffer Packet;
		Packet.Write<char>(Packet::TRADE_CANCEL);
		OldServerNetwork->SendPacketToPeer(&Packet, TradePlayer->OldPeer);
	}

	// Remove from battle
	RemovePlayerFromBattle(Player);

	// Save character info
	Player->Save();
	*/

	// Delete peer from network
	Network->DeletePeer(Event.Peer);
}

// Handle packet data
void _Server::HandlePacket(_Buffer &Data, _Peer *Peer) {
	char PacketType = Data.Read<char>();

	//std::cout << (int)PacketType << std::endl;
	switch(PacketType) {
		case Packet::ACCOUNT_LOGININFO:
			HandleLoginInfo(Data, Peer);
		break;
		case Packet::CHARACTERS_REQUEST:
			HandleCharacterListRequest(Data, Peer);
		break;
		case Packet::CHARACTERS_PLAY:
			HandleCharacterPlay(Data, Peer);
		break;
		case Packet::CREATECHARACTER_INFO:
			HandleCharacterCreate(Data, Peer);
		break;
		case Packet::CHARACTERS_DELETE:
			HandleCharacterDelete(Data, Peer);
		break;
		case Packet::WORLD_MOVECOMMAND:
			HandleMoveCommand(Data, Peer);
		break;
		case Packet::CHAT_MESSAGE:
			HandleChatMessage(Data, Peer);
		break;
		/*
		case Packet::BATTLE_COMMAND:
			HandleBattleCommand(Data, Peer);
		break;
		case Packet::BATTLE_CLIENTDONE:
			HandleBattleFinished(Data, Peer);
		break;
		case Packet::INVENTORY_MOVE:
			HandleInventoryMove(Data, Peer);
		break;
		case Packet::INVENTORY_USE:
			HandleInventoryUse(Data, Peer);
		break;
		case Packet::INVENTORY_SPLIT:
			HandleInventorySplit(Data, Peer);
		break;
		case Packet::EVENT_END:
			HandleEventEnd(Data, Peer);
		break;
		case Packet::VENDOR_EXCHANGE:
			HandleVendorExchange(Data, Peer);
		break;
		case Packet::TRADER_ACCEPT:
			HandleTraderAccept(Data, Peer);
		break;
		case Packet::HUD_ACTIONBAR:
			HandleActionBar(Data, Peer);
		break;
		case Packet::SKILLS_SKILLADJUST:
			HandleSkillAdjust(Data, Peer);
		break;
		case Packet::WORLD_BUSY:
			HandlePlayerBusy(Data, Peer);
		break;
		case Packet::WORLD_ATTACKPLAYER:
			HandleAttackPlayer(Data, Peer);
		break;
		case Packet::WORLD_TELEPORT:
			HandleTeleport(Data, Peer);
		break;
		case Packet::TRADE_REQUEST:
			HandleTradeRequest(Data, Peer);
		break;
		case Packet::TRADE_CANCEL:
			HandleTradeCancel(Data, Peer);
		break;
		case Packet::TRADE_GOLD:
			HandleTradeGold(Data, Peer);
		break;
		case Packet::TRADE_ACCEPT:
			HandleTradeAccept(Data, Peer);
		break;*/
	}
}

// Login information
void _Server::HandleLoginInfo(_Buffer &Data, _Peer *Peer) {

	// Read packet
	bool CreateAccount = Data.ReadBit();
	std::string Username(Data.ReadString());
	std::string Password(Data.ReadString());

	std::stringstream Query;

	// Create account or login
	if(CreateAccount) {

		// Check for existing account
		Query << "SELECT id FROM account WHERE username = '" << Username << "'";
		Save->Database->RunDataQuery(Query.str());
		int Result = Save->Database->FetchRow();
		Save->Database->CloseQuery();
		Query.str("");

		if(Result) {
			_Buffer NewPacket;
			NewPacket.Write<char>(Packet::ACCOUNT_EXISTS);
			Network->SendPacket(NewPacket, Peer);
			return;
		}
		else {
			Query << "INSERT INTO account(username, password) VALUES('" << Username << "', '" << Password << "')";
			Save->Database->RunQuery(Query.str());
			Query.str("");
		}
	}

	// Get account information
	Query << "SELECT id FROM account WHERE username = '" << Username << "' AND password = '" << Password << "'";
	Save->Database->RunDataQuery(Query.str());
	if(Save->Database->FetchRow()) {
		Peer->AccountID = Save->Database->GetInt(0);
	}
	Save->Database->CloseQuery();
	Query.str("");

	// Make sure account exists
	if(Peer->AccountID == 0) {
		_Buffer Packet;
		Packet.Write<char>(Packet::ACCOUNT_NOTFOUND);
		Network->SendPacket(Packet, Peer);
	}
	else {

		_Buffer Packet;
		Packet.Write<char>(Packet::ACCOUNT_SUCCESS);
		Network->SendPacket(Packet, Peer);
	}
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
	int PortraitID = Data.Read<int32_t>();
	if(Name.size() > PLAYER_NAME_SIZE)
		return;

	std::stringstream Query;

	// Check character limit
	Query << "SELECT count(id) FROM character WHERE account_id = " << Peer->AccountID;
	int CharacterCount = Save->Database->RunCountQuery(Query.str());
	if(CharacterCount >= SAVE_COUNT)
		return;
	Query.str("");

	// Check for existing names
	Query << "SELECT id FROM character WHERE name = '" << Name << "'";
	Save->Database->RunDataQuery(Query.str());
	int FindResult = Save->Database->FetchRow();
	Save->Database->CloseQuery();
	Query.str("");

	// Found an existing name
	if(FindResult) {
		_Buffer NewPacket;
		NewPacket.Write<char>(Packet::CREATECHARACTER_INUSE);
		Network->SendPacket(NewPacket, Peer);
		return;
	}

	// Create the character
	Save->Database->RunQuery("BEGIN TRANSACTION");
	Query << "INSERT INTO character(account_id, name, portrait_id, actionbar0) VALUES(" << Peer->AccountID << ", '" << Name << "', " << PortraitID << ", 1)";
	Save->Database->RunQuery(Query.str());
	Query.str("");

	int CharacterID = Save->Database->GetLastInsertID();
	Query << "INSERT INTO inventory VALUES(" << CharacterID << ", 1, 2, 1)";
	Save->Database->RunQuery(Query.str());
	Query.str("");

	Query << "INSERT INTO inventory VALUES(" << CharacterID << ", 3, 1, 1)";
	Save->Database->RunQuery(Query.str());
	Query.str("");

	Query << "INSERT INTO skilllevel VALUES(" << CharacterID << ", 1, 1)";
	Save->Database->RunQuery(Query.str());
	Query.str("");

	Save->Database->RunQuery("END TRANSACTION");

	// Notify the client
	_Buffer NewPacket;
	NewPacket.Write<char>(Packet::CREATECHARACTER_SUCCESS);
	Network->SendPacket(NewPacket, Peer);
}

// Handle a character delete request
void _Server::HandleCharacterDelete(_Buffer &Data, _Peer *Peer) {
	if(!Peer->AccountID)
		return;

	std::stringstream Query;

	// Get delete slot
	int Index = Data.Read<char>();
	int CharacterID = 0;

	// Get character ID
	Query << "SELECT id FROM character WHERE account_id = " << Peer->AccountID << " LIMIT " << Index << ", 1";
	Save->Database->RunDataQuery(Query.str());
	if(Save->Database->FetchRow()) {
		CharacterID = Save->Database->GetInt(0);
	}
	Save->Database->CloseQuery();
	Query.str("");

	// Delete character
	Query << "DELETE FROM character WHERE id = " << CharacterID;
	Save->Database->RunQuery(Query.str());
	Query.str("");

	// Delete items
	Query << "DELETE FROM inventory WHERE character_id = " << CharacterID;
	Save->Database->RunQuery(Query.str());
	Query.str("");

	// Delete skill levels
	Query << "DELETE FROM skilllevel WHERE character_id = " << CharacterID;
	Save->Database->RunQuery(Query.str());
	Query.str("");

	// Update the player
	SendCharacterList(Peer);
}

// Loads the player, updates the world, notifies clients
void _Server::HandleCharacterPlay(_Buffer &Data, _Peer *Peer) {

	// Read packet
	int Slot = Data.Read<char>();
	int MapID = 0;
	//int SpawnPoint = 0;

	// Get character info
	std::stringstream Query;
	Query << "SELECT id, map_id, spawnpoint FROM character WHERE account_id = " << Peer->AccountID << " LIMIT " << Slot << ", 1";
	Save->Database->RunDataQuery(Query.str());
	if(Save->Database->FetchRow()) {
		Peer->CharacterID = Save->Database->GetInt(0);
		MapID = Save->Database->GetInt(1);
		//SpawnPoint = Save->Database->GetInt(2);
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
}

// Handles move commands from a client
void _Server::HandleMoveCommand(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;
	Player->InputState = Data.Read<char>();
	/*
	if(Player->MovePlayer(Direction)) {


	}*/

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
	Packet.Write<char>(Packet::CHAT_MESSAGE);
	Packet.Write<glm::vec4>(COLOR_WHITE);
	Packet.WriteString(Message.c_str());

	// Broadcast message
	for(auto &ReceivePeer : Network->GetPeers()) {
		Network->SendPacket(Packet, ReceivePeer);
	}
}

// Send character list
void _Server::SendCharacterList(_Peer *Peer) {
	std::stringstream Query;

	// Get a count of the account's characters
	Query << "SELECT count(id) FROM character WHERE account_id = " << Peer->AccountID;
	int CharacterCount = Save->Database->RunCountQuery(Query.str());
	Query.str("");

	// Create the packet
	_Buffer Packet;
	Packet.Write<char>(Packet::CHARACTERS_LIST);
	Packet.Write<char>(CharacterCount);

	// Generate a list of characters
	Query << "SELECT name, portrait_id, experience FROM character WHERE account_id = " << Peer->AccountID;
	Save->Database->RunDataQuery(Query.str());
	while(Save->Database->FetchRow()) {
		Packet.WriteString(Save->Database->GetString(0));
		Packet.Write<int32_t>(Save->Database->GetInt(1));
		Packet.Write<int32_t>(Save->Database->GetInt(2));
	}
	Save->Database->CloseQuery();
	Query.str("");

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

		// Delete old player
		if(Player)
			Player->Deleted = true;

		// Create new player
		Player = CreatePlayer(Peer);
		Player->NetworkID = Map->GenerateObjectID();
		Player->Map = Map;
		Player->State = _Object::STATE_NONE;

		// Find spawn point in map
		int SpawnPoint = Player->SpawnPoint;
		if(EventType == _Map::EVENT_MAPCHANGE)
			SpawnPoint = OldMap->ID;
		Map->FindEvent(EventType, SpawnPoint, Player->Position);

		// Add player to map
		Map->AddObject(Player);

		// Send new map id
		_Buffer Packet;
		Packet.Write<char>(Packet::WORLD_CHANGEMAPS);
		Packet.Write<int32_t>(MapID);
		Network->SendPacket(Packet, Peer);

		// Add peer to map
		Map->AddPeer(Peer);

		// Set player object list
		Map->SendObjectList(Peer);

		// Set full player data to peer
		SendPlayerInfo(Peer);
	}
	else {
		//Map->FindEvent(EventType, EventData, Player->Position);

		//SendPlayerPosition(Player);
	}
}

// Gets a map from the manager. Loads the level if it doesn't exist
_Map *_Server::GetMap(int MapID) {

	// Loop through loaded maps
	for(auto &Map : Maps) {

		// Check id
		if(Map->ID == MapID)
			return Map;
	}

	// Not found, so create it
	_Map *NewMap = new _Map(MapID, Stats);
	NewMap->Server = this;
	Maps.push_back(NewMap);

	return NewMap;
}

// Create player object and load stats from save
_Object *_Server::CreatePlayer(_Peer *Peer) {

	// Create object
	_Object *Player = new _Object();
	Player->Peer = Peer;
	Player->Stats = Stats;
	Peer->Object = Player;

	std::stringstream Query;

	// Get character
	Query << "SELECT * FROM character WHERE id = " << Peer->CharacterID;
	Save->Database->RunDataQuery(Query.str());
	Query.str("");

	// Get row
	if(!Save->Database->FetchRow()) {
		Save->Database->CloseQuery();
		return nullptr;
	}

	// Set properties
	Player->CharacterID = Save->Database->GetInt(0);
	Player->SpawnMapID = Save->Database->GetInt(3);
	Player->SpawnPoint = Save->Database->GetInt(4);
	Player->Name = Save->Database->GetString(5);
	Player->PortraitID = Save->Database->GetInt(6);
	Player->Experience = Save->Database->GetInt(7);
	Player->Gold = Save->Database->GetInt(8);
	for(int i = 0; i < ACTIONBAR_SIZE; i++) {
		uint32_t SkillID = Save->Database->GetInt(i + 9);
		Player->ActionBar[i] = Stats->Skills[SkillID];
	}
	Player->PlayTime = Save->Database->GetInt(17);
	Player->Deaths = Save->Database->GetInt(18);
	Player->MonsterKills = Save->Database->GetInt(19);
	Player->PlayerKills = Save->Database->GetInt(20);
	Player->Bounty = Save->Database->GetInt(21);
	Save->Database->CloseQuery();

	// Set inventory
	Query << "SELECT slot, item_id, count FROM inventory WHERE character_id = " << Player->CharacterID;
	Save->Database->RunDataQuery(Query.str());
	int ItemCount = 0;
	while(Save->Database->FetchRow()) {
		Player->SetInventory(Save->Database->GetInt(0), Save->Database->GetInt(1), Save->Database->GetInt(2));
		ItemCount++;
	}
	Save->Database->CloseQuery();
	Query.str("");

	// Set skills
	Query << "SELECT skill_id, level FROM skilllevel WHERE character_id = " << Player->CharacterID;
	Save->Database->RunDataQuery(Query.str());
	int SkillCount = 0;
	while(Save->Database->FetchRow()) {
		int SkillLevel = Save->Database->GetInt(1);
		Player->SetSkillLevel(Save->Database->GetInt(0), SkillLevel);
		if(SkillLevel > 0)
			SkillCount++;
	}
	Save->Database->CloseQuery();
	Query.str("");

	// Get stats
	Player->CalculatePlayerStats();
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
	Packet.Write<char>(Packet::WORLD_YOURCHARACTERINFO);
	Packet.WriteString(Player->Name.c_str());
	Packet.Write<int32_t>(Player->PortraitID);
	Packet.Write<int32_t>(Player->Experience);
	Packet.Write<int32_t>(Player->Gold);
	Packet.Write<int32_t>(Player->PlayTime);
	Packet.Write<int32_t>(Player->Deaths);
	Packet.Write<int32_t>(Player->MonsterKills);
	Packet.Write<int32_t>(Player->PlayerKills);
	Packet.Write<int32_t>(Player->Bounty);

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
	for(int i = 0; i < ACTIONBAR_SIZE; i++) {
		Packet.Write<uint32_t>(Player->GetActionBarID(i));
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

// Send position to player
void _Server::SendPlayerPosition(_Object *Player) {

	_Buffer Packet;
	Packet.Write<char>(Packet::WORLD_POSITION);
	Packet.Write<glm::ivec2>(Player->Position);

	Network->SendPacket(Packet, Player->Peer);
}

/*
// Handles battle commands from a client
void _Server::HandleBattleCommand(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	_ServerBattle *Battle = (_ServerBattle *)Player->Battle;
	if(!Battle)
		return;

	int Command = Data.Read<char>();
	int Target = Data.Read<char>();
	Battle->HandleInput(Player, Command, Target);

	//printf("HandleBattleCommand: %d\n", Command);
}

// The client is done with the battle results screen
void _Server::HandleBattleFinished(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	_ServerBattle *Battle = (_ServerBattle *)Player->Battle;
	if(!Battle)
		return;

	// Check for the last player leaving the battle
	RemovePlayerFromBattle(Player);

	// Check for death
	if(Player->Health == 0) {
		Player->RestoreHealthMana();
		SpawnPlayer(Player, Player->SpawnMapID, _Map::EVENT_SPAWN, Player->SpawnPoint);
		Player->Save();
	}

	// Send updates
	SendHUD(Player);
}

// Handles a player's inventory move
void _Server::HandleInventoryMove(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	int OldSlot = Data.Read<char>();
	int NewSlot = Data.Read<char>();

	// Move items
	Player->MoveInventory(OldSlot, NewSlot);
	Player->CalculatePlayerStats();

	// Check for trading players
	_Object *TradePlayer = Player->TradePlayer;
	if(Player->State == _Object::STATE_TRADE && TradePlayer && (_Object::IsSlotTrade(OldSlot) || _Object::IsSlotTrade(NewSlot))) {

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
		_Buffer NewPacket;
		NewPacket.Write<char>(Packet::TRADE_ITEM);
		NewPacket.Write<int32_t>(OldItemID);
		NewPacket.Write<char>(OldSlot);
		if(OldItemID > 0)
			NewPacket.Write<char>(OldItemCount);
		NewPacket.Write<int32_t>(NewItemID);
		NewPacket.Write<char>(NewSlot);
		if(NewItemID > 0)
			NewPacket.Write<char>(NewItemCount);

		// Send updates
		OldServerNetwork->SendPacketToPeer(&NewPacket, TradePlayer->OldPeer);
	}
}

// Handle a player's inventory use request
void _Server::HandleInventoryUse(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	// Use an item
	Player->UseInventory(Packet->Read<char>());
}

// Handle a player's inventory split stack request
void _Server::HandleInventorySplit(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	int Slot = Data.Read<char>();
	int Count = Data.Read<char>();

	// Inventory only
	if(!_Object::IsSlotInventory(Slot))
		return;

	Player->SplitStack(Slot, Count);
}

// Handles a player's event end message
void _Server::HandleEventEnd(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	Player->Vendor = nullptr;
	Player->State = _Object::STATE_NONE;
}

// Handles a vendor exchange message
void _Server::HandleVendorExchange(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

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
		Player->CalculatePlayerStats();
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

// Handle a skill bar change
void _Server::HandleActionBar(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	// Read skills
	for(int i = 0; i < ACTIONBAR_SIZE; i++) {
		Player->SetActionBar(i, Stats->GetSkill(Packet->Read<char>()));
	}

	Player->CalculatePlayerStats();
}

// Handles a skill adjust
void _Server::HandleSkillAdjust(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	// Process packet
	bool Spend = Data.ReadBit();
	int SkillID = Data.Read<char>();
	if(Spend) {
		Player->AdjustSkillLevel(SkillID, 1);
	}
	else {
		Player->AdjustSkillLevel(SkillID, -1);
	}

	Player->CalculateSkillPoints();
	Player->CalculatePlayerStats();
}

// Handles a player's request to not start a battle with other players
void _Server::HandlePlayerBusy(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	bool Value = Data.ReadBit();
	Player->SetBusy(Value);

	//printf("HandlePlayerBusy: Value=%d\n", Value);
}

// Handles a player's request to attack another player
void _Server::HandleAttackPlayer(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
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

// Handle a trade request
void _Server::HandleTradeRequest(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	// Get map object
	_Map *Map = Player->Map;
	if(!Map)
		return;

	// Find the nearest player to trade with
	_Object *TradePlayer = Map->FindTradePlayer(Player, 2.0f * 2.0f);
	if(TradePlayer == nullptr) {

		// Set up trade post
		Player->State = _Object::STATE_TRADE;
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

		Player->State = _Object::STATE_TRADE;
		TradePlayer->State = _Object::STATE_TRADE;
	}
}

// Handles a trade cancel
void _Server::HandleTradeCancel(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	// Notify trading player
	_Object *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {
		TradePlayer->State = _Object::STATE_TRADE;
		TradePlayer->TradePlayer = nullptr;
		TradePlayer->TradeAccepted = false;

		_Buffer NewPacket;
		NewPacket.Write<char>(Packet::TRADE_CANCEL);
		OldServerNetwork->SendPacketToPeer(&NewPacket, TradePlayer->OldPeer);
	}

	// Set state back to normal
	Player->State = _Object::STATE_NONE;
}

// Handle a trade gold update
void _Server::HandleTradeGold(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

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

		_Buffer NewPacket;
		NewPacket.Write<char>(Packet::TRADE_GOLD);
		NewPacket.Write<int32_t>(Gold);
		OldServerNetwork->SendPacketToPeer(&NewPacket, TradePlayer->OldPeer);
	}
}

// Handles a trade accept from a player
void _Server::HandleTradeAccept(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	// Get trading player
	_Object *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {

		// Set the player's state
		bool Accepted = !!Packet->Read<char>();
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
				_Buffer NewPacket;
				NewPacket.Write<char>(Packet::TRADE_EXCHANGE);
				BuildTradeItemsPacket(Player, &NewPacket, Player->Gold);
				OldServerNetwork->SendPacketToPeer(&NewPacket, Player->OldPeer);
			}
			{
				_Buffer NewPacket;
				NewPacket.Write<char>(Packet::TRADE_EXCHANGE);
				BuildTradeItemsPacket(TradePlayer, &NewPacket, TradePlayer->Gold);
				OldServerNetwork->SendPacketToPeer(&NewPacket, TradePlayer->OldPeer);
			}

			Player->State = _Object::STATE_NONE;
			Player->TradePlayer = nullptr;
			Player->TradeGold = 0;
			Player->MoveTradeToInventory();
			TradePlayer->State = _Object::STATE_NONE;
			TradePlayer->TradePlayer = nullptr;
			TradePlayer->TradeGold = 0;
			TradePlayer->MoveTradeToInventory();
		}
		else {

			// Notify trading player
			_Buffer NewPacket;
			NewPacket.Write<char>(Packet::TRADE_ACCEPT);
			NewPacket.Write<char>(Accepted);
			OldServerNetwork->SendPacketToPeer(&NewPacket, TradePlayer->OldPeer);
		}
	}
}

// Handles a teleport request
void _Server::HandleTeleport(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	Player->StartTeleport();
}

// Handles a trader accept
void _Server::HandleTraderAccept(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	const _Trader *Trader = Player->Trader;
	if(!Trader)
		return;

	// Get trader information
	int RequiredItemSlots[8];
	int RewardSlot = Player->GetRequiredItemSlots(RequiredItemSlots);
	if(RewardSlot == -1)
		return;

	// Exchange items
	Player->AcceptTrader(RequiredItemSlots, RewardSlot);
	Player->Trader = nullptr;
	Player->State = _Object::STATE_NONE;
	Player->CalculatePlayerStats();
}

// Send a message to the player
void _OldServerState::SendMessage(_Object *Player, const std::string &Message, const glm::vec4 &Color) {
	if(!Player)
		return;

	// Build message
	_Buffer Packet;
	Packet.Write<char>(Packet::CHAT_MESSAGE);
	Packet.Write<glm::vec4>(Color);
	Packet.WriteString(Message.c_str());

	// Send
	Network->SendPacket(Packet, Peer);
}

// Updates the player's HUD
void _OldServerState::SendHUD(_Object *Player) {

	_Buffer Packet;
	Packet.Write<char>(Packet::WORLD_HUD);
	Packet.Write<int32_t>(Player->Experience);
	Packet.Write<int32_t>(Player->Gold);
	Packet.Write<int32_t>(Player->Health);
	Packet.Write<int32_t>(Player->Mana);
	Packet.Write<float>(Player->HealthAccumulator);
	Packet.Write<float>(Player->ManaAccumulator);

	Network->SendPacket(Packet, Peer);
}

// Sends information to another player about items they're trading
void _OldServerState::SendTradeInformation(_Object *Sender, _Object *Receiver) {

	// Send items to trader player
	_Buffer Packet;
	Packet.Write<char>(Packet::TRADE_REQUEST);
	Packet.Write<NetworkIDType>(Sender->NetworkID);
	BuildTradeItemsPacket(Sender, &Packet, Sender->TradeGold);
	OldServerNetwork->SendPacketToPeer(&Packet, Receiver->OldPeer);
}

// Adds trade item information to a packet
void _OldServerState::BuildTradeItemsPacket(_Object *Player, _Buffer *Packet, int Gold) {
	Packet->Write<int32_t>(Gold);
	for(int i = _Object::INVENTORY_TRADE; i < _Object::INVENTORY_COUNT; i++) {
		if(Player->Inventory[i].Item) {
			Packet->Write<int32_t>(Player->Inventory[i].Item->ID);
			Packet->Write<char>(Player->Inventory[i].Count);
		}
		else
			Packet->Write<int32_t>(0);
	}
}

// Removes a player from a battle and deletes the battle if necessary
void _OldServerState::RemovePlayerFromBattle(_Object *Player) {
	_ServerBattle *Battle = (_ServerBattle *)Player->Battle;
	if(!Battle)
		return;

	// Delete instance
	if(Battle->RemoveFighter(Player) == 0) {

		// Loop through loaded battles
		for(auto BattleIterator = Battles.begin(); BattleIterator != Battles.end(); ++BattleIterator) {
			if(*BattleIterator == Battle) {
				Battles.erase(BattleIterator);
				delete Battle;
				return;
			}
		}
	}
}

// Teleports a player back to town
void _OldServerState::PlayerTeleport(_Object *Player) {
	Player->RestoreHealthMana();
	SpawnPlayer(Player, Player->SpawnMapID, _Map::EVENT_SPAWN, Player->SpawnPoint);
	SendHUD(Player);
	Player->Save();
}
*/