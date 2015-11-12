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
#include <objects/fighter.h>
#include <vector>

// Classes
class _Monster : public _Fighter {
	friend class _Stats;

	public:

		_Monster(int TMonsterID);
		~_Monster();

		void Update();
		void UpdateTarget(const std::vector<_Fighter *> &TFighters);

		// Stats
		int GetID() const { return ID; }
		int GetExperienceGiven() const { return ExperienceGiven; }
		int GetGoldGiven() const { return GoldGiven; }
		int GetCommand();

	private:

		// Objects
		std::vector<_Fighter *> Opponents;
		int ID;

		// Stats
		int ExperienceGiven, GoldGiven;
		int AI;
};
