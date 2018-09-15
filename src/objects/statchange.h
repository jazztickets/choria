/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2018  Alan Witkowski
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
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <unordered_map>

// Forward Declarations
class _Object;
class _Battle;

namespace ae {
	template<class T> class _Manager;
	class _Font;
	class _Buffer;
}

// Hash function for StatType
struct StatTypeHash {
	template <typename T>
	std::size_t operator()(T t) const {
		return static_cast<std::uint64_t>(t);
	}
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

const std::unordered_map<std::string, _StatStorage> StatStringToType = {
	{ "ID",            { StatType::ID            , StatValueType::INTEGER } },
	{ "Buff",          { StatType::BUFF          , StatValueType::POINTER } },
	{ "BuffLevel",     { StatType::BUFFLEVEL     , StatValueType::INTEGER } },
	{ "BuffDuration",  { StatType::BUFFDURATION  , StatValueType::FLOAT   } },
	{ "Health",        { StatType::HEALTH        , StatValueType::INTEGER } },
	{ "MaxHealth",     { StatType::MAXHEALTH     , StatValueType::INTEGER } },
	{ "Mana",          { StatType::MANA          , StatValueType::INTEGER } },
	{ "MaxMana",       { StatType::MAXMANA       , StatValueType::INTEGER } },
	{ "HealthRegen",   { StatType::HEALTHREGEN   , StatValueType::INTEGER } },
	{ "ManaRegen",     { StatType::MANAREGEN     , StatValueType::INTEGER } },
	{ "HealPower",     { StatType::HEALPOWER     , StatValueType::FLOAT   } },
	{ "AttackPower",   { StatType::ATTACKPOWER   , StatValueType::FLOAT   } },
	{ "MinDamage",     { StatType::MINDAMAGE     , StatValueType::INTEGER } },
	{ "MaxDamage",     { StatType::MAXDAMAGE     , StatValueType::INTEGER } },
	{ "Armor",         { StatType::ARMOR         , StatValueType::INTEGER } },
	{ "DamageBlock",   { StatType::DAMAGEBLOCK   , StatValueType::INTEGER } },
	{ "MoveSpeed",     { StatType::MOVESPEED     , StatValueType::INTEGER } },
	{ "Stamina",       { StatType::STAMINA       , StatValueType::FLOAT   } },
	{ "BattleSpeed",   { StatType::BATTLESPEED   , StatValueType::INTEGER } },
	{ "DropRate",      { StatType::DROPRATE      , StatValueType::INTEGER } },
	{ "HitChance",     { StatType::HITCHANCE     , StatValueType::INTEGER } },
	{ "Evasion",       { StatType::EVASION       , StatValueType::INTEGER } },
	{ "Stunned",       { StatType::STUNNED       , StatValueType::BOOLEAN } },
	{ "ResistType",    { StatType::RESISTTYPE    , StatValueType::INTEGER } },
	{ "Resist",        { StatType::RESIST        , StatValueType::INTEGER } },
	{ "Experience",    { StatType::EXPERIENCE    , StatValueType::INTEGER } },
	{ "Gold",          { StatType::GOLD          , StatValueType::INTEGER } },
	{ "GoldStolen",    { StatType::GOLDSTOLEN    , StatValueType::INTEGER } },
	{ "Invisible",     { StatType::INVISIBLE     , StatValueType::BOOLEAN } },
	{ "ActionBarSize", { StatType::ACTIONBARSIZE , StatValueType::INTEGER } },
	{ "Miss",          { StatType::MISS          , StatValueType::BOOLEAN } },
	{ "Crit",          { StatType::CRIT          , StatValueType::BOOLEAN } },
	{ "Flee",          { StatType::FLEE          , StatValueType::BOOLEAN } },
	{ "Battle",        { StatType::BATTLE        , StatValueType::INTEGER } },
	{ "Hunt",          { StatType::HUNT          , StatValueType::FLOAT   } },
	{ "BountyHunt",    { StatType::BOUNTYHUNT    , StatValueType::FLOAT   } },
	{ "Teleport",      { StatType::TELEPORT      , StatValueType::FLOAT   } },
	{ "Light",         { StatType::LIGHT         , StatValueType::INTEGER } },
	{ "Clock",         { StatType::CLOCK         , StatValueType::FLOAT   } },
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

		void Reset() { Object = nullptr; Values.clear(); }
		bool HasStat(StatType Type) const { return Values.find(Type) != Values.end();	}

		void Serialize(ae::_Buffer &Data);
		void Unserialize(ae::_Buffer &Data, ae::_Manager<_Object> *Manager);

		// Owner
		_Object *Object;

		// Data
		std::unordered_map<StatType, _Value> Values;
};

// Graphical stat change
class _StatChangeUI {

	public:

		_StatChangeUI();

		void Render(double BlendFactor);
		void SetText(const glm::vec4 &NegativeColor, const glm::vec4 &PositiveColor);

		_Object *Object;
		const ae::_Font *Font;
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
