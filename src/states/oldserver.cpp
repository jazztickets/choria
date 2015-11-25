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
#include <states/oldserver.h>
#include <database.h>
#include <objectmanager.h>
#include <stats.h>
#include <globals.h>
#include <config.h>
#include <framework.h>
#include <constants.h>
#include <packet.h>
#include <ui/textbox.h>
#include <network/oldnetwork.h>
#include <buffer.h>
#include <instances/map.h>
#include <instances/serverbattle.h>
#include <objects/player.h>
#include <objects/fighter.h>
#include <iostream>
#include <string>

_OldServerState OldServerState;

static void ObjectDeleted(_Object *Object);

// Loop to run server commands
void HandleCommands(void *Arguments) {
	std::string Input;
	bool Done = false;

	std::this_thread::sleep_for(std::chrono::milliseconds(300));

	Framework.Log << "Listening on port " << OldServerNetwork->GetPort() << std::endl;
	Framework.Log << "Type stop to stop the server" << std::endl;
	while(!Done) {
		std::getline(std::cin, Input);
		if(Input == "stop")
			Done = true;
		else
			std::cout << "Command not recognized" << std::endl;
	}

	OldServerState.StopServer();
}

// Initializes the state
void _OldServerState::Init() {
	ServerTime = 0;
	StopRequested = false;
	CommandThread = nullptr;

	// Load database that stores accounts and characters
	Database = new _Database();

	std::string DatabasePath = Config.ConfigPath + "save.dat";
	if(!Database->OpenDatabase(DatabasePath.c_str())) {

		// Create a new database
		printf("Creating new database...\n");
		if(!Database->OpenDatabaseCreate(DatabasePath.c_str())) {
			throw std::runtime_error("OpenDatabaseCreate failed");
		}

		// Populate data
		CreateDefaultDatabase();
	}

	ObjectManager = new _ObjectManager();
	ObjectManager->SetObjectDeletedCallback(ObjectDeleted);
}

// Shuts the state down
void _OldServerState::Close() {
	if(CommandThread) {
		CommandThread->join();
		delete CommandThread;
	}

	// Disconnect peers
	std::list<_Object *> Objects = ObjectManager->GetObjects();
	for(auto &Object : Objects) {
		if(Object->Type == _Object::PLAYER) {
			_Player *Player = (_Player *)Object;
			Player->Save();
			OldServerNetwork->Disconnect(Player->Peer);
		}
	}

	delete Database;
	delete ObjectManager;
}

// Start run the command thread
void _OldServerState::StartCommandThread() {
	CommandThread = new std::thread(HandleCommands, this);
}

