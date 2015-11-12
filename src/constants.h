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

#include <SColor.h>

const double GAME_PORTALTIME = 3.0;
const double GAME_SLEEPRATE = 1 / 120.0;
const double GAME_AUTOSAVEPERIOD = 60.0;
const double GAME_DOUBLECLICKTIME = 0.5;

const double NETWORK_UPDATEPERIOD = 0.2;

const int32_t MAP_VERSION = 1;
const int MAP_TILE_WIDTH = 32;
const int MAP_TILE_HEIGHT = 32;

const int CAMERA_SCROLLMIN_X = 2;
const int CAMERA_SCROLLMIN_Y = 2;

const int NETWORKING_PORT = 60006;
const int NETWORKING_CHAT_SIZE = 100;
const double NETWORKING_CHAT_TIMEOUT = 15.0;

const int SKILL_STARTX = 35;
const int SKILL_STARTY = 40;
const int SKILL_SPACINGX = 50;
const int SKILL_SPACINGY = 80;
const int SKILL_BARX = 272;
const int SKILL_BARY = 464;
const int SKILL_COLUMNS = 8;

const int TRADE_WINDOWX = 142;
const int TRADE_WINDOWYTHEM = 10;
const int TRADE_WINDOWYYOU = 138;

const int MAX_OBJECTS = 255;

const int BATTLE_MAXFIGHTERS = 6;
const double BATTLE_ROUNDTIME = 5.0;
const int BATTLE_MAXSKILLS = 8;
const double BATTLE_SHOWRESULTTIME = 2.0;
const double BATTLE_WAITRESULTTIME = 0.375;
const double BATTLE_WAITENDTIME = 0.30;

const double PLAYER_MOVETIME = 0.125;
const int PLAYER_TRADEITEMS = 8;
const double PLAYER_ATTACKTIME = 1.0;

const int STATS_MAXGOLD = 1000000;

const irr::video::SColor COLOR_GOLD(255, 196, 187, 44);
const irr::video::SColor COLOR_GRAY(255, 150, 150, 150);
const irr::video::SColor COLOR_LIGHTGRAY(255, 200, 200, 200);
