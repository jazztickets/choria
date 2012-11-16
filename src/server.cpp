/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#include "server.h"
#include "engine/database.h"
#include "engine/objectmanager.h"
#include "engine/instances.h"
#include "engine/stats.h"
#include "engine/globals.h"
#include "engine/config.h"
#include "engine/game.h"
#include "network/network.h"
#include "network/packetstream.h"
#include "instances/map.h"
#include "instances/serverbattle.h"
#include "objects/player.h"
#include "objects/monster.h"

static void ObjectDeleted(ObjectClass *Object);

ServerStateClass ServerState;

// Initializes the state
int ServerStateClass::Init() {

	ServerTime = 0;

	// Load database that stores accounts and characters
	Database = new DatabaseClass();

	stringc DatabasePath;
	Config.GetSavePath("server.s3db", DatabasePath);
	if(!Database->OpenDatabase(DatabasePath.c_str())) {

		// Create a new database
		printf("Creating new database...\n");
		if(!Database->OpenDatabaseCreate(DatabasePath.c_str())) {
			return 0;
		}

		// Populate data
		CreateDefaultDatabase();
	}

	Instances = new InstanceClass();
	ObjectManager = new ObjectManagerClass();
	ObjectManager->SetObjectDeletedCallback(ObjectDeleted);

	return 1;
}

// Shuts the state down
int ServerStateClass::Close() {

	delete Database;
	delete ObjectManager;
	delete Instances;

	return 1;
}

// Populates the server database with the default data
void ServerStateClass::CreateDefaultDatabase() {

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
						", 'ActionBarLength' INTEGER DEFAULT(0)"
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
	Database->RunQuery(	"CREATE TABLE Skills("
						"'CharactersID' INTEGER"
						", 'SkillsID' INTEGER"
						")");
	Database->RunQuery(	"CREATE TABLE ActionBar("
					"'CharactersID' INTEGER"
					", 'Type' INTEGER"
					", 'ActionID' INTEGER"
					", 'Slot' INTEGER"
					")");

	Database->RunQuery("INSERT INTO ServerInfo(Version) VALUES(" SERVER_DATABASEVERSION ")");
	Database->RunQuery("INSERT INTO Accounts(Username, Password) VALUES('singleplayer', 'singleplayer')");
	Database->RunQuery("END TRANSACTION");
}

// Handles a new client connection
void ServerStateClass::HandleConnect(ENetEvent *Event) {
	printf("HandleConnect: %x:%x\n", Event->peer->address.host, Event->peer->address.port);

	// Create the player and add it to the object list
	PlayerClass *NewPlayer = new PlayerClass();
	ObjectManager->AddObject(NewPlayer);

	// Store the player in the peer struct
	Event->peer->data = NewPlayer;
	NewPlayer->SetPeer(Event->peer);

	// Send player the game version
	PacketClass Packet(NetworkClass::GAME_VERSION);
	Packet.WriteInt(GAME_VERSION);
	ServerNetwork->SendPacketToPeer(&Packet, Event->peer);
}

