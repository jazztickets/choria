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

#include <ae/state.h>
#include <ae/opengl.h>

// Forward Declarations
namespace ae {
	class _Camera;
	class _Atlas;
}

// Benchmarking state
class _BenchmarkState : public ae::_State {

	public:

		enum VertexBufferType {
			VBO_STATIC,
			VBO_DYNAMIC,
			VBO_ATLAS,
			VBO_ATLAS_DYNAMIC,
			VBO_COUNT,
		};

		enum StageType {
			STAGE_COLOR_STATIC,
			STAGE_COLOR_DYNAMIC,
			STAGE_ATLAS_STATIC,
			STAGE_ATLAS_DYNAMIC,
			STAGE_COUNT,
		};

		// Setup
		_BenchmarkState();
		void Init() override;
		void Close() override;

		// Input
		bool HandleKey(const ae::_KeyEvent &KeyEvent) override;
		void HandleWindow(uint8_t Event) override;
		void HandleQuit() override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		// Attributes
		ae::_Camera *Camera;
		double Time;

	protected:

		GLuint VertexBuffer[VBO_COUNT];
		const ae::_Atlas *Atlas;
		int Stage;
		int Frames;
};

extern _BenchmarkState BenchmarkState;
