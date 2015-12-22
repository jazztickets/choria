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
#include <save.h>
#include <objects/object.h>
#include <objects/item.h>
#include <objects/inventory.h>
#include <database.h>
#include <config.h>
#include <stats.h>
#include <constants.h>
#include <utils.h>
#include <stdexcept>

// Constructor
_Save::_Save() {
	std::string SavePath = Config.ConfigPath + "save.db";

	// Open file
	Database = new _Database(SavePath);

	// Get save version
	int SaveVersion = 0;
	try {
		SaveVersion = GetSaveVersion();
	}
	catch(std::exception &Error) {
	}

	// Check version
	if(SaveVersion != SAVE_VERSION) {

		// Rename old file
		if(SaveVersion > 0) {
			delete Database;
			std::rename(SavePath.c_str(), (SavePath + "." + std::to_string(SaveVersion)).c_str());
			Database = new _Database(SavePath);
		}

		// Load defaults
		CreateDefaultDatabase();
	}

	Database->RunQuery("PRAGMA foreign_keys = ON");
}

// Destructor
_Save::~_Save() {

	delete Database;
}

// Get clock
double _Save::GetClock() {
	double Time = 0.0;

	Database->PrepareQuery("SELECT time FROM clock");
	if(Database->FetchRow())
		Time = Database->GetReal(0);
	Database->CloseQuery();

	return Time;
}

// Save clock
void _Save::SaveClock(double Time) {
	Database->PrepareQuery("UPDATE clock SET time = @time");
	Database->BindReal(1, Time);
	Database->FetchRow();
	Database->CloseQuery();
}

// Check for a username
bool _Save::CheckUsername(const std::string &Username) {
	std::string TrimmedUsername = TrimString(Username);

	Database->PrepareQuery("SELECT id FROM account WHERE username = @username");
	Database->BindString(1, TrimmedUsername);
	int Result = Database->FetchRow();
	Database->CloseQuery();

	return Result;
}

// Create account
void _Save::CreateAccount(const std::string &Username, const std::string &Password) {
	std::string TrimmedUsername = TrimString(Username);

	Database->PrepareQuery("INSERT INTO account(username, password) VALUES(@username, @password)");
	Database->BindString(1, TrimmedUsername);
	Database->BindString(2, Password);
	Database->FetchRow();
	Database->CloseQuery();
}

// Get account id from login credentials
uint32_t _Save::GetAccountID(const std::string &Username, const std::string &Password) {
	uint32_t AccountID = 0;

	std::string TrimmedUsername = TrimString(Username);

	// Get account information
	Database->PrepareQuery("SELECT id FROM account WHERE username = @username AND password = @password");
	Database->BindString(1, TrimmedUsername);
	Database->BindString(2, Password);
	if(Database->FetchRow())
		AccountID = Database->GetInt<uint32_t>(0);
	Database->CloseQuery();

	return AccountID;
}

// Get character count for an account
uint32_t _Save::GetCharacterCount(uint32_t AccountID) {
	Database->PrepareQuery("SELECT count(id) FROM character WHERE account_id = @account_id");
	Database->BindInt(1, AccountID);
	Database->FetchRow();
	uint32_t Count = Database->GetInt<uint32_t>(0);
	Database->CloseQuery();

	return Count;
}

// Find character id by character name
uint32_t _Save::GetCharacterIDByName(const std::string &Name) {
	std::string TrimmedName = TrimString(Name);

	Database->PrepareQuery("SELECT id FROM character WHERE name = @name");
	Database->BindString(1, TrimmedName);
	Database->FetchRow();
	uint32_t CharacterID = Database->GetInt<uint32_t>(0);
	Database->CloseQuery();

	return CharacterID;
}

// Find character id by slot number
uint32_t _Save::GetCharacterIDBySlot(uint32_t AccountID, uint32_t Slot) {
	Database->PrepareQuery("SELECT id FROM character WHERE account_id = @account_id AND slot = @slot");
	Database->BindInt(1, AccountID);
	Database->BindInt(2, Slot);
	Database->FetchRow();
	uint32_t CharacterID = Database->GetInt<uint32_t>(0);
	Database->CloseQuery();

	return CharacterID;
}

