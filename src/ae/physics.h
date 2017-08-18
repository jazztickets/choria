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
#include <ae/type.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

// Physics shape
class _Shape {

	public:

		// AABB
		bool IsAABB() { return HalfWidth[1] != 0.0f; }
		glm::vec4 GetAABB(const glm::vec2 &Position);

		// Properties
		glm::vec2 HalfWidth;
};

// Physics response
struct _Manifold {
	_Manifold() : ObjectA(nullptr), ObjectB(nullptr), Penetration(0.0f) { }
	bool IsDiagonal() const { return Normal.x != 0 && Normal.y != 0; }

	void *ObjectA;
	void *ObjectB;
	glm::vec2 Normal;
	float Penetration;
};

// Rigid body
class _RigidBody {

	public:

		_RigidBody();
		_RigidBody(const glm::vec2 &Position, const glm::vec2 &Velocity, const glm::vec2 &Acceleration);

		// Update
		void Update(float DeltaTime);
		void ForcePosition(const glm::vec2 &Position) { this->Position = this->LastPosition = Position; }
		void SetMass(float Mass) { InverseMass = Mass > 0.0f ? 1.0f / Mass : 0.0f; }

		// State
		glm::vec2 LastPosition;
		glm::vec2 Position;
		glm::vec2 Velocity;
		glm::vec2 Acceleration;
		float InverseMass;
		float Restitution;
		int CollisionMask;
		int CollisionGroup;
		bool CollisionResponse;

	private:

		void RungeKutta4Evaluate(const _RigidBody &Derivative, float DeltaTime, _RigidBody &Output);

};
