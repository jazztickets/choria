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
#ifndef MONSTER_H
#define MONSTER_H

// Libraries
#include <irrlicht/irrlicht.h>
#include "fighter.h"

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

		MonsterClass(int MonsterID);
		~MonsterClass();

		// AI
		bool UpdateAI(u32 FrameTime);

		// Stats
		int GetID() const { return ID; }
		int GetExperienceGiven() const { return ExperienceGiven; }
		int GetGoldGiven() const { return GoldGiven; }

	private:
		
		// Objects
		array<FighterClass *> Opponents;
		int ID;

		// Stats
		int ExperienceGiven, GoldGiven;
		int AI;
};

#endif
