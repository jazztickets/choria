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
#include <ae/physics.h>
#include <ae/baseobject.h>
#include <string>

// Forward Declarations
namespace ae {
	class _Texture;
}

// Sprite
class _Sprite : public ae::_BaseObject {

	public:

		_Sprite();

		// Update
		void Update(double FrameTime);
		void Render(double BlendFactor);

		// Collision
		bool CheckCircle(const glm::vec2 &Position, float Radius, glm::vec2 &Normal, float &Penetration, bool &AxisAlignedPush);

		// Attributes
		std::string Name;
		const ae::_Texture *Texture;
		ae::_RigidBody RigidBody;
		ae::_Shape Shape;
		glm::vec2 Scale;
		bool Visible;
		bool Touching;
		bool LastTouching;

	private:

};
