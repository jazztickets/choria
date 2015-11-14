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
#include <states/playserver.h>
#include <database.h>
#include <objectmanager.h>
#include <instances.h>
#include <stats.h>
#include <globals.h>
#include <config.h>
#include <framework.h>
#include <constants.h>
#include <network/network.h>
#include <buffer.h>
#include <instances/map.h>
#include <instances/serverbattle.h>
#include <objects/player.h>
#include <objects/monster.h>
#include <iostream>
#include <string>

_PlayServerState PlayServerState;

using namespace irr;

static void ObjectDeleted(_Object *TObject);

// Loop to run server commands
void HandleCommands(void *Arguments) {
	std::string Input;
	bool Done = false;

	std::this_thread::sleep_for(std::chrono::milliseconds(300));

	std::cout << "Type stop to stop the server" << std::endl;
	while(!Done) {
		std::getline(std::cin, Input);
		if(Input == "stop")
			Done = true;
		else
			std::cout << "Command not recognized" << std::endl;
	}

	PlayServerState.StopServer();
}

// Initializes the state
void _PlayServerState::Init() {
	ServerTime = 0;
	StopRequested = false;
	CommandThread = nullptr;

	// Load database that stores accounts and characters
	Database = new _Database();

	std::string DatabasePath = Config.ConfigPath + "server.s3db";
	if(!Database->OpenDatabase(DatabasePath.c_str())) {

		// Create a new database
		printf("Creating new database...\n");
		if(!Database->OpenDatabaseCreate(DatabasePath.c_str())) {
			throw std::runtime_error("OpenDatabaseCreate failed");
		}

		// Populate data
		CreateDefaultDatabase();
	}

	Instances = new _Instance();
	ObjectManager = new _ObjectManager();
	ObjectManager->SetObjectDeletedCallback(ObjectDeleted);
}

// Shuts the state down
void _PlayServerState::Close() {
	if(CommandThread) {
		CommandThread->join();
		delete CommandThread;
	}

	// Disconnect peers
	std::list<_Object *> Objects = ObjectManager->GetObjects();
	for(std::list<_Object *>::iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		if((*Iterator)->GetType() == _Object::PLAYER) {
			_Player *Player = static_cast<_Player *>(*Iterator);
			Player->Save();
			ServerNetwork->Disconnect(Player->GetPeer());
		}
	}

	delete Database;
	delete ObjectManager;
	delete Instances;
}

// Start run the command thread
void _PlayServerState::StartCommandThread() {
	CommandThread = new std::thread(HandleCommands, this);
}

