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
#include <ae/input.h>
#include <SDL_scancode.h>
#include <list>
#include <string>
#include <vector>

// Constants
const int ACTIONS_MAXINPUTS = SDL_NUM_SCANCODES;

// Forward Declarations
class _State;

// Structure for an input binding
struct _ActionMap {
	_ActionMap(size_t Action, float Scale, float DeadZone) : Action(Action), DeadZone(DeadZone), Scale(Scale) { }

	size_t Action;
	float DeadZone;
	float Scale;
};

// State of an action
struct _ActionState {
	std::string Name;
	float Value;
	int Source;
};

// Actions class
class _Actions {

	public:

		// State
		void ResetState();
		void Serialize(std::ofstream &File, int InputType);

		// Mappping
		void ClearMappings(int InputType);
		void ClearMappingsForAction(int InputType, size_t Action);
		void ClearAllMappingsForAction(size_t Action);
		void AddInputMap(int InputType, int Input, size_t Action, float Scale=1.0f, float DeadZone=-1.0f, bool IfNone=true);
		int GetInputForAction(int InputType, size_t Action);
		std::string GetInputNameForAction(size_t Action);

		// Handlers
		void InputEvent(_State *GameState, int InputType, int Input, float Value);

		// State of each action
		std::vector<_ActionState> State;

	private:

		// Input bindings
		std::list<_ActionMap> InputMap[_Input::INPUT_COUNT][ACTIONS_MAXINPUTS];
};

extern _Actions Actions;
