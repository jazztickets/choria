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
#include <cstdint>

// Forward Declarations
struct sqlite3;
struct sqlite3_stmt;

// Classes
class _Database {

	public:

		_Database();
		~_Database();

		int OpenDatabase(const std::string &Path);
		int OpenDatabaseCreate(const std::string &Path);

		void RunQuery(const std::string &Query);
		void RunDataQuery(const std::string &Query, int Handle=0);
		int RunCountQuery(const std::string &Query);
		int FetchRow(int Handle=0);
		int CloseQuery(int Handle=0);
		int64_t GetLastInsertID();

		int GetInt(int ColumnIndex, int Handle=0);
		float GetFloat(int ColumnIndex, int Handle=0);
		const char *GetString(int ColumnIndex, int Handle=0);

	private:

		sqlite3 *Database;
		sqlite3_stmt *QueryHandle[2];

};