// Populates the server database with the default data
void _PlayServerState::CreateDefaultDatabase() {

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
void _PlayServerState::HandleConnect(ENetEvent *TEvent) {
	printf("HandleConnect: %x:%x\n", TEvent->peer->address.host, TEvent->peer->address.port);
	fflush(stdout);

	// Create the player and add it to the object list
	_Player *NewPlayer = new _Player();
	ObjectManager->AddObject(NewPlayer);

	// Store the player in the peer struct
	TEvent->peer->data = NewPlayer;
	NewPlayer->SetPeer(TEvent->peer);

	// Send player the game version
	_Buffer Packet;
	Packet.Write<char>(_Network::VERSION);
	Packet.WriteString(GAME_VERSION);
	ServerNetwork->SendPacketToPeer(&Packet, TEvent->peer);
}

// Handles a client disconnect
void _PlayServerState::HandleDisconnect(ENetEvent *TEvent) {
	printf("HandleDisconnect: %x:%x\n", TEvent->peer->address.host, TEvent->peer->address.port);

	_Player *Player = static_cast<_Player *>(TEvent->peer->data);
	if(!Player)
		return;

	// Leave trading screen
	_Player *TradePlayer = Player->GetTradePlayer();
	if(TradePlayer) {
		TradePlayer->SetTradePlayer(nullptr);

		_Buffer Packet;
		Packet.Write<char>(_Network::TRADE_CANCEL);
		ServerNetwork->SendPacketToPeer(&Packet, TradePlayer->GetPeer());
	}

	// Remove from battle
	RemovePlayerFromBattle(Player);

	// Save character info
	Player->Save();

	// Delete object
	ObjectManager->DeleteObject(Player);
}

// Handles a client packet
void _PlayServerState::HandlePacket(ENetEvent *TEvent) {
	//printf("HandlePacket: type=%d\n", TEvent->packet->data[0]);

	_Buffer Packet((char *)TEvent->packet->data, TEvent->packet->dataLength);
	int PacketType = Packet.Read<char>();
	switch(PacketType) {
		case _Network::ACCOUNT_LOGININFO:
			HandleLoginInfo(&Packet, TEvent->peer);
		break;
		case _Network::CHARACTERS_REQUEST:
			HandleCharacterListRequest(&Packet, TEvent->peer);
		break;
		case _Network::CHARACTERS_PLAY:
			HandleCharacterSelect(&Packet, TEvent->peer);
		break;
		case _Network::CHARACTERS_DELETE:
			HandleCharacterDelete(&Packet, TEvent->peer);
		break;
		case _Network::CREATECHARACTER_INFO:
			HandleCharacterCreate(&Packet, TEvent->peer);
		break;
		case _Network::WORLD_MOVECOMMAND:
			HandleMoveCommand(&Packet, TEvent->peer);
		break;
		case _Network::BATTLE_COMMAND:
			HandleBattleCommand(&Packet, TEvent->peer);
		break;
		case _Network::BATTLE_CLIENTDONE:
			HandleBattleFinished(&Packet, TEvent->peer);
		break;
		case _Network::INVENTORY_MOVE:
			HandleInventoryMove(&Packet, TEvent->peer);
		break;
		case _Network::INVENTORY_USE:
			HandleInventoryUse(&Packet, TEvent->peer);
		break;
		case _Network::INVENTORY_SPLIT:
			HandleInventorySplit(&Packet, TEvent->peer);
		break;
		case _Network::EVENT_END:
			HandleEventEnd(&Packet, TEvent->peer);
		break;
		case _Network::VENDOR_EXCHANGE:
			HandleVendorExchange(&Packet, TEvent->peer);
		break;
		case _Network::TRADER_ACCEPT:
			HandleTraderAccept(&Packet, TEvent->peer);
		break;
		case _Network::SKILLS_SKILLBAR:
			HandleSkillBar(&Packet, TEvent->peer);
		break;
		case _Network::SKILLS_SKILLADJUST:
			HandleSkillAdjust(&Packet, TEvent->peer);
		break;
		case _Network::WORLD_BUSY:
			HandlePlayerBusy(&Packet, TEvent->peer);
		break;
		case _Network::WORLD_ATTACKPLAYER:
			HandleAttackPlayer(&Packet, TEvent->peer);
		break;
		case _Network::WORLD_TOWNPORTAL:
			HandleTownPortal(&Packet, TEvent->peer);
		break;
		case _Network::CHAT_MESSAGE:
			HandleChatMessage(&Packet, TEvent->peer);
		break;
		case _Network::TRADE_REQUEST:
			HandleTradeRequest(&Packet, TEvent->peer);
		break;
		case _Network::TRADE_CANCEL:
			HandleTradeCancel(&Packet, TEvent->peer);
		break;
		case _Network::TRADE_GOLD:
			HandleTradeGold(&Packet, TEvent->peer);
		break;
		case _Network::TRADE_ACCEPT:
			HandleTradeAccept(&Packet, TEvent->peer);
		break;
	}
}

// Updates the current state
void _PlayServerState::Update(double FrameTime) {
	ServerTime += FrameTime;

	Instances->Update(FrameTime);
	ObjectManager->Update(FrameTime);

	if(StopRequested) {
		Framework.SetDone(true);
	}
}

// Login information
void _PlayServerState::HandleLoginInfo(_Buffer *TPacket, ENetPeer *TPeer) {
	char QueryString[512];

	// Read packet
	bool CreateAccount = TPacket->ReadBit();
	std::string Username(TPacket->ReadString());
	std::string Password(TPacket->ReadString());
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
			_Buffer Packet;
			Packet.Write<char>(_Network::ACCOUNT_EXISTS);
			ServerNetwork->SendPacketToPeer(&Packet, TPeer);
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

		_Buffer Packet;
		Packet.Write<char>(_Network::ACCOUNT_NOTFOUND);
		ServerNetwork->SendPacketToPeer(&Packet, TPeer);
	}
	else {

		// Store the account id
		_Player *Player = (_Player *)TPeer->data;
		Player->SetAccountID(AccountID);

		_Buffer Packet;
		Packet.Write<char>(_Network::ACCOUNT_SUCCESS);
		ServerNetwork->SendPacketToPeer(&Packet, TPeer);
		//printf("HandleLoginInfo: AccountID=%d, CharacterCount=%d\n", AccountID, CharacterCount);
	}
}

// Sends a player his/her character list
void _PlayServerState::HandleCharacterListRequest(_Buffer *TPacket, ENetPeer *TPeer) {

	_Player *Player = static_cast<_Player *>(TPeer->data);
	SendCharacterList(Player);
}

