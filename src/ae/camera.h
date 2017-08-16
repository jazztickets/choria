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
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

// Camera class
class _Camera {

	public:

		_Camera(const glm::vec3 &Position, float UpdateDivisor, float Fovy, float Near, float Far);
		~_Camera() { }

		// Updates
		void CalculateFrustum(float AspectRatio);
		void Set3DProjection(double BlendFactor);
		void ConvertScreenToWorld(const glm::ivec2 &Point, glm::vec2 &WorldPosition);
		void ConvertWorldToScreen(const glm::vec2 &WorldPosition, glm::ivec2 &Point);

		void Update(double FrameTime);
		void UpdatePosition(const glm::vec2 &UpdatePosition) { this->TargetPosition += glm::vec3(UpdatePosition, 0.0f); }
		void UpdateDistance(float Update) { this->TargetPosition.z += Update; }

		void ForcePosition(const glm::vec3 &Position) { this->TargetPosition = this->LastPosition = this->Position = Position; }
		void Set2DPosition(const glm::vec2 &Position) { this->TargetPosition = glm::vec3(Position.x, Position.y, this->Position.z); }
		void SetDistance(float Distance) { this->TargetPosition.z = Distance; }

		void GetDrawPosition(double BlendFactor, glm::vec3 &DrawPosition);
		const glm::vec3 &GetPosition() const { return Position; }
		const glm::vec4 &GetAABB() const { return AABB; }
		glm::mat4 Transform;

	private:

		glm::mat4 Projection;

		glm::vec3 LastPosition, Position, TargetPosition;
		float UpdateDivisor;

		glm::vec2 Frustum;
		float Fovy;
		float Near;
		float Far;

		glm::vec4 AABB;
};
