/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020 Alan Witkowski
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
#include <objects/action.h>
#include <cstdint>
#include <list>
#include <glm/vec2.hpp>

// Forward Declarations
class _Object;

namespace ae {
	class _Element;
}

// Classes
class _Fighter {

	public:

		_Fighter(_Object *Object);

		// Battle
		int GetStartingSide(const _Item *Item);

		// UI
		void CreateBattleElement(ae::_Element *Parent);
		void RemoveBattleElement();

		// Base
		_Object *Object;

		// Render
		ae::_Element *BattleElement;
		glm::vec2 BattleBaseOffset;
		glm::vec2 ResultPosition;
		glm::vec2 StatPosition;

		// Targets
		_Action PotentialAction;
		_Object *LastTarget[2];

		// State
		std::vector<uint32_t> ItemDropsReceived;
		double TurnTimer;
		int64_t GoldStolen;
		int Corpse;
		int TargetIndex;
		bool JoinedBattle;
		bool FleeBattle;
		uint8_t BattleSide;

	private:

};