// Loads the player, updates the world, notifies clients
void _PlayServerState::HandleCharacterSelect(_Buffer *TPacket, ENetPeer *TPeer) {

	int Slot = TPacket->Read<char>();
	_Player *Player = static_cast<_Player *>(TPeer->data);
	//printf("HandleCharacterSelect: accountid=%d, slot=%d\n", Player->GetAccountID(), Slot);

	// Check account
	if(Player->GetAccountID() <= 0) {
		printf(" Bad account id\n");
		return;
	}

	// Get character info
	char QueryString[512];
	sprintf(QueryString, "SELECT * FROM Characters WHERE AccountsID = %d LIMIT %d, 1", Player->GetAccountID(), Slot);
	Database->RunDataQuery(QueryString);
	if(!Database->FetchRow()) {
		printf(" Didn't find a character for slot %d\n", Slot);
		return;
	}

	// Set player properties
	Player->SetDatabase(Database);
	Player->SetCharacterID(Database->GetInt(0));
	Player->SetSpawnMapID(Database->GetInt(2));
	Player->SetSpawnPoint(Database->GetInt(3));
	Player->SetName(std::string(Database->GetString(4)));
	Player->SetPortraitID(Database->GetInt(5));
	Player->SetExperience(Database->GetInt(6));
	Player->SetGold(Database->GetInt(7));
	for(int i = 0; i < 8; i++)
		Player->SetSkillBar(i, Stats.GetSkill(Database->GetInt(i + 8)));
	Player->SetPlayTime(Database->GetInt(16));
	Player->SetDeaths(Database->GetInt(17));
	Player->SetMonsterKills(Database->GetInt(18));
	Player->SetPlayerKills(Database->GetInt(19));
	Player->SetBounty(Database->GetInt(20));

	Database->CloseQuery();

	// Set inventory
	sprintf(QueryString, "SELECT Slot, ItemsID, Count FROM Inventory WHERE CharactersID = %d", Player->GetCharacterID());
	Database->RunDataQuery(QueryString);
	int ItemCount = 0;
	while(Database->FetchRow()) {
		Player->SetInventory(Database->GetInt(0), Database->GetInt(1), Database->GetInt(2));
		ItemCount++;
	}
	Database->CloseQuery();

	// Set skills
	sprintf(QueryString, "SELECT SkillsID, Level FROM SkillLevel WHERE CharactersID = %d", Player->GetCharacterID());
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
	_Buffer Packet;
	Packet.Write<char>(_Network::WORLD_YOURCHARACTERINFO);
	Packet.Write<char>(Player->GetNetworkID());
	Packet.WriteString(Player->GetName().c_str());
	Packet.Write<int32_t>(Player->GetPortraitID());
	Packet.Write<int32_t>(Player->GetExperience());
	Packet.Write<int32_t>(Player->GetGold());
	Packet.Write<int32_t>(Player->GetPlayTime());
	Packet.Write<int32_t>(Player->GetDeaths());
	Packet.Write<int32_t>(Player->GetMonsterKills());
	Packet.Write<int32_t>(Player->GetPlayerKills());
	Packet.Write<int32_t>(Player->GetBounty());

	// Write items
	Packet.Write<char>(ItemCount);
	for(int i = 0; i < _Player::INVENTORY_COUNT; i++) {
		if(Player->GetInventory(i)->Item) {
			Packet.Write<char>(i);
			Packet.Write<char>(Player->GetInventory(i)->Count);
			Packet.Write<int32_t>(Player->GetInventory(i)->Item->GetID());
		}
	}

	// Write skills
	Packet.Write<char>(SkillCount);
	for(int i = 0; i < _Player::SKILL_COUNT; i++) {
		if(Player->GetSkillLevel(i) > 0) {
			Packet.Write<int32_t>(Player->GetSkillLevel(i));
			Packet.Write<char>(i);
		}
	}

	// Write skill bar
	for(int i = 0; i < FIGHTER_MAXSKILLS; i++) {
		Packet.Write<char>(Player->GetSkillBarID(i));
	}
	ServerNetwork->SendPacketToPeer(&Packet, TPeer);

	// Send map and players to new player
	SpawnPlayer(Player, Player->GetSpawnMapID(), _Map::EVENT_SPAWN, Player->GetSpawnPoint());
}

