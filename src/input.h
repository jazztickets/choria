/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#pragma once

// Libraries
#include <IEventReceiver.h>
#include <Keycodes.h>
#include <position2d.h>

// Classes
class _Input : public irr::IEventReceiver {

	public:

		enum MouseButtonType {
			MOUSE_LEFT,
			MOUSE_RIGHT,
			MOUSE_MIDDLE,
			MOUSE_COUNT,
		};

		int Init();
		int Close();

		bool OnEvent(const irr::SEvent &TEvent);

		void ResetInputState();
		bool GetKeyState(irr::EKEY_CODE TKey) const { return Keys[TKey]; }
		bool GetMouseState(int TButton) const { return MouseButtons[TButton]; }

		const irr::core::position2di &GetMousePosition() const { return MousePosition; }

		const char *GetKeyName(irr::EKEY_CODE TKey);

		bool IsShiftDown() { return GetKeyState(irr::KEY_SHIFT) || GetKeyState(irr::KEY_LSHIFT) || GetKeyState(irr::KEY_RSHIFT); }
		bool IsControlDown() { return GetKeyState(irr::KEY_CONTROL) || GetKeyState(irr::KEY_LCONTROL) || GetKeyState(irr::KEY_RCONTROL); }

	private:

		void SetKeyState(irr::EKEY_CODE TKey, bool TState) { Keys[TKey] = TState; }
		void SetMouseState(int TButton, bool TState) { MouseButtons[TButton] = TState; }

		// Input
		bool Keys[irr::KEY_KEY_CODES_COUNT], MouseButtons[MOUSE_COUNT];
		irr::core::position2di MousePosition;

};

extern _Input Input;
