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
#include <objects/skill.h>
#include <objects/inventory.h>
#include <database.h>
#include <config.h>
#include <stats.h>
#include <constants.h>
#include <utils.h>
#include <stdexcept>
#include <cstdio>

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
void _Save::CreateCharacter(uint32_t AccountID, uint32_t Slot, const std::string &Name, uint32_t PortraitID) {
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
	Database->PrepareQuery("INSERT INTO inventory VALUES(@character_id, 1, 2, 1), (@character_id, 3, 1, 1), (@character_id, 7, 6, 3)");
	Database->BindInt(1, CharacterID);
	Database->FetchRow();
	Database->CloseQuery();

	Database->PrepareQuery("INSERT INTO skilllevel VALUES(@character_id, 1, 1)");
	Database->BindInt(1, CharacterID);
	Database->FetchRow();
	Database->CloseQuery();

	Database->PrepareQuery("INSERT INTO actionbar VALUES(@character_id, 0, 1, 0), (@character_id, 1, 0, 6)");
	Database->BindInt(1, CharacterID);
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
		Player->SpawnMapID = Database->GetInt<uint32_t>("map_id");
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

	// Set skills
	Database->PrepareQuery("SELECT skill_id, level FROM skilllevel WHERE character_id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	while(Database->FetchRow()) {
		int SkillLevel = Database->GetInt<int>(1);
		Player->SetSkillLevel(Database->GetInt<uint32_t>(0), SkillLevel);
	}
	Database->CloseQuery();

	// Set actionbar
	Database->PrepareQuery("SELECT slot, skill_id, item_id FROM actionbar WHERE character_id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	while(Database->FetchRow()) {
		uint32_t Slot = Database->GetInt<uint32_t>("slot");
		uint32_t SkillID = Database->GetInt<uint32_t>("skill_id");
		uint32_t ItemID = Database->GetInt<uint32_t>("item_id");
		Player->ActionBar[Slot].Skill = Player->Stats->Skills[SkillID];
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
	std::stringstream Query;
	Query
		<< "UPDATE character SET "
		<< "  map_id = " << Player->SpawnMapID
		<< ", spawnpoint = " << Player->SpawnPoint
		<< ", experience = " <<Player->Experience
		<< ", gold = " << Player->Gold
		<< ", playtime = " << Player->PlayTime
		<< ", deaths = " << Player->Deaths
		<< ", monsterkills = " << Player->MonsterKills
		<< ", playerkills = " << Player->PlayerKills
		<< ", bounty = " << Player->Bounty
		<< " WHERE id = " << Player->CharacterID;
	Database->RunQuery(Query.str());
	Query.str("");

	// Save items
	Query << "DELETE FROM inventory WHERE character_id = " << Player->CharacterID;
	Database->RunQuery(Query.str());
	Query.str("");

	const _InventorySlot *Item;
	for(size_t i = 0; i < InventoryType::COUNT; i++) {
		Item = &Player->Inventory->Slots[i];
		if(Item->Item) {
			Query << "INSERT INTO inventory VALUES(" << Player->CharacterID << ", " << i << ", " << Item->Item->ID << ", " << Item->Count << ")";;
			Database->RunQuery(Query.str());
			Query.str("");
		}
	}

	// Save skill points
	Query << "DELETE FROM skilllevel WHERE character_id = " << Player->CharacterID;
	Database->RunQuery(Query.str());
	Query.str("");

	for(auto &SkillLevel : Player->SkillLevels) {
		if(SkillLevel.second > 0) {
			Query << "INSERT INTO skilllevel VALUES(" << Player->CharacterID << ", " << SkillLevel.first << ", " << SkillLevel.second << ")";
			Database->RunQuery(Query.str());
			Query.str("");
		}
	}

	// Save actionbar
	Query << "DELETE FROM actionbar WHERE character_id = " << Player->CharacterID;
	Database->RunQuery(Query.str());
	Query.str("");

	for(size_t i = 0; i < Player->ActionBar.size(); i++) {
		if(Player->ActionBar[i].IsSet()) {
			uint32_t SkillID = 0;
			uint32_t ItemID = 0;
			if(Player->ActionBar[i].Skill)
				SkillID = Player->ActionBar[i].Skill->ID;
			if(Player->ActionBar[i].Item)
				ItemID = Player->ActionBar[i].Item->ID;

			Query << "INSERT INTO actionbar VALUES(" << Player->CharacterID << ", " << i << ", " << SkillID << ", " << ItemID << ")";
			Database->RunQuery(Query.str());
			Query.str("");
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
				"	skill_id INTEGER DEFAULT(0),\n"
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

	// Skill levels
	Database->RunQuery(
				"CREATE TABLE skilllevel(\n"
				"	character_id INTEGER REFERENCES character(id) ON DELETE CASCADE,\n"
				"	skill_id INTEGER,\n"
				"	level INTEGER\n"
				")"
	);

	Database->RunQuery(
				"END TRANSACTION"
	);
}
