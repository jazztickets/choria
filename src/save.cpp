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

// Populates the server database with the default data
void _Save::CreateDefaultDatabase() {

	Database->RunQuery(
				"BEGIN TRANSACTION"
	);

	Database->RunQuery(
				"CREATE TABLE ServerInfo('Version' INTEGER)"
	);

	Database->RunQuery(
				"CREATE TABLE Accounts("
				"'ID' INTEGER PRIMARY KEY"
				", 'Username' TEXT"
				", 'Password' TEXT"
				")"
	);

	Database->RunQuery(
				"CREATE TABLE Characters("
				"'ID' INTEGER PRIMARY KEY"
				", 'AccountsID' INTEGER DEFAULT(0)"
				", 'MapID' INTEGER DEFAULT(1)"
				", 'SpawnPoint' INTEGER DEFAULT(0)"
				", 'Name' TEXT"
				", 'PortraitID' INTEGER DEFAULT(1)"
				", 'Experience' INTEGER DEFAULT(0)"
				", 'Gold' INTEGER DEFAULT(0)"
				", 'ActionBar0' INTEGER DEFAULT(0)"
				", 'ActionBar1' INTEGER DEFAULT(0)"
				", 'ActionBar2' INTEGER DEFAULT(0)"
				", 'ActionBar3' INTEGER DEFAULT(0)"
				", 'ActionBar4' INTEGER DEFAULT(0)"
				", 'ActionBar5' INTEGER DEFAULT(0)"
				", 'ActionBar6' INTEGER DEFAULT(0)"
				", 'ActionBar7' INTEGER DEFAULT(0)"
				", 'PlayTime' INTEGER DEFAULT(0)"
				", 'Deaths' INTEGER DEFAULT(0)"
				", 'MonsterKills' INTEGER DEFAULT(0)"
				", 'PlayerKills' INTEGER DEFAULT(0)"
				", 'Bounty' INTEGER DEFAULT(0)"
				")"
	);

	Database->RunQuery(
				"CREATE TABLE Inventory("
				"'CharactersID' INTEGER"
				", 'Slot' INTEGER"
				", 'ItemsID' INTEGER"
				", 'Count' INTEGER"
				")"
	);

	Database->RunQuery(
				"CREATE TABLE SkillLevel("
				"'CharactersID' INTEGER"
				", 'SkillsID' INTEGER"
				", 'Level' INTEGER"
				")"
	);

	Database->RunQuery(
				"INSERT INTO ServerInfo(Version) VALUES(" + std::to_string(SAVE_VERSION) + ")"
	);

	Database->RunQuery(
				"INSERT INTO Accounts(Username, Password) VALUES('singleplayer', 'singleplayer')"
	);

	Database->RunQuery(
				"END TRANSACTION"
	);
}
