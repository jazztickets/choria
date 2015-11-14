/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2015  Alan Witkowski
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
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

struct _KeyEvent {
	_KeyEvent(int Key, bool Pressed) : Key(Key), Pressed(Pressed) { }
	int Key;
	bool Pressed;
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

		_Input();

		void Update(double FrameTime);

		int KeyDown(int Key) { return KeyState[Key]; }
		bool ModKeyDown(int Key);
		bool MouseDown(Uint32 Button);

		const glm::ivec2 &GetMouse() { return Mouse; }

		static const char *GetKeyName(int Key);
		static const std::string &GetMouseButtonName(Uint32 Button);

	private:

		// States
		const Uint8 *KeyState;
		Uint32 MouseState;
		glm::ivec2 Mouse;
};

extern _Input Input;
