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
#include <vector>
#include <cstdint>

// Forward Declarations
class _Object;
class _Stats;

// Structures
struct _ActionResult {
	_ActionResult() : Fighter(nullptr), SkillID(-1), Target(-1), DamageDealt(0), HealthChange(0), ManaChange(0) { }
	_Object *Fighter;
	int SkillID;
	int Target;
	int DamageDealt;
	int HealthChange;
	int ManaChange;
};

struct _BattleResult {
	_BattleResult() : FighterCount(0), PlayerCount(0), MonsterCount(0), ExperienceGiven(0), GoldGiven(0), Dead(true) { }
	int FighterCount;
	int PlayerCount;
	int MonsterCount;
	int ExperienceGiven;
	int GoldGiven;
	bool Dead;
};

// Classes
class _Battle {

	public:

		_Battle();
		virtual ~_Battle();

		// Objects
		void AddFighter(_Object *Fighter, int Side);
		virtual int RemoveFighter(_Object *RemoveFighter) { return 0; }

		// Updates
		virtual void Update(double FrameTime) { }
		virtual void Render(double BlendFactor) { }

		virtual void HandleAction(int Action) { }

		// States
		int GetState() const { return State; }

		_Stats *Stats;

	protected:

		void GetFighterList(int Side, std::vector<_Object *> &SideFighters);
		void GetAliveFighterList(int Side, std::vector<_Object *> &AliveFighters);
		void GetMonsterList(std::vector<_Object *> &Monsters);
		void GetPlayerList(int Side, std::vector<_Object *> &Players);
		int GetFighterFromSlot(int Slot);

		// State
		int State, TargetState;
		double Timer;

		// Objects
		std::vector<_Object *> Fighters;
		int LeftFighterCount, RightFighterCount;
		int PlayerCount, MonsterCount;
};
