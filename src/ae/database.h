/******************************************************************************
* Copyright (c) 2017 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
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

		_Database(const std::string &Path, bool ReadOnly=false);
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
