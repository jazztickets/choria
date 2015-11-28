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
#include <database.h>
#include <sqlite3.h>
#include <stdexcept>

// Constructor
_Database::_Database()
:	Database(nullptr) {

	QueryHandle[0] = nullptr;
	QueryHandle[1] = nullptr;
}

// Destructor
_Database::~_Database() {

	// Close database
	if(Database)
		sqlite3_close(Database);
}

// Load a database file
void _Database::OpenDatabase(const std::string &Path) {

	// Open database file
	int Result = sqlite3_open_v2(Path.c_str(), &Database, SQLITE_OPEN_READWRITE, nullptr);
	if(Result != SQLITE_OK) {
		std::string Error = sqlite3_errmsg(Database);
		sqlite3_close(Database);

		throw std::runtime_error(Error);
	}
}

// Load a database file
void _Database::OpenDatabaseCreate(const std::string &Path) {

	// Open database file
	int Result = sqlite3_open_v2(Path.c_str(), &Database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
	if(Result != SQLITE_OK) {
		std::string Error = sqlite3_errmsg(Database);
		sqlite3_close(Database);

		throw std::runtime_error(Error);
	}
}

// Runs a query
void _Database::RunQuery(const std::string &Query) {

	sqlite3_stmt *NewQueryHandle;
	int Result = sqlite3_prepare_v2(Database, Query.c_str(), -1, &NewQueryHandle, 0);
	if(Result != SQLITE_OK)
		throw std::runtime_error(std::string(sqlite3_errmsg(Database)));

	Result = sqlite3_step(NewQueryHandle);
	if(Result != SQLITE_DONE)
		throw std::runtime_error(std::string(sqlite3_errmsg(Database)));

	Result = sqlite3_finalize(NewQueryHandle);
	if(Result != SQLITE_OK)
		throw std::runtime_error(std::string(sqlite3_errmsg(Database)));
}

// Runs a query that returns data
void _Database::RunDataQuery(const std::string &Query, int Handle) {
	if(QueryHandle[Handle])
		throw std::runtime_error("Query handle already exists!");

	int Result = sqlite3_prepare_v2(Database, Query.c_str(), -1, &QueryHandle[Handle], 0);
	if(Result != SQLITE_OK)
		throw std::runtime_error(std::string(sqlite3_errmsg(Database)));
}

// Runs a query that counts a row and returns the result
int _Database::RunCountQuery(const std::string &Query) {

	RunDataQuery(Query);
	FetchRow();
	int Count = GetInt(0);
	CloseQuery();

	return Count;
}

// Fetch 1 row from a query
int _Database::FetchRow(int Handle) {

	int Result = sqlite3_step(QueryHandle[Handle]);
	switch(Result) {
		case SQLITE_ROW:
			return 1;
		break;
		case SQLITE_DONE:
		break;
		default:
		break;
	}

	return 0;
}

// Shut down a query
int _Database::CloseQuery(int Handle) {

	int Result = sqlite3_finalize(QueryHandle[Handle]);
	if(Result != SQLITE_OK)
		throw std::runtime_error(std::string(sqlite3_errmsg(Database)));

	QueryHandle[Handle] = nullptr;

	return 1;
}

// Gets the last insert id
int64_t _Database::GetLastInsertID() {

	return sqlite3_last_insert_rowid(Database);
}

// Returns an integer column
int _Database::GetInt(int ColumnIndex, int Handle) {

	return sqlite3_column_int(QueryHandle[Handle], ColumnIndex);
}

// Returns a float column
float _Database::GetFloat(int ColumnIndex, int Handle) {

	return (float)sqlite3_column_double(QueryHandle[Handle], ColumnIndex);
}

// Returns a string column
const char *_Database::GetString(int ColumnIndex, int Handle) {

	return (const char *)sqlite3_column_text(QueryHandle[Handle], ColumnIndex);
}

// Bind integer to parameter
void _Database::BindInt(int ColumnIndex, int Value, int Handle) {
	int Result = sqlite3_bind_int(QueryHandle[Handle], 1, Value);
	if(Result != SQLITE_OK)
		throw std::runtime_error(std::string(sqlite3_errmsg(Database)));
}

// Bind string to parameter
void _Database::BindString(int ColumnIndex, const std::string &String, int Handle) {
	int Result = sqlite3_bind_text(QueryHandle[Handle], ColumnIndex, String.c_str(), -1, SQLITE_STATIC);
	if(Result != SQLITE_OK)
		throw std::runtime_error(std::string(sqlite3_errmsg(Database)));
}
