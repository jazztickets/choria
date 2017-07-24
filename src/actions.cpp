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
#include <actions.h>
#include <framework.h>
#include <state.h>
#include <assets.h>
#include <ui/label.h>

_Actions Actions;

// Constructor
_Actions::_Actions() {

	// Reset state
	State.resize(COUNT);
	Names.resize(COUNT);
	ResetState();

	// Set names for actions
	Names[UP] = "up";
	Names[DOWN] = "down";
	Names[LEFT] = "left";
	Names[RIGHT] = "right";
	Names[BACK] = "back";
	Names[MENU] = "menu";
	Names[JOIN] = "join";
	Names[INVENTORY] = "inventory";
	Names[SKILLS] = "skills";
	Names[TRADE] = "trade";
	Names[PARTY] = "party";
	Names[CHAT] = "chat";
	Names[SKILL1] = "skill1";
	Names[SKILL2] = "skill2";
	Names[SKILL3] = "skill3";
	Names[SKILL4] = "skill4";
	Names[SKILL5] = "skill5";
	Names[SKILL6] = "skill6";
	Names[SKILL7] = "skill7";
	Names[SKILL8] = "skill8";
}

// Reset the action state
void _Actions::ResetState() {
	for(size_t i = 0; i < COUNT; i++) {
		State[i].Value = 0.0f;
		State[i].Source = -1;
	}
}

// Clear all mappings
void _Actions::ClearMappings(int InputType) {
	for(int i = 0; i < ACTIONS_MAXINPUTS; i++)
		InputMap[InputType][i].clear();
}

// Remove a mapping for an action
void _Actions::ClearMappingsForAction(int InputType, size_t Action) {
	for(int i = 0; i < ACTIONS_MAXINPUTS; i++) {
		for(auto MapIterator = InputMap[InputType][i].begin(); MapIterator != InputMap[InputType][i].end(); ) {
			if(MapIterator->Action == Action) {
				MapIterator = InputMap[InputType][i].erase(MapIterator);
			}
			else
				++MapIterator;
		}
	}
}

// Remove all input mappings for an action
void _Actions::ClearAllMappingsForAction(size_t Action) {
	for(int i = 0; i < _Input::INPUT_COUNT; i++) {
		ClearMappingsForAction(i, Action);
	}
}

// Serialize input map to stream
void _Actions::Serialize(std::ofstream &File, int InputType) {
	for(int i = 0; i < ACTIONS_MAXINPUTS; i++) {
		for(auto &Iterator : InputMap[InputType][i]) {
			File << "action_" << Names[Iterator.Action] << "=" << InputType << "_" << i << std::endl;
		}
	}
}

// Get action
float _Actions::GetState(size_t Action) {
	if(Action >= COUNT)
		return 0.0f;

	return State[Action].Value;
}

// Add an input mapping
void _Actions::AddInputMap(int InputType, int Input, size_t Action, float Scale, float DeadZone, bool IfNone) {
	if(Action >= COUNT || Input < 0 || Input >= ACTIONS_MAXINPUTS)
		return;

	if(!IfNone || (IfNone && GetInputForAction(InputType, Action) == -1))
		InputMap[InputType][Input].push_back(_ActionMap(Action, Scale, DeadZone));
}

// Returns the first input for an action
int _Actions::GetInputForAction(int InputType, size_t Action) {
	for(int i = 0; i < ACTIONS_MAXINPUTS; i++) {
		for(auto &MapIterator : InputMap[InputType][i]) {
			if(MapIterator.Action == Action) {
				return i;
			}
		}
	}

	return -1;
}

// Get name of input key/button for a given action
std::string _Actions::GetInputNameForAction(size_t Action) {

	for(int i = 0; i < _Input::INPUT_COUNT; i++) {
		int Input = GetInputForAction(i, Action);

		if(Input != -1) {
			switch(i) {
				case _Input::KEYBOARD:
					return _Input::GetKeyName(Input);
				case _Input::MOUSE_BUTTON:
					return _Input::GetMouseButtonName((uint32_t)Input);
			}
		}
	}

	return "";
}

// Inject an input into the action handler
void _Actions::InputEvent(int InputType, int Input, float Value) {
	if(Input < 0 || Input >= ACTIONS_MAXINPUTS)
		return;

	for(auto &MapIterator : InputMap[InputType][Input]) {

		// Only let joystick overwrite action state if the keyboard isn't being used
		if(InputType != _Input::JOYSTICK_AXIS || (InputType == _Input::JOYSTICK_AXIS && (State[MapIterator.Action].Source == -1 || State[MapIterator.Action].Source == _Input::JOYSTICK_AXIS))) {

			// If key was released, set source to -1 so that joystick can overwrite it
			if(InputType == _Input::KEYBOARD && Value == 0.0f)
				State[MapIterator.Action].Source = -1;
			else
				State[MapIterator.Action].Source = InputType;

			// Check for deadzone
			if(fabs(Value) <= MapIterator.DeadZone)
				Value = 0.0f;

			State[MapIterator.Action].Value = Value;
		}

		// Check for deadzone
		if(fabs(Value) <= MapIterator.DeadZone)
			Value = 0.0f;

		// Apply input scale to action
		float InputValue = Value * MapIterator.Scale;

		// If true is returned, stop handling the same key
		if(Framework.GetState()->HandleAction(InputType, MapIterator.Action, (int)InputValue))
			break;
	}
}
