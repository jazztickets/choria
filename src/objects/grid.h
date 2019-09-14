/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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
#include <list>

// Forward Declarations
namespace ae {
	struct _Bounds;
}

// Holds data for a single tile
struct _GridTile {
	_GridTile() { }

	std::list<void *> Objects;
};

// Uniform grid class
class _Grid {

	public:

		_Grid(const glm::ivec2 &Size, const glm::vec2 &Offset);
		~_Grid();

		// Objects
		void AddObject(void *Object, const glm::vec2 &ObjectPosition, const glm::vec2 &ObjectHalfWidths);
		void RemoveObject(void *Object) { }

		// Collision
		void GetTileBounds(const glm::vec2 &ObjectPosition, const glm::vec2 &ObjectHalfWidths, ae::_Bounds &Bounds) const;
		void GetObjectList(const glm::vec2 &ObjectPosition, const glm::vec2 &ObjectHalfWidths, std::list<void *> &PotentialObjects);

		// Debug
		void Render();

	private:

		// Attributes
		glm::ivec2 Size;
		glm::vec2 Offset;
		_GridTile **Tiles;
};
