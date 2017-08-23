/******************************************************************************
* Copyright (c) 2017 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
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
