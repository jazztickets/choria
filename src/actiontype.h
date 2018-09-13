/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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

#include <cstddef>

namespace Action {
	enum size_t {
		GAME_UP,
		GAME_DOWN,
		GAME_LEFT,
		GAME_RIGHT,
		GAME_JOIN,
		GAME_INVENTORY,
		GAME_SKILLS,
		GAME_TRADE,
		GAME_PARTY,
		GAME_CHAT,
		GAME_USE,
		GAME_SKILL1,
		GAME_SKILL2,
		GAME_SKILL3,
		GAME_SKILL4,
		GAME_SKILL5,
		GAME_SKILL6,
		GAME_SKILL7,
		GAME_SKILL8,
		MENU_UP,
		MENU_DOWN,
		MENU_LEFT,
		MENU_RIGHT,
		MENU_GO,
		MENU_BACK,
		MENU_PAUSE,
		MISC_CONSOLE,
		MISC_DEBUG,
		COUNT,
	};
}
