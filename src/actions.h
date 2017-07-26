/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
#include <vector>
#include <SDL_scancode.h>

// Constants
const int ACTIONS_MAXINPUTS = SDL_NUM_SCANCODES;

struct _ActionMap {
	_ActionMap(size_t Action, float Scale, float DeadZone) : Action(Action), DeadZone(DeadZone), Scale(Scale) { }

	size_t Action;
	float DeadZone;
	float Scale;
};

struct _ActionState {
	std::string Name;
	float Value;
	int Source;
};

// Actions class
class _Actions {

	public:

		_Actions();

		void ResetState();
		void ClearMappings(int InputType);
		void ClearMappingsForAction(int InputType, size_t Action);
		void ClearAllMappingsForAction(size_t Action);
		void Serialize(std::ofstream &File, int InputType);

		// Actions
		float GetState(size_t Action);
		const std::string &GetName(size_t Action) { return State[Action].Name; }

		// Maps
		void AddInputMap(int InputType, int Input, size_t Action, float Scale=1.0f, float DeadZone=-1.0f, bool IfNone=true);
		int GetInputForAction(int InputType, size_t Action);
		std::string GetInputNameForAction(size_t Action);

		// Handlers
		void InputEvent(int InputType, int Input, float Value);

	private:

		// Input bindings
		std::list<_ActionMap> InputMap[_Input::INPUT_COUNT][ACTIONS_MAXINPUTS];

		// State of each action
		std::vector<_ActionState> State;
};

extern _Actions Actions;
