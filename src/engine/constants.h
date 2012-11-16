/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef CONSTANTS_H
#define CONSTANTS_H

// Version of the server.s3db database
#define SERVER_DATABASEVERSION "3"

// Version of the game that's used to check compatibility between client and server
const char * const GAME_VERSIONSTR = "v0.1.0";
const int GAME_VERSION = 010;

// Maximum action bar count
const int GAME_MAXACTIONS = 8;

// Starting action bar length
const int GAME_SKILLBARLENGTH = 4;

// Map file version
const int MAP_VERSION = 1;

// Maximum amount of gold
const int STATS_MAXGOLD = 1000000;

// Maximum number of fighters in a battle
const int BATTLE_MAXFIGHTERS = 6;

// Battle delay times
const int BATTLE_SHOWRESULTTIME = 2000;
const int BATTLE_WAITENDTIME = 300;

// Number of inventory slots for trading
const int TRADE_SLOTS = 8;

// Player
const int PLAYER_MOVETIME = 125;

#endif
