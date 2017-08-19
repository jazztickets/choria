/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#include <ae/manager.h>
#include <glm/vec4.hpp>
#include <random>

// Forward Declarations
class _Sprite;
class _Camera;
struct _MouseEvent;

// Base minigame class
class _Minigame {

	public:

		_Minigame(uint64_t Seed);
		~_Minigame();

		// Update
		void Update(double FrameTime);
		void Render(double BlendFactor);
		void HandleMouseButton(const _MouseEvent &MouseEvent);

		// Attributes
		glm::vec4 Boundary;
		_Manager<_Sprite> *Sprites;
		_Sprite *Ball;
		_Camera *Camera;

		// State
		bool Done;
		bool Dropped;
		std::mt19937 Random;

	private:

};
