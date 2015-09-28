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
#include <objects/monster.h>
#include <engine/globals.h>
#include <engine/graphics.h>
#include <engine/random.h>
#include <engine/stats.h>

// Constructor
MonsterClass::MonsterClass(int TMonsterID)
:	FighterClass(TYPE_MONSTER),
	ID(TMonsterID) {

	Stats::Instance().GetMonsterStats(TMonsterID, this);
}

// Destructor
MonsterClass::~MonsterClass() {

}

// Updates the monster
void MonsterClass::Update() {

}

// Updates the monster's target based on AI
void MonsterClass::UpdateTarget(const array<FighterClass *> &TFighters) {

	// Get count of fighters
	int Count = TFighters.size();

	// Get a random index
	int RandomIndex = Random::Instance().GenerateRange(0, Count-1);

	Target = TFighters[RandomIndex]->GetSlot();
}

// Returns the monsters command
int MonsterClass::GetCommand() {

	return 0;
}
