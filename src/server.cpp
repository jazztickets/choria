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
		//Log << "NeedsUpdate " << TimeSteps << std::endl;
		Network->ResetUpdateTimer();
		if(0 && (rand() % 10) == 0) {
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
	if(Username.size() > ACCOUNT_MAX_USERNAME_SIZE || Password.size() > ACCOUNT_MAX_PASSWORD_SIZE)
		return;

	std::stringstream Query;

	// Create account or login
	if(CreateAccount) {

		// Check for existing account
		Query << "SELECT ID FROM Accounts WHERE Username = '" << Username << "'";
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
			Query << "INSERT INTO Accounts(Username, Password) VALUES('" << Username << "', '" << Password << "')";
			Save->Database->RunQuery(Query.str());
			Query.str("");
		}
	}

	// Get account information
	Query << "SELECT ID FROM Accounts WHERE Username = '" << Username << "' AND Password = '" << Password << "'";
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
	Query << "SELECT Count(ID) FROM Characters WHERE AccountsID = " << Peer->AccountID;
	int CharacterCount = Save->Database->RunCountQuery(Query.str());
	if(CharacterCount >= SAVE_COUNT)
		return;
	Query.str("");

	// Check for existing names
	Query << "SELECT ID FROM Characters WHERE Name = '" << Name << "'";
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
	Query << "INSERT INTO Characters(AccountsID, Name, PortraitID, ActionBar0) VALUES(" << Peer->AccountID << ", '" << Name << "', " << PortraitID << ", 1)";
	Save->Database->RunQuery(Query.str());
	Query.str("");

	int CharacterID = Save->Database->GetLastInsertID();
	Query << "INSERT INTO Inventory VALUES(" << CharacterID << ", 1, 2, 1)";
	Save->Database->RunQuery(Query.str());
	Query.str("");

	Query << "INSERT INTO Inventory VALUES(" << CharacterID << ", 3, 1, 1)";
	Save->Database->RunQuery(Query.str());
	Query.str("");

	Query << "INSERT INTO SkillLevel VALUES(" << CharacterID << ", 1, 1)";
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
	Query << "SELECT ID FROM Characters WHERE AccountsID = " << Peer->AccountID << " LIMIT " << Index << ", 1";
	Save->Database->RunDataQuery(Query.str());
	if(Save->Database->FetchRow()) {
		CharacterID = Save->Database->GetInt(0);
	}
	Save->Database->CloseQuery();
	Query.str("");

	// Delete character
	Query << "DELETE FROM Characters WHERE ID = " << CharacterID;
	Save->Database->RunQuery(Query.str());
	Query.str("");

	// Delete items
	Query << "DELETE FROM Inventory WHERE CharactersID = " << CharacterID;
	Save->Database->RunQuery(Query.str());
	Query.str("");

	// Delete skill levels
	Query << "DELETE FROM SkillLevel WHERE CharactersID = " << CharacterID;
	Save->Database->RunQuery(Query.str());
	Query.str("");

	// Update the player
	SendCharacterList(Peer);
}

// Loads the player, updates the world, notifies clients
void _Server::HandleCharacterPlay(_Buffer &Data, _Peer *Peer) {

	// Read packet
	int Slot = Data.Read<char>();

	// Get character info
	std::stringstream Query;
	Query << "SELECT ID FROM Characters WHERE AccountsID = " << Peer->AccountID << " LIMIT " << Slot << ", 1";
	Save->Database->RunDataQuery(Query.str());
	if(Save->Database->FetchRow()) {
		Peer->CharacterID = Save->Database->GetInt(0);
	}
	Save->Database->CloseQuery();
	Query.str("");

	// Check for valid character id
	if(!Peer->CharacterID) {
		Log << "Character slot " << Slot << " empty!";
		return;
	}

	// Send map and players to new player
	SpawnPlayer(Peer, _Map::EVENT_SPAWN);
}

