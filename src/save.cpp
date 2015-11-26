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
#include <database.h>
#include <config.h>
#include <constants.h>
#include <string>
#include <stdexcept>

// Constructor
_Save::_Save() {

	// Create connection
	Database = new _Database();

	// Open database
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
_Save::~_Save() {

	delete Database;
}

// Create default save database
void _Save::CreateDefaultDatabase() {

	Database->RunQuery(
				"BEGIN TRANSACTION"
	);

	// Settings
	Database->RunQuery(
				"CREATE TABLE settings('version' INTEGER)"
	);

	Database->RunQuery(
				"INSERT INTO settings(version) VALUES(" + std::to_string(SAVE_VERSION) + ")"
	);

	// Accounts
	Database->RunQuery(
				"CREATE TABLE account("
				"'id' INTEGER PRIMARY KEY,"
				"'username' TEXT,"
				"'password' TEXT"
				")"
	);

	Database->RunQuery(
				"INSERT INTO account(username, password) VALUES('choria_singleplayer', 'choria_singleplayer')"
	);

	// Characters
	Database->RunQuery(
				"CREATE TABLE character("
				"'id' INTEGER PRIMARY KEY,"
				"'account_id' INTEGER DEFAULT(0),"
				"'map_id' INTEGER DEFAULT(1),"
				"'spawnpoint' INTEGER DEFAULT(0),"
				"'name' TEXT,"
				"'portrait_id' INTEGER DEFAULT(1),"
				"'experience' INTEGER DEFAULT(0),"
				"'gold' INTEGER DEFAULT(0),"
				"'actionbar0' INTEGER DEFAULT(0),"
				"'actionbar1' INTEGER DEFAULT(0),"
				"'actionbar2' INTEGER DEFAULT(0),"
				"'actionbar3' INTEGER DEFAULT(0),"
				"'actionbar4' INTEGER DEFAULT(0),"
				"'actionbar5' INTEGER DEFAULT(0),"
				"'actionbar6' INTEGER DEFAULT(0),"
				"'actionbar7' INTEGER DEFAULT(0),"
				"'playtime' INTEGER DEFAULT(0),"
				"'deaths' INTEGER DEFAULT(0),"
				"'monsterkills' INTEGER DEFAULT(0),"
				"'playerkills' INTEGER DEFAULT(0),"
				"'bounty' INTEGER DEFAULT(0)"
				")"
	);

	// Inventory
	Database->RunQuery(
				"CREATE TABLE inventory("
				"'character_id' INTEGER,"
				"'slot' INTEGER,"
				"'item_id' INTEGER,"
				"'count' INTEGER"
				")"
	);

	// Skill levels
	Database->RunQuery(
				"CREATE TABLE skilllevel("
				"'character_id' INTEGER,"
				"'skill_id' INTEGER,"
				"'level' INTEGER"
				")"
	);

	Database->RunQuery(
				"END TRANSACTION"
	);
}
