/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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

// Enumerations
enum class PingType : uint8_t {
	SERVER_INFO,
	SERVER_INFO_RESPONSE,
};

enum class PacketType : uint8_t {
	VERSION,
	ACCOUNT_EXISTS,
	ACCOUNT_INUSE,
	ACCOUNT_LOGININFO,
	ACCOUNT_NOTFOUND,
	ACCOUNT_SUCCESS,
	ACTIONBAR_CHANGED,
	ACTION_CLEAR,
	ACTION_RESULTS,
	ACTION_USE,
	BATTLE_ACTION,
	BATTLE_END,
	BATTLE_JOIN,
	BATTLE_LEAVE,
	BATTLE_START,
	BLACKSMITH_UPGRADE,
	CHARACTERS_DELETE,
	CHARACTERS_LIST,
	CHARACTERS_PLAY,
	CHARACTERS_REQUEST,
	CHAT_MESSAGE,
	COMMAND,
	CREATECHARACTER_INFO,
	CREATECHARACTER_INUSE,
	CREATECHARACTER_SUCCESS,
	EVENT_START,
	INVENTORY,
	INVENTORY_ADD,
	INVENTORY_DELETE,
	INVENTORY_GOLD,
	INVENTORY_MOVE,
	INVENTORY_SPLIT,
	INVENTORY_SWAP,
	INVENTORY_UPDATE,
	INVENTORY_USE,
	MINIGAME_GETPRIZE,
	MINIGAME_PAY,
	MINIGAME_SEED,
	OBJECT_STATS,
	PARTY_INFO,
	PLAYER_STATUS,
	PLAYER_UPDATEBUFF,
	PLAYER_STATUSEFFECTS,
	SKILLS_SKILLADJUST,
	STAT_CHANGE,
	TRADE_ACCEPT,
	TRADE_CANCEL,
	TRADE_EXCHANGE,
	TRADE_GOLD,
	TRADE_ITEM,
	TRADE_REQUEST,
	TRADER_ACCEPT,
	VENDOR_EXCHANGE,
	WORLD_ATTACKPLAYER,
	WORLD_CHANGEMAPS,
	WORLD_CLOCK,
	WORLD_CREATEOBJECT,
	WORLD_DELETEOBJECT,
	WORLD_EXIT,
	WORLD_HUD,
	WORLD_JOIN,
	WORLD_MOVECOMMAND,
	WORLD_OBJECTLIST,
	WORLD_OBJECTUPDATES,
	WORLD_POSITION,
	WORLD_RESPAWN,
	WORLD_TELEPORTSTART,
	WORLD_USECOMMAND,
};
