/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
#include <objects/buff.h>
#include <objects/inventory.h>
#include <database.h>
#include <random.h>
#include <config.h>
#include <stats.h>
#include <constants.h>
#include <utils.h>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <json/writer.h>
#include <json/reader.h>

// Constructor
_Save::_Save() :
	Secret(0),
	Clock(0) {

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
	if(SaveVersion != DEFAULT_SAVE_VERSION) {

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

	// Load settings
	GetSettings();
}

// Destructor
_Save::~_Save() {

	delete Database;
}

// Start database transaction
void _Save::StartTransaction() {
	Database->RunQuery("BEGIN TRANSACTION");
}

// End database transaction
void _Save::EndTransaction() {
	Database->RunQuery("END TRANSACTION");
}

// Save settings
void _Save::SaveSettings() {

	// Write settings
	Json::Value SettingsNode;
	SettingsNode["secret"] = (Json::Value::UInt64)Secret;
	SettingsNode["clock"] = Clock;

	// Convert to JSON string
	Json::FastWriter Writer;
	std::string JsonString = Writer.write(SettingsNode);
	if(JsonString.back() == '\n')
		JsonString.pop_back();

	// Update database
	Database->PrepareQuery("UPDATE settings SET data = @data");
	Database->BindString(1, JsonString);
	Database->FetchRow();
	Database->CloseQuery();
}

// Load settings from database
void _Save::GetSettings() {
	Database->PrepareQuery("SELECT data FROM settings");
	if(Database->FetchRow()) {

		// Parse JSON
		Json::Value Data;
		Json::Reader Reader;
		if(!Reader.parse(Database->GetString(0), Data))
			throw std::runtime_error("_Save::GetSettings: Error parsing JSON string!");

		// Get settings
		Secret = Data["secret"].asUInt64();
		Clock = Data["clock"].asDouble();
	}

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

	Database->PrepareQuery("INSERT INTO account(username, password, data) VALUES(@username, @password, '')");
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

// Get character id from account_id and slot
uint32_t _Save::GetCharacterID(uint32_t AccountID, uint32_t Slot) {
	uint32_t CharacterID = 0;

	// Run query
	Database->PrepareQuery("SELECT id FROM character WHERE account_id = @account_id and slot = @slot");
	Database->BindInt(1, AccountID);
	Database->BindInt(2, Slot);
	if(Database->FetchRow())
		CharacterID = Database->GetInt<uint32_t>("id");

	Database->CloseQuery();

	return CharacterID;
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
void _Save::CreateCharacter(_Stats *Stats, _Scripting *Scripting, uint32_t AccountID, uint32_t Slot, bool Hardcore, const std::string &Name, uint32_t PortraitID, uint32_t BuildID) {
	if(!BuildID)
		BuildID = 1;

	// Get build
	const _Object *Build = Stats->Builds[BuildID];
	if(!Build)
		Build = Stats->Builds[1];

	// Create new database row
	int Index = 1;
	Database->PrepareQuery("INSERT INTO character(account_id, slot, name) VALUES(@account_id, @slot, @name)");
	Database->BindInt(Index++, AccountID);
	Database->BindInt(Index++, Slot);
	Database->BindString(Index++, TrimString(Name));
	Database->FetchRow();
	Database->CloseQuery();

	// Copy object stats from build
	_Object Object;
	Object.Stats = Stats;
	Object.Scripting = Scripting;
	Object.Hardcore = Hardcore;
	Object.PortraitID = PortraitID;
	Object.ModelID = Build->ModelID;
	Object.CharacterID = (uint32_t)Database->GetLastInsertID();
	Object.ActionBar = Build->ActionBar;
	Object.Inventory->Bags = Build->Inventory->Bags;
	Object.Skills = Build->Skills;
	Object.CalculateStats();

	// Set health/mana
	Object.Health = Object.MaxHealth;
	Object.Mana = Object.MaxMana;
	Object.GenerateNextBattle();

	// Save new character
	StartTransaction();
	SavePlayer(&Object, 0);
	EndTransaction();
}

// Saves the player
void _Save::SavePlayer(const _Object *Player, NetworkIDType MapID) {
	if(Player->CharacterID == 0)
		return;

	// Reset spawn point if player is dead
	if(!Player->IsAlive())
		MapID = 0;

	// Get player stats
	Json::Value Data;
	Player->SerializeSaveData(Data);
	Data["stats"]["map_id"] = MapID;

	// Get JSON string
	Json::FastWriter Writer;
	std::string JsonString = Writer.write(Data);
	if(JsonString.back() == '\n')
		JsonString.pop_back();

	// Save character stats
	Database->PrepareQuery("UPDATE character SET data = @data WHERE id = @character_id");
	Database->BindString(1, JsonString);
	Database->BindInt(2, Player->CharacterID);
	Database->FetchRow();
	Database->CloseQuery();
}

// Load player from database
void _Save::LoadPlayer(_Stats *Stats, _Object *Player) {

	// Get character info
	Database->PrepareQuery("SELECT * FROM character WHERE id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	if(Database->FetchRow()) {
		Player->Name = Database->GetString("name");
		Player->UnserializeSaveData(Database->GetString("data"));
	}
	Database->CloseQuery();

	// Get stats
	Player->CalculateStats();

	// Max sure player has health
	if(!Player->IsAlive() && !Player->Hardcore)
		Player->Health = Player->MaxHealth / 2;
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
	StartTransaction();

	// Settings
	Database->RunQuery(
		"CREATE TABLE settings(\n"
		"	version INTEGER,\n"
		"	data TEXT\n"
		")"
	);

	// Create settings row
	Database->PrepareQuery("INSERT INTO settings(version) VALUES (@version)");
	Database->BindInt(1, DEFAULT_SAVE_VERSION);
	Database->FetchRow();
	Database->CloseQuery();

	// Write settings
	Secret = GetRandomInt((uint64_t)1, std::numeric_limits<uint64_t>::max());
	Clock = MAP_CLOCK_START;
	SaveSettings();

	// Accounts
	Database->RunQuery(
		"CREATE TABLE account(\n"
		"	id INTEGER PRIMARY KEY,\n"
		"	username TEXT,\n"
		"	password TEXT,\n"
		"	data TEXT\n"
		")"
	);

	Database->RunQuery("INSERT INTO account(id, username, password, data) VALUES(1, '', '', '')");

	// Characters
	Database->RunQuery(
		"CREATE TABLE character(\n"
		"	id INTEGER PRIMARY KEY,\n"
		"	account_id INTEGER REFERENCES account(id) ON DELETE CASCADE,\n"
		"	slot INTEGER DEFAULT(0),\n"
		"	name TEXT,\n"
		"	data TEXT\n"
		")"
	);

	// Indexes
	Database->RunQuery("CREATE INDEX character_account_id ON character(account_id)");
	Database->RunQuery("CREATE INDEX character_name ON character(name)");

	EndTransaction();
}
