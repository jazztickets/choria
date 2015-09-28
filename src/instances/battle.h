/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#pragma once

// Libraries
#include <irrlicht.h>
#include <vector>
#include <cstdint>

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Forward Declarations
class FighterClass;
class PlayerClass;
class MonsterClass;
class _Packet;

// Structures
struct FighterResultStruct {
	FighterResultStruct() : Fighter(NULL), SkillID(-1), Target(-1), DamageDealt(0), HealthChange(0), ManaChange(0) { }
	FighterClass *Fighter;
	int SkillID;
	int Target;
	int DamageDealt;
	int HealthChange;
	int ManaChange;
};

struct BattleResultStruct {
	BattleResultStruct() : FighterCount(0), PlayerCount(0), MonsterCount(0), ExperienceGiven(0), GoldGiven(0), Dead(true) { }
	int FighterCount;
	int PlayerCount;
	int MonsterCount;
	int ExperienceGiven;
	int GoldGiven;
	bool Dead;
};

// Classes
class BattleClass {

	public:

		BattleClass();
		virtual ~BattleClass();

		// Objects
		void AddFighter(FighterClass *TFighter, int TSide);

		// Updates
		virtual void Update(uint32_t TDeltaTime) { }

		// States
		int GetState() const { return State; }

	protected:

		void GetFighterList(int TSide, std::vector<FighterClass *> &TFighters);
		void GetAliveFighterList(int TSide, std::vector<FighterClass *> &TFighters);
		void GetMonsterList(std::vector<MonsterClass *> &TMonsters);
		void GetPlayerList(int TSide, std::vector<PlayerClass *> &TPlayers);
		int GetFighterFromSlot(int TSlot);

		// State
		int State, TargetState;
		uint32_t Timer;

		// Objects
		std::vector<FighterClass *> Fighters;
		int LeftFighterCount, RightFighterCount;
		int PlayerCount, MonsterCount;
};
