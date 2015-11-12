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
#include <input.h>
#include <state.h>
#include <framework.h>

_Input Input;

//TODO remove
_OldInput OldInput;

const std::string MouseButtonNames[] = {
	"",
	"LMB",
	"MMB",
	"RMB",
	"Mouse 4",
	"Mouse 5",
	"Mouse 6",
	"Mouse 7",
	"Mouse 8",
	"Mouse 9",
	"Mouse 10",
	"Mouse 11",
	"Mouse 12",
	"Mouse 13",
	"Mouse 14",
	"Mouse 15",
	"Mouse 16",
	"Mouse 17",
	"Mouse 18",
	"Mouse 19",
	"Mouse 20",
};

// Initialize
_Input::_Input() {
}

// Update input state
void _Input::Update(double FrameTime) {

	// Update state
	KeyState = SDL_GetKeyboardState(nullptr);
	MouseState = SDL_GetMouseState(&Mouse.x, &Mouse.y);
}

// Returns the name of a key
const char *_Input::GetKeyName(int Key) {
	return SDL_GetScancodeName((SDL_Scancode)Key);
}

// Returns the name of a mouse button
const std::string &_Input::GetMouseButtonName(Uint32 Button) {
	if(Button >= sizeof(MouseButtonNames))
		return MouseButtonNames[0];

	return MouseButtonNames[Button];
}

// Returns true if a mod key is down
bool _Input::ModKeyDown(int Key) {
	return SDL_GetModState() & Key;
}

// Returns true if a mouse button is down
bool _Input::MouseDown(Uint32 Button) {
	return MouseState & SDL_BUTTON(Button);
}
