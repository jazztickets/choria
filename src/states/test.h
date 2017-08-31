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

#include <ae/state.h>
#include <ae/log.h>
#include <ae/manager.h>
#include <unordered_map>
#include <glm/vec4.hpp>

// Forward Declarations
class _Camera;
class _Stats;
class _Minigame;

// Test state
class _TestState : public _State {

	public:

		// Setup
		_TestState();
		void Init() override;
		void Close() override;

		// Input
		void HandleKey(const _KeyEvent &KeyEvent) override;
		void HandleMouseButton(const _MouseEvent &MouseEvent) override;
		void HandleMouseMove(const glm::ivec2 &Position) override;
		void HandleWindow(uint8_t Event) override;
		void HandleQuit() override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		// Attributes
		_Camera *Camera;
		const _Stats *Stats;
		_Minigame *Minigame;
		double Time;

	protected:

};

extern _TestState TestState;