// Delete character by id
void _Save::DeleteCharacter(uint32_t CharacterID) {
	Database->PrepareQuery("DELETE FROM character WHERE id = @character_id");
	Database->BindInt(1, CharacterID);
	Database->FetchRow();
	Database->CloseQuery();
}

// Create character
void _Save::CreateCharacter(_Stats *Stats, uint32_t AccountID, uint32_t Slot, const std::string &Name, uint32_t PortraitID) {
	Database->RunQuery("BEGIN TRANSACTION");

	std::string TrimmedName = TrimString(Name);

	Database->PrepareQuery("INSERT INTO character(account_id, slot, name, portrait_id, actionbar_size) VALUES(@account_id, @slot, @name, @portrait_id, @actionbar_size)");
	Database->BindInt(1, AccountID);
	Database->BindInt(2, Slot);
	Database->BindString(3, TrimmedName);
	Database->BindInt(4, PortraitID);
	Database->BindInt(5, ACTIONBAR_STARTING_SIZE);
	Database->FetchRow();
	Database->CloseQuery();

	uint32_t CharacterID = (uint32_t)Database->GetLastInsertID();
	uint32_t ItemIDs[4];
	ItemIDs[0] = Stats->GetItemIDByName("Small Knife");
	ItemIDs[1] = Stats->GetItemIDByName("Dirty Shirt");
	ItemIDs[2] = Stats->GetItemIDByName("Small Health Potion");
	ItemIDs[3] = Stats->GetItemIDByName("Attack");
	Database->PrepareQuery("INSERT INTO inventory "
						   "VALUES(@character_id, @hand_slot, @item1_id, 1) "
						   ",(@character_id, @body_slot, @item2_id, 1) "
						   ",(@character_id, @bag_slot, @item3_id, 3) ");
	Database->BindInt(1, CharacterID);
	Database->BindInt(2, (int)InventoryType::HAND1);
	Database->BindInt(3, ItemIDs[0]);
	Database->BindInt(4, (int)InventoryType::BODY);
	Database->BindInt(5, ItemIDs[1]);
	Database->BindInt(6, (int)InventoryType::BAG);
	Database->BindInt(7, ItemIDs[2]);
	Database->FetchRow();
	Database->CloseQuery();

	Database->PrepareQuery("INSERT INTO skill VALUES(@character_id, 0, @item_id, 1)");
	Database->BindInt(1, CharacterID);
	Database->BindInt(2, ItemIDs[3]);
	Database->FetchRow();
	Database->CloseQuery();

	Database->PrepareQuery("INSERT INTO actionbar VALUES(@character_id, 0, @skill_id), (@character_id, 1, @item_id)");
	Database->BindInt(1, CharacterID);
	Database->BindInt(2, ItemIDs[3]);
	Database->BindInt(3, ItemIDs[2]);
	Database->FetchRow();
	Database->CloseQuery();

	Database->RunQuery("END TRANSACTION");
}

