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
#include <string>
#include <glm/vec2.hpp>

struct _KeyEvent {
	_KeyEvent(const char *Text, int Scancode, bool Pressed, bool Repeat) : Text(Text), Scancode(Scancode), Pressed(Pressed), Repeat(Repeat) { }
	const char *Text;
	int Scancode;
	bool Pressed;
	bool Repeat;
};

struct _MouseEvent {
	_MouseEvent(const glm::ivec2 &Position, int Button, bool Pressed) : Position(Position), Button(Button), Pressed(Pressed) { }
	glm::ivec2 Position;
	int Button;
	bool Pressed;
};

// Classes
class _Input {

	public:

		enum InputType {
			KEYBOARD,
			MOUSE_BUTTON,
			MOUSE_AXIS,
			JOYSTICK_BUTTON,
			JOYSTICK_AXIS,
			INPUT_COUNT,
		};

		_Input() : KeyState(nullptr), MouseState(0), Mouse(0) { }

		// Update
		void Update(double FrameTime);

		// State
		int KeyDown(int Key) { return KeyState[Key]; }
		bool ModKeyDown(int Key);
		bool MouseDown(uint32_t Button);
		const glm::ivec2 &GetMouse() { return Mouse; }
		static const char *GetKeyName(int Key);
		static const std::string &GetMouseButtonName(uint32_t Button);

	private:

		// States
		const uint8_t *KeyState;
		uint32_t MouseState;
		glm::ivec2 Mouse;
};

extern _Input Input;
