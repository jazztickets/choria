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
#pragma once

// Libraries
#include <string>

// Forward Declarations
class _Database;
class _Object;

// Classes
class _Save {

	public:

		_Save();
		~_Save();

		_Database *Database;

		// Accounts
		bool CheckUsername(const std::string &Username);
		void CreateAccount(const std::string &Username, const std::string &Password);
		uint32_t GetAccountID(const std::string &Username, const std::string &Password);
		uint32_t GetCharacterCount(uint32_t AccountID);
		uint32_t GetCharacterIDByName(const std::string &Name);
		uint32_t GetCharacterIDBySlot(uint32_t AccountID, int Slot);
		void DeleteCharacter(uint32_t CharacterID);
		void CreateCharacter(uint32_t AccountID, int Slot, const std::string &Name, uint32_t PortraitID);

		void SavePlayer(const _Object *Player);

	private:

		std::string TrimString(const std::string &String);

		void CreateDefaultDatabase();

};
