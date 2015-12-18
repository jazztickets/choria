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
#include <glm/vec2.hpp>
#include <manager.h>

// Forward Declarations
class _Object;
class _Battle;
class _Buffer;

// Types of stats
enum StatType : int {
	HEALTH      = 0x01,
	MANA        = 0x02,
	EXPERIENCE  = 0x04,
	GOLD        = 0x08,
};

// Stat changes
class _StatChange {

	public:

		_StatChange();

		void Serialize(_Buffer &Data);
		void Unserialize(_Buffer &Data, _Manager<_Object> *Manager);
		void Render(double BlendFactor);

		bool IsChanged();
		int GetChangedFlag();

		_Object *Object;
		glm::vec2 LastPosition;
		glm::vec2 Position;
		float Direction;
		double Time;
		double TimeOut;
		int HealthChange;
		int ManaChange;
		int Experience;
		int Gold;

};