// Handle a character delete request
void _PlayServerState::HandleCharacterDelete(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	char QueryString[512];

	// Get delete slot
	int Index = TPacket->Read<char>();

	// Get character ID
	int CharacterID = 0;
	sprintf(QueryString, "SELECT ID FROM Characters WHERE AccountsID = %d LIMIT %d, 1", Player->GetAccountID(), Index);
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
void _PlayServerState::HandleCharacterCreate(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	char QueryString[512];

	// Get character information
	std::string Name(TPacket->ReadString());
	int PortraitID = TPacket->Read<int32_t>();
	if(Name.size() > PLAYER_NAME_SIZE)
		return;

	// Check character limit
	sprintf(QueryString, "SELECT Count(ID) FROM Characters WHERE AccountsID = %d", Player->GetAccountID());
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
		_Buffer Packet;
		Packet.Write<char>(_Network::CREATECHARACTER_INUSE);
		ServerNetwork->SendPacketToPeer(&Packet, TPeer);
		return;
	}

	// Create the character
	Database->RunQuery("BEGIN TRANSACTION");
	sprintf(QueryString, "INSERT INTO Characters(AccountsID, Name, PortraitID, SkillBar0) VALUES(%d, '%s', %d, 0)", Player->GetAccountID(), Name.c_str(), PortraitID);
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
	_Buffer Packet;
	Packet.Write<char>(_Network::CREATECHARACTER_SUCCESS);
	ServerNetwork->SendPacketToPeer(&Packet, TPeer);
}

