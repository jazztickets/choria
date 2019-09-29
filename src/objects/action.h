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
#include <enums.h>
#include <glm/vec2.hpp>
#include <cstdint>

// Forward Declarations
class _Stats;
class _Object;
class _Usable;
class _Buff;
class _BaseItem;
struct _ActionResult;

namespace ae {
	class _Texture;
	class _Buffer;
}

struct _Summon {
	_Summon() : ID(0), Health(0), Mana(0), Armor(0), MinDamage(0), MaxDamage(0) { }

	uint32_t ID;
	int Health;
	int Mana;
	int Armor;
	int MinDamage;
	int MaxDamage;
};

// Action used in battle
struct _BattleAction {
	_BattleAction() : Source(nullptr), Target(nullptr), AttackDelay(0.0), AttackTime(0.0), Time(0.0), LastPosition({0.0f, 0.0f}), Position({0.0f, 0.0f}), Texture(nullptr) { }

	_Object *Source;
	_Object *Target;
	double AttackDelay;
	double AttackTime;
	double Time;
	glm::vec2 LastPosition;
	glm::vec2 Position;
	const ae::_Texture *Texture;
};

// Action
class _Action {

	public:

		_Action() : Usable(nullptr), State(ActionStateType::NONE), ApplyTime(0.0), Time(0.0), Duration(0.0), Level(0), Count(0), InventorySlot(-1), ActionBarSlot(-1) { }
		_Action(const _Usable *Usable) : _Action() { this->Usable = Usable; }

		bool operator==(const _Action &Action) const { return Action.Usable == Usable; }
		bool operator!=(const _Action &Action) const { return Action.Usable != Usable; }

		void Serialize(ae::_Buffer &Data);
		void Unserialize(ae::_Buffer &Data, const _Stats *Stats);

		bool Start(_Object *Source, ScopeType Scope);
		bool Apply(ae::_Buffer &Data, _Object *Source, ScopeType Scope);
		void HandleSummons(_ActionResult &ActionResult);

		bool IsSet() const { return State != ActionStateType::NONE; }
		void Unset() { Usable = nullptr; State = ActionStateType::NONE; Count = 0; ApplyTime = 0.0; Time = 0.0; Duration = 0.0; Level = 0; InventorySlot = -1; ActionBarSlot = -1; }

		TargetType GetTargetType();

		const _Usable *Usable;
		ActionStateType State;
		double ApplyTime;
		double Time;
		double Duration;
		int Level;
		int Count;
		int InventorySlot;
		int ActionBarSlot;
};

// Result of an action use
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
	ScopeType Scope;
};