// Populates the server database with the default data
void _OldServerState::CreateDefaultDatabase() {

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

// Handles a new client connection
void _OldServerState::HandleConnect(ENetEvent *Event) {
	char Buffer[16];
	enet_address_get_host_ip(&Event->peer->address, Buffer, 16);
	Framework.Log << "HandleConnect: " << Buffer << ":" << Event->peer->address.port << std::endl;

	// Create the player and add it to the object list
	_Player *NewPlayer = new _Player();
	ObjectManager->AddObject(NewPlayer);

	// Store the player in the peer struct
	Event->peer->data = NewPlayer;
	NewPlayer->Peer = Event->peer;

	// Send player the game version
	_Buffer Packet;
	Packet.Write<char>(Packet::VERSION);
	Packet.WriteString(GAME_VERSION);
	OldServerNetwork->SendPacketToPeer(&Packet, Event->peer);
}

// Handles a client disconnect
void _OldServerState::HandleDisconnect(ENetEvent *Event) {
	char Buffer[16];
	enet_address_get_host_ip(&Event->peer->address, Buffer, 16);
	Framework.Log << "HandleDisconnect: " << Buffer << ":" << Event->peer->address.port << std::endl;

	_Player *Player = (_Player *)Event->peer->data;
	if(!Player)
		return;

	// Leave trading screen
	_Player *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {
		TradePlayer->TradePlayer = nullptr;

		_Buffer Packet;
		Packet.Write<char>(Packet::TRADE_CANCEL);
		OldServerNetwork->SendPacketToPeer(&Packet, TradePlayer->Peer);
	}

	// Remove from battle
	RemovePlayerFromBattle(Player);

	// Save character info
	Player->Save();

	// Delete object
	ObjectManager->DeleteObject(Player);
}

// Handles a client packet
void _OldServerState::HandlePacket(ENetEvent *Event) {
	//printf("HandlePacket: type=%d\n", Event->packet->data[0]);

	_Buffer Packet((char *)Event->packet->data, Event->packet->dataLength);
	int PacketType = Packet.Read<char>();
	switch(PacketType) {
		case Packet::ACCOUNT_LOGININFO:
			HandleLoginInfo(&Packet, Event->peer);
		break;
		case Packet::CHARACTERS_REQUEST:
			HandleCharacterListRequest(&Packet, Event->peer);
		break;
		case Packet::CHARACTERS_PLAY:
			HandleCharacterSelect(&Packet, Event->peer);
		break;
		case Packet::CHARACTERS_DELETE:
			HandleCharacterDelete(&Packet, Event->peer);
		break;
		case Packet::CREATECHARACTER_INFO:
			HandleCharacterCreate(&Packet, Event->peer);
		break;
		case Packet::WORLD_MOVECOMMAND:
			HandleMoveCommand(&Packet, Event->peer);
		break;
		case Packet::BATTLE_COMMAND:
			HandleBattleCommand(&Packet, Event->peer);
		break;
		case Packet::BATTLE_CLIENTDONE:
			HandleBattleFinished(&Packet, Event->peer);
		break;
		case Packet::INVENTORY_MOVE:
			HandleInventoryMove(&Packet, Event->peer);
		break;
		case Packet::INVENTORY_USE:
			HandleInventoryUse(&Packet, Event->peer);
		break;
		case Packet::INVENTORY_SPLIT:
			HandleInventorySplit(&Packet, Event->peer);
		break;
		case Packet::EVENT_END:
			HandleEventEnd(&Packet, Event->peer);
		break;
		case Packet::VENDOR_EXCHANGE:
			HandleVendorExchange(&Packet, Event->peer);
		break;
		case Packet::TRADER_ACCEPT:
			HandleTraderAccept(&Packet, Event->peer);
		break;
		case Packet::SKILLS_SKILLBAR:
			HandleSkillBar(&Packet, Event->peer);
		break;
		case Packet::SKILLS_SKILLADJUST:
			HandleSkillAdjust(&Packet, Event->peer);
		break;
		case Packet::WORLD_BUSY:
			HandlePlayerBusy(&Packet, Event->peer);
		break;
		case Packet::WORLD_ATTACKPLAYER:
			HandleAttackPlayer(&Packet, Event->peer);
		break;
		case Packet::WORLD_TELEPORT:
			HandleTeleport(&Packet, Event->peer);
		break;
		case Packet::CHAT_MESSAGE:
			HandleChatMessage(&Packet, Event->peer);
		break;
		case Packet::TRADE_REQUEST:
			HandleTradeRequest(&Packet, Event->peer);
		break;
		case Packet::TRADE_CANCEL:
			HandleTradeCancel(&Packet, Event->peer);
		break;
		case Packet::TRADE_GOLD:
			HandleTradeGold(&Packet, Event->peer);
		break;
		case Packet::TRADE_ACCEPT:
			HandleTradeAccept(&Packet, Event->peer);
		break;
	}
}

// Updates the current state
void _OldServerState::Update(double FrameTime) {
	ServerTime += FrameTime;

	// Update maps
	for(auto &Map : Maps)
		Map->Update(FrameTime);

	// Update battles
	for(auto &Battle : Battles)
		Battle->Update(FrameTime);

	// Update objects
	ObjectManager->Update(FrameTime);

	if(StopRequested) {
		Framework.Done = true;
	}
}

// Login information
void _OldServerState::HandleLoginInfo(_Buffer *Packet, ENetPeer *Peer) {
	char QueryString[512];

	// Read packet
	bool CreateAccount = Packet->ReadBit();
	std::string Username(Packet->ReadString());
	std::string Password(Packet->ReadString());
	if(Username.size() > 15 || Password.size() > 15)
		return;
	//Username.make_lower();
	//Password.make_lower();

	//printf("HandleLoginInfo: CreateAccount=%d, username=%s, password=%s\n", CreateAccount, Username.c_str(), Password.c_str());

	// Create account or login
	if(CreateAccount) {

		// Check for existing account
		sprintf(QueryString, "SELECT ID FROM Accounts WHERE Username = '%s'", Username.c_str());
		Database->RunDataQuery(QueryString);
		int Result = Database->FetchRow();
		Database->CloseQuery();

		if(Result) {
			_Buffer NewPacket;
			NewPacket.Write<char>(Packet::ACCOUNT_EXISTS);
			OldServerNetwork->SendPacketToPeer(&NewPacket, Peer);
			return;
		}
		else {
			sprintf(QueryString, "INSERT INTO Accounts(Username, Password) VALUES('%s', '%s')", Username.c_str(), Password.c_str());
			Database->RunQuery(QueryString);
		}
	}

	// Get account information
	int AccountID = 0;
	sprintf(QueryString, "SELECT ID FROM Accounts WHERE Username = '%s' AND Password = '%s'", Username.c_str(), Password.c_str());
	Database->RunDataQuery(QueryString);
	if(Database->FetchRow()) {
		AccountID = Database->GetInt(0);
	}
	Database->CloseQuery();

	// Make sure account exists
	if(AccountID == 0) {
		//printf("HandleLoginInfo: Account not found\n");

		_Buffer NewPacket;
		NewPacket.Write<char>(Packet::ACCOUNT_NOTFOUND);
		OldServerNetwork->SendPacketToPeer(&NewPacket, Peer);
	}
	else {

		// Store the account id
		_Player *Player = (_Player *)Peer->data;
		Player->AccountID = AccountID;

		_Buffer NewPacket;
		NewPacket.Write<char>(Packet::ACCOUNT_SUCCESS);
		OldServerNetwork->SendPacketToPeer(&NewPacket, Peer);
		//printf("HandleLoginInfo: AccountID=%d, CharacterCount=%d\n", AccountID, CharacterCount);
	}
}

// Sends a player his/her character list
void _OldServerState::HandleCharacterListRequest(_Buffer *Packet, ENetPeer *Peer) {

	_Player *Player = (_Player *)Peer->data;
	SendCharacterList(Player);
}

// Loads the player, updates the world, notifies clients
void _OldServerState::HandleCharacterSelect(_Buffer *Packet, ENetPeer *Peer) {

	int Slot = Packet->Read<char>();
	_Player *Player = (_Player *)Peer->data;
	//printf("HandleCharacterSelect: accountid=%d, slot=%d\n", Player->AccountID, Slot);

	// Check account
	if(Player->AccountID <= 0) {
		printf(" Bad account id\n");
		return;
	}

	// Get character info
	char QueryString[512];
	sprintf(QueryString, "SELECT * FROM Characters WHERE AccountsID = %d LIMIT %d, 1", Player->AccountID, Slot);
	Database->RunDataQuery(QueryString);
	if(!Database->FetchRow()) {
		printf(" Didn't find a character for slot %d\n", Slot);
		return;
	}

	// Set player properties
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
	sprintf(QueryString, "SELECT Slot, ItemsID, Count FROM Inventory WHERE CharactersID = %d", Player->CharacterID);
	Database->RunDataQuery(QueryString);
	int ItemCount = 0;
	while(Database->FetchRow()) {
		Player->SetInventory(Database->GetInt(0), Database->GetInt(1), Database->GetInt(2));
		ItemCount++;
	}
	Database->CloseQuery();

	// Set skills
	sprintf(QueryString, "SELECT SkillsID, Level FROM SkillLevel WHERE CharactersID = %d", Player->CharacterID);
	Database->RunDataQuery(QueryString);
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
}

// Handle a character delete request
void _OldServerState::HandleCharacterDelete(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	char QueryString[512];

	// Get delete slot
	int Index = Packet->Read<char>();

	// Get character ID
	int CharacterID = 0;
	sprintf(QueryString, "SELECT ID FROM Characters WHERE AccountsID = %d LIMIT %d, 1", Player->AccountID, Index);
	Database->RunDataQuery(QueryString);
	if(Database->FetchRow()) {
		CharacterID = Database->GetInt(0);
	}
	Database->CloseQuery();

	// Delete character
	sprintf(QueryString, "DELETE FROM Characters WHERE ID = %d", CharacterID);
	Database->RunQuery(QueryString);

	// Delete items
	sprintf(QueryString, "DELETE FROM Inventory WHERE CharactersID = %d", CharacterID);
	Database->RunQuery(QueryString);

	// Delete skill levels
	sprintf(QueryString, "DELETE FROM SkillLevel WHERE CharactersID = %d", CharacterID);
	Database->RunQuery(QueryString);

	// Update the player
	SendCharacterList(Player);
}

// Handles the character create request
void _OldServerState::HandleCharacterCreate(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	char QueryString[512];

	// Get character information
	std::string Name(Packet->ReadString());
	int PortraitID = Packet->Read<int32_t>();
	if(Name.size() > PLAYER_NAME_SIZE)
		return;

	// Check character limit
	sprintf(QueryString, "SELECT Count(ID) FROM Characters WHERE AccountsID = %d", Player->AccountID);
	int CharacterCount = Database->RunCountQuery(QueryString);
	if(CharacterCount >= SAVE_COUNT)
		return;

	// Check for existing names
	sprintf(QueryString, "SELECT ID FROM Characters WHERE Name = '%s'", Name.c_str());
	Database->RunDataQuery(QueryString);
	int FindResult = Database->FetchRow();
	Database->CloseQuery();

	// Found an existing name
	if(FindResult) {
		_Buffer NewPacket;
		NewPacket.Write<char>(Packet::CREATECHARACTER_INUSE);
		OldServerNetwork->SendPacketToPeer(&NewPacket, Peer);
		return;
	}

	// Create the character
	Database->RunQuery("BEGIN TRANSACTION");
	sprintf(QueryString, "INSERT INTO Characters(AccountsID, Name, PortraitID, SkillBar0) VALUES(%d, '%s', %d, 0)", Player->AccountID, Name.c_str(), PortraitID);
	Database->RunQuery(QueryString);

	int CharacterID = Database->GetLastInsertID();
	sprintf(QueryString, "INSERT INTO Inventory VALUES(%d, 1, 2, 1)", CharacterID);
	Database->RunQuery(QueryString);
	sprintf(QueryString, "INSERT INTO Inventory VALUES(%d, 3, 1, 1)", CharacterID);
	Database->RunQuery(QueryString);
	sprintf(QueryString, "INSERT INTO SkillLevel VALUES(%d, 0, 1)", CharacterID);
	Database->RunQuery(QueryString);
	Database->RunQuery("END TRANSACTION");

	// Notify the client
	_Buffer NewPacket;
	NewPacket.Write<char>(Packet::CREATECHARACTER_SUCCESS);
	OldServerNetwork->SendPacketToPeer(&NewPacket, Peer);
}

// Handles move commands from a client
void _OldServerState::HandleMoveCommand(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	int Direction = Packet->Read<char>();
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
				Player->State = _Player::STATE_VENDOR;
				Player->Vendor = OldStats.GetVendor(Tile->EventData);
				SendEvent(Player, Tile->EventType, Tile->EventData);
			break;
			case _Map::EVENT_TRADER:
				Player->State = _Player::STATE_TRADER;
				Player->Trader = OldStats.GetTrader(Tile->EventData);
				SendEvent(Player, Tile->EventType, Tile->EventData);
			break;
			default:

				// Start a battle
				if(Player->NextBattle <= 0) {

					// Get monsters
					std::vector<int> Monsters;
					OldStats.GenerateMonsterListFromZone(Player->GetCurrentZone(), Monsters);
					size_t MonsterCount = Monsters.size();
					if(MonsterCount > 0) {

						// Create a new battle instance
						_ServerBattle *Battle = new _ServerBattle();
						Battles.push_back(Battle);

						// Add players
						Battle->AddFighter(Player, 0);
						if(1) {

							// Get a list of players
							std::list<_Player *> Players;
							Player->Map->GetClosePlayers(Player, 7*7, Players);

							// Add players to battle
							int PlayersAdded = 0;
							for(std::list<_Player *>::iterator Iterator = Players.begin(); Iterator != Players.end(); ++Iterator) {
								_Player *PartyPlayer = *Iterator;
								if(PartyPlayer->State == _Player::STATE_WALK && !PartyPlayer->IsInvisible()) {
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
							_Fighter *Monster = new _Fighter(Monsters[i]);
							Monster->ID = Monsters[i];
							Monster->Type = _Object::MONSTER;
							OldStats.GetMonsterStats(Monsters[i], Monster);
							Battle->AddFighter(Monster, 1);
						}

						Battle->StartBattle();
					}
				}
			break;
		}
	}
}

// Handles battle commands from a client
void _OldServerState::HandleBattleCommand(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	_ServerBattle *Battle = (_ServerBattle *)Player->Battle;
	if(!Battle)
		return;

	int Command = Packet->Read<char>();
	int Target = Packet->Read<char>();
	Battle->HandleInput(Player, Command, Target);

	//printf("HandleBattleCommand: %d\n", Command);
}

// The client is done with the battle results screen
void _OldServerState::HandleBattleFinished(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
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
void _OldServerState::HandleInventoryMove(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	int OldSlot = Packet->Read<char>();
	int NewSlot = Packet->Read<char>();

	// Move items
	Player->MoveInventory(OldSlot, NewSlot);
	Player->CalculatePlayerStats();

	// Check for trading players
	_Player *TradePlayer = Player->TradePlayer;
	if(Player->State == _Player::STATE_TRADE && TradePlayer && (_Player::IsSlotTrade(OldSlot) || _Player::IsSlotTrade(NewSlot))) {

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
		OldServerNetwork->SendPacketToPeer(&NewPacket, TradePlayer->Peer);
	}
}

// Handle a player's inventory use request
void _OldServerState::HandleInventoryUse(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	// Use an item
	Player->UseInventory(Packet->Read<char>());
}

// Handle a player's inventory split stack request
void _OldServerState::HandleInventorySplit(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	int Slot = Packet->Read<char>();
	int Count = Packet->Read<char>();

	// Inventory only
	if(!_Player::IsSlotInventory(Slot))
		return;

	Player->SplitStack(Slot, Count);
}

// Handles a player's event end message
void _OldServerState::HandleEventEnd(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	Player->Vendor = nullptr;
	Player->State = _Player::STATE_WALK;
}

// Handles a vendor exchange message
void _OldServerState::HandleVendorExchange(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	// Get vendor
	const _Vendor *Vendor = Player->Vendor;
	if(!Vendor)
		return;

	// Get info
	bool Buy = Packet->ReadBit();
	int Amount = Packet->Read<char>();
	int Slot = Packet->Read<char>();
	if(Slot < 0)
		return;

	// Update player
	if(Buy) {
		if(Slot >= (int)Vendor->Items.size())
			return;

		// Get optional inventory slot
		int TargetSlot = Packet->Read<char>();

		// Get item info
		const _Item *Item = Vendor->Items[Slot];
		int Price = Item->GetPrice(Vendor, Amount, Buy);

		// Update player
		Player->UpdateGold(-Price);
		Player->AddItem(Item, Amount, TargetSlot);
		Player->CalculatePlayerStats();
	}
	else {
		if(Slot >= _Player::INVENTORY_COUNT)
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
void _OldServerState::HandleSkillBar(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	// Read skills
	for(int i = 0; i < BATTLE_MAXSKILLS; i++) {
		Player->SetSkillBar(i, OldStats.GetSkill(Packet->Read<char>()));
	}

	Player->CalculatePlayerStats();
}

// Handles a skill adjust
void _OldServerState::HandleSkillAdjust(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	// Process packet
	bool Spend = Packet->ReadBit();
	int SkillID = Packet->Read<char>();
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
void _OldServerState::HandlePlayerBusy(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	bool Value = Packet->ReadBit();
	Player->SetBusy(Value);

	//printf("HandlePlayerBusy: Value=%d\n", Value);
}

// Handles a player's request to attack another player
void _OldServerState::HandleAttackPlayer(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
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
		std::list<_Player *> Players;
		Map->GetClosePlayers(Player, 1.5f * 1.5f, Players);

		// Find a suitable player to attack
		for(std::list<_Player *>::iterator Iterator = Players.begin(); Iterator != Players.end(); ++Iterator) {
			_Player *VictimPlayer = *Iterator;
			if(VictimPlayer->State != _Player::STATE_BATTLE) {
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

// Handle a chat message
void _OldServerState::HandleChatMessage(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	// Get message
	std::string Message = Packet->ReadString();
	if(Message.length() > NETWORKING_CHAT_SIZE)
		Message.resize(NETWORKING_CHAT_SIZE);

	// Append name
	Message = Player->Name + ": " + Message;

	// Send message to other players
	_Buffer NewPacket;
	NewPacket.Write<char>(Packet::CHAT_MESSAGE);
	NewPacket.Write<glm::vec4>(COLOR_WHITE);
	NewPacket.WriteString(Message.c_str());

	// Broadcast message
	auto &Objects = ObjectManager->GetObjects();
	for(auto &Object : Objects) {
		OldServerNetwork->SendPacketToPeer(&NewPacket, Object->Peer);
	}
}

// Handle a trade request
void _OldServerState::HandleTradeRequest(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	// Get map object
	_Map *Map = Player->Map;
	if(!Map)
		return;

	// Find the nearest player to trade with
	_Player *TradePlayer = Map->FindTradePlayer(Player, 2.0f * 2.0f);
	if(TradePlayer == nullptr) {

		// Set up trade post
		Player->State = _Player::STATE_TRADE;
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

		Player->State = _Player::STATE_TRADE;
		TradePlayer->State = _Player::STATE_TRADE;
	}
}

// Handles a trade cancel
void _OldServerState::HandleTradeCancel(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	// Notify trading player
	_Player *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {
		TradePlayer->State = _Player::STATE_TRADE;
		TradePlayer->TradePlayer = nullptr;
		TradePlayer->TradeAccepted = false;

		_Buffer NewPacket;
		NewPacket.Write<char>(Packet::TRADE_CANCEL);
		OldServerNetwork->SendPacketToPeer(&NewPacket, TradePlayer->Peer);
	}

	// Set state back to normal
	Player->State = _Player::STATE_WALK;
}

// Handle a trade gold update
void _OldServerState::HandleTradeGold(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	// Set gold amount
	int Gold = Packet->Read<int32_t>();
	if(Gold < 0)
		Gold = 0;
	else if(Gold > Player->Gold)
		Gold = Player->Gold;
	Player->TradeGold = Gold;
	Player->TradeAccepted = false;

	// Notify player
	_Player *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {
		TradePlayer->TradeAccepted = false;

		_Buffer NewPacket;
		NewPacket.Write<char>(Packet::TRADE_GOLD);
		NewPacket.Write<int32_t>(Gold);
		OldServerNetwork->SendPacketToPeer(&NewPacket, TradePlayer->Peer);
	}
}

// Handles a trade accept from a player
void _OldServerState::HandleTradeAccept(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	// Get trading player
	_Player *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {

		// Set the player's state
		bool Accepted = !!Packet->Read<char>();
		Player->TradeAccepted = Accepted;

		// Check if both player's agree
		if(Accepted && TradePlayer->TradeAccepted) {

			// Exchange items
			_InventorySlot TempItems[PLAYER_TRADEITEMS];
			for(int i = 0; i < PLAYER_TRADEITEMS; i++) {
				int InventorySlot = i + _Player::INVENTORY_TRADE;
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
				OldServerNetwork->SendPacketToPeer(&NewPacket, Player->Peer);
			}
			{
				_Buffer NewPacket;
				NewPacket.Write<char>(Packet::TRADE_EXCHANGE);
				BuildTradeItemsPacket(TradePlayer, &NewPacket, TradePlayer->Gold);
				OldServerNetwork->SendPacketToPeer(&NewPacket, TradePlayer->Peer);
			}

			Player->State = _Player::STATE_WALK;
			Player->TradePlayer = nullptr;
			Player->TradeGold = 0;
			Player->MoveTradeToInventory();
			TradePlayer->State = _Player::STATE_WALK;
			TradePlayer->TradePlayer = nullptr;
			TradePlayer->TradeGold = 0;
			TradePlayer->MoveTradeToInventory();
		}
		else {

			// Notify trading player
			_Buffer NewPacket;
			NewPacket.Write<char>(Packet::TRADE_ACCEPT);
			NewPacket.Write<char>(Accepted);
			OldServerNetwork->SendPacketToPeer(&NewPacket, TradePlayer->Peer);
		}
	}
}

// Handles a teleport request
void _OldServerState::HandleTeleport(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
	if(!Player)
		return;

	Player->StartTeleport();
}

// Handles a trader accept
void _OldServerState::HandleTraderAccept(_Buffer *Packet, ENetPeer *Peer) {
	_Player *Player = (_Player *)Peer->data;
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
	Player->State = _Player::STATE_WALK;
	Player->CalculatePlayerStats();
}

// Send a message to the player
void _OldServerState::SendMessage(_Player *Player, const std::string &Message, const glm::vec4 &Color) {
	if(!Player)
		return;

	// Build message
	_Buffer Packet;
	Packet.Write<char>(Packet::CHAT_MESSAGE);
	Packet.Write<glm::vec4>(Color);
	Packet.WriteString(Message.c_str());

	// Send
	OldServerNetwork->SendPacketToPeer(&Packet, Player->Peer);
}

// Spawns a player at a particular spawn point
void _OldServerState::SpawnPlayer(_Player *Player, int NewMapID, int EventType, int EventData) {

	// Get new map
	_Map *NewMap = GetMap(NewMapID);

	// Remove old player if map has changed
	_Map *OldMap = Player->Map;
	if(OldMap && NewMap != OldMap)
		OldMap->RemoveObject(Player);

	// Find spawn point in map
	NewMap->FindEvent(EventType, EventData, Player->Position);
	SendPlayerPosition(Player);

	// Set state
	Player->State = _Player::STATE_WALK;

	// Send new object list
	if(NewMap != OldMap) {

		// Update pointers
		Player->Map = NewMap;

		// Add player to map
		NewMap->AddObject(Player);

		// Build packet for player
		_Buffer Packet;
		Packet.Write<char>(Packet::WORLD_CHANGEMAPS);

		// Send player info
		Packet.Write<int32_t>(NewMapID);

		// Write object data
		Packet.Write<int32_t>(NewMap->Objects.size());
		for(auto Iterator = NewMap->Objects.begin(); Iterator != NewMap->Objects.end(); ++Iterator) {
			_Object *Object = *Iterator;

			Packet.Write<char>(Object->NetworkID);
			Packet.Write<char>(Object->Position.x);
			Packet.Write<char>(Object->Position.y);
			Packet.Write<char>(Object->Type);
			switch(Object->Type) {
				case _Object::PLAYER: {
					_Player *PlayerObject = (_Player *)Object;
					Packet.WriteString(PlayerObject->Name.c_str());
					Packet.Write<char>(PlayerObject->PortraitID);
					Packet.WriteBit((PlayerObject->IsInvisible()));
				}
				break;
				default:
				break;
			}
		}

		// Send object list to the player
		OldServerNetwork->SendPacketToPeer(&Packet, Player->Peer);
	}

	//printf("SpawnPlayer: MapID=%d, NetworkID=%d\n", TNewMapID, TPlayer->NetworkID);
}

// Updates the player's HUD
void _OldServerState::SendHUD(_Player *Player) {

	_Buffer Packet;
	Packet.Write<char>(Packet::WORLD_HUD);
	Packet.Write<int32_t>(Player->Experience);
	Packet.Write<int32_t>(Player->Gold);
	Packet.Write<int32_t>(Player->Health);
	Packet.Write<int32_t>(Player->Mana);
	Packet.Write<float>(Player->HealthAccumulator);
	Packet.Write<float>(Player->ManaAccumulator);

	OldServerNetwork->SendPacketToPeer(&Packet, Player->Peer);
}

// Send player their position
void _OldServerState::SendPlayerPosition(_Player *Player) {

	_Buffer Packet;
	Packet.Write<char>(Packet::WORLD_POSITION);
	Packet.Write<char>(Player->Position.x);
	Packet.Write<char>(Player->Position.y);

	OldServerNetwork->SendPacketToPeer(&Packet, Player->Peer);
}

// Sends the player a list of his/her characters
void _OldServerState::SendCharacterList(_Player *Player) {
	char QueryString[512];

	// Get a count of the account's characters
	sprintf(QueryString, "SELECT Count(ID) FROM Characters WHERE AccountsID = %d", Player->AccountID);
	int CharacterCount = Database->RunCountQuery(QueryString);

	// Create the packet
	_Buffer Packet;
	Packet.Write<char>(Packet::CHARACTERS_LIST);
	Packet.Write<char>(CharacterCount);

	// Generate a list of characters
	sprintf(QueryString, "SELECT Name, PortraitID, Experience FROM Characters WHERE AccountsID = %d", Player->AccountID);
	Database->RunDataQuery(QueryString);
	while(Database->FetchRow()) {
		Packet.WriteString(Database->GetString(0));
		Packet.Write<int32_t>(Database->GetInt(1));
		Packet.Write<int32_t>(Database->GetInt(2));
	}
	Database->CloseQuery();

	// Send list
	OldServerNetwork->SendPacketToPeer(&Packet, Player->Peer);
}

// Sends a player an event message
void _OldServerState::SendEvent(_Player *Player, int Type, int Data) {

	// Create packet
	_Buffer Packet;
	Packet.Write<char>(Packet::EVENT_START);
	Packet.Write<char>(Type);
	Packet.Write<int32_t>(Data);
	Packet.Write<char>(Player->Position.x);
	Packet.Write<char>(Player->Position.y);

	OldServerNetwork->SendPacketToPeer(&Packet, Player->Peer);
}

// Sends information to another player about items they're trading
void _OldServerState::SendTradeInformation(_Player *Sender, _Player *Receiver) {

	// Send items to trader player
	_Buffer Packet;
	Packet.Write<char>(Packet::TRADE_REQUEST);
	Packet.Write<char>(Sender->NetworkID);
	BuildTradeItemsPacket(Sender, &Packet, Sender->TradeGold);
	OldServerNetwork->SendPacketToPeer(&Packet, Receiver->Peer);
}

// Adds trade item information to a packet
void _OldServerState::BuildTradeItemsPacket(_Player *Player, _Buffer *Packet, int Gold) {
	Packet->Write<int32_t>(Gold);
	for(int i = _Player::INVENTORY_TRADE; i < _Player::INVENTORY_COUNT; i++) {
		if(Player->Inventory[i].Item) {
			Packet->Write<int32_t>(Player->Inventory[i].Item->ID);
			Packet->Write<char>(Player->Inventory[i].Count);
		}
		else
			Packet->Write<int32_t>(0);
	}
}

// Removes a player from a battle and deletes the battle if necessary
void _OldServerState::RemovePlayerFromBattle(_Player *Player) {
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

// Deletes an object on the server and broadcasts it to the clients
void _OldServerState::DeleteObject(_Object *Object) {

	// Remove the object from their current map
	_Map *Map = Object->Map;
	if(Map)
		Map->RemoveObject(Object);
}

// Called when object gets deleted
void ObjectDeleted(_Object *Object) {

	OldServerState.DeleteObject(Object);
}

// Teleports a player back to town
void _OldServerState::PlayerTeleport(_Player *Player) {
	Player->RestoreHealthMana();
	SpawnPlayer(Player, Player->SpawnMapID, _Map::EVENT_SPAWN, Player->SpawnPoint);
	SendHUD(Player);
	Player->Save();
}

// Gets a map from the manager. Loads the level if it doesn't exist
_Map *_OldServerState::GetMap(int MapID) {

	// Loop through loaded maps
	for(auto &Map : Maps) {

		// Check id
		if(Map->ID == MapID)
			return Map;
	}

	// Not found, so create it
	_Map *NewMap = new _Map(MapID);
	Maps.push_back(NewMap);

	return NewMap;
}
