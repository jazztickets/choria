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
#include <database.h>
#include <config.h>
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
		AccountID = Database->GetInt(0);
	Database->CloseQuery();

	return AccountID;
}

// Get character count for an account
uint32_t _Save::GetCharacterCount(uint32_t AccountID) {
	Database->PrepareQuery("SELECT count(id) FROM character WHERE account_id = @account_id");
	Database->BindInt(1, AccountID);
	Database->FetchRow();
	int Count = Database->GetInt(0);
	Database->CloseQuery();

	return Count;
}

// Find character id by character name
uint32_t _Save::GetCharacterIDByName(const std::string &Name) {
	std::string TrimmedName = TrimString(Name);

	Database->PrepareQuery("SELECT id FROM character WHERE name = @name");
	Database->BindString(1, TrimmedName);
	Database->FetchRow();
	uint32_t CharacterID = Database->GetInt(0);
	Database->CloseQuery();

	return CharacterID;
}

// Find character id by slot number
uint32_t _Save::GetCharacterIDBySlot(uint32_t AccountID, int Slot) {
	Database->PrepareQuery("SELECT id FROM character WHERE account_id = @account_id AND slot = @slot");
	Database->BindInt(1, AccountID);
	Database->BindInt(2, Slot);
	Database->FetchRow();
	uint32_t CharacterID = Database->GetInt(0);
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
void _Save::CreateCharacter(uint32_t AccountID, int Slot, const std::string &Name, uint32_t PortraitID) {
	Database->RunQuery("BEGIN TRANSACTION");

	std::string TrimmedName = TrimString(Name);

	Database->PrepareQuery("INSERT INTO character(account_id, slot, name, portrait_id, actionbar0) VALUES(@account_id, @slot, @name, @portrait_id, 1)");
	Database->BindInt(1, AccountID);
	Database->BindInt(2, Slot);
	Database->BindString(3, TrimmedName);
	Database->BindInt(4, PortraitID);
	Database->FetchRow();
	Database->CloseQuery();

	int64_t CharacterID = Database->GetLastInsertID();
	Database->PrepareQuery("INSERT INTO inventory VALUES(@character_id, 1, 2, 1), (@character_id, 3, 1, 1)");
	Database->BindInt(1, CharacterID);
	Database->FetchRow();
	Database->CloseQuery();

	Database->PrepareQuery("INSERT INTO skilllevel VALUES(@character_id, 1, 1)");
	Database->BindInt(1, CharacterID);
	Database->FetchRow();
	Database->CloseQuery();

	Database->RunQuery("END TRANSACTION");
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
		<< ", actionbar0 = " << Player->GetActionBarID(0)
		<< ", actionbar1 = " << Player->GetActionBarID(1)
		<< ", actionbar2 = " << Player->GetActionBarID(2)
		<< ", actionbar3 = " << Player->GetActionBarID(3)
		<< ", actionbar4 = " << Player->GetActionBarID(4)
		<< ", actionbar5 = " << Player->GetActionBarID(5)
		<< ", actionbar6 = " << Player->GetActionBarID(6)
		<< ", actionbar7 = " << Player->GetActionBarID(7)
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
	for(int i = 0; i < _Object::INVENTORY_COUNT; i++) {
		Item = &Player->Inventory[i];
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

	Database->RunQuery("END TRANSACTION");
}

// Get save version from database
int _Save::GetSaveVersion() {
	Database->PrepareQuery("SELECT version FROM settings");
	Database->FetchRow();
	int Version = Database->GetInt("version");
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
				"	experience INTEGER DEFAULT(0),\n"
				"	gold INTEGER DEFAULT(0),\n"
				"	actionbar0 INTEGER DEFAULT(0),\n"
				"	actionbar1 INTEGER DEFAULT(0),\n"
				"	actionbar2 INTEGER DEFAULT(0),\n"
				"	actionbar3 INTEGER DEFAULT(0),\n"
				"	actionbar4 INTEGER DEFAULT(0),\n"
				"	actionbar5 INTEGER DEFAULT(0),\n"
				"	actionbar6 INTEGER DEFAULT(0),\n"
				"	actionbar7 INTEGER DEFAULT(0),\n"
				"	playtime INTEGER DEFAULT(0),\n"
				"	deaths INTEGER DEFAULT(0),\n"
				"	monsterkills INTEGER DEFAULT(0),\n"
				"	playerkills INTEGER DEFAULT(0),\n"
				"	bounty INTEGER DEFAULT(0)\n"
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
