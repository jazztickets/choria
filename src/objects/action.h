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
#include <objects/statchange.h>
#include <glm/vec2.hpp>
#include <cstdint>

// Forward Declarations
class _Stats;
class _Object;
class _Item;
class _Buff;
struct _ActionResult;

namespace ae {
	class _Texture;
	class _Buffer;
}

// Types of targets
enum class TargetType : uint32_t {
	NONE,
	SELF,
	ENEMY,
	ALLY,
	ENEMY_MULTI,
	ALLY_MULTI,
	ENEMY_ALL,
	ALLY_ALL,
	ANY,
	ENEMY_CORPSE_AOE,
};

// Scope of action
enum class ScopeType : uint8_t {
	NONE,
	WORLD,
	BATTLE,
	ALL
};

struct _Summon {
	_Summon() : ID(0), SpellID(0), Health(0), Mana(0), Armor(0), Limit(0), MinDamage(0), MaxDamage(0) { }

	uint32_t ID;
	uint32_t SpellID;
	int Health;
	int Mana;
	int Armor;
	int Limit;
	int MinDamage;
	int MaxDamage;
};

// Action
class _Action {

	public:

		_Action() : Item(nullptr), Duration(0.0), Level(0), Count(0), InventorySlot(-1), ActionBarSlot(-1) { }
		_Action(const _Item *Item) : _Action() { this->Item = Item; }

		bool operator==(const _Action &Action) const { return Action.Item == Item; }
		bool operator!=(const _Action &Action) const { return !(Action.Item == Item); }

		void Serialize(ae::_Buffer &Data);
		void Unserialize(ae::_Buffer &Data, const _Stats *Stats);

		bool Resolve(ae::_Buffer &Data, _Object *Source, ScopeType Scope);
		void HandleSummons(_ActionResult &ActionResult);

		bool IsSet() const { return !(Item == nullptr); }
		void Unset() { Item = nullptr; Count = 0; Duration = 0.0; Level = 0; InventorySlot = -1; ActionBarSlot = -1; }

		TargetType GetTargetType();

		const _Item *Item;
		double Duration;
		int Level;
		int Count;
		int InventorySlot;
		int ActionBarSlot;
};

// Structures
struct _ActionResult {
	_ActionResult();

	_StatChange Source;
	_StatChange Target;
	_Summon Summon;
	glm::vec2 LastPosition;
	glm::vec2 Position;
	_Action ActionUsed;
	const ae::_Texture *Texture;
	double Time;
	double Timeout;
	double Speed;
	bool Miss;
	ScopeType Scope;
};
