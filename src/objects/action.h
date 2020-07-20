/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020  Alan Witkowski
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
#include <objects/components/inventory.h>
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
	class _Buffer;
}

// Struct for summons
struct _Summon {
	_Summon() : ID(0), Health(0), Mana(0), Armor(0), MinDamage(0), MaxDamage(0) { }

	uint32_t ID;
	int Health;
	int Mana;
	int Armor;
	int MinDamage;
	int MaxDamage;
};

// Action used in battle for client
struct _BattleAction {
	_BattleAction() : Source(nullptr), Target(nullptr), Usable(nullptr), AttackDelay(0.0), AttackTime(0.0), Time(0.0), LastPosition({0.0f, 0.0f}), Position({0.0f, 0.0f}) { }

	_Object *Source;
	_Object *Target;
	const _Usable *Usable;
	double AttackDelay;
	double AttackTime;
	double Time;
	glm::vec2 LastPosition;
	glm::vec2 Position;
};

// Action used
class _Action {

	public:

		_Action() : Usable(nullptr), State(ActionStateType::NONE), ApplyTime(0.0), Time(0.0), Cooldown(0.0), Level(0), ActionBarSlot(-1) { }
		_Action(const _Usable *Usable) : _Action() { this->Usable = Usable; }

		bool operator==(const _Action &Action) const { return Action.Usable == Usable; }
		bool operator!=(const _Action &Action) const { return Action.Usable != Usable; }

		bool Start(_Object *Source, ScopeType Scope);
		bool Apply(ae::_Buffer &Data, _Object *Source, ScopeType Scope);
		void StartCooldown();
		void HandleSummons(_ActionResult &ActionResult);

		bool IsSet() const { return State != ActionStateType::NONE; }
		void Unset() { Usable = nullptr; State = ActionStateType::NONE; ApplyTime = 0.0; Time = 0.0; Cooldown = 0.0; Level = 0; Slot.Reset(); ActionBarSlot = -1; }

		TargetType GetTargetType();

		const _Usable *Usable;
		_Slot Slot;
		ActionStateType State;
		double ApplyTime;
		double Time;
		double Cooldown;
		int Level;
		int ActionBarSlot;
};

// Result of an action use
struct _ActionResult {
	_ActionResult() : Scope(ScopeType::ALL) { }

	_StatChange Source;
	_StatChange Target;
	_Summon Summon;
	_Action ActionUsed;
	ScopeType Scope;
};
