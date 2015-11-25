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
#include <instances/battle.h>
#include <objects/object.h>

// Constructor
_Battle::_Battle() {
	Timer = 0;
	LeftFighterCount = 0;
	RightFighterCount = 0;
	PlayerCount = 0;
	MonsterCount = 0;
}

// Destructor
_Battle::~_Battle() {

	// Delete monsters
	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->Type == _Object::MONSTER)
			delete Fighters[i];
	}
}

// Add a fighter to the battle
void _Battle::AddFighter(_Object *Fighter, int Side) {

	// Count fighters and set slots
	if(Side == 0) {
		Fighter->BattleSlot = Side + LeftFighterCount * 2;
		LeftFighterCount++;
	}
	else {
		Fighter->BattleSlot = Side + RightFighterCount * 2;
		RightFighterCount++;
	}

	if(Fighter->Type == _Object::PLAYER)
		PlayerCount++;
	else
		MonsterCount++;

	Fighters.push_back(Fighter);
}

// Get a list of fighters from a side
void _Battle::GetFighterList(int Side, std::vector<_Object *> &SideFighters) {

	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == Side) {
			SideFighters.push_back(Fighters[i]);
		}
	}
}

// Get a list of alive fighters from a side
void _Battle::GetAliveFighterList(int Side, std::vector<_Object *> &AliveFighters) {

	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == Side && Fighters[i]->Health > 0) {
			AliveFighters.push_back(Fighters[i]);
		}
	}
}

// Get a list of monster from the right side
void _Battle::GetMonsterList(std::vector<_Object *> &Monsters) {

	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == 1 && Fighters[i]->Type == _Object::MONSTER) {
			Monsters.push_back((_Object *)Fighters[i]);
		}
	}
}

// Get a list of players from a side
void _Battle::GetPlayerList(int Side, std::vector<_Object *> &Players) {

	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == Side && Fighters[i]->Type == _Object::PLAYER) {
			Players.push_back((_Object *)Fighters[i]);
		}
	}
}

// Gets a fighter index from a slot number
int _Battle::GetFighterFromSlot(int Slot) {
	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->BattleSlot == Slot) {
			return i;
		}
	}

	return -1;
}
