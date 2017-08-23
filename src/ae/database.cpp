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
#include <ae/database.h>
#include <sqlite3.h>
#include <stdexcept>

// Constructor
_Database::_Database(const std::string &Path, bool ReadOnly) {
	Database = nullptr;
	QueryHandle[0] = nullptr;
	QueryHandle[1] = nullptr;

	// Open database file
	int Flags = 0;
	if(ReadOnly)
		Flags |= SQLITE_OPEN_READONLY;
	else
		Flags |= SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE;

	int Result = sqlite3_open_v2(Path.c_str(), &Database, Flags, nullptr);
	if(Result != SQLITE_OK) {
		std::string Error = sqlite3_errmsg(Database);
		sqlite3_close(Database);

		throw std::runtime_error(Error);
	}
}

// Destructor
_Database::~_Database() {

	// Close database
	if(Database)
		sqlite3_close(Database);
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
void _Database::PrepareQuery(const std::string &Query, int Handle) {
	if(QueryHandle[Handle])
		throw std::runtime_error("Query handle already exists!");

	// Prepare query
	int Result = sqlite3_prepare_v2(Database, Query.c_str(), -1, &QueryHandle[Handle], 0);
	if(Result != SQLITE_OK)
		throw std::runtime_error(std::string(sqlite3_errmsg(Database)));

	// Load column name map
	int ColumnCount = sqlite3_column_count(QueryHandle[Handle]);
	for(int i = 0; i < ColumnCount; i++) {
		ColumnIndexes[Handle][sqlite3_column_name(QueryHandle[Handle], i)] = i;
	}
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
	ColumnIndexes[Handle].clear();

	return 1;
}

// Gets the last insert id
int64_t _Database::GetLastInsertID() {

	return sqlite3_last_insert_rowid(Database);
}

// Get column name by index
int _Database::GetColumnIndex(const std::string &Name, int Handle) {
	if(ColumnIndexes[Handle].find(Name) == ColumnIndexes[Handle].end())
		throw std::runtime_error("unknown column " + Name);

	return ColumnIndexes[Handle][Name];
}

// Returns an integer column
template<typename T> T _Database::GetInt(int ColumnIndex, int Handle) {

	return (T)sqlite3_column_int(QueryHandle[Handle], ColumnIndex);
}

// Returns an integer column by column name
template<typename T> T _Database::GetInt(const std::string &ColumnName, int Handle) {

	return (T)sqlite3_column_int(QueryHandle[Handle], GetColumnIndex(ColumnName, Handle));
}

// Returns a float column
double _Database::GetReal(int ColumnIndex, int Handle) {

	return sqlite3_column_double(QueryHandle[Handle], ColumnIndex);
}

// Returns an float column by column name
double _Database::GetReal(const std::string &ColumnName, int Handle) {

	return sqlite3_column_double(QueryHandle[Handle], GetColumnIndex(ColumnName, Handle));
}

// Returns a string column
const char *_Database::GetString(int ColumnIndex, int Handle) {

	return (const char *)sqlite3_column_text(QueryHandle[Handle], ColumnIndex);
}

// Returns a string column by column name
const char *_Database::GetString(const std::string &ColumnName, int Handle) {

	return (const char *)sqlite3_column_text(QueryHandle[Handle], GetColumnIndex(ColumnName, Handle));
}

// Bind integer to parameter
void _Database::BindInt(int ColumnIndex, int Value, int Handle) {
	int Result = sqlite3_bind_int(QueryHandle[Handle], ColumnIndex, Value);
	if(Result != SQLITE_OK)
		throw std::runtime_error(std::string(sqlite3_errmsg(Database)));
}

// Bind integer to parameter
void _Database::BindInt(int ColumnIndex, uint32_t Value, int Handle) {
	int Result = sqlite3_bind_int(QueryHandle[Handle], ColumnIndex, (int)Value);
	if(Result != SQLITE_OK)
		throw std::runtime_error(std::string(sqlite3_errmsg(Database)));
}

// Bind real to parameter
void _Database::BindReal(int ColumnIndex, double Value, int Handle) {
	int Result = sqlite3_bind_double(QueryHandle[Handle], ColumnIndex, Value);
	if(Result != SQLITE_OK)
		throw std::runtime_error(std::string(sqlite3_errmsg(Database)));
}

// Bind string to parameter
void _Database::BindString(int ColumnIndex, const std::string &String, int Handle) {
	int Result = sqlite3_bind_text(QueryHandle[Handle], ColumnIndex, String.c_str(), -1, SQLITE_STATIC);
	if(Result != SQLITE_OK)
		throw std::runtime_error(std::string(sqlite3_errmsg(Database)));
}

template uint8_t _Database::GetInt<uint8_t>(int ColumnIndex, int Handle);
template uint32_t _Database::GetInt<uint32_t>(int ColumnIndex, int Handle);
template uint64_t _Database::GetInt<uint64_t>(int ColumnIndex, int Handle);
template int _Database::GetInt<int>(int ColumnIndex, int Handle);

template uint8_t _Database::GetInt(const std::string &ColumnName, int Handle);
template uint32_t _Database::GetInt(const std::string &ColumnName, int Handle);
template uint64_t _Database::GetInt(const std::string &ColumnName, int Handle);
template int _Database::GetInt(const std::string &ColumnName, int Handle);