// Handles move commands from a client
void _PlayServerState::HandleMoveCommand(_Buffer *TPacket, ENetPeer *TPeer) {

	int Direction = TPacket->Read<char>();
	//printf("HandleMoveCommand: %d\n", Direction);

	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(Player->MovePlayer(Direction)) {

		// Handle events
		const _Tile *Tile = Player->GetTile();
		switch(Tile->EventType) {
			case _Map::EVENT_SPAWN:
				Player->SetSpawnMapID(Player->GetMapID());
				Player->SetSpawnPoint(Tile->EventData);
				Player->RestoreHealthMana();
				SendHUD(Player);
				Player->Save();
			break;
			case _Map::EVENT_MAPCHANGE:
				Player->GenerateNextBattle();
				SpawnPlayer(Player, Tile->EventData, _Map::EVENT_MAPCHANGE, Player->GetMapID());
			break;
			case _Map::EVENT_VENDOR:
				Player->SetState(_Player::STATE_VENDOR);
				Player->SetVendor(Stats.GetVendor(Tile->EventData));
				SendEvent(Player, Tile->EventType, Tile->EventData);
			break;
			case _Map::EVENT_TRADER:
				Player->SetState(_Player::STATE_TRADER);
				Player->SetTrader(Stats.GetTrader(Tile->EventData));
				SendEvent(Player, Tile->EventType, Tile->EventData);
			break;
			default:

				// Start a battle
				if(Player->GetNextBattle() <= 0) {

					// Get monsters
					std::vector<int> Monsters;
					Stats.GenerateMonsterListFromZone(Player->GetCurrentZone(), Monsters);
					size_t MonsterCount = Monsters.size();
					if(MonsterCount > 0) {

						// Create a new battle instance
						_ServerBattle *Battle = Instances->CreateServerBattle();

						// Add players
						Battle->AddFighter(Player, 0);
						if(1) {

							// Get a list of players
							std::list<_Player *> Players;
							Player->GetMap()->GetClosePlayers(Player, 7*7, Players);

							// Add players to battle
							int PlayersAdded = 0;
							for(std::list<_Player *>::iterator Iterator = Players.begin(); Iterator != Players.end(); ++Iterator) {
								_Player *PartyPlayer = *Iterator;
								if(PartyPlayer->GetState() == _Player::STATE_WALK && !PartyPlayer->IsInvisible()) {
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
							Battle->AddFighter(new _Monster(Monsters[i]), 1);
						}

						Battle->StartBattle();
					}
				}
			break;
		}
	}
}

// Handles battle commands from a client
void _PlayServerState::HandleBattleCommand(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	_ServerBattle *Battle = static_cast<_ServerBattle *>(Player->GetBattle());
	if(!Battle)
		return;

	int Command = TPacket->Read<char>();
	int Target = TPacket->Read<char>();
	Battle->HandleInput(Player, Command, Target);

	//printf("HandleBattleCommand: %d\n", Command);
}

// The client is done with the battle results screen
void _PlayServerState::HandleBattleFinished(_Buffer *TPacket, ENetPeer *TPeer) {
	//printf("HandleBattleFinished:\n");

	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	_ServerBattle *Battle = static_cast<_ServerBattle *>(Player->GetBattle());
	if(!Battle)
		return;

	// Check for the last player leaving the battle
	RemovePlayerFromBattle(Player);

	// Check for death
	if(Player->GetHealth() == 0) {
		Player->RestoreHealthMana();
		SpawnPlayer(Player, Player->GetSpawnMapID(), _Map::EVENT_SPAWN, Player->GetSpawnPoint());
		Player->Save();
	}
	SendHUD(Player);
}

// Handles a player's inventory move
void _PlayServerState::HandleInventoryMove(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	int OldSlot = TPacket->Read<char>();
	int NewSlot = TPacket->Read<char>();

	// Move items
	Player->MoveInventory(OldSlot, NewSlot);
	Player->CalculatePlayerStats();

	// Check for trading players
	_Player *TradePlayer = Player->GetTradePlayer();
	if(Player->GetState() == _Player::STATE_TRADE && TradePlayer && (_Player::IsSlotTrade(OldSlot) || _Player::IsSlotTrade(NewSlot))) {

		// Reset agreement
		Player->SetTradeAccepted(false);
		TradePlayer->SetTradeAccepted(false);

		// Send item updates to trading player
		_InventorySlot *OldSlotItem = Player->GetInventory(OldSlot);
		_InventorySlot *NewSlotItem = Player->GetInventory(NewSlot);

		// Get item information
		int OldItemID = 0, NewItemID = 0, OldItemCount = 0, NewItemCount = 0;
		if(OldSlotItem->Item) {
			OldItemID = OldSlotItem->Item->GetID();
			OldItemCount = OldSlotItem->Count;
		}
		if(NewSlotItem->Item) {
			NewItemID = NewSlotItem->Item->GetID();
			NewItemCount = NewSlotItem->Count;
		}

		// Build packet
		_Buffer Packet;
		Packet.Write<char>(_Network::TRADE_ITEM);
		Packet.Write<int32_t>(OldItemID);
		Packet.Write<char>(OldSlot);
		if(OldItemID > 0)
			Packet.Write<char>(OldItemCount);
		Packet.Write<int32_t>(NewItemID);
		Packet.Write<char>(NewSlot);
		if(NewItemID > 0)
			Packet.Write<char>(NewItemCount);

		// Send updates
		ServerNetwork->SendPacketToPeer(&Packet, TradePlayer->GetPeer());
	}
}

// Handle a player's inventory use request
void _PlayServerState::HandleInventoryUse(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	// Use an item
	Player->UseInventory(TPacket->Read<char>());
}

// Handle a player's inventory split stack request
void _PlayServerState::HandleInventorySplit(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	int Slot = TPacket->Read<char>();
	int Count = TPacket->Read<char>();

	// Inventory only
	if(!_Player::IsSlotInventory(Slot))
		return;

	Player->SplitStack(Slot, Count);
}

// Handles a player's event end message
void _PlayServerState::HandleEventEnd(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	Player->SetVendor(nullptr);
	Player->SetState(_Player::STATE_WALK);
}

// Handles a vendor exchange message
void _PlayServerState::HandleVendorExchange(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	// Get vendor
	const _Vendor *Vendor = Player->GetVendor();
	if(!Vendor)
		return;

	// Get info
	bool Buy = TPacket->ReadBit();
	int Amount = TPacket->Read<char>();
	int Slot = TPacket->Read<char>();
	if(Slot < 0)
		return;

	// Update player
	if(Buy) {
		if(Slot >= (int)Vendor->Items.size())
			return;

		// Get optional inventory slot
		int TargetSlot = TPacket->Read<char>();

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
		_InventorySlot *Item = Player->GetInventory(Slot);
		if(Item && Item->Item) {
			int Price = Item->Item->GetPrice(Vendor, Amount, Buy);
			Player->UpdateGold(Price);
			Player->UpdateInventory(Slot, -Amount);
		}
	}
}

// Handle a skill bar change
void _PlayServerState::HandleSkillBar(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	// Read skills
	for(int i = 0; i < 8; i++) {
		Player->SetSkillBar(i, Stats.GetSkill(TPacket->Read<char>()));
	}

	Player->CalculatePlayerStats();
}

// Handles a skill adjust
void _PlayServerState::HandleSkillAdjust(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	// Process packet
	bool Spend = TPacket->ReadBit();
	int SkillID = TPacket->Read<char>();
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
void _PlayServerState::HandlePlayerBusy(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	bool Value = TPacket->ReadBit();
	Player->ToggleBusy(Value);

	//printf("HandlePlayerBusy: Value=%d\n", Value);
}

// Handles a player's request to attack another player
void _PlayServerState::HandleAttackPlayer(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player || !Player->CanAttackPlayer())
		return;

	_Map *Map = Player->GetMap();
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
			if(VictimPlayer->GetState() != _Player::STATE_BATTLE) {
				_ServerBattle *Battle = Instances->CreateServerBattle();
				Battle->AddFighter(Player, 1);
				Battle->AddFighter(VictimPlayer, 0);
				Battle->StartBattle();
				break;
			}
		}
	}
}

