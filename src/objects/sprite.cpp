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
#include <objects/sprite.h>
#include <ae/graphics.h>
#include <glm/gtx/norm.hpp>
#include <iostream>

// Constructor
_Sprite::_Sprite() :
	Texture(nullptr),
	Scale(1.0f),
	Visible(true) {

}

// Update
void _Sprite::Update(double FrameTime) {
	RigidBody.Update((float)FrameTime);
}

// Render
void _Sprite::Render(double BlendFactor) {
	if(!Visible)
		return;

	glm::vec3 RenderPosition(
		RigidBody.Position.x * BlendFactor + RigidBody.LastPosition.x * (1.0f - BlendFactor),
		RigidBody.Position.y * BlendFactor + RigidBody.LastPosition.y * (1.0f - BlendFactor),
		0.0f
	);

	if(Texture) {
		Graphics.SetVBO(VBO_QUAD);
		Graphics.DrawSprite(RenderPosition, Texture, 0.0f, Scale);
	}
}

// Check collision with a circle
bool _Sprite::CheckCircle(const glm::vec2 &Position, float Radius, glm::vec2 &Normal, float &Penetration, bool &AxisAlignedPush) {

	// Get vector to circle center
	glm::vec2 Point = Position - glm::vec2(RigidBody.Position);

	// Shape is AABB
	if(Shape.IsAABB()) {

		glm::vec2 ClosestPoint = Point;
		int ClampCount = 0;
		if(ClosestPoint.x < -Shape.HalfWidth[0]) {
			ClosestPoint.x = -Shape.HalfWidth[0];
			ClampCount++;
		}
		if(ClosestPoint.y < -Shape.HalfWidth[1]) {
			ClosestPoint.y = -Shape.HalfWidth[1];
			ClampCount++;
		}
		if(ClosestPoint.x > Shape.HalfWidth[0]) {
			ClosestPoint.x = Shape.HalfWidth[0];
			ClampCount++;
		}
		if(ClosestPoint.y > Shape.HalfWidth[1]) {
			ClosestPoint.y = Shape.HalfWidth[1];
			ClampCount++;
		}

		bool Hit = glm::distance2(Point, ClosestPoint) < Radius * Radius;
		if(Hit) {

			// Get push direction
			Normal = Point - ClosestPoint;

			// Check for zero vector
			if(Normal.x == 0.0f && Normal.y == 0.0f) {
				Normal.x = 1.0f;
				return true;
			}

			// Get penetration amount
			float NormalLength = glm::length(Normal);
			Penetration = Radius - NormalLength;

			// Normalize
			Normal /= NormalLength;

			if(ClampCount == 1)
				AxisAlignedPush = true;

			return true;
		}
	}
	else {

		float SquareDistance = Point.x * Point.x + Point.y * Point.y;
		float RadiiSum = Radius + Shape.HalfWidth[0];

		bool Hit = SquareDistance < RadiiSum * RadiiSum;
		if(Hit) {

			// Check for zero vector
			if(Point.x == 0.0f && Point.y == 0.0f) {
				Normal.x = 1.0f;
				Penetration = 1.0f;
				return true;
			}

			// Get penetration amount
			Normal = Point;
			float NormalLength = glm::length(Normal);
			Penetration = RadiiSum - NormalLength;

			// Normalize
			Normal /= NormalLength;

			return true;
		}
	}

	return false;
}
