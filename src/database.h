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
#pragma once

// Libraries
#include <string>
#include <unordered_map>
#include <cstdint>

// Forward Declarations
struct sqlite3;
struct sqlite3_stmt;

// Classes
class _Database {

	public:

		_Database(const std::string &Path);
		~_Database();

		void RunQuery(const std::string &Query);
		void PrepareQuery(const std::string &Query, int Handle=0);
		int FetchRow(int Handle=0);
		int CloseQuery(int Handle=0);
		int64_t GetLastInsertID();

		int GetColumnIndex(const std::string &Name, int Handle);

		template<typename T> T GetInt(int ColumnIndex, int Handle=0);
		template<typename T> T GetInt(const std::string &ColumnName, int Handle=0);
		double GetReal(int ColumnIndex, int Handle=0);
		double GetReal(const std::string &ColumnName, int Handle=0);
		const char *GetString(int ColumnIndex, int Handle=0);
		const char *GetString(const std::string &ColumnName, int Handle=0);

		void BindInt(int ColumnIndex, int Value, int Handle=0);
		void BindInt(int ColumnIndex, uint32_t Value, int Handle=0);
		void BindReal(int ColumnIndex, double Value, int Handle=0);
		void BindString(int ColumnIndex, const std::string &String, int Handle=0);

	private:

		sqlite3 *Database;
		sqlite3_stmt *QueryHandle[2];

		std::unordered_map<std::string, int> ColumnIndexes[2];

};