// Handle a chat message
void _PlayServerState::HandleChatMessage(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	// Get map object
	_Map *Map = Player->GetMap();
	if(!Map)
		return;

	// Get message
	char Message[256];
	strncpy(Message, TPacket->ReadString(), NETWORKING_CHAT_SIZE);
	Message[NETWORKING_CHAT_SIZE] = 0;

	// Send message to other players
	_Buffer Packet;
	Packet.Write<char>(_Network::CHAT_MESSAGE);
	Packet.Write<char>(Player->GetNetworkID());
	Packet.WriteString(Message);

	Map->SendPacketToPlayers(&Packet, Player);
}

// Handle a trade request
void _PlayServerState::HandleTradeRequest(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	// Get map object
	_Map *Map = Player->GetMap();
	if(!Map)
		return;

	// Find the nearest player to trade with
	_Player *TradePlayer = Map->GetClosestPlayer(Player, 2.0f * 2.0f, _Player::STATE_WAITTRADE);
	if(TradePlayer == nullptr) {

		// Set up trade post
		Player->SetState(_Player::STATE_WAITTRADE);
		Player->SetTradeGold(0);
		Player->SetTradeAccepted(false);
		Player->SetTradePlayer(nullptr);
	}
	else {

		// Set up trade screen for both players
		SendTradeInformation(Player, TradePlayer);
		SendTradeInformation(TradePlayer, Player);

		Player->SetTradePlayer(TradePlayer);
		Player->SetTradeAccepted(false);
		TradePlayer->SetTradePlayer(Player);
		TradePlayer->SetTradeAccepted(false);

		Player->SetState(_Player::STATE_TRADE);
		TradePlayer->SetState(_Player::STATE_TRADE);
	}
}

// Handles a trade cancel
void _PlayServerState::HandleTradeCancel(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	// Notify trading player
	_Player *TradePlayer = Player->GetTradePlayer();
	if(TradePlayer) {
		TradePlayer->SetState(_Player::STATE_WAITTRADE);
		TradePlayer->SetTradePlayer(nullptr);
		TradePlayer->SetTradeAccepted(false);

		_Buffer Packet;
		Packet.Write<char>(_Network::TRADE_CANCEL);
		ServerNetwork->SendPacketToPeer(&Packet, TradePlayer->GetPeer());
	}

	// Set state back to normal
	Player->SetState(_Player::STATE_WALK);
}

// Handle a trade gold update
void _PlayServerState::HandleTradeGold(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	// Set gold amount
	int Gold = TPacket->Read<int32_t>();
	if(Gold < 0)
		Gold = 0;
	else if(Gold > Player->GetGold())
		Gold = Player->GetGold();
	Player->SetTradeGold(Gold);
	Player->SetTradeAccepted(false);

	// Notify player
	_Player *TradePlayer = Player->GetTradePlayer();
	if(TradePlayer) {
		TradePlayer->SetTradeAccepted(false);

		_Buffer Packet;
		Packet.Write<char>(_Network::TRADE_GOLD);
		Packet.Write<int32_t>(Gold);
		ServerNetwork->SendPacketToPeer(&Packet, TradePlayer->GetPeer());
	}
}

// Handles a trade accept from a player
void _PlayServerState::HandleTradeAccept(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	// Get trading player
	_Player *TradePlayer = Player->GetTradePlayer();
	if(TradePlayer) {

		// Set the player's state
		bool Accepted = !!TPacket->Read<char>();
		Player->SetTradeAccepted(Accepted);

		// Check if both player's agree
		if(Accepted && TradePlayer->GetTradeAccepted()) {

			// Exchange items
			_InventorySlot TempItems[PLAYER_TRADEITEMS];
			for(int i = 0; i < PLAYER_TRADEITEMS; i++) {
				int InventorySlot = i + _Player::INVENTORY_TRADE;
				TempItems[i] = *Player->GetInventory(InventorySlot);

				Player->SetInventory(InventorySlot, TradePlayer->GetInventory(InventorySlot));
				TradePlayer->SetInventory(InventorySlot, &TempItems[i]);
			}

			// Exchange gold
			Player->UpdateGold(TradePlayer->GetTradeGold() - Player->GetTradeGold());
			TradePlayer->UpdateGold(Player->GetTradeGold() - TradePlayer->GetTradeGold());

			// Send packet to players
			{
				_Buffer Packet;
				Packet.Write<char>(_Network::TRADE_EXCHANGE);
				BuildTradeItemsPacket(Player, &Packet, Player->GetGold());
				ServerNetwork->SendPacketToPeer(&Packet, Player->GetPeer());
			}
			{
				_Buffer Packet;
				Packet.Write<char>(_Network::TRADE_EXCHANGE);
				BuildTradeItemsPacket(TradePlayer, &Packet, TradePlayer->GetGold());
				ServerNetwork->SendPacketToPeer(&Packet, TradePlayer->GetPeer());
			}

			Player->SetState(_Player::STATE_WALK);
			Player->SetTradePlayer(nullptr);
			Player->SetTradeGold(0);
			Player->MoveTradeToInventory();
			TradePlayer->SetState(_Player::STATE_WALK);
			TradePlayer->SetTradePlayer(nullptr);
			TradePlayer->SetTradeGold(0);
			TradePlayer->MoveTradeToInventory();
		}
		else {

			// Notify trading player
			_Buffer Packet;
			Packet.Write<char>(_Network::TRADE_ACCEPT);
			Packet.Write<char>(Accepted);
			ServerNetwork->SendPacketToPeer(&Packet, TradePlayer->GetPeer());
		}
	}
}

