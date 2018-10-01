/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2018  Alan Witkowski
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

// Forward Declarations
namespace ae {
	class _Camera;
}

// Benchmarking state
class _BenchmarkState : public ae::_State {

	public:

		// Setup
		_BenchmarkState();
		void Init() override;
		void Close() override;

		// Input
		bool HandleKey(const ae::_KeyEvent &KeyEvent) override;
		void HandleMouseButton(const ae::_MouseEvent &MouseEvent) override;
		void HandleMouseMove(const glm::ivec2 &Position) override;
		void HandleWindow(uint8_t Event) override;
		void HandleQuit() override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		// Attributes
		ae::_Camera *Camera;
		double Time;

	protected:

};

extern _BenchmarkState BenchmarkState;
