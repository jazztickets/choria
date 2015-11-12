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
#include <input.h>
#include <list>
#include <string>
#include <SDL_scancode.h>

// Constants
const int ACTIONS_MAXINPUTS = SDL_NUM_SCANCODES;

// Actions class
class _Actions {

	public:

		enum Types {
			UP,
			DOWN,
			LEFT,
			RIGHT,
			FIRE,
			AIM,
			USE,
			INVENTORY,
			RELOAD,
			WEAPONSWITCH,
			MEDKIT,
			COUNT,
		};

		_Actions();

		void ResetState() { State = 0; }
		void LoadActionNames();
		void ClearMappings(int InputType);
		void ClearMappingsForAction(int InputType, int Action);
		void ClearAllMappingsForAction(int Action);

		// Actions
		int GetState(int Action);
		int GetState() { return State; }
		const std::string &GetName(int Action) { return Names[Action]; }

		// Maps
		void AddInputMap(int InputType, int Input, int Action, bool IfNone=true);
		int GetInputForAction(int InputType, int Action);
		std::string GetInputNameForAction(int Action);

		// Handlers
		void InputEvent(int InputType, int Input, int Value);

	private:

		// Input bindings
		std::list<int> InputMap[_Input::INPUT_COUNT][ACTIONS_MAXINPUTS];

		// State of each action
		int State;

		// Nice names for each action
		std::string Names[COUNT];
};

extern _Actions Actions;