// Load player from database
void _Save::LoadPlayer(_Object *Player) {

	// Get character info
	Database->PrepareQuery("SELECT * FROM character WHERE id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	if(Database->FetchRow()) {
		Player->SpawnMapID = (NetworkIDType)Database->GetInt<uint32_t>("map_id");
		Player->SpawnPoint = Database->GetInt<uint32_t>("spawnpoint");
		Player->Name = Database->GetString("name");
		Player->PortraitID = Database->GetInt<uint32_t>("portrait_id");
		Player->Experience = Database->GetInt<int>("experience");
		Player->Gold = Database->GetInt<int>("gold");
		Player->ActionBar.resize(Database->GetInt<uint32_t>("actionbar_size"));
		Player->PlayTime = Database->GetInt<int>("playtime");
		Player->Deaths = Database->GetInt<int>("deaths");
		Player->MonsterKills = Database->GetInt<int>("monsterkills");
		Player->PlayerKills = Database->GetInt<int>("playerkills");
		Player->Bounty = Database->GetInt<int>("bounty");
	}
	Database->CloseQuery();

	// Set inventory
	Database->PrepareQuery("SELECT slot, item_id, count FROM inventory WHERE character_id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	while(Database->FetchRow()) {
		size_t Slot = Database->GetInt<uint32_t>(0);
		_InventorySlot InventorySlot;
		InventorySlot.Item = Player->Stats->Items[Database->GetInt<uint32_t>(1)];
		InventorySlot.Count = Database->GetInt<int>(2);
		Player->Inventory->Slots[Slot] = InventorySlot;
	}
	Database->CloseQuery();

	Player->Skills.clear();

	// Set skills
	Database->PrepareQuery("SELECT item_id, level FROM skill WHERE character_id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	while(Database->FetchRow()) {
		uint32_t ItemID = Database->GetInt<uint32_t>(0);
		int SkillLevel = Database->GetInt<int>(1);
		Player->Skills[ItemID] = SkillLevel;
	}
	Database->CloseQuery();

	// Set actionbar
	Database->PrepareQuery("SELECT slot, item_id FROM actionbar WHERE character_id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	while(Database->FetchRow()) {
		uint32_t Slot = Database->GetInt<uint32_t>("slot");
		uint32_t ItemID = Database->GetInt<uint32_t>("item_id");
		Player->ActionBar[Slot].Item = Player->Stats->Items[ItemID];
	}
	Database->CloseQuery();
}

