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

// Libraries
#include <cstdint>

// Types
typedef uint16_t NetworkIDType;

// Enumerations
enum class PacketType : uint8_t {
	VERSION,
	CHAT_MESSAGE,
	ACCOUNT_LOGININFO,
	ACCOUNT_EXISTS,
	ACCOUNT_NOTFOUND,
	ACCOUNT_ALREADYLOGGEDIN,
	ACCOUNT_SUCCESS,
	CHARACTERS_REQUEST,
	CHARACTERS_LIST,
	CHARACTERS_PLAY,
	CHARACTERS_DELETE,
	CREATECHARACTER_INFO,
	CREATECHARACTER_SUCCESS,
	CREATECHARACTER_INUSE,
	WORLD_MOVECOMMAND,
	WORLD_YOURCHARACTERINFO,
	WORLD_CHANGEMAPS,
	WORLD_OBJECTLIST,
	WORLD_CREATEOBJECT,
	WORLD_DELETEOBJECT,
	WORLD_OBJECTUPDATES,
	WORLD_HUD,
	WORLD_STARTBATTLE,
	WORLD_ATTACKPLAYER,
	WORLD_POSITION,
	PLAYER_STATUS,
	BATTLE_COMMAND,
	BATTLE_TURNRESULTS,
	BATTLE_CLIENTDONE,
	BATTLE_END,
	EVENT_START,
	INVENTORY_MOVE,
	INVENTORY_USE,
	INVENTORY_SPLIT,
	VENDOR_EXCHANGE,
	TRADER_ACCEPT,
	HUD_ACTIONBAR,
	SKILLS_SKILLADJUST,
	TRADE_REQUEST,
	TRADE_ITEM,
	TRADE_GOLD,
	TRADE_ACCEPT,
	TRADE_CANCEL,
	TRADE_EXCHANGE,
};
