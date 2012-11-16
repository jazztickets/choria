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
#include "monster.h"
#include "../engine/globals.h"
#include "../engine/graphics.h"
#include "../engine/random.h"
#include "../engine/stats.h"
#include "../instances/battle.h"

// Constructor
MonsterClass::MonsterClass(int MonsterID)
:	FighterClass(TYPE_MONSTER),
	ID(MonsterID) {

	Stats.GetMonsterStats(MonsterID, this);
}

// Destructor
MonsterClass::~MonsterClass() {

}

// Updates the monster's AI
bool MonsterClass::UpdateAI(u32 FrameTime) {

	// Already has an action
	if(BattleAction != NULL)
		return false;

	// Get timer percent
	float TurnPercent = 1.0f;
	if(TurnTimerMax > 0)
		TurnPercent = (float)TurnTimer / TurnTimerMax;

	// Pick an action based on behavior
	if(TurnPercent > 0.5f) {
		BattleAction = Stats.GetSkill(1);

		// Pick a player to target
		array<FighterClass *> Players;
		Battle->GetAliveFighterList(0, Players);

		// Random target
		int RandomIndex = Random.GenerateRange(0, (int)(Players.size()) - 1);

		// Get slot
		Target = Players[RandomIndex]->GetSlot();

		return true;
	}

	return false;
}
