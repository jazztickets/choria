/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
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
*******************************************************************************/
#pragma once

// Libraries
#include <sqlite3.h>
#include <cstdio>
#include <cstring>

// Classes
class DatabaseClass {

	public:

		DatabaseClass();
		~DatabaseClass();

		int OpenDatabase(const char *TFilename);
		int OpenDatabaseCreate(const char *TFilename);

		int RunQuery(const char *TQueryString);
		int RunDataQuery(const char *TQueryString, int THandle=0);
		int RunCountQuery(const char *TQueryString);
		int FetchRow(int THandle=0);
		int CloseQuery(int THandle=0);
		int GetLastInsertID();

		int GetInt(int TColumnIndex, int THandle=0);
		float GetFloat(int TColumnIndex, int THandle=0);
		const char *GetString(int TColumnIndex, int THandle=0);

	private:

		sqlite3 *Database;
		sqlite3_stmt *QueryHandle[2];

};
