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
#pragma once

// Libraries
#include <ae/input.h>
#include <cstdint>

// Classes
class _State {

	public:

		// Setup
		virtual void Init() { }
		virtual void Close() { }
		virtual ~_State() { }

		// Input
		virtual bool HandleAction(int InputType, size_t Action, int Value) { return false; }
		virtual void HandleKey(const _KeyEvent &KeyEvent) { }
		virtual void HandleMouseButton(const _MouseEvent &MouseEvent) { }
		virtual void HandleMouseMove(const glm::ivec2 &Position) { }
		virtual void HandleMouseWheel(int Direction) { }
		virtual void HandleWindow(uint8_t Event) { }
		virtual void HandleQuit() { }

		// Update
		virtual void Update(double FrameTime) { }
		virtual void Render(double BlendFactor) { }

	private:

};
