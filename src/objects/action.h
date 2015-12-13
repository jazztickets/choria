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
#include <objects/statchange.h>
#include <glm/vec2.hpp>
#include <cstdint>

// Forward Declarations
class _Stats;
class _Object;
class _Skill;
class _Texture;
class _Item;
class _Buff;
class _Buffer;

// Types of targets
enum class TargetType : uint32_t {
	NONE,
	SELF,
	ENEMY,
	ALLY,
	ENEMY_ALL,
	ALLY_ALL,
	ALL,
};

// Scope of action
enum class ScopeType : uint8_t {
	NONE,
	WORLD,
	BATTLE,
	ALL
};

// Structures
struct _ActionResult {
	_ActionResult() :
		LastPosition(0, 0),
		Position(0, 0),
		Texture(nullptr),
		SkillUsed(nullptr),
		ItemUsed(nullptr),
		Buff(nullptr),
		Time(0.0),
		Scope(ScopeType::ALL) { }

	_StatChange Source;
	_StatChange Target;
	glm::vec2 LastPosition;
	glm::vec2 Position;
	const _Texture *Texture;
	const _Skill *SkillUsed;
	const _Item *ItemUsed;
	const _Buff *Buff;
	double Time;
	ScopeType Scope;
};

// Action
class _Action {

	public:

		_Action() : Skill(nullptr), Item(nullptr), Count(0) { }
		_Action(const _Skill *Skill) : _Action() { this->Skill = Skill; }
		_Action(const _Item *Item) : _Action() { this->Item = Item; }

		bool operator==(const _Action &Action) const { return Action.Skill == Skill && Action.Item == Item; }
		bool operator!=(const _Action &Action) const { return !(Action.Skill == Skill && Action.Item == Item); }

		void Serialize(_Buffer &Data);
		void Unserialize(_Buffer &Data, _Stats *Stats);

		bool IsSet() const { return !(Skill == nullptr && Item == nullptr); }
		void Unset() { Skill = nullptr; Item = nullptr; Count = 0; }

		const _Skill *Skill;
		const _Item *Item;
		int Count;
};
