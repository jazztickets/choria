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
