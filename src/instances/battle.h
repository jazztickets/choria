/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef BATTLE_H
#define BATTLE_H

// Libraries
#include <irrlicht/irrlicht.h>

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
class PacketClass;

// Structures
struct BattleUpdateStruct {
	BattleUpdateStruct() : Giver(false), Fighter(-1), DamageDealt(0), DamageTaken(0), TurnTimer(-1), TurnTimerMax(-1) { }
	bool Giver;
	int Fighter;
	int DamageDealt, DamageTaken;
	int TurnTimer, TurnTimerMax;
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
		void AddFighter(FighterClass *Fighter, int Side);
		void GetAliveFighterList(int Side, array<FighterClass *> &AliveFighters);

		// Updates
		virtual void Update(u32 FrameTime) { }

		// States
		int GetState() const { return State; }

	protected:

		void GetFighterList(int Side, array<FighterClass *> &SideFighters);
		void GetMonsterList(array<MonsterClass *> &Monsters);
		void GetPlayerList(int Side, array<PlayerClass *> &Players);
		int GetFighterFromSlot(int Slot);

		// State
		int State, TargetState;
		u32 Timer;

		// Objects
		array<FighterClass *> Fighters;
		int LeftFighterCount, RightFighterCount;
		int PlayerCount, MonsterCount;
};

#endif
