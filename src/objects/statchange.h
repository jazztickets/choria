/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
#include <objects/statuseffect.h>
#include <manager.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <map>

// Forward Declarations
class _Object;
class _Battle;
class _Buffer;
class _Font;

// Hash function for StatType
struct StatTypeHash {
	template <typename T>
	std::size_t operator()(T t) const {
		return static_cast<std::uint64_t>(t);
	}
};

// Types of stats
enum class StatType : uint64_t {
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
	MOVESPEED,
	STAMINA,
	BATTLESPEED,
	HITCHANCE,
	EVASION,
	STUNNED,
	RESISTTYPE,
	RESIST,
	EXPERIENCE,
	GOLD,
	INVISIBLE,
	ACTIONBARSIZE,
	MISS,
	CRIT,
	FLEE,
	BATTLE,
	TELEPORT,
	COUNT,
};

enum class StatValueType : int {
	BOOLEAN,
	INTEGER,
	FLOAT,
	POINTER,
};

struct _StatStorage {
	StatType Type;
	StatValueType ValueType;
};

const std::map<std::string, StatType> StatStringToType = {
	{ "ID",            StatType::ID            },
	{ "Buff",          StatType::BUFF          },
	{ "BuffLevel",     StatType::BUFFLEVEL     },
	{ "BuffDuration",  StatType::BUFFDURATION  },
	{ "Health",        StatType::HEALTH        },
	{ "MaxHealth",     StatType::MAXHEALTH     },
	{ "Mana",          StatType::MANA          },
	{ "MaxMana",       StatType::MAXMANA       },
	{ "HealthRegen",   StatType::HEALTHREGEN   },
	{ "ManaRegen",     StatType::MANAREGEN     },
	{ "HealPower",     StatType::HEALPOWER     },
	{ "AttackPower",   StatType::ATTACKPOWER   },
	{ "MinDamage",     StatType::MINDAMAGE     },
	{ "MaxDamage",     StatType::MAXDAMAGE     },
	{ "Armor",         StatType::ARMOR         },
	{ "DamageBlock",   StatType::DAMAGEBLOCK   },
	{ "MoveSpeed",     StatType::MOVESPEED     },
	{ "Stamina",       StatType::STAMINA       },
	{ "BattleSpeed",   StatType::BATTLESPEED   },
	{ "HitChance",     StatType::HITCHANCE     },
	{ "Evasion",       StatType::EVASION       },
	{ "Stunned",       StatType::STUNNED       },
	{ "ResistType",    StatType::RESISTTYPE    },
	{ "Resist",        StatType::RESIST        },
	{ "Experience",    StatType::EXPERIENCE    },
	{ "Gold",          StatType::GOLD          },
	{ "Invisible",     StatType::INVISIBLE     },
	{ "ActionBarSize", StatType::ACTIONBARSIZE },
	{ "Miss",          StatType::MISS          },
	{ "Crit",          StatType::CRIT          },
	{ "Flee",          StatType::FLEE          },
	{ "Battle",        StatType::BATTLE        },
	{ "Teleport",      StatType::TELEPORT      },
};

const _StatStorage StatValueTypes[] = {
	{ StatType::ID,            StatValueType::INTEGER },
	{ StatType::BUFF,          StatValueType::POINTER },
	{ StatType::BUFFLEVEL,     StatValueType::INTEGER },
	{ StatType::BUFFDURATION,  StatValueType::FLOAT   },
	{ StatType::HEALTH,        StatValueType::INTEGER },
	{ StatType::MAXHEALTH,     StatValueType::INTEGER },
	{ StatType::MANA,          StatValueType::INTEGER },
	{ StatType::MAXMANA,       StatValueType::INTEGER },
	{ StatType::HEALTHREGEN,   StatValueType::INTEGER },
	{ StatType::MANAREGEN,     StatValueType::INTEGER },
	{ StatType::HEALPOWER,     StatValueType::FLOAT   },
	{ StatType::ATTACKPOWER,   StatValueType::FLOAT   },
	{ StatType::MINDAMAGE,     StatValueType::INTEGER },
	{ StatType::MAXDAMAGE,     StatValueType::INTEGER },
	{ StatType::ARMOR,         StatValueType::INTEGER },
	{ StatType::DAMAGEBLOCK,   StatValueType::INTEGER },
	{ StatType::MOVESPEED,     StatValueType::INTEGER },
	{ StatType::STAMINA,       StatValueType::FLOAT   },
	{ StatType::BATTLESPEED,   StatValueType::INTEGER },
	{ StatType::HITCHANCE,     StatValueType::INTEGER },
	{ StatType::EVASION,       StatValueType::INTEGER },
	{ StatType::STUNNED,       StatValueType::BOOLEAN },
	{ StatType::RESISTTYPE,    StatValueType::INTEGER },
	{ StatType::RESIST,        StatValueType::INTEGER },
	{ StatType::EXPERIENCE,    StatValueType::INTEGER },
	{ StatType::GOLD,          StatValueType::INTEGER },
	{ StatType::INVISIBLE,     StatValueType::BOOLEAN },
	{ StatType::ACTIONBARSIZE, StatValueType::INTEGER },
	{ StatType::MISS,          StatValueType::BOOLEAN },
	{ StatType::CRIT,          StatValueType::BOOLEAN },
	{ StatType::FLEE,          StatValueType::BOOLEAN },
	{ StatType::BATTLE,        StatValueType::INTEGER },
	{ StatType::TELEPORT,      StatValueType::FLOAT   },
};

union _Value {
	int Integer;
	float Float;
	void *Pointer;
};

// Stat changes
class _StatChange {

	public:

		_StatChange();

		void Reset();
		void Serialize(_Buffer &Data);
		void Unserialize(_Buffer &Data, _Manager<_Object> *Manager);

		uint64_t GetChangedFlag();
		bool HasStat(StatType Type);

		// Owner
		_Object *Object;

		// Data
		std::map<StatType, _Value> Values;
};

// Graphical stat change
class _StatChangeUI {

	public:

		_StatChangeUI();

		void Render(double BlendFactor);
		void SetText(const glm::vec4 &NegativeColor, const glm::vec4 &PositiveColor);

		_Object *Object;
		const _Font *Font;
		std::string Text;
		glm::vec4 Color;
		glm::vec2 StartPosition;
		glm::vec2 LastPosition;
		glm::vec2 Position;
		float Direction;
		double Time;
		double Timeout;
		float Change;
		bool Battle;

};

