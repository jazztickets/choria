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
#include <objects/grid.h>
#include <ae/assets.h>
#include <ae/graphics.h>
#include <ae/bounds.h>
#include <ae/font.h>
#include <glm/common.hpp>
#include <unordered_map>

// Constructor
_Grid::_Grid(const glm::ivec2 &Size, const glm::vec2 &Offset) :
	Size(Size),
	Offset(Offset),
	Tiles(nullptr) {

	// Allocate memory
	Tiles = new _GridTile*[Size.x];

	for(int i = 0; i < Size.x; i++)
		Tiles[i] = new _GridTile[Size.y];
}

// Destructor
_Grid::~_Grid() {

	// Delete tile data
	if(Tiles) {
		for(int i = 0; i < Size.x; i++)
			delete[] Tiles[i];
		delete[] Tiles;
	}
}

// Adds an object to the collision grid
void _Grid::AddObject(const void *Object, const glm::vec2 &ObjectPosition, const glm::vec2 &ObjectHalfWidths) {

	// Get the object's bounding rectangle in world space
	ae::_Bounds Bounds;
	GetTileBounds(ObjectPosition + Offset, ObjectHalfWidths, Bounds);

	// Add object to tiles' lists
	for(int i = Bounds.Start.x; i <= Bounds.End.x; i++) {
		for(int j = Bounds.Start.y; j <= Bounds.End.y; j++) {
			Tiles[i][j].Objects.push_front(Object);
		}
	}
}

// Render the grid for debugging
void _Grid::Render() {
	for(int i = 0; i < Size.x; i++) {
		for(int j = 0; j < Size.y; j++) {
			ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
			ae::Graphics.SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.1f));
			ae::Graphics.DrawRectangle3D(glm::vec2(i - Offset.x, j - Offset.y), glm::vec2(i - Offset.x + 1, j - Offset.y + 1), false);

			ae::Assets.Fonts["hud_medium"]->DrawText(std::to_string(Tiles[i][j].Objects.size()), glm::vec2(i - Offset.x + 0.5, j - Offset.y + 0.5), ae::CENTER_MIDDLE, glm::vec4(1.0f), 1.0f / 64.0f);
		}
	}
}

// Returns the tile range that an object touches
void _Grid::GetTileBounds(const glm::vec2 &ObjectPosition, const glm::vec2 &ObjectHalfWidths, ae::_Bounds &Bounds) const {

	// Shape is AABB
	if(ObjectHalfWidths.y != 0.0f) {
		Bounds.Start.x = glm::clamp((int)(ObjectPosition.x - ObjectHalfWidths.x), 0, Size.x - 1);
		Bounds.Start.y = glm::clamp((int)(ObjectPosition.y - ObjectHalfWidths.y), 0, Size.y - 1);
		Bounds.End.x = glm::clamp((int)(ObjectPosition.x + ObjectHalfWidths.x), 0, Size.x - 1);
		Bounds.End.y = glm::clamp((int)(ObjectPosition.y + ObjectHalfWidths.y), 0, Size.y - 1);
	}
	else {
		Bounds.Start.x = glm::clamp((int)(ObjectPosition.x - ObjectHalfWidths.x), 0, Size.x - 1);
		Bounds.Start.y = glm::clamp((int)(ObjectPosition.y - ObjectHalfWidths.x), 0, Size.y - 1);
		Bounds.End.x = glm::clamp((int)(ObjectPosition.x + ObjectHalfWidths.x), 0, Size.x - 1);
		Bounds.End.y = glm::clamp((int)(ObjectPosition.y + ObjectHalfWidths.x), 0, Size.y - 1);
	}
}

// Get a list of pontential objects that an object could collide with
void _Grid::GetObjectList(const glm::vec2 &ObjectPosition, const glm::vec2 &ObjectHalfWidths, std::list<const void *> &PotentialObjects) {

	// Get the object's bounding rectangle in world space
	ae::_Bounds Bounds;
	GetTileBounds(ObjectPosition + Offset, ObjectHalfWidths, Bounds);

	// Check tiles for objects
	std::unordered_map<const void *, int> ObjectMap;
	for(int i = Bounds.Start.x; i <= Bounds.End.x; i++) {
		for(int j = Bounds.Start.y; j <= Bounds.End.y; j++) {
			for(const auto &Object : Tiles[i][j].Objects) {
				if(ObjectMap.find(Object) == ObjectMap.end()) {
					PotentialObjects.push_front(Object);
					ObjectMap[Object] = 1;
				}
			}
		}
	}
}
