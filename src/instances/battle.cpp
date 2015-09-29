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
#include <instances/battle.h>
#include <objects/fighter.h>
#include <objects/monster.h>
#include <objects/player.h>

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
		if(Fighters[i] && Fighters[i]->GetType() == _Fighter::TYPE_MONSTER)
			delete Fighters[i];
	}
}

// Add a fighter to the battle
void _Battle::AddFighter(_Fighter *TFighter, int TSide) {

	// Count fighters and set slots
	if(TSide == 0) {
		TFighter->SetSlot(TSide + LeftFighterCount * 2);
		LeftFighterCount++;
	}
	else {
		TFighter->SetSlot(TSide + RightFighterCount * 2);
		RightFighterCount++;
	}

	if(TFighter->GetType() == _Fighter::TYPE_PLAYER)
		PlayerCount++;
	else
		MonsterCount++;

	Fighters.push_back(TFighter);
}

// Get a list of fighters from a side
void _Battle::GetFighterList(int TSide, std::vector<_Fighter *> &TFighters) {

	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == TSide) {
			TFighters.push_back(Fighters[i]);
		}
	}
}

// Get a list of alive fighters from a side
void _Battle::GetAliveFighterList(int TSide, std::vector<_Fighter *> &TFighters) {

	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == TSide && Fighters[i]->GetHealth() > 0) {
			TFighters.push_back(Fighters[i]);
		}
	}
}

// Get a list of monster from the right side
void _Battle::GetMonsterList(std::vector<_Monster *> &TMonsters) {

	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == 1 && Fighters[i]->GetType() == _Fighter::TYPE_MONSTER) {
			TMonsters.push_back(static_cast<_Monster *>(Fighters[i]));
		}
	}
}

// Get a list of players from a side
void _Battle::GetPlayerList(int TSide, std::vector<_Player *> &TPlayers) {

	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == TSide && Fighters[i]->GetType() == _Fighter::TYPE_PLAYER) {
			TPlayers.push_back(static_cast<_Player *>(Fighters[i]));
		}
	}
}

// Gets a fighter index from a slot number
int _Battle::GetFighterFromSlot(int TSlot) {
	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSlot() == TSlot) {
			return i;
		}
	}

	return -1;
}
