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
#include "input.h"
#include "state.h"
#include "game.h"

// Initializes the system
int InputClass::Init()  {

	MousePosition.X = MousePosition.Y = 0;

	return 1;
}

// Shuts down the system
int InputClass::Close() {

	return 1;
}

// Handle events
bool InputClass::OnEvent(const SEvent &TEvent) {
	if(Game::Instance().GetManagerState() != GameClass::STATE_UPDATE)
		return false;

	bool Processed = false;

	switch(TEvent.EventType) {
		case EET_KEY_INPUT_EVENT:

			// Send key press events
			if(TEvent.KeyInput.PressedDown && !GetKeyState(TEvent.KeyInput.Key))
				Processed = Game::Instance().GetState()->HandleKeyPress(TEvent.KeyInput.Key);
			else if(!TEvent.KeyInput.PressedDown)
				Processed = Game::Instance().GetState()->HandleKeyRelease(TEvent.KeyInput.Key);

			// Set the current key state
			SetKeyState(TEvent.KeyInput.Key, TEvent.KeyInput.PressedDown);

			return Processed;
		break;
		case EET_MOUSE_INPUT_EVENT:

			switch(TEvent.MouseInput.Event) {
				case EMIE_LMOUSE_PRESSED_DOWN:
				case EMIE_RMOUSE_PRESSED_DOWN:
				case EMIE_MMOUSE_PRESSED_DOWN:
					SetMouseState(TEvent.MouseInput.Event, true);
					return Game::Instance().GetState()->HandleMousePress(TEvent.MouseInput.Event, TEvent.MouseInput.X, TEvent.MouseInput.Y);
				break;
				case EMIE_LMOUSE_LEFT_UP:
				case EMIE_RMOUSE_LEFT_UP:
				case EMIE_MMOUSE_LEFT_UP:
					SetMouseState(TEvent.MouseInput.Event - MOUSE_COUNT, false);
					Game::Instance().GetState()->HandleMouseRelease(TEvent.MouseInput.Event - MOUSE_COUNT, TEvent.MouseInput.X, TEvent.MouseInput.Y);
				break;
				case EMIE_MOUSE_MOVED:
					MousePosition.X = TEvent.MouseInput.X;
					MousePosition.Y = TEvent.MouseInput.Y;
					Game::Instance().GetState()->HandleMouseMotion(MousePosition.X, MousePosition.Y);
				break;
				case EMIE_MOUSE_WHEEL:
					Game::Instance().GetState()->HandleMouseWheel(TEvent.MouseInput.Wheel);
				break;
				default:
				break;
			}

			return false;
		break;
		case EET_GUI_EVENT:
			Game::Instance().GetState()->HandleGUI(TEvent.GUIEvent.EventType, TEvent.GUIEvent.Caller);
		break;
		default:
		break;
	}

	return false;
}

// Resets the keyboard state
void InputClass::ResetInputState() {

	for(int i = 0; i < KEY_KEY_CODES_COUNT; i++)
		Keys[i] = 0;
}

// Converts an irrlicht key code into a string
const char *InputClass::GetKeyName(EKEY_CODE TKey) {

	switch(TKey) {
		case KEY_KEY_0:
			return "0";
		break;
		case KEY_KEY_1:
			return "1";
		break;
		case KEY_KEY_2:
			return "2";
		break;
		case KEY_KEY_3:
			return "3";
		break;
		case KEY_KEY_4:
			return "4";
		break;
		case KEY_KEY_5:
			return "5";
		break;
		case KEY_KEY_6:
			return "6";
		break;
		case KEY_KEY_7:
			return "7";
		break;
		case KEY_KEY_8:
			return "8";
		break;
		case KEY_KEY_9:
			return "9";
		break;
		case KEY_KEY_A:
			return "a";
		break;
		case KEY_KEY_B:
			return "b";
		break;
		case KEY_KEY_C:
			return "c";
		break;
		case KEY_KEY_D:
			return "d";
		break;
		case KEY_KEY_E:
			return "e";
		break;
		case KEY_KEY_F:
			return "f";
		break;
		case KEY_KEY_G:
			return "g";
		break;
		case KEY_KEY_H:
			return "h";
		break;
		case KEY_KEY_I:
			return "i";
		break;
		case KEY_KEY_J:
			return "j";
		break;
		case KEY_KEY_K:
			return "k";
		break;
		case KEY_KEY_L:
			return "l";
		break;
		case KEY_KEY_M:
			return "m";
		break;
		case KEY_KEY_N:
			return "n";
		break;
		case KEY_KEY_O:
			return "o";
		break;
		case KEY_KEY_P:
			return "p";
		break;
		case KEY_KEY_Q:
			return "q";
		break;
		case KEY_KEY_R:
			return "r";
		break;
		case KEY_KEY_S:
			return "s";
		break;
		case KEY_KEY_T:
			return "t";
		break;
		case KEY_KEY_U:
			return "u";
		break;
		case KEY_KEY_V:
			return "v";
		break;
		case KEY_KEY_W:
			return "w";
		break;
		case KEY_KEY_X:
			return "x";
		break;
		case KEY_KEY_Y:
			return "y";
		break;
		case KEY_KEY_Z:
			return "z";
		break;
		case KEY_LEFT:
			return "left";
		break;
		case KEY_UP:
			return "up";
		break;
		case KEY_RIGHT:
			return "right";
		break;
		case KEY_DOWN:
			return "down";
		break;
		case KEY_SPACE:
			return "space";
		break;
		case KEY_SHIFT:
			return "shift";
		break;
		case KEY_CONTROL:
			return "control";
		break;
		case KEY_TAB:
			return "tab";
		break;
		case KEY_RETURN:
			return "enter";
		break;
		default:
		break;
	}

	return "";
}

