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
#include "battle.h"
#include "../objects/fighter.h"
#include "../objects/monster.h"
#include "../objects/player.h"
#include "../engine/constants.h"

// Constructor
BattleClass::BattleClass() {
	Timer = 0;
	LeftFighterCount = 0;
	RightFighterCount = 0;
	PlayerCount = 0;
	MonsterCount = 0;
}

// Destructor
BattleClass::~BattleClass() {

	// Delete monsters
	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetType() == FighterClass::TYPE_MONSTER)
			delete Fighters[i];
	}
}

// Add a fighter to the battle
void BattleClass::AddFighter(FighterClass *Fighter, int Side) {
	
	// Count fighters and set slots
	if(Side == 0) {
		Fighter->SetSlot(Side + LeftFighterCount * 2);
		LeftFighterCount++;
	}
	else {
		Fighter->SetSlot(Side + RightFighterCount * 2);
		RightFighterCount++;
	}

	if(Fighter->GetType() == FighterClass::TYPE_PLAYER)
		PlayerCount++;
	else
		MonsterCount++;

	Fighter->SetBattle(this);
	Fighters.push_back(Fighter);	
}

// Get a list of fighters from a side
void BattleClass::GetFighterList(int Side, array<FighterClass *> &SideFighters) {

	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == Side) {
			SideFighters.push_back(Fighters[i]);
		}
	}
}

// Get a list of alive fighters from a side
void BattleClass::GetAliveFighterList(int Side, array<FighterClass *> &AliveFighters) {

	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == Side && Fighters[i]->GetHealth() > 0) {
			AliveFighters.push_back(Fighters[i]);
		}
	}
}

// Get a list of monster from the right side
void BattleClass::GetMonsterList(array<MonsterClass *> &Monsters) {

	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == 1 && Fighters[i]->GetType() == FighterClass::TYPE_MONSTER) {
			Monsters.push_back(static_cast<MonsterClass *>(Fighters[i]));
		}
	}
}

// Get a list of players from a side
void BattleClass::GetPlayerList(int Side, array<PlayerClass *> &Players) {

	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == Side && Fighters[i]->GetType() == FighterClass::TYPE_PLAYER) {
			Players.push_back(static_cast<PlayerClass *>(Fighters[i]));
		}
	}
}

// Gets a fighter index from a slot number
int BattleClass::GetFighterFromSlot(int Slot) {
	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSlot() == Slot) {
			return i;
		}
	}

	return -1;
}