// Handles a client disconnect
void ServerStateClass::HandleDisconnect(ENetEvent *Event) {
	printf("HandleDisconnect: %x:%x\n", Event->peer->address.host, Event->peer->address.port);

	PlayerClass *Player = static_cast<PlayerClass *>(Event->peer->data);
	if(!Player)
		return;

	// Leave trading screen
	PlayerClass *TradePlayer = Player->GetTradePlayer();
	if(TradePlayer) {
		TradePlayer->SetTradePlayer(NULL);

		PacketClass Packet(NetworkClass::TRADE_CANCEL);
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
void ServerStateClass::HandlePacket(ENetEvent *Event) {
	//printf("HandlePacket: type=%d\n", Event->packet->data[0]);

	PacketClass Packet(Event->packet);
	int PacketType = Packet.ReadChar();
	switch(PacketType) {
		case NetworkClass::ACCOUNT_LOGININFO:
			HandleLoginInfo(&Packet, Event->peer);
		break;
		case NetworkClass::CHARACTERS_REQUEST:
			HandleCharacterListRequest(&Packet, Event->peer);
		break;
		case NetworkClass::CHARACTERS_PLAY:
			HandleCharacterSelect(&Packet, Event->peer);
		break;
		case NetworkClass::CHARACTERS_DELETE:
			HandleCharacterDelete(&Packet, Event->peer);
		break;
		case NetworkClass::CREATECHARACTER_INFO:
			HandleCharacterCreate(&Packet, Event->peer);
		break;
		case NetworkClass::WORLD_MOVECOMMAND:
			HandleMoveCommand(&Packet, Event->peer);
		break;
		case NetworkClass::BATTLE_ACTION:
			HandleBattleAction(&Packet, Event->peer);
		break;
		case NetworkClass::BATTLE_CLIENTDONE:
			HandleBattleFinished(&Packet, Event->peer);
		break;
		case NetworkClass::INVENTORY_MOVE:
			HandleInventoryMove(&Packet, Event->peer);
		break;
		case NetworkClass::INVENTORY_USE:
			HandleInventoryUse(&Packet, Event->peer);
		break;
		case NetworkClass::INVENTORY_SPLIT:
			HandleInventorySplit(&Packet, Event->peer);
		break;
		case NetworkClass::EVENT_END:
			HandleEventEnd(&Packet, Event->peer);
		break;
		case NetworkClass::VENDOR_EXCHANGE:
			HandleVendorExchange(&Packet, Event->peer);
		break;
		case NetworkClass::TRADER_ACCEPT:
			HandleTraderAccept(&Packet, Event->peer);
		break;
		case NetworkClass::ACTIONS_ACTIONBAR:
			HandleActionBar(&Packet, Event->peer);
		break;
		case NetworkClass::WORLD_BUSY:
			HandlePlayerBusy(&Packet, Event->peer);
		break;
		case NetworkClass::WORLD_ATTACKPLAYER:
			HandleAttackPlayer(&Packet, Event->peer);
		break;
		case NetworkClass::WORLD_TOWNPORTAL:
			HandleTownPortal(&Packet, Event->peer);
		break;
		case NetworkClass::CHAT_MESSAGE:
			HandleChatMessage(&Packet, Event->peer);
		break;
		case NetworkClass::TRADE_REQUEST:
			HandleTradeRequest(&Packet, Event->peer);
		break;
		case NetworkClass::TRADE_CANCEL:
			HandleTradeCancel(&Packet, Event->peer);
		break;
		case NetworkClass::TRADE_GOLD:
			HandleTradeGold(&Packet, Event->peer);
		break;
		case NetworkClass::TRADE_ACCEPT:
			HandleTradeAccept(&Packet, Event->peer);
		break;
	}
}

// Updates the current state
void ServerStateClass::Update(u32 FrameTime) {
	ServerTime += FrameTime;

	Instances->Update(FrameTime);
	ObjectManager->Update(FrameTime);
}

// Login information
void ServerStateClass::HandleLoginInfo(PacketClass *Packet, ENetPeer *Peer) {
	char QueryString[512];

	// Read packet
	bool CreateAccount = Packet->ReadBit();
	stringc Username(Packet->ReadString());
	stringc Password(Packet->ReadString());
	if(Username.size() > 15 || Password.size() > 15)
		return;
	Username.make_lower();
	Password.make_lower();

	//printf("HandleLoginInfo: CreateAccount=%d, username=%s, password=%s\n", CreateAccount, Username.c_str(), Password.c_str());

	// Create account or login
	if(CreateAccount) {

		// Check for existing account
		sprintf(QueryString, "SELECT ID FROM Accounts WHERE Username = '%s'", Username.c_str());
		Database->RunDataQuery(QueryString);
		int Result = Database->FetchRow();
		Database->CloseQuery();

		if(Result) {
			PacketClass Packet(NetworkClass::ACCOUNT_EXISTS);
			ServerNetwork->SendPacketToPeer(&Packet, Peer);
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

		PacketClass Packet(NetworkClass::ACCOUNT_NOTFOUND);
		ServerNetwork->SendPacketToPeer(&Packet, Peer);
	}
	else {

		// Store the account id
		PlayerClass *Player = (PlayerClass *)Peer->data;
		Player->SetAccountID(AccountID);

		PacketClass Packet(NetworkClass::ACCOUNT_SUCCESS);
		ServerNetwork->SendPacketToPeer(&Packet, Peer);
		//printf("HandleLoginInfo: AccountID=%d, CharacterCount=%d\n", AccountID, CharacterCount);
	}
}

// Sends a player his/her character list
void ServerStateClass::HandleCharacterListRequest(PacketClass *Packet, ENetPeer *Peer) {

	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	SendCharacterList(Player);
}

// Loads the player, updates the world, notifies clients
void ServerStateClass::HandleCharacterSelect(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	int Slot = Packet->ReadChar();

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
	Player->SetName(stringc(Database->GetString(4)));
	Player->SetPortraitID(Database->GetInt(5));
	Player->SetExperience(Database->GetInt(6));
	Player->SetGold(Database->GetInt(7));
	Player->SetActionBarLength(Database->GetInt(8));
	Player->SetPlayTime(Database->GetInt(9));
	Player->SetDeaths(Database->GetInt(10));
	Player->SetMonsterKills(Database->GetInt(11));
	Player->SetPlayerKills(Database->GetInt(12));
	Player->SetBounty(Database->GetInt(13));

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
	sprintf(QueryString, "SELECT SkillsID FROM Skills WHERE CharactersID = %d", Player->GetCharacterID());
	Database->RunDataQuery(QueryString);
	int SkillCount = 0;
	while(Database->FetchRow()) {
		int SkillID = Database->GetInt(0);
		Player->AddSkillUnlocked(Stats.GetSkill(SkillID));
		SkillCount++;
	}
	Database->CloseQuery();

	// Set action bar
	sprintf(QueryString, "SELECT Type, ActionID, Slot FROM ActionBar WHERE CharactersID = %d", Player->GetCharacterID());
	Database->RunDataQuery(QueryString);
	int ActionBarCount = 0;
	while(Database->FetchRow()) {
		int Type = Database->GetInt(0);
		int ActionID = Database->GetInt(1);
		int Slot = Database->GetInt(2);

		// Get action from database
		const ActionClass *Action;
		if(Type == ActionClass::YPE_SKILL)
			Action = Stats.GetSkill(ActionID);
		else
			Action = Stats.GetItem(ActionID);
		
		Player->SetActionBar(Slot, Action);
		ActionBarCount++;
	}
	
	Database->CloseQuery();

	// Get stats
	Player->CalculatePlayerStats();
	Player->RestoreHealthMana();

	// Send character packet
	PacketClass NewPacket(NetworkClass::WORLD_YOURCHARACTERINFO);
	NewPacket.WriteInt(Player->NetworkID);
	NewPacket.WriteString(Player->GetName().c_str());
	NewPacket.WriteInt(Player->GetPortraitID());
	NewPacket.WriteInt(Player->GetActionBarLength());
	NewPacket.WriteInt(Player->GetExperience());
	NewPacket.WriteInt(Player->GetGold());
	NewPacket.WriteInt(Player->GetPlayTime());
	NewPacket.WriteInt(Player->GetDeaths());
	NewPacket.WriteInt(Player->GetMonsterKills());
	NewPacket.WriteInt(Player->GetPlayerKills());
	NewPacket.WriteInt(Player->GetBounty());

	// Write items
	NewPacket.WriteChar(ItemCount);
	for(int i = 0; i < PlayerClass::INVENTORY_COUNT; i++) {
		if(Player->GetInventory(i)->Item) {
			NewPacket.WriteChar(i);
			NewPacket.WriteChar(Player->GetInventory(i)->Count);
			NewPacket.WriteInt(Player->GetInventory(i)->Item->GetID());
		}
	}

	// Write skills
	const array<const SkillClass *> &SkillsUnlocked = Player->GetSkillsUnlocked();
	NewPacket.WriteChar(SkillsUnlocked.size());
	for(u32 i = 0; i < SkillsUnlocked.size(); i++) {
		NewPacket.WriteChar(SkillsUnlocked[i]->GetID());
	}

	// Write action bar
	NewPacket.WriteChar(ActionBarCount);
	for(int i = 0; i < Player->GetActionBarLength(); i++) {
		const ActionClass *Action = Player->GetActionBar(i);
		if(Action) {
			NewPacket.WriteChar(Action->GetType());
			NewPacket.WriteChar(Action->GetID());
			NewPacket.WriteChar(i);
		}
	}

	ServerNetwork->SendPacketToPeer(&NewPacket, Peer);

	// Send map and players to new player
	SpawnPlayer(Player, Player->GetSpawnMapID(), MapClass::EVENT_SPAWN, Player->GetSpawnPoint());

	//printf("HandleCharacterSelect: accountid=%d, slot=%d\n", Player->GetAccountID(), Slot);
}

// Handle a character delete request
void ServerStateClass::HandleCharacterDelete(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	char QueryString[512];

	// Get delete slot
	int Index = Packet->ReadChar();

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
	sprintf(QueryString, "DELETE FROM Skills WHERE CharactersID = %d", CharacterID);
	Database->RunQuery(QueryString);

	// Delete action bar
	sprintf(QueryString, "DELETE FROM ActionBar WHERE CharactersID = %d", CharacterID);
	Database->RunQuery(QueryString);

	// Update the player
	SendCharacterList(Player);
}

// Handles the character create request
void ServerStateClass::HandleCharacterCreate(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	char QueryString[512];

	// Get character information
	stringc Name(Packet->ReadString());
	int PortraitID = Packet->ReadInt();
	if(Name.size() > 10)
		return;

	// Check character limit
	sprintf(QueryString, "SELECT Count(ID) FROM Characters WHERE AccountsID = %d", Player->GetAccountID());
	int CharacterCount = Database->RunCountQuery(QueryString);
	if(CharacterCount >= 6)
		return;

	// Check for existing names
	sprintf(QueryString, "SELECT ID FROM Characters WHERE Name = '%s'", Name.c_str());
	Database->RunDataQuery(QueryString);
	int FindResult = Database->FetchRow();
	Database->CloseQuery();

	// Found an existing name
	if(FindResult) {
		PacketClass Packet(NetworkClass::CREATECHARACTER_INUSE);
		ServerNetwork->SendPacketToPeer(&Packet, Peer);
		return;
	}

	// Create the character
	Database->RunQuery("BEGIN TRANSACTION");
	sprintf(QueryString, "INSERT INTO Characters(AccountsID, Name, PortraitID, ActionBarLength) VALUES(%d, '%s', %d, %d)", Player->GetAccountID(), Name.c_str(), PortraitID, GAME_SKILLBARLENGTH);
	Database->RunQuery(QueryString);

	int CharacterID = Database->GetLastInsertID();
	sprintf(QueryString, "INSERT INTO Inventory VALUES(%d, 1, 2, 1)", CharacterID);
	Database->RunQuery(QueryString);
	sprintf(QueryString, "INSERT INTO Inventory VALUES(%d, 3, 1, 1)", CharacterID);
	Database->RunQuery(QueryString);
	sprintf(QueryString, "INSERT INTO Skills VALUES(%d, 1)", CharacterID);
	Database->RunQuery(QueryString);
	sprintf(QueryString, "INSERT INTO ActionBar VALUES(%d, 0, 1, 0)", CharacterID);
	Database->RunQuery(QueryString);
	Database->RunQuery("END TRANSACTION");

	// Notify the client
	PacketClass NewPacket(NetworkClass::CREATECHARACTER_SUCCESS);
	ServerNetwork->SendPacketToPeer(&NewPacket, Peer);
}

// Handles move commands from a client
void ServerStateClass::HandleMoveCommand(PacketClass *Packet, ENetPeer *Peer) {

	int Direction = Packet->ReadChar();
	//printf("HandleMoveCommand: %d\n", Direction);

	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(Player->MovePlayer(Direction)) {

		// Handle events
		const TileStruct *Tile = Player->GetTile();
		switch(Tile->EventType) {
			case MapClass::EVENT_SPAWN:
				Player->SetSpawnMapID(Player->GetMapID());
				Player->SetSpawnPoint(Tile->EventData);
				Player->RestoreHealthMana();
				SendHUD(Player);
				Player->Save();
			break;
			case MapClass::EVENT_MAPCHANGE:
				Player->GenerateNextBattle();
				SpawnPlayer(Player, Tile->EventData, MapClass::EVENT_MAPCHANGE, Player->GetMapID());
			break;
			case MapClass::EVENT_VENDOR:
				Player->SetState(PlayerClass::STATE_VENDOR);
				Player->SetVendor(Stats.GetVendor(Tile->EventData));
				SendEvent(Player, Tile->EventType, Tile->EventData);
			break;
			case MapClass::EVENT_TRADER:
				Player->SetState(PlayerClass::STATE_TRADER);
				Player->SetTrader(Stats.GetTrader(Tile->EventData));
				SendEvent(Player, Tile->EventType, Tile->EventData);
			break;
			default:

				// Start a battle
				if(Player->GetNextBattle() <= 0) {

					// Get monsters
					array<int> Monsters;
					Stats.GenerateMonsterListFromZone(Player->GetCurrentZone(), Monsters);
					u32 MonsterCount = Monsters.size();
					if(MonsterCount > 0) {

						// Create a new battle instance
						ServerBattleClass *Battle = Instances->CreateServerBattle();

						// Add players
						Battle->AddFighter(Player, 0);
						if(1) {

							// Get a list of players
							list<PlayerClass *> Players;
							Player->Map->GetClosePlayers(Player, 7*7, Players);

							// Add players to battle
							int PlayersAdded = 0;
							for(list<PlayerClass *>::Iterator Iterator = Players.begin(); Iterator != Players.end(); ++Iterator) {
								PlayerClass *PartyPlayer = *Iterator;
								if(PartyPlayer->GetState() == PlayerClass::STATE_WALK) {
									SendPlayerPosition(PartyPlayer);
									Battle->AddFighter(PartyPlayer, 0);
									PlayersAdded++;
									if(PlayersAdded == 2)
										break;
								}
							}						
						}

						// Add monsters
						for(u32 i = 0; i < Monsters.size(); i++) {
							Battle->AddFighter(new MonsterClass(Monsters[i]), 1);
						}

						Battle->StartBattle();
					}
				}
			break;
		}
	}
}

// Handles battle commands from a client
void ServerStateClass::HandleBattleAction(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	ServerBattleClass *Battle = static_cast<ServerBattleClass *>(Player->GetBattle());
	if(!Battle)
		return;

	int Command = Packet->ReadChar();
	int Target = Packet->ReadChar();
	Battle->HandleInput(Player, Command, Target);

	//printf("HandleBattleAction: %d\n", Command);
}

// The client is done with the battle results screen
void ServerStateClass::HandleBattleFinished(PacketClass *Packet, ENetPeer *Peer) {
	//printf("HandleBattleFinished:\n");

	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	ServerBattleClass *Battle = static_cast<ServerBattleClass *>(Player->GetBattle());
	if(!Battle)
		return;

	// Check for the last player leaving the battle
	RemovePlayerFromBattle(Player);

	// Check for death
	if(Player->GetHealth() == 0) {
		Player->RestoreHealthMana();
		SpawnPlayer(Player, Player->GetSpawnMapID(), MapClass::EVENT_SPAWN, Player->GetSpawnPoint());
		Player->Save();
	}
	SendHUD(Player);
}

// Handles a player's inventory move
void ServerStateClass::HandleInventoryMove(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	int OldSlot = Packet->ReadChar();
	int NewSlot = Packet->ReadChar();

	// Move items
	Player->MoveInventory(OldSlot, NewSlot);
	Player->CalculatePlayerStats();

	// Check for trading players
	PlayerClass *TradePlayer = Player->GetTradePlayer();
	if(Player->GetState() == PlayerClass::STATE_TRADE && TradePlayer && (PlayerClass::IsSlotTrade(OldSlot) || PlayerClass::IsSlotTrade(NewSlot))) {

		// Reset agreement
		Player->SetTradeAccepted(false);
		TradePlayer->SetTradeAccepted(false);

		// Send item updates to trading player
		InventoryStruct *OldSlotItem = Player->GetInventory(OldSlot);
		InventoryStruct *NewSlotItem = Player->GetInventory(NewSlot);

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
		PacketClass Packet(NetworkClass::TRADE_ITEM);
		Packet.WriteInt(OldItemID);
		Packet.WriteChar(OldSlot);
		if(OldItemID > 0)
			Packet.WriteChar(OldItemCount);
		Packet.WriteInt(NewItemID);
		Packet.WriteChar(NewSlot);
		if(NewItemID > 0)
			Packet.WriteChar(NewItemCount);

		// Send updates
		ServerNetwork->SendPacketToPeer(&Packet, TradePlayer->GetPeer());
	}
}

// Handle a player's inventory use request
void ServerStateClass::HandleInventoryUse(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	// Use an item
	Player->UseInventory(Packet->ReadChar());
}

// Handle a player's inventory split stack request
void ServerStateClass::HandleInventorySplit(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	int Slot = Packet->ReadChar();
	int Count = Packet->ReadChar();

	// Inventory only
	if(!PlayerClass::IsSlotInventory(Slot))
		return;

	Player->SplitStack(Slot, Count);
}

// Handles a player's event end message
void ServerStateClass::HandleEventEnd(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	Player->SetVendor(NULL);
	Player->SetState(PlayerClass::STATE_WALK);
}

// Handles a vendor exchange message
void ServerStateClass::HandleVendorExchange(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	// Get vendor
	const VendorStruct *Vendor = Player->GetVendor();
	if(!Vendor)
		return;

	// Get info
	bool Buy = Packet->ReadBit();
	int Amount = Packet->ReadChar();
	int Slot = Packet->ReadChar();
	if(Slot < 0)
		return;

	// Update player
	if(Buy) {
		if(Slot >= (int)Vendor->Items.size())
			return;

		// Get optional inventory slot
		int TargetSlot = Packet->ReadChar();

		// Get item info
		const ItemClass *Item = Vendor->Items[Slot];
		int Price = Item->GetPrice(Vendor, Amount, Buy);

		// Update player
		Player->UpdateGold(-Price);
		Player->AddItem(Item, Amount, TargetSlot);
		Player->CalculatePlayerStats();
	}
	else {
		if(Slot >= PlayerClass::INVENTORY_COUNT)
			return;

		// Get item info
		InventoryStruct *Item = Player->GetInventory(Slot);
		if(Item && Item->Item) {
			int Price = Item->Item->GetPrice(Vendor, Amount, Buy);
			Player->UpdateGold(Price);
			Player->UpdateInventory(Slot, -Amount);
		}
	}
}

// Handle a action bar change
void ServerStateClass::HandleActionBar(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	// Read skills
	for(int i = 0; i < Player->GetActionBarLength(); i++) {
		int Type = Packet->ReadChar();
		if(Type == -1)
			Player->SetActionBar(i, NULL);
		else {
			int ID = Packet->ReadChar();

			Player->SetActionBar(i, Stats.GetSkill(ID));
		}
	}

	Player->CalculatePlayerStats();
}

// Handles a player's request to not start a battle with other players
void ServerStateClass::HandlePlayerBusy(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	bool Value = Packet->ReadBit();
	Player->ToggleBusy(Value);

	//printf("HandlePlayerBusy: Value=%d\n", Value);
}

// Handles a player's request to attack another player
void ServerStateClass::HandleAttackPlayer(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player || !Player->CanAttackPlayer())
		return;

	MapClass *Map = Player->Map;
	if(!Map)
		return;
	
	// Check for a valid pvp tile
	if(Player->GetTile()->PVP) {

		// Reset timer
		Player->ResetAttackPlayerTime();

		// Get a list of players next to the player
		list<PlayerClass *> Players;
		Map->GetClosePlayers(Player, 1.5f * 1.5f, Players);

		// Find a suitable player to attack
		for(list<PlayerClass *>::Iterator Iterator = Players.begin(); Iterator != Players.end(); ++Iterator) {
			PlayerClass *VictimPlayer = *Iterator;
			if(VictimPlayer->GetState() != PlayerClass::STATE_BATTLE) {
				ServerBattleClass *Battle = Instances->CreateServerBattle();
				Battle->AddFighter(Player, 1);
				Battle->AddFighter(VictimPlayer, 0);
				Battle->StartBattle();
				break;
			}
		}
	}
}

// Handle a chat message
void ServerStateClass::HandleChatMessage(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	// Get map object
	MapClass *Map = Player->Map;
	if(!Map)
		return;

	// Get message
	char Message[256];
	strncpy(Message, Packet->ReadString(), NETWORKING_MESSAGESIZE);
	Message[NETWORKING_MESSAGESIZE] = 0;

	// Send message to other players
	PacketClass NewPacket(NetworkClass::CHAT_MESSAGE);
	NewPacket.WriteInt(Player->NetworkID);
	NewPacket.WriteString(Message);

	Map->SendPacketToPlayers(&NewPacket, Player);
}

// Handle a trade request
void ServerStateClass::HandleTradeRequest(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	// Get map object
	MapClass *Map = Player->Map;
	if(!Map)
		return;

	// Find the nearest player to trade with
	PlayerClass *TradePlayer = Map->GetClosestPlayer(Player, 2.0f * 2.0f, PlayerClass::STATE_WAITTRADE);
	if(TradePlayer == NULL) {
		
		// Set up trade post
		Player->SetState(PlayerClass::STATE_WAITTRADE);
		Player->SetTradeGold(0);
		Player->SetTradeAccepted(false);
		Player->SetTradePlayer(NULL);
	}
	else {

		// Set up trade screen for both players
		SendTradeInformation(Player, TradePlayer);
		SendTradeInformation(TradePlayer, Player);

		Player->SetTradePlayer(TradePlayer);
		Player->SetTradeAccepted(false);
		TradePlayer->SetTradePlayer(Player);
		TradePlayer->SetTradeAccepted(false);

		Player->SetState(PlayerClass::STATE_TRADE);
		TradePlayer->SetState(PlayerClass::STATE_TRADE);
	}
}

// Handles a trade cancel
void ServerStateClass::HandleTradeCancel(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	// Notify trading player
	PlayerClass *TradePlayer = Player->GetTradePlayer();
	if(TradePlayer) {
		TradePlayer->SetState(PlayerClass::STATE_WAITTRADE);
		TradePlayer->SetTradePlayer(NULL);
		TradePlayer->SetTradeAccepted(false);

		PacketClass Packet(NetworkClass::TRADE_CANCEL);
		ServerNetwork->SendPacketToPeer(&Packet, TradePlayer->GetPeer());
	}

	// Set state back to normal
	Player->SetState(PlayerClass::STATE_WALK);
}

// Handle a trade gold update
void ServerStateClass::HandleTradeGold(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	// Set gold amount
	int Gold = Packet->ReadInt();
	if(Gold < 0)
		Gold = 0;
	else if(Gold > Player->GetGold())
		Gold = Player->GetGold();
	Player->SetTradeGold(Gold);
	Player->SetTradeAccepted(false);

	// Notify player
	PlayerClass *TradePlayer = Player->GetTradePlayer();
	if(TradePlayer) {
		TradePlayer->SetTradeAccepted(false);

		PacketClass Packet(NetworkClass::TRADE_GOLD);
		Packet.WriteInt(Gold);
		ServerNetwork->SendPacketToPeer(&Packet, TradePlayer->GetPeer());
	}
}

// Handles a trade accept from a player
void ServerStateClass::HandleTradeAccept(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	// Get trading player
	PlayerClass *TradePlayer = Player->GetTradePlayer();
	if(TradePlayer) {

		// Set the player's state
		bool Accepted = Packet->ReadChar();
		Player->SetTradeAccepted(Accepted);

		// Check if both player's agree
		if(Accepted && TradePlayer->GetTradeAccepted()) {

			// Exchange items
			InventoryStruct TempItems[PLAYER_TRADEITEMS];
			for(int i = 0; i < PLAYER_TRADEITEMS; i++) {
				int InventorySlot = i + PlayerClass::INVENTORY_TRADE;
				TempItems[i] = *Player->GetInventory(InventorySlot);

				Player->SetInventory(InventorySlot, TradePlayer->GetInventory(InventorySlot));
				TradePlayer->SetInventory(InventorySlot, &TempItems[i]);
			}

			// Exchange gold
			Player->UpdateGold(TradePlayer->GetTradeGold() - Player->GetTradeGold());
			TradePlayer->UpdateGold(Player->GetTradeGold() - TradePlayer->GetTradeGold());

			// Send packet to players
			{
				PacketClass Packet(NetworkClass::TRADE_EXCHANGE);
				BuildTradeItemsPacket(Player, &Packet, Player->GetGold());
				ServerNetwork->SendPacketToPeer(&Packet, Player->GetPeer());
			}
			{
				PacketClass Packet(NetworkClass::TRADE_EXCHANGE);
				BuildTradeItemsPacket(TradePlayer, &Packet, TradePlayer->GetGold());
				ServerNetwork->SendPacketToPeer(&Packet, TradePlayer->GetPeer());
			}
			
			Player->SetState(PlayerClass::STATE_WALK);
			Player->SetTradePlayer(NULL);
			Player->SetTradeGold(0);
			Player->MoveTradeToInventory();
			TradePlayer->SetState(PlayerClass::STATE_WALK);
			TradePlayer->SetTradePlayer(NULL);
			TradePlayer->SetTradeGold(0);
			TradePlayer->MoveTradeToInventory();
		}
		else {

			// Notify trading player
			PacketClass Packet(NetworkClass::TRADE_ACCEPT);
			Packet.WriteChar(Accepted);
			ServerNetwork->SendPacketToPeer(&Packet, TradePlayer->GetPeer());
		}
	}
}

// Handles a town portal request
void ServerStateClass::HandleTownPortal(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	Player->StartTownPortal();
}

// Handles a trader accept
void ServerStateClass::HandleTraderAccept(PacketClass *Packet, ENetPeer *Peer) {
	PlayerClass *Player = static_cast<PlayerClass *>(Peer->data);
	if(!Player)
		return;

	const TraderStruct *Trader = Player->GetTrader();
	if(!Trader)
		return;

	// Get trader information
	int RequiredItemSlots[8];
	int RewardSlot = Player->GetRequiredItemSlots(Trader, RequiredItemSlots);
	if(RewardSlot == -1)
		return;

	// Exchange items
	Player->AcceptTrader(Trader, RequiredItemSlots, RewardSlot);
	Player->SetTrader(NULL);
	Player->SetState(PlayerClass::STATE_WALK);
}

// Spawns a player at a particular spawn point
void ServerStateClass::SpawnPlayer(PlayerClass *Player, int NewMapID, int EventType, int EventData) {

	// Get new map
	MapClass *NewMap = Instances->GetMap(NewMapID);

	// Remove old player if map has changed
	MapClass *OldMap = Player->Map;
	if(OldMap && NewMap != OldMap)
		OldMap->RemoveObject(Player);

	// Get spawn position
	IndexedEventStruct *SpawnEvent = NewMap->GetIndexedEvent(EventType, EventData);
	if(SpawnEvent) {
		Player->Position = SpawnEvent->Position;
		SendPlayerPosition(Player);
	}

	// Set state
	Player->SetState(PlayerClass::STATE_WALK);

	// Send new object list
	if(NewMap != OldMap) {

		// Update pointers
		Player->Map = NewMap;

		// Add player to map
		NewMap->AddObject(Player);

		// Build packet for player
		PacketClass Packet(NetworkClass::WORLD_CHANGEMAPS);

		// Send player info
		Packet.WriteInt(NewMapID);

		// Write object data
		list<ObjectClass *> Objects = NewMap->GetObjects();
		Packet.WriteInt(Objects.getSize());
		for(list<ObjectClass *>::Iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
			ObjectClass *Object = *Iterator;

			Packet.WriteInt(Object->NetworkID);
			Packet.WriteChar(Object->Position.X);
			Packet.WriteChar(Object->Position.Y);
			Packet.WriteChar(Object->Type);
			switch(Object->Type) {
				case ObjectClass::PLAYER: {
					PlayerClass *Player = static_cast<PlayerClass *>(Object);
					Packet.WriteString(Player->GetName().c_str());
					Packet.WriteChar(Player->GetPortraitID());
				}
				break;
			}
		}

		// Send object list to the player
		ServerNetwork->SendPacketToPeer(&Packet, Player->GetPeer());
	}

	//printf("SpawnPlayer: MapID=%d, NetworkID=%d\n", NewMapID, Player->NetworkID);
}

// Updates the player's HUD
void ServerStateClass::SendHUD(PlayerClass *Player) {

	PacketClass Packet(NetworkClass::WORLD_HUD);
	Packet.WriteInt(Player->GetExperience());
	Packet.WriteInt(Player->GetGold());
	Packet.WriteInt(Player->GetHealth());
	Packet.WriteInt(Player->GetMana());
	Packet.WriteFloat(Player->GetHealthAccumulator());
	Packet.WriteFloat(Player->GetManaAccumulator());

	ServerNetwork->SendPacketToPeer(&Packet, Player->GetPeer());
}

// Send player their position
void ServerStateClass::SendPlayerPosition(PlayerClass *Player) {

	PacketClass Packet(NetworkClass::WORLD_POSITION);
	Packet.WriteChar(Player->Position.X);
	Packet.WriteChar(Player->Position.Y);

	ServerNetwork->SendPacketToPeer(&Packet, Player->GetPeer());
}

// Sends the player a list of his/her characters
void ServerStateClass::SendCharacterList(PlayerClass *Player) {
	char QueryString[512];

	// Get a count of the account's characters
	sprintf(QueryString, "SELECT Count(ID) FROM Characters WHERE AccountsID = %d", Player->GetAccountID());
	int CharacterCount = Database->RunCountQuery(QueryString);

	// Create the packet
	PacketClass Packet(NetworkClass::CHARACTERS_LIST);
	Packet.WriteChar(CharacterCount);

	// Generate a list of characters
	sprintf(QueryString, "SELECT Name, PortraitID, Experience FROM Characters WHERE AccountsID = %d", Player->GetAccountID());
	Database->RunDataQuery(QueryString);
	while(Database->FetchRow()) {
		Packet.WriteString(Database->GetString(0));
		Packet.WriteInt(Database->GetInt(1));
		Packet.WriteInt(Database->GetInt(2));
	}
	Database->CloseQuery();

	// Send list
	ServerNetwork->SendPacketToPeer(&Packet, Player->GetPeer());
}

// Sends a player an event message
void ServerStateClass::SendEvent(PlayerClass *Player, int Type, int Data) {

	// Create packet
	PacketClass Packet(NetworkClass::EVENT_START);
	Packet.WriteChar(Type);
	Packet.WriteInt(Data);

	ServerNetwork->SendPacketToPeer(&Packet, Player->GetPeer());
}

// Sends information to another player about items they're trading
void ServerStateClass::SendTradeInformation(PlayerClass *Sender, PlayerClass *Receiver) {

	// Send items to trader player
	PacketClass Packet(NetworkClass::TRADE_REQUEST);
	Packet.WriteInt(Sender->NetworkID);
	BuildTradeItemsPacket(Sender, &Packet, Sender->GetTradeGold());
	ServerNetwork->SendPacketToPeer(&Packet, Receiver->GetPeer());
}

// Adds trade item information to a packet
void ServerStateClass::BuildTradeItemsPacket(PlayerClass *Player, PacketClass *Packet, int Gold) {
	Packet->WriteInt(Gold);
	for(int i = PlayerClass::INVENTORY_TRADE; i < PlayerClass::INVENTORY_COUNT; i++) {
		if(Player->GetInventory(i)->Item) {
			Packet->WriteInt(Player->GetInventory(i)->Item->GetID());
			Packet->WriteChar(Player->GetInventory(i)->Count);
		}
		else
			Packet->WriteInt(0);
	}
}

// Removes a player from a battle and deletes the battle if necessary
void ServerStateClass::RemovePlayerFromBattle(PlayerClass *Player) {
	ServerBattleClass *Battle = static_cast<ServerBattleClass *>(Player->GetBattle());
	if(!Battle)
		return;

	// Delete instance
	if(Battle->RemovePlayer(Player) == 0) {
		Instances->DeleteBattle(Battle);
	}
}

// Deletes an object on the server and broadcasts it to the clients
void ServerStateClass::DeleteObject(ObjectClass *Object) {

	// Remove the object from their current map
	if(Object->Map) {
		Object->Map->RemoveObject(Object);
	}
}

// Called when object gets deleted
void ObjectDeleted(ObjectClass *Object) {

	ServerState.DeleteObject(Object);
}

// Teleports a player back to town
void ServerStateClass::PlayerTownPortal(PlayerClass *Player) {

	Player->RestoreHealthMana();
	SpawnPlayer(Player, Player->GetSpawnMapID(), MapClass::EVENT_SPAWN, Player->GetSpawnPoint());
	SendHUD(Player);
	Player->Save();
}