// Handles move commands from a client
void _Server::HandleMoveCommand(_Buffer &Data, _Peer *Peer) {
	if(!ValidatePeer(Peer))
	   return;

	_Object *Player = Peer->Object;
	Player->InputState = Data.Read<char>();
	/*
	if(Player->MovePlayer(Direction)) {

		// Handle events
		const _Tile *Tile = Player->GetTile();
		switch(Tile->EventType) {
			case _Map::EVENT_SPAWN:
				Player->SpawnMapID = Player->Map->ID;
				Player->SpawnPoint = Tile->EventData;
				Player->RestoreHealthMana();
				SendHUD(Player);
				Player->Save();
			break;
			case _Map::EVENT_MAPCHANGE:
				Player->GenerateNextBattle();
				SpawnPlayer(Player, Tile->EventData, _Map::EVENT_MAPCHANGE, Player->Map->ID);
			break;
			case _Map::EVENT_VENDOR:
				Player->State = _Object::STATE_VENDOR;
				Player->Vendor = Stats->GetVendor(Tile->EventData);
				SendEvent(Player, Tile->EventType, Tile->EventData);
			break;
			case _Map::EVENT_TRADER:
				Player->State = _Object::STATE_TRADER;
				Player->Trader = Stats->GetTrader(Tile->EventData);
				SendEvent(Player, Tile->EventType, Tile->EventData);
			break;
			default:

				// Start a battle
				if(Player->NextBattle <= 0) {

					// Get monsters
					std::vector<int> Monsters;
					Stats->GenerateMonsterListFromZone(Player->GetCurrentZone(), Monsters);
					size_t MonsterCount = Monsters.size();
					if(MonsterCount > 0) {

						// Create a new battle instance
						_ServerBattle *Battle = new _ServerBattle();
						Battles.push_back(Battle);

						// Add players
						Battle->AddFighter(Player, 0);
						if(1) {

							// Get a list of players
							std::list<_Object *> Players;
							Player->Map->GetClosePlayers(Player, 7*7, Players);

							// Add players to battle
							int PlayersAdded = 0;
							for(std::list<_Object *>::iterator Iterator = Players.begin(); Iterator != Players.end(); ++Iterator) {
								_Object *PartyPlayer = *Iterator;
								if(PartyPlayer->State == _Object::STATE_WALK && !PartyPlayer->IsInvisible()) {
									SendPlayerPosition(PartyPlayer);
									Battle->AddFighter(PartyPlayer, 0);
									PlayersAdded++;
									if(PlayersAdded == 2)
										break;
								}
							}
						}

						// Add monsters
						for(size_t i = 0; i < Monsters.size(); i++) {
							_Object *Monster = new _Object(Monsters[i]);
							Monster->ID = Monsters[i];
							Monster->Type = _Object::MONSTER;
							Stats->GetMonsterStats(Monsters[i], Monster);
							Battle->AddFighter(Monster, 1);
						}

						Battle->StartBattle();
					}
				}
			break;
		}
	}*/

}

// Send character list
void _Server::SendCharacterList(_Peer *Peer) {
	std::stringstream Query;

	// Get a count of the account's characters
	Query << "SELECT Count(ID) FROM Characters WHERE AccountsID = " << Peer->AccountID;
	int CharacterCount = Save->Database->RunCountQuery(Query.str());
	Query.str("");

	// Create the packet
	_Buffer Packet;
	Packet.Write<char>(Packet::CHARACTERS_LIST);
	Packet.Write<char>(CharacterCount);

	// Generate a list of characters
	Query << "SELECT Name, PortraitID, Experience FROM Characters WHERE AccountsID = " << Peer->AccountID;
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
void _Server::SpawnPlayer(_Peer *Peer, int MapID) {
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
		Player->State = _Object::STATE_WALK;

		// Find spawn point in map
		Map->FindEvent(_Map::EVENT_SPAWN, Player->SpawnPoint, Player->Position);

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
	NewMap->ServerNetwork = Network.get();
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
	Query << "SELECT * FROM Characters WHERE ID = " << Peer->CharacterID;
	Save->Database->RunDataQuery(Query.str());
	Query.str("");

	// Get row
	if(!Save->Database->FetchRow()) {
		Save->Database->CloseQuery();
		return nullptr;
	}

	// Set properties
	Peer->CharacterID = Save->Database->GetInt(0);
	Player->CharacterID = Save->Database->GetInt(0);
	Player->SpawnMapID = Save->Database->GetInt(2);
	Player->SpawnPoint = Save->Database->GetInt(3);
	Player->Name = Save->Database->GetString(4);
	Player->PortraitID = Save->Database->GetInt(5);
	Player->Experience = Save->Database->GetInt(6);
	Player->Gold = Save->Database->GetInt(7);
	for(int i = 0; i < ACTIONBAR_SIZE; i++) {
		uint32_t SkillID = Save->Database->GetInt(i + 8);
		Player->ActionBar[i] = Stats->Skills[SkillID];
	}
	Player->PlayTime = Save->Database->GetInt(16);
	Player->Deaths = Save->Database->GetInt(17);
	Player->MonsterKills = Save->Database->GetInt(18);
	Player->PlayerKills = Save->Database->GetInt(19);
	Player->Bounty = Save->Database->GetInt(20);
	Save->Database->CloseQuery();

	// Set inventory
	Query << "SELECT Slot, ItemsID, Count FROM Inventory WHERE CharactersID = " << Player->CharacterID;
	Save->Database->RunDataQuery(Query.str());
	int ItemCount = 0;
	while(Save->Database->FetchRow()) {
		Player->SetInventory(Save->Database->GetInt(0), Save->Database->GetInt(1), Save->Database->GetInt(2));
		ItemCount++;
	}
	Save->Database->CloseQuery();
	Query.str("");

	// Set skills
	Query << "SELECT SkillsID, Level FROM SkillLevel WHERE CharactersID = " << Player->CharacterID;
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
