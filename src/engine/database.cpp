/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2010  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#include "database.h"

// Constructor
DatabaseClass::DatabaseClass()
:	Database(NULL) {

	QueryHandle[0] = NULL;
	QueryHandle[1] = NULL;
}

// Destructor
DatabaseClass::~DatabaseClass() {

	// Close database
	if(Database)
		sqlite3_close(Database);
}

// Load a database file
int DatabaseClass::OpenDatabase(const char *TFilename) {

	// Open database file
	int Result = sqlite3_open_v2(TFilename, &Database, SQLITE_OPEN_READWRITE, NULL);
	if(Result != SQLITE_OK) {
		printf("OpenDatabase: %s\n", sqlite3_errmsg(Database));
		sqlite3_close(Database);

		return 0;
	}

	return 1;
}

// Load a database file
int DatabaseClass::OpenDatabaseCreate(const char *TFilename) {

	// Open database file
	int Result = sqlite3_open_v2(TFilename, &Database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	if(Result != SQLITE_OK) {
		printf("OpenDatabaseCreate: %s\n", sqlite3_errmsg(Database));
		sqlite3_close(Database);

		return 0;
	}

	return 1;
}

// Runs a query 
int DatabaseClass::RunQuery(const char *TQueryString) {
	
	sqlite3_stmt *NewQueryHandle;
	const char *Tail;
	int Result = sqlite3_prepare_v2(Database, TQueryString, strlen(TQueryString), &NewQueryHandle, &Tail);
	if(Result != SQLITE_OK) {
		printf("RunQuery: %s\n", sqlite3_errmsg(Database));
		return 0;
	}

	Result = sqlite3_step(NewQueryHandle);
	if(Result != SQLITE_DONE) {
		printf("RunQuery: %s\n", sqlite3_errmsg(Database));
		return 0;
	}

	Result = sqlite3_finalize(NewQueryHandle);
	if(Result != SQLITE_OK) {
		printf("RunQuery: %s\n", sqlite3_errmsg(Database));
		return 0;
	}

	return 1;
}

// Runs a query that returns data
int DatabaseClass::RunDataQuery(const char *TQueryString, int THandle) {

	const char *Tail;
	int Result = sqlite3_prepare_v2(Database, TQueryString, strlen(TQueryString), &QueryHandle[THandle], &Tail);
	if(Result != SQLITE_OK) {
		printf("RunDataQuery: %s\n", sqlite3_errmsg(Database));
		return 0;
	}

	return 1;
}

// Runs a query that counts a row and returns the result
int DatabaseClass::RunCountQuery(const char *TQueryString) {

	RunDataQuery(TQueryString);
	FetchRow();
	int Count = GetInt(0);

	CloseQuery();

	return Count;
}

// Fetch 1 row from a query
int DatabaseClass::FetchRow(int THandle) {

	int Result = sqlite3_step(QueryHandle[THandle]);
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
int DatabaseClass::CloseQuery(int THandle) {
	
	int Result = sqlite3_finalize(QueryHandle[THandle]);
	if(Result != SQLITE_OK) {
		printf("RunQuery: %s\n", sqlite3_errmsg(Database));
		return 0;
	}

	return 1;
}

// Gets the last insert id
int DatabaseClass::GetLastInsertID() {
	
	return (int)sqlite3_last_insert_rowid(Database);
}

// Returns an integer column
int DatabaseClass::GetInt(int TColumnIndex, int THandle) {

	return sqlite3_column_int(QueryHandle[THandle], TColumnIndex);
}

// Returns a float column
float DatabaseClass::GetFloat(int TColumnIndex, int THandle) {

	return static_cast<float>(sqlite3_column_double(QueryHandle[THandle], TColumnIndex));
}

// Returns a string column
const char *DatabaseClass::GetString(int TColumnIndex, int THandle) {

	return (const char *)sqlite3_column_text(QueryHandle[THandle], TColumnIndex);
}
