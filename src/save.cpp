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

// Get secret number
uint64_t _Save::GetSecret() {
	uint64_t Secret = 0;

	Database->PrepareQuery("SELECT secret FROM settings");
	if(Database->FetchRow())
		Secret = Database->GetInt<uint64_t>(0);
	Database->CloseQuery();

	return Secret;
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
	Object.Inventory->Slots = Build->Inventory->Slots;
	Object.Skills = Build->Skills;
	Object.CalculateStats();

	// Set health/mana
	Object.Health = Object.MaxHealth;
	Object.Mana = Object.MaxMana;

	// Save new character
	StartTransaction();
	SavePlayer(&Object, 0);
	EndTransaction();
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

	// Set inventory
	Database->PrepareQuery("SELECT slot, item_id, upgrades, count FROM inventory WHERE character_id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	while(Database->FetchRow()) {
		size_t Slot = Database->GetInt<uint32_t>(0);
		_InventorySlot InventorySlot;
		InventorySlot.Item = Player->Stats->Items[Database->GetInt<uint32_t>(1)];
		InventorySlot.Upgrades = Database->GetInt<int>(2);
		InventorySlot.Count = Database->GetInt<int>(3);
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
		Player->Skills[ItemID] = std::min(SkillLevel, Stats->Items[ItemID]->MaxLevel);
	}
	Database->CloseQuery();

	// Set actionbar
	Database->PrepareQuery("SELECT slot, item_id FROM actionbar WHERE character_id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	while(Database->FetchRow()) {
		uint32_t Slot = Database->GetInt<uint32_t>("slot");
		uint32_t ItemID = Database->GetInt<uint32_t>("item_id");
		const _Item *Item = Player->Stats->Items[ItemID];
		if(Item->IsSkill() && !Player->HasLearned(Item))
			Item = nullptr;
		if(Slot < Player->ActionBar.size())
			Player->ActionBar[Slot].Item = Item;
	}
	Database->CloseQuery();

	// Set unlocks
	Database->PrepareQuery("SELECT unlock_id, level FROM unlock WHERE character_id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	while(Database->FetchRow()) {
		uint32_t UnlockID = Database->GetInt<uint32_t>(0);
		int Level = Database->GetInt<int>(1);
		Player->Unlocks[UnlockID].Level = Level;
	}
	Database->CloseQuery();

	// Set status effects
	Database->PrepareQuery("SELECT buff_id, level, duration FROM statuseffect WHERE character_id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	while(Database->FetchRow()) {
		uint32_t BuffID = Database->GetInt<uint32_t>(0);
		int Level = Database->GetInt<int>(1);
		double Duration = Database->GetReal(2);

		// Create status effect
		_StatusEffect *StatusEffect = new _StatusEffect();
		StatusEffect->Buff = Stats->Buffs[BuffID];
		StatusEffect->Level = Level;
		StatusEffect->Duration = Duration;
		StatusEffect->Time = 1.0 - (Duration - (int)Duration);
		Player->StatusEffects.push_back(StatusEffect);
	}
	Database->CloseQuery();

	// Get stats
	Player->GenerateNextBattle();
	Player->CalculateStats();

	// Max sure player has health
	if(!Player->IsAlive() && !Player->Hardcore)
		Player->Health = Player->MaxHealth / 2;
}

// Saves the player
void _Save::SavePlayer(const _Object *Player, NetworkIDType MapID) {
	if(Player->CharacterID == 0)
		return;

	// Reset spawn point if player is dead
	if(!Player->IsAlive())
		MapID = 0;

	// Get player stats
	std::string Data;
	Player->SerializeSaveData(Data);
	Data += std::string("\nmap_id=") + std::to_string(MapID);

	// Save character stats
	int Index = 1;
	Database->PrepareQuery("UPDATE character SET data = @data WHERE id = @character_id");
	Database->BindString(Index++, Data);
	Database->BindInt(Index++, Player->CharacterID);
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
			Database->PrepareQuery("INSERT INTO inventory VALUES(@character_id, @slot, @item_id, @upgrades, @count)");
			Database->BindInt(1, Player->CharacterID);
			Database->BindInt(2, (uint32_t)i);
			Database->BindInt(3, InventorySlot->Item->ID);
			Database->BindInt(4, InventorySlot->Upgrades);
			Database->BindInt(5, InventorySlot->Count);
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
		Database->PrepareQuery("INSERT INTO skill VALUES(@character_id, 0, @item_id, @level)");
		Database->BindInt(1, Player->CharacterID);
		Database->BindInt(2, Skill.first);
		Database->BindInt(3, Skill.second);
		Database->FetchRow();
		Database->CloseQuery();
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

	// Save unlocks
	Database->PrepareQuery("DELETE FROM unlock WHERE character_id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	Database->FetchRow();
	Database->CloseQuery();

	for(auto &Unlock : Player->Unlocks) {
		Database->PrepareQuery("INSERT INTO unlock VALUES(@character_id, @unlock_id, @level)");
		Database->BindInt(1, Player->CharacterID);
		Database->BindInt(2, Unlock.first);
		Database->BindInt(3, Unlock.second.Level);
		Database->FetchRow();
		Database->CloseQuery();
	}

	// Save status effects
	Database->PrepareQuery("DELETE FROM statuseffect WHERE character_id = @character_id");
	Database->BindInt(1, Player->CharacterID);
	Database->FetchRow();
	Database->CloseQuery();

	for(auto &StatusEffect : Player->StatusEffects) {
		Database->PrepareQuery("INSERT INTO statuseffect VALUES(@character_id, @buff_id, @level, @duration)");
		Database->BindInt(1, Player->CharacterID);
		Database->BindInt(2, StatusEffect->Buff->ID);
		Database->BindInt(3, StatusEffect->Level);
		Database->BindReal(4, StatusEffect->Duration);
		Database->FetchRow();
		Database->CloseQuery();
	}
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
				"	secret INTEGER\n"
				")"
	);

	Database->RunQuery(
				"INSERT INTO settings VALUES(" + std::to_string(DEFAULT_SAVE_VERSION) + ", " + std::to_string(GetRandomInt((uint64_t)1, std::numeric_limits<uint64_t>::max())) + ")"
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
				"INSERT INTO account(id, username, password) VALUES(1, '', '')"
	);

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

	// Status Effects
	Database->RunQuery(
				"CREATE TABLE statuseffect(\n"
				"	character_id INTEGER REFERENCES character(id) ON DELETE CASCADE,\n"
				"	buff_id INTEGER DEFAULT(0),\n"
				"	level INTEGER DEFAULT(0),\n"
				"	duration REAL DEFAULT(0)\n"
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
				"	upgrades INTEGER,\n"
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
				"	unlock_id INTEGER,\n"
				"	level INTEGER DEFAULT(0)\n"
				")"
	);

	Database->RunQuery("CREATE INDEX inventory_character_id ON inventory(character_id)");
	Database->RunQuery("CREATE INDEX skill_character_id ON skill(character_id)");
	Database->RunQuery("CREATE INDEX actionbar_character_id ON actionbar(character_id)");
	Database->RunQuery("CREATE INDEX unlock_character_id ON unlock(character_id)");
	Database->RunQuery("CREATE INDEX character_account_id ON character(account_id)");
	Database->RunQuery("CREATE INDEX character_name ON character(name)");

	EndTransaction();
}
