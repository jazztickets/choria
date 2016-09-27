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
#include <SDL_scancode.h>

// Constants
const int ACTIONS_MAXINPUTS = SDL_NUM_SCANCODES;

struct _ActionMap {
	_ActionMap(int Action, float Scale, float DeadZone) : Action(Action), DeadZone(DeadZone), Scale(Scale) { }

	int Action;
	float DeadZone;
	float Scale;
};

struct _ActionState {
	float Value;
	int Source;
};

// Actions class
class _Actions {

	public:

		enum Types {
			UP,
			DOWN,
			LEFT,
			RIGHT,
			INVENTORY,
			SKILLS,
			TELEPORT,
			HELP,
			MENU,
			UNUSED,
			TRADE,
			CHAT,
			SKILL1,
			SKILL2,
			SKILL3,
			SKILL4,
			SKILL5,
			SKILL6,
			SKILL7,
			SKILL8,
			COUNT,
		};

		_Actions();

		void ResetState();
		void LoadActionNames();
		void ClearMappings(int InputType);
		void ClearMappingsForAction(int InputType, int Action);
		void ClearAllMappingsForAction(int Action);
		void Serialize(std::ofstream &File, int InputType);

		// Actions
		float GetState(int Action);
		const std::string &GetName(int Action) { return Names[Action]; }

		// Maps
		void AddInputMap(int InputType, int Input, int Action, float Scale=1.0f, float DeadZone=-1.0f, bool IfNone=true);
		int GetInputForAction(int InputType, int Action);
		std::string GetInputNameForAction(int Action);

		// Handlers
		void InputEvent(int InputType, int Input, float Value);

	private:

		// Input bindings
		std::list<_ActionMap> InputMap[_Input::INPUT_COUNT][ACTIONS_MAXINPUTS];

		// State of each action
		_ActionState State[COUNT];

		// Nice names for each action
		std::string Names[COUNT];
};

extern _Actions Actions;
