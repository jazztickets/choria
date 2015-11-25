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
	Stats(nullptr),
	Network(new _ServerNetwork(NetworkPort)),
	NextMapID(0),
	Thread(nullptr) {

	if(!Network->HasConnection())
		throw std::runtime_error("Unable to bind address!");

	Network->SetFakeLag(Config.FakeLag);
	Network->SetUpdatePeriod(Config.NetworkRate);
	Log.Open((Config.ConfigPath + "server.log").c_str());
	//Log.SetToStdOut(true);

	Database = new _Database();

	std::string DatabasePath = Config.ConfigPath + "save.dat";
	if(!Database->OpenDatabase(DatabasePath.c_str())) {

		// Create a new database
		if(!Database->OpenDatabaseCreate(DatabasePath.c_str()))
			throw std::runtime_error("OpenDatabaseCreate failed");

		// Populate data
		CreateDefaultDatabase();
	}
}

// Destructor
_Server::~_Server() {
	Done = true;
	if(Thread)
		Thread->join();

	// Delete maps
	for(auto &Map : Maps) {
		delete Map;
	}

	delete Database;

	delete Thread;
}

// Start the server thread
void _Server::StartThread() {
	Thread = new std::thread(RunThread, this);
}

// Stop the server
void _Server::StopServer() {
	Network->DisconnectAll();
	StartShutdown = true;
}

// Update
void _Server::Update(double FrameTime) {
	//Log << "ServerUpdate " << TimeSteps << std::endl;

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

	// Check if updates should be sent
	if(Network->NeedsUpdate()) {
		//Log << "NeedsUpdate " << TimeSteps << std::endl;
		Network->ResetUpdateTimer();
		if(0 && (rand() % 10) == 0) {
			//printf("droppin pack\n");
		}
		else if(Network->GetPeers().size() > 0) {

			// Notify
			/*
			for(auto &Map : Maps) {
				_Buffer Buffer;
				Buffer.Write<char>(Packet::OBJECT_UPDATES);
				Buffer.Write<uint8_t>(Map->ID);
				Buffer.Write<uint16_t>(TimeSteps);
				Map->BuildObjectUpdate(Buffer, TimeSteps);

				const std::list<const _Peer *> Peers = Map->GetPeers();
				for(auto &Peer : Peers) {
					Network->SendPacket(Buffer, Peer, _Network::UNSEQUENCED, 1);
				}
			}*/
		}
	}

	if(StartShutdown && Network->GetPeers().size() == 0) {
		Done = true;
	}

	TimeSteps++;
	Time += FrameTime;
}

// Handle client connect
void _Server::HandleConnect(_NetworkEvent &Event) {
	//Log << TimeSteps << " -- connect peer_count=" << (int)Network->GetPeers().size() << std::endl;
}

// Handle client disconnect
void _Server::HandleDisconnect(_NetworkEvent &Event) {

	// Get object
	_Object *Object = Event.Peer->Object;
	if(!Object)
		return;

	// Update map
	//_Map *Map = Object->Map;
	//if(Map) {
	//	Map->RemovePeer(Event.Peer);
	//}

	// Remove from list
	//Object->Deleted = true;
	delete Object;

	// Delete peer from network
	Network->DeletePeer(Event.Peer);
}

