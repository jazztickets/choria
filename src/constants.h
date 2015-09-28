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

#include <SColor.h>

const int GAME_PORTALTIME = 3000;
const int PLAYER_MOVETIME = 125;

const int MAP_VERSION = 1;
const int SCROLLMIN_X = 2;
const int SCROLLMIN_Y = 2;

const irr::video::SColor COLOR_GOLD(255, 196, 187, 44);
const irr::video::SColor COLOR_GRAY(255, 150, 150, 150);
const irr::video::SColor COLOR_LIGHTGRAY(255, 200, 200, 200);

const int MESSAGE_TIME = 15000;
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
const int BATTLE_ROUNDTIME = 5 * 1000;
const int BATTLE_MAXSKILLS = 8;
const int BATTLE_SHOWRESULTTIME = 2000;
const int BATTLE_WAITRESULTTIME = 375;
const int BATTLE_WAITENDTIME = 300;
