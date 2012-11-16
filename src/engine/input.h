/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef INPUT_H
#define INPUT_H

// Libraries
#include <irrlicht/irrlicht.h>

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Classes
class InputClass : public IEventReceiver {

	public:
	
		enum MouseButtonType {
			MOUSE_LEFT,
			MOUSE_RIGHT,
			MOUSE_MIDDLE,
			MOUSE_COUNT,
		};

		int Init();
		int Close();

		bool OnEvent(const SEvent &Event);

		void ResetInputState();
		bool GetKeyState(EKEY_CODE Key) const { return Keys[Key]; }
		bool GetMouseState(int Button) const { return MouseButtons[Button]; }

		const position2di &GetMousePosition() const { return MousePosition; }

		const char *GetKeyName(EKEY_CODE Key);

		bool IsShiftDown() { return GetKeyState(KEY_SHIFT) || GetKeyState(KEY_LSHIFT) || GetKeyState(KEY_RSHIFT); }
		bool IsControlDown() { return GetKeyState(KEY_CONTROL) || GetKeyState(KEY_LCONTROL) || GetKeyState(KEY_RCONTROL); }

	private:

		void SetKeyState(EKEY_CODE Key, bool State) { Keys[Key] = State; }
		void SetMouseState(int Button, bool State) { MouseButtons[Button] = State; }

		// Input
		bool Keys[KEY_KEY_CODES_COUNT], MouseButtons[MOUSE_COUNT];
		position2di MousePosition;

};

// Singletons
extern InputClass Input;

#endif