// Handle packet data
void _Server::HandlePacket(_Buffer &Data, _Peer *Peer) {
	char PacketType = Data.Read<char>();

	switch(PacketType) {
		case Packet::ACCOUNT_LOGININFO:
			HandleLoginInfo(Data, Peer);
		break;
		case Packet::CHARACTERS_REQUEST:
			HandleCharacterListRequest(Data, Peer);
		break;
			/*
		case Packet::CHARACTERS_PLAY:
			HandleCharacterSelect(Data, Peer);
		break;
		case Packet::CHARACTERS_DELETE:
			HandleCharacterDelete(Data, Peer);
		break;
		case Packet::CREATECHARACTER_INFO:
			HandleCharacterCreate(Data, Peer);
		break;
		case Packet::WORLD_MOVECOMMAND:
			HandleMoveCommand(Data, Peer);
		break;
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
		case Packet::SKILLS_SKILLBAR:
			HandleSkillBar(Data, Peer);
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
		case Packet::CHAT_MESSAGE:
			HandleChatMessage(Data, Peer);
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
	if(Username.size() > 15 || Password.size() > 15)
		return;

	// Create account or login
	if(CreateAccount) {

		// Check for existing account
		std::string Query = "SELECT ID FROM Accounts WHERE Username = '" + Username + "'";
		Database->RunDataQuery(Query);
		int Result = Database->FetchRow();
		Database->CloseQuery();

		if(Result) {
			_Buffer NewPacket;
			NewPacket.Write<char>(Packet::ACCOUNT_EXISTS);
			Network->SendPacket(NewPacket, Peer);
			return;
		}
		else {
			std::string Query = "INSERT INTO Accounts(Username, Password) VALUES('" + Username + "', '" + Password + "')";
			Database->RunQuery(Query);
		}
	}

	// Get account information
	int AccountID = 0;
	std::string Query = "SELECT ID FROM Accounts WHERE Username = '" + Username + "' AND Password = '" + Password + "'";
	Database->RunDataQuery(Query);
	if(Database->FetchRow()) {
		AccountID = Database->GetInt(0);
	}
	Database->CloseQuery();

	// Make sure account exists
	if(AccountID == 0) {
		_Buffer Packet;
		Packet.Write<char>(Packet::ACCOUNT_NOTFOUND);
		Network->SendPacket(Packet, Peer);
	}
	else {

		// Create object
		_Object *Object = new _Object(_Object::PLAYER);
		Object->AccountID = AccountID;
		Object->Peer = Peer;
		Peer->Object = Object;

		_Buffer Packet;
		Packet.Write<char>(Packet::ACCOUNT_SUCCESS);
		Network->SendPacket(Packet, Peer);
	}
}
// Sends a player his/her character list
void _Server::HandleCharacterListRequest(_Buffer &Data, _Peer *Peer) {
	_Object *Player = Peer->Object;
	if(!Player)
		return;

	std::stringstream Query;

	// Get a count of the account's characters
	Query << "SELECT Count(ID) FROM Characters WHERE AccountsID = " << Player->AccountID;
	int CharacterCount = Database->RunCountQuery(Query.str());
	Query.str("");

	// Create the packet
	_Buffer NewPacket;
	NewPacket.Write<char>(Packet::CHARACTERS_LIST);
	NewPacket.Write<char>(CharacterCount);

	// Generate a list of characters
	Query << "SELECT Name, PortraitID, Experience FROM Characters WHERE AccountsID = " << Player->AccountID;
	Database->RunDataQuery(Query.str());
	while(Database->FetchRow()) {
		NewPacket.WriteString(Database->GetString(0));
		NewPacket.Write<int32_t>(Database->GetInt(1));
		NewPacket.Write<int32_t>(Database->GetInt(2));
	}
	Database->CloseQuery();
	Query.str("");

	// Send list
	Network->SendPacket(NewPacket, Peer);
}

// Loads the player, updates the world, notifies clients
void _Server::HandleCharacterSelect(_Buffer &Data, _Peer *Peer) {

	int Slot = Data.Read<char>();
	_Object *Player = Peer->Object;
	//printf("HandleCharacterSelect: accountid=%d, slot=%d\n", Player->AccountID, Slot);

	// Check account
	if(Player->AccountID <= 0) {
		return;
	}

	// Get character info
	std::stringstream Query;
	Query << "SELECT * FROM Characters WHERE AccountsID = " << Player->AccountID << " LIMIT " << Slot<< ", 1";
	Database->RunDataQuery(Query.str());
	if(!Database->FetchRow()) {
		printf(" Didn't find a character for slot %d\n", Slot);
		return;
	}
	Query.str("");

	// Set player properties
	/*
	Player->Database = Database;
	Player->CharacterID = Database->GetInt(0);
	Player->SpawnMapID = Database->GetInt(2);
	Player->SpawnPoint = Database->GetInt(3);
	Player->Name = Database->GetString(4);
	Player->SetPortraitID(Database->GetInt(5));
	Player->Experience = Database->GetInt(6);
	Player->Gold = Database->GetInt(7);
	for(int i = 0; i < 8; i++)
		Player->SetSkillBar(i, OldStats.GetSkill(Database->GetInt(i + 8)));
	Player->PlayTime = Database->GetInt(16);
	Player->Deaths = Database->GetInt(17);
	Player->MonsterKills = Database->GetInt(18);
	Player->PlayerKills = Database->GetInt(19);
	Player->Bounty = Database->GetInt(20);

	Database->CloseQuery();

	// Set inventory
	sprintf(Query.str(), "SELECT Slot, ItemsID, Count FROM Inventory WHERE CharactersID = %d", Player->CharacterID);
	Database->RunDataQuery(Query.str());
	int ItemCount = 0;
	while(Database->FetchRow()) {
		Player->SetInventory(Database->GetInt(0), Database->GetInt(1), Database->GetInt(2));
		ItemCount++;
	}
	Database->CloseQuery();

	// Set skills
	sprintf(Query.str(), "SELECT SkillsID, Level FROM SkillLevel WHERE CharactersID = %d", Player->CharacterID);
	Database->RunDataQuery(Query.str());
	int SkillCount = 0;
	while(Database->FetchRow()) {
		int SkillLevel = Database->GetInt(1);
		Player->SetSkillLevel(Database->GetInt(0), SkillLevel);
		if(SkillLevel > 0)
			SkillCount++;
	}
	Database->CloseQuery();

	// Get stats
	Player->CalculatePlayerStats();
	Player->RestoreHealthMana();

	// Send character packet
	_Buffer NewPacket;
	NewPacket.Write<char>(Packet::WORLD_YOURCHARACTERINFO);
	NewPacket.Write<char>(Player->NetworkID);
	NewPacket.WriteString(Player->Name.c_str());
	NewPacket.Write<int32_t>(Player->PortraitID);
	NewPacket.Write<int32_t>(Player->Experience);
	NewPacket.Write<int32_t>(Player->Gold);
	NewPacket.Write<int32_t>(Player->PlayTime);
	NewPacket.Write<int32_t>(Player->Deaths);
	NewPacket.Write<int32_t>(Player->MonsterKills);
	NewPacket.Write<int32_t>(Player->PlayerKills);
	NewPacket.Write<int32_t>(Player->Bounty);

	// Write items
	NewPacket.Write<char>(ItemCount);
	for(int i = 0; i < _Player::INVENTORY_COUNT; i++) {
		if(Player->Inventory[i].Item) {
			NewPacket.Write<char>(i);
			NewPacket.Write<char>(Player->Inventory[i].Count);
			NewPacket.Write<int32_t>(Player->Inventory[i].Item->ID);
		}
	}

	// Write skills
	NewPacket.Write<char>(SkillCount);
	for(int i = 0; i < _Player::SKILL_COUNT; i++) {
		if(Player->GetSkillLevel(i) > 0) {
			NewPacket.Write<int32_t>(Player->GetSkillLevel(i));
			NewPacket.Write<char>(i);
		}
	}

	// Write skill bar
	for(int i = 0; i < FIGHTER_MAXSKILLS; i++) {
		NewPacket.Write<char>(Player->GetSkillBarID(i));
	}
	OldServerNetwork->SendPacketToPeer(&NewPacket, Peer);

	// Send map and players to new player
	SpawnPlayer(Player, Player->SpawnMapID, _Map::EVENT_SPAWN, Player->SpawnPoint);
	*/
}

// Send map information to a client
void _Server::ChangePlayerMap(const std::string &MapName, _Peer *Peer) {
	_Map *Map = GetMap(MapName);
	if(!Map)
		return;

	// Delete old player
	_Object *OldPlayer = Peer->Object;
	if(OldPlayer)
		OldPlayer->Deleted = true;

	/*
	// Create new player
	_Object *Object = Stats->CreateObject("player", true);
	Object->ID = Map->GenerateObjectID();
	Object->Map = Map;
	Object->Physics->RenderDelay = false;
	if(OldPlayer)
		Object->Physics->Rotation = OldPlayer->Physics->Rotation;
	Object->Physics->ForcePosition(Map->GetStartingPositionByCheckpoint(0));
	Object->Physics->UpdateAutomatically = false;
	Object->Peer = Peer;
	Map->AddObject(Object);
	Map->Grid->AddObject(Object);

	// Create object create packet
	{
		_Buffer Buffer;
		Buffer.Write<char>(Packet::OBJECT_CREATE);
		Buffer.Write<uint8_t>(Map->ID);
		Object->NetworkSerialize(Buffer);

		// Broadcast to all other peers
		Map->BroadcastPacket(Buffer);
	}

	Peer->Object = Object;
	Peer->LastAck = TimeSteps;

	// Send map name
	_Buffer Buffer;
	Buffer.Write<char>(Packet::MAP_INFO);
	Buffer.Write<uint8_t>(Map->ID);
	Buffer.WriteString(MapName.c_str());
	Network->SendPacket(Buffer, Peer);

	// Add peer to map
	Map->AddPeer(Peer);
	Map->SendObjectList(Object, TimeSteps);
	*/
}

// Get a map if it's already loaded, if not load it and return it
_Map *_Server::GetMap(const std::string &MapName) {

	// Search for loaded map
	for(auto &Map : Maps) {
		if(Map->Filename == MapName) {
			return Map;
		}
	}

	/*
	// Load map
	_Map *Map = nullptr;
	try {
		//TODO fix NextMapID
		Map = new _Map(MapName, Stats, true, NextMapID++, Network.get());
	}
	catch(std::exception &Error) {
		Log << TimeSteps << " -- Error loading map: " << MapName << std::endl;
	}

	// Add to list of maps
	if(Map)
		Maps.push_back(Map);

	return Map;*/

	return 0;
}


// Populates the server database with the default data
void _Server::CreateDefaultDatabase() {

	// Server information
	Database->RunQuery("BEGIN TRANSACTION");
	Database->RunQuery("CREATE TABLE ServerInfo('Version' INTEGER)");
	Database->RunQuery("CREATE TABLE Accounts("
						"'ID' INTEGER PRIMARY KEY"
						", 'Username' TEXT"
						", 'Password' TEXT"
						")");
	Database->RunQuery(	"CREATE TABLE Characters("
						"'ID' INTEGER PRIMARY KEY"
						", 'AccountsID' INTEGER DEFAULT(0)"
						", 'MapID' INTEGER DEFAULT(1)"
						", 'SpawnPoint' INTEGER DEFAULT(0)"
						", 'Name' TEXT"
						", 'PortraitID' INTEGER DEFAULT(1)"
						", 'Experience' INTEGER DEFAULT(0)"
						", 'Gold' INTEGER DEFAULT(0)"
						", 'SkillBar0' INTEGER DEFAULT(-1)"
						", 'SkillBar1' INTEGER DEFAULT(-1)"
						", 'SkillBar2' INTEGER DEFAULT(-1)"
						", 'SkillBar3' INTEGER DEFAULT(-1)"
						", 'SkillBar4' INTEGER DEFAULT(-1)"
						", 'SkillBar5' INTEGER DEFAULT(-1)"
						", 'SkillBar6' INTEGER DEFAULT(-1)"
						", 'SkillBar7' INTEGER DEFAULT(-1)"
						", 'PlayTime' INTEGER DEFAULT(0)"
						", 'Deaths' INTEGER DEFAULT(0)"
						", 'MonsterKills' INTEGER DEFAULT(0)"
						", 'PlayerKills' INTEGER DEFAULT(0)"
						", 'Bounty' INTEGER DEFAULT(0)"
						")");
	Database->RunQuery(	"CREATE TABLE Inventory("
						"'CharactersID' INTEGER"
						", 'Slot' INTEGER"
						", 'ItemsID' INTEGER"
						", 'Count' INTEGER"
						")");
	Database->RunQuery(	"CREATE TABLE SkillLevel("
						"'CharactersID' INTEGER"
						", 'SkillsID' INTEGER"
						", 'Level' INTEGER"
						")");

	Database->RunQuery("INSERT INTO ServerInfo(Version) VALUES(1)");
	Database->RunQuery("INSERT INTO Accounts(Username, Password) VALUES('singleplayer', 'singleplayer')");
	Database->RunQuery("END TRANSACTION");
}
