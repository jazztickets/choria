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
#include <objects/fighter.h>

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Constants
const int MONSTER_MAXACTIONS = 8;

// Classes
class MonsterClass : public FighterClass {
	friend class StatsClass;

	public:

		MonsterClass(int TMonsterID);
		~MonsterClass();

		void Update();
		void UpdateTarget(const array<FighterClass *> &TFighters);

		// Stats
		int GetID() const { return ID; }
		int GetExperienceGiven() const { return ExperienceGiven; }
		int GetGoldGiven() const { return GoldGiven; }
		int GetCommand();

	private:

		// Objects
		array<FighterClass *> Opponents;
		int ID;

		// Stats
		int ExperienceGiven, GoldGiven;
		int AI;
};