// Handles a town portal request
void _PlayServerState::HandleTownPortal(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	Player->StartTownPortal();
}

// Handles a trader accept
void _PlayServerState::HandleTraderAccept(_Buffer *TPacket, ENetPeer *TPeer) {
	_Player *Player = static_cast<_Player *>(TPeer->data);
	if(!Player)
		return;

	const _Trader *Trader = Player->GetTrader();
	if(!Trader)
		return;

	// Get trader information
	int RequiredItemSlots[8];
	int RewardSlot = Player->GetRequiredItemSlots(Trader, RequiredItemSlots);
	if(RewardSlot == -1)
		return;

	// Exchange items
	Player->AcceptTrader(Trader, RequiredItemSlots, RewardSlot);
	Player->SetTrader(nullptr);
	Player->SetState(_Player::STATE_WALK);
	Player->CalculatePlayerStats();
}

// Spawns a player at a particular spawn point
void _PlayServerState::SpawnPlayer(_Player *TPlayer, int TNewMapID, int TEventType, int TEventData) {

	// Get new map
	_Map *NewMap = Instances->GetMap(TNewMapID);

	// Remove old player if map has changed
	_Map *OldMap = TPlayer->GetMap();
	if(OldMap && NewMap != OldMap)
		OldMap->RemoveObject(TPlayer);

	// Get spawn position
	_IndexedEvent *SpawnEvent = NewMap->GetIndexedEvent(TEventType, TEventData);
	if(SpawnEvent) {
		TPlayer->SetPosition(SpawnEvent->Position);
		SendPlayerPosition(TPlayer);
	}

	// Set state
	TPlayer->SetState(_Player::STATE_WALK);

	// Send new object list
	if(NewMap != OldMap) {

		// Update pointers
		TPlayer->SetMap(NewMap);

		// Add player to map
		NewMap->AddObject(TPlayer);

		// Build packet for player
		_Buffer Packet;
		Packet.Write<char>(_Network::WORLD_CHANGEMAPS);

		// Send player info
		Packet.Write<int32_t>(TNewMapID);

		// Write object data
		std::list<_Object *> Objects = NewMap->GetObjects();
		Packet.Write<int32_t>(Objects.size());
		for(std::list<_Object *>::iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
			_Object *Object = *Iterator;

			Packet.Write<char>(Object->GetNetworkID());
			Packet.Write<char>(Object->GetPosition().X);
			Packet.Write<char>(Object->GetPosition().Y);
			Packet.Write<char>(Object->GetType());
			switch(Object->GetType()) {
				case _Object::PLAYER: {
					_Player *Player = static_cast<_Player *>(Object);
					Packet.WriteString(Player->GetName().c_str());
					Packet.Write<char>(Player->GetPortraitID());
					Packet.WriteBit((Player->IsInvisible()));
				}
				break;
				default:
				break;
			}
		}

		// Send object list to the player
		ServerNetwork->SendPacketToPeer(&Packet, TPlayer->GetPeer());
	}

	//printf("SpawnPlayer: MapID=%d, NetworkID=%d\n", TNewMapID, TPlayer->GetNetworkID());
}

// Updates the player's HUD
void _PlayServerState::SendHUD(_Player *TPlayer) {

	_Buffer Packet;
	Packet.Write<char>(_Network::WORLD_HUD);
	Packet.Write<int32_t>(TPlayer->GetExperience());
	Packet.Write<int32_t>(TPlayer->GetGold());
	Packet.Write<int32_t>(TPlayer->GetHealth());
	Packet.Write<int32_t>(TPlayer->GetMana());
	Packet.Write<float>(TPlayer->GetHealthAccumulator());
	Packet.Write<float>(TPlayer->GetManaAccumulator());

	ServerNetwork->SendPacketToPeer(&Packet, TPlayer->GetPeer());
}

