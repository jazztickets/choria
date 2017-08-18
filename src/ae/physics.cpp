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
#include <ae/physics.h>

// Constructor
_RigidBody::_RigidBody() :
	InverseMass(0.0f),
	Restitution(1.0f),
	CollisionMask(0),
	CollisionGroup(0),
	CollisionResponse(true) {

}

// Constructor
_RigidBody::_RigidBody(const glm::vec2 &Position, const glm::vec2 &Velocity, const glm::vec2 &Acceleration) :
	LastPosition(Position),
	Position(Position),
	Velocity(Velocity),
	Acceleration(Acceleration) {
}

// Integrate
void _RigidBody::Update(float DeltaTime) {
	if(InverseMass <= 0.0f)
		return;

	// RK4 increments
	_RigidBody A, B, C, D;
	RungeKutta4Evaluate(_RigidBody(glm::vec2(0, 0), glm::vec2(0, 0), glm::vec2(0, 0)), 0.0f, A);
	RungeKutta4Evaluate(A, DeltaTime * 0.5f, B);
	RungeKutta4Evaluate(B, DeltaTime * 0.5f, C);
	RungeKutta4Evaluate(C, DeltaTime, D);

	// Calculate weighted sum
	glm::vec2 PositionChange = (A.Position + (B.Position + C.Position) * 2.0f + D.Position) * (1.0f / 6.0f);
	glm::vec2 VelocityChange = (A.Velocity + (B.Velocity + C.Velocity) * 2.0f + D.Velocity) * (1.0f / 6.0f);

	// Update physics state
	LastPosition = Position;
	Position = Position + PositionChange * DeltaTime;
	Velocity = Velocity + VelocityChange * DeltaTime;
}

// Evaluate increments
void _RigidBody::RungeKutta4Evaluate(const _RigidBody &Derivative, float DeltaTime, _RigidBody &Output) {

	_RigidBody NewState;
	NewState.Position = Position + Derivative.Position * DeltaTime;
	NewState.Velocity = Velocity + Derivative.Velocity * DeltaTime;

	// Set derivative
	Output.Position = NewState.Velocity;
	Output.Velocity = Acceleration;
}

// Get AABB of shape from position
glm::vec4 _Shape::GetAABB(const glm::vec2 &Position) {

	if(IsAABB()) {
		return glm::vec4(
			Position.x - HalfWidth.x,
			Position.y - HalfWidth.y,
			Position.x + HalfWidth.x,
			Position.y + HalfWidth.y
		);
	}
	else {
		return glm::vec4(
			Position.x - HalfWidth.x,
			Position.y - HalfWidth.x,
			Position.x + HalfWidth.x,
			Position.y + HalfWidth.x
		);
	}
}
