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
#include <ae/camera.h>
#include <ae/graphics.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <cmath>

// Initialize
_Camera::_Camera(const glm::vec3 &Position, float UpdateDivisor, float Fovy, float Near, float Far)
:	LastPosition(Position),
	Position(Position),
	TargetPosition(Position),
	UpdateDivisor(UpdateDivisor),
	Fovy(Fovy),
	Near(Near),
	Far(Far) {
}

// Calculate the frustum
void _Camera::CalculateFrustum(float AspectRatio) {
	Frustum.y = (float)std::tan(Fovy / 360 * glm::pi<float>()) * Near;
	Frustum.x = Frustum.y * AspectRatio;
	Projection = glm::frustum(-Frustum.x, Frustum.x, Frustum.y, -Frustum.y, Near, Far);
}

// Set up 3d projection matrix
void _Camera::Set3DProjection(double BlendFactor) {
	glm::vec3 DrawPosition;
	GetDrawPosition(BlendFactor, DrawPosition);

	float Width = DrawPosition.z * Graphics.AspectRatio;
	float Height = DrawPosition.z;

	// Get AABB at z=0
	AABB[0] = -Width + DrawPosition.x;
	AABB[1] = -Height + DrawPosition.y;
	AABB[2] = Width + DrawPosition.x;
	AABB[3] = Height + DrawPosition.y;

	Transform = Projection * glm::translate(glm::mat4(1.0f), -DrawPosition);
}

// Converts screen space to world space
void _Camera::ConvertScreenToWorld(const glm::ivec2 &Point, glm::vec2 &WorldPosition) {
	WorldPosition.x = (Point.x / (float)(Graphics.ViewportSize.x) - 0.5f) * Position.z * Graphics.AspectRatio * 2 + Position.x;
	WorldPosition.y = (Point.y / (float)(Graphics.ViewportSize.y) - 0.5f) * Position.z * 2 + Position.y;
}

// Converts world space to screen space
void _Camera::ConvertWorldToScreen(const glm::vec2 &WorldPosition, glm::vec2 &Point) {
	Point.x = (int)(Graphics.ViewportSize.x * (0.5f + ((WorldPosition.x - Position.x) / (Position.z * Graphics.AspectRatio * 2))));
	Point.y = (int)(Graphics.ViewportSize.y * (0.5f + ((WorldPosition.y - Position.y) / (Position.z * 2))));
}

// Update camera
void _Camera::Update(double FrameTime) {
	LastPosition = Position;

	// Cap distance
	if(TargetPosition.z <= 1.0f)
		TargetPosition.z = 1.0f;
	else if(TargetPosition.z >= Far)
		TargetPosition.z = Far;

	// Update position
	glm::vec2 Delta(TargetPosition - Position);
	if(std::abs(Delta.x) > 0.01f)
		Position.x += Delta.x / UpdateDivisor;
	else
		Position.x = TargetPosition.x;

	if(std::abs(Delta.y) > 0.01f)
		Position.y += Delta.y / UpdateDivisor;
	else
		Position.y = TargetPosition.y;

	// Update distance
	float DeltaZ = TargetPosition.z - Position.z;
	if(std::abs(DeltaZ) > 0.01f)
		Position.z += DeltaZ / UpdateDivisor;
	else
		Position.z = TargetPosition.z;
}

// Interpolate between position and last position
void _Camera::GetDrawPosition(double BlendFactor, glm::vec3 &DrawPosition) {
	DrawPosition = Position * (float)BlendFactor + LastPosition * (1.0f - (float)BlendFactor);
}