// Send player their position
void _PlayServerState::SendPlayerPosition(_Player *TPlayer) {

	_Buffer Packet;
	Packet.Write<char>(_Network::WORLD_POSITION);
	Packet.Write<char>(TPlayer->GetPosition().X);
	Packet.Write<char>(TPlayer->GetPosition().Y);

	ServerNetwork->SendPacketToPeer(&Packet, TPlayer->GetPeer());
}

// Sends the player a list of his/her characters
void _PlayServerState::SendCharacterList(_Player *TPlayer) {
	char QueryString[512];

	// Get a count of the account's characters
	sprintf(QueryString, "SELECT Count(ID) FROM Characters WHERE AccountsID = %d", TPlayer->GetAccountID());
	int CharacterCount = Database->RunCountQuery(QueryString);

	// Create the packet
	_Buffer Packet;
	Packet.Write<char>(_Network::CHARACTERS_LIST);
	Packet.Write<char>(CharacterCount);

	// Generate a list of characters
	sprintf(QueryString, "SELECT Name, PortraitID, Experience FROM Characters WHERE AccountsID = %d", TPlayer->GetAccountID());
	Database->RunDataQuery(QueryString);
	while(Database->FetchRow()) {
		Packet.WriteString(Database->GetString(0));
		Packet.Write<int32_t>(Database->GetInt(1));
		Packet.Write<int32_t>(Database->GetInt(2));
	}
	Database->CloseQuery();

	// Send list
	ServerNetwork->SendPacketToPeer(&Packet, TPlayer->GetPeer());
}

// Sends a player an event message
void _PlayServerState::SendEvent(_Player *TPlayer, int TType, int TData) {

	// Create packet
	_Buffer Packet;
	Packet.Write<char>(_Network::EVENT_START);
	Packet.Write<char>(TType);
	Packet.Write<int32_t>(TData);
	Packet.Write<char>(TPlayer->GetPosition().X);
	Packet.Write<char>(TPlayer->GetPosition().Y);

	ServerNetwork->SendPacketToPeer(&Packet, TPlayer->GetPeer());
}

// Sends information to another player about items they're trading
void _PlayServerState::SendTradeInformation(_Player *TSender, _Player *TReceiver) {

	// Send items to trader player
	_Buffer Packet;
	Packet.Write<char>(_Network::TRADE_REQUEST);
	Packet.Write<char>(TSender->GetNetworkID());
	BuildTradeItemsPacket(TSender, &Packet, TSender->GetTradeGold());
	ServerNetwork->SendPacketToPeer(&Packet, TReceiver->GetPeer());
}

// Adds trade item information to a packet
void _PlayServerState::BuildTradeItemsPacket(_Player *TPlayer, _Buffer *TPacket, int TGold) {
	TPacket->Write<int32_t>(TGold);
	for(int i = _Player::INVENTORY_TRADE; i < _Player::INVENTORY_COUNT; i++) {
		if(TPlayer->GetInventory(i)->Item) {
			TPacket->Write<int32_t>(TPlayer->GetInventory(i)->Item->GetID());
			TPacket->Write<char>(TPlayer->GetInventory(i)->Count);
		}
		else
			TPacket->Write<int32_t>(0);
	}
}

// Removes a player from a battle and deletes the battle if necessary
void _PlayServerState::RemovePlayerFromBattle(_Player *TPlayer) {
	_ServerBattle *Battle = static_cast<_ServerBattle *>(TPlayer->GetBattle());
	if(!Battle)
		return;

	// Delete instance
	if(Battle->RemovePlayer(TPlayer) == 0) {
		Instances->DeleteBattle(Battle);
	}
}

// Deletes an object on the server and broadcasts it to the clients
void _PlayServerState::DeleteObject(_Object *TObject) {

	// Remove the object from their current map
	_Map *Map = TObject->GetMap();
	if(Map) {
		Map->RemoveObject(TObject);
	}
}

// Called when object gets deleted
void ObjectDeleted(_Object *TObject) {

	PlayServerState.DeleteObject(TObject);
}

// Teleports a player back to town
void _PlayServerState::PlayerTownPortal(_Player *TPlayer) {

	TPlayer->RestoreHealthMana();
	SpawnPlayer(TPlayer, TPlayer->GetSpawnMapID(), _Map::EVENT_SPAWN, TPlayer->GetSpawnPoint());
	SendHUD(TPlayer);
	TPlayer->Save();
}
