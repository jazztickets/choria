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

#include <cstdint>

// Event types
enum class EventType : uint8_t {
	NONE,
	SCRIPT,
	SPAWN,
	MAPENTRANCE,
	MAPCHANGE,
	PORTAL,
	JUMP,
	KEY,
	VENDOR,
	TRADER,
	BLACKSMITH,
	MINIGAME,
};

// Item types
enum class ItemType : uint32_t {
	NONE,
	SKILL,
	HELMET,
	ARMOR,
	BOOTS,
	ONEHANDED_WEAPON,
	TWOHANDED_WEAPON,
	SHIELD,
	RING,
	AMULET,
	CONSUMABLE,
	TRADABLE,
	UNLOCKABLE,
	KEY,
};

// Types of targets
enum class TargetType : uint32_t {
	NONE,
	SELF,
	ENEMY,
	ALLY,
	MULTIPLE_ENEMIES,
	MULTIPLE_ALLIES,
	ALL_ENEMIES,
	ALL_ALLIES,
	ANY,
	ALL,
};

// Scope of action
enum class ScopeType : uint8_t {
	NONE,
	WORLD,
	BATTLE,
	ALL
};

// State of action
enum class ActionStateType : uint8_t {
	NONE,
	START,
	ANIMATION,
	APPLY,
	COOLDOWN,
};

// Types of stats
enum class StatType : int {
	ID,
	BUFF,
	BUFFLEVEL,
	BUFFDURATION,
	HEALTH,
	MAXHEALTH,
	MANA,
	MAXMANA,
	HEALTHREGEN,
	MANAREGEN,
	HEALPOWER,
	ATTACKPOWER,
	MINDAMAGE,
	MAXDAMAGE,
	ARMOR,
	DAMAGEBLOCK,
	PIERCE,
	MOVESPEED,
	STAMINA,
	BATTLESPEED,
	DROPRATE,
	HITCHANCE,
	EVASION,
	STUNNED,
	RESISTTYPE,
	RESIST,
	EXPERIENCE,
	GOLD,
	GOLDSTOLEN,
	INVISIBLE,
	ACTIONBARSIZE,
	MISS,
	CRIT,
	FLEE,
	BATTLE,
	HUNT,
	BOUNTYHUNT,
	TELEPORT,
	LIGHT,
	CLOCK,
	COUNT,
};

// Render flags
enum MapRenderType {
	MAP_RENDER_BOUNDARY          = (1 << 1),
	MAP_RENDER_TEXTURE           = (1 << 2),
	MAP_RENDER_WALL              = (1 << 3),
	MAP_RENDER_PVP               = (1 << 4),
	MAP_RENDER_ZONE              = (1 << 5),
	MAP_RENDER_EVENTTYPE         = (1 << 6),
	MAP_RENDER_EVENTDATA         = (1 << 7),
	MAP_RENDER_EDITOR_AMBIENT    = (1 << 8),
};