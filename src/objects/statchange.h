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

// Forward Declarations
class _Object;
class _Battle;
class _Buffer;

// Stat changes
class _StatChange {

	public:

		_StatChange();

		void SerializeBattle(_Buffer &Data);
		void UnserializeBattle(_Buffer &Data, _Battle *Battle);

		_Object *Object;
		glm::vec2 LastPosition;
		glm::vec2 Position;
		double Time;
		int HealthChange;
		int ManaChange;

};
