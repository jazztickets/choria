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
#include <ae/input.h>
#include <ae/state.h>
#include <framework.h>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

_Input Input;

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
const std::string &_Input::GetMouseButtonName(uint32_t Button) {
	if(Button >= sizeof(MouseButtonNames))
		return MouseButtonNames[0];

	return MouseButtonNames[Button];
}

// Returns true if a mod key is down
bool _Input::ModKeyDown(int Key) {
	return SDL_GetModState() & Key;
}

// Returns true if a mouse button is down
bool _Input::MouseDown(uint32_t Button) {
	return MouseState & SDL_BUTTON(Button);
}
