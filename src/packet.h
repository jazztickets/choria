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
	ACCOUNT_EXISTS,
	ACCOUNT_INUSE,
	ACCOUNT_LOGININFO,
	ACCOUNT_NOTFOUND,
	ACCOUNT_SUCCESS,
	ACTIONBAR_CHANGED,
	BATTLE_ACTION,
	BATTLE_END,
	BATTLE_START,
	CHARACTERS_DELETE,
	CHARACTERS_LIST,
	CHARACTERS_PLAY,
	CHARACTERS_REQUEST,
	CHAT_MESSAGE,
	CREATECHARACTER_INFO,
	CREATECHARACTER_INUSE,
	CREATECHARACTER_SUCCESS,
	EVENT_START,
	INVENTORY_GOLD,
	INVENTORY_MOVE,
	INVENTORY_SPLIT,
	INVENTORY_SWAP,
	INVENTORY_UPDATE,
	INVENTORY_USE,
	OBJECT_STATS,
	PLAYER_STATUS,
	ACTION_USE,
	ACTION_RESULTS,
	SKILLS_SKILLADJUST,
	STAT_CHANGE,
	TRADER_ACCEPT,
	TRADE_ACCEPT,
	TRADE_CANCEL,
	TRADE_EXCHANGE,
	TRADE_GOLD,
	TRADE_ITEM,
	TRADE_REQUEST,
	VENDOR_EXCHANGE,
	WORLD_ATTACKPLAYER,
	WORLD_CHANGEMAPS,
	WORLD_CREATEOBJECT,
	WORLD_DELETEOBJECT,
	WORLD_HUD,
	WORLD_MOVECOMMAND,
	WORLD_OBJECTLIST,
	WORLD_OBJECTUPDATES,
	WORLD_POSITION,
	WORLD_TELEPORTSTART,
};