// Saves the player
void _Save::SavePlayer(const _Object *Player) {
	if(Player->CharacterID == 0)
		return;

	Database->RunQuery("BEGIN TRANSACTION");

	// Save character stats
	Database->PrepareQuery(
		"UPDATE character SET"
		" map_id = @map_id,"
		" spawnpoint = @spawnpoint,"
		" experience = @experience,"
		" gold = @gold,"
		" playtime = @playtime,"
		" deaths = @deaths,"
		" monsterkills = @monsterkills,"
		" playerkills = @playerkills,"
		" bounty = @bounty"
		" WHERE id = @character_id"
	);
	Database->BindInt(1, Player->SpawnMapID);
	Database->BindInt(2, Player->SpawnPoint);
	Database->BindInt(3, Player->Experience);
	Database->BindInt(4, Player->Gold);
	Database->BindInt(5, Player->PlayTime);
	Database->BindInt(6, Player->Deaths);
	Database->BindInt(7, Player->MonsterKills);
	Database->BindInt(8, Player->PlayerKills);
	Database->BindInt(9, Player->Bounty);
	Database->BindInt(10, Player->CharacterID);
	Database->FetchRow();
	Database->CloseQuery();

	// Save items
	Database->PrepareQuery("DELETE FROM inventory WHERE character_id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	Database->FetchRow();
	Database->CloseQuery();

	const _InventorySlot *InventorySlot;
	for(size_t i = 0; i < InventoryType::COUNT; i++) {
		InventorySlot = &Player->Inventory->Slots[i];
		if(InventorySlot->Item) {
			Database->PrepareQuery("INSERT INTO inventory VALUES(@character_id, @slot, @item_id, @count)");
			Database->BindInt(1, Player->CharacterID);
			Database->BindInt(2, (uint32_t)i);
			Database->BindInt(3, InventorySlot->Item->ID);
			Database->BindInt(4, (uint32_t)InventorySlot->Count);
			Database->FetchRow();
			Database->CloseQuery();
		}
	}

	// Save skills
	Database->PrepareQuery("DELETE FROM skill WHERE character_id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	Database->FetchRow();
	Database->CloseQuery();

	for(auto &Skill : Player->Skills) {
		if(Skill.second > 0) {
			Database->PrepareQuery("INSERT INTO skill VALUES(@character_id, 0, @item_id, @level)");
			Database->BindInt(1, Player->CharacterID);
			Database->BindInt(2, Skill.first);
			Database->BindInt(3, (uint32_t)Skill.second);
			Database->FetchRow();
			Database->CloseQuery();
		}
	}

	// Save actionbar
	Database->PrepareQuery("DELETE FROM actionbar WHERE character_id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	Database->FetchRow();
	Database->CloseQuery();

	for(size_t i = 0; i < Player->ActionBar.size(); i++) {
		if(Player->ActionBar[i].IsSet()) {
			uint32_t ItemID = 0;
			if(Player->ActionBar[i].Item)
				ItemID = Player->ActionBar[i].Item->ID;

			Database->PrepareQuery("INSERT INTO actionbar VALUES(@character_id, @slot, @item_id)");
			Database->BindInt(1, Player->CharacterID);
			Database->BindInt(2, (uint32_t)i);
			Database->BindInt(3, ItemID);
			Database->FetchRow();
			Database->CloseQuery();
		}
	}

	Database->RunQuery("END TRANSACTION");
}

// Get save version from database
int _Save::GetSaveVersion() {
	Database->PrepareQuery("SELECT version FROM settings");
	Database->FetchRow();
	int Version = Database->GetInt<int>("version");
	Database->CloseQuery();

	return Version;
}

// Create default save database
void _Save::CreateDefaultDatabase() {

	Database->RunQuery(
				"BEGIN TRANSACTION"
	);

	// Settings
	Database->RunQuery(
				"CREATE TABLE settings(version INTEGER)"
	);

	Database->RunQuery(
				"INSERT INTO settings(version) VALUES(" + std::to_string(SAVE_VERSION) + ")"
	);

	// Clock
	Database->RunQuery(
				"CREATE TABLE clock(time REAL DEFAULT(0))"
	);

	Database->RunQuery(
				"INSERT INTO clock(time) VALUES(" + std::to_string(MAP_CLOCK_START) + ")"
	);

	// Accounts
	Database->RunQuery(
				"CREATE TABLE account(\n"
				"	id INTEGER PRIMARY KEY,\n"
				"	username TEXT,\n"
				"	password TEXT\n"
				")"
	);

	Database->RunQuery(
				"INSERT INTO account(username, password) VALUES('choria_singleplayer', 'choria_singleplayer')"
	);

	// Characters
	Database->RunQuery(
				"CREATE TABLE character(\n"
				"	id INTEGER PRIMARY KEY,\n"
				"	account_id INTEGER REFERENCES account(id) ON DELETE CASCADE,\n"
				"	slot INTEGER DEFAULT(0),\n"
				"	map_id INTEGER DEFAULT(1),\n"
				"	spawnpoint INTEGER DEFAULT(0),\n"
				"	name TEXT,\n"
				"	portrait_id INTEGER DEFAULT(1),\n"
				"	actionbar_size INTEGER DEFAULT(0),\n"
				"	experience INTEGER DEFAULT(0),\n"
				"	gold INTEGER DEFAULT(0),\n"
				"	battletime INTEGER DEFAULT(0),\n"
				"	playtime INTEGER DEFAULT(0),\n"
				"	deaths INTEGER DEFAULT(0),\n"
				"	monsterkills INTEGER DEFAULT(0),\n"
				"	playerkills INTEGER DEFAULT(0),\n"
				"	bounty INTEGER DEFAULT(0)\n"
				")"
	);

	// Actionbar
	Database->RunQuery(
				"CREATE TABLE actionbar(\n"
				"	character_id INTEGER REFERENCES character(id) ON DELETE CASCADE,\n"
				"	slot INTEGER DEFAULT(0),\n"
				"	item_id INTEGER DEFAULT(0)\n"
				")"
	);

	// Inventory
	Database->RunQuery(
				"CREATE TABLE inventory(\n"
				"	character_id INTEGER REFERENCES character(id) ON DELETE CASCADE,\n"
				"	slot INTEGER,\n"
				"	item_id INTEGER,\n"
				"	count INTEGER\n"
				")"
	);

	// Skills
	Database->RunQuery(
				"CREATE TABLE skill(\n"
				"	character_id INTEGER REFERENCES character(id) ON DELETE CASCADE,\n"
				"	rank INTEGER,\n"
				"	item_id INTEGER,\n"
				"	level INTEGER\n"
				")"
	);

	// Unlocks
	Database->RunQuery(
				"CREATE TABLE unlock(\n"
				"	character_id INTEGER REFERENCES character(id) ON DELETE CASCADE,\n"
				"	quest_id INTEGER,\n"
				"	level INTEGER DEFAULT(0)\n"
				")"
	);

	Database->RunQuery(
				"END TRANSACTION"
	);
}
