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
#include <actions.h>
#include <framework.h>
#include <state.h>
#include <assets.h>
#include <ui/label.h>

_Actions Actions;

// Constructor
_Actions::_Actions() {
	ResetState();
}

// Load action names from database;
void _Actions::LoadActionNames() {
	Names[UP] = Assets.Labels["label_options_up"]->Text;
	Names[DOWN] = Assets.Labels["label_options_down"]->Text;
	Names[LEFT] = Assets.Labels["label_options_left"]->Text;
	Names[RIGHT] = Assets.Labels["label_options_right"]->Text;
	Names[FIRE] = Assets.Labels["label_options_fire"]->Text;
	Names[AIM] = Assets.Labels["label_options_aim"]->Text;
	Names[USE] = Assets.Labels["label_options_use"]->Text;
	Names[INVENTORY] = Assets.Labels["label_options_inventory"]->Text;
	Names[RELOAD] = Assets.Labels["label_options_reload"]->Text;
	Names[WEAPONSWITCH] = Assets.Labels["label_options_weaponswitch"]->Text;
	Names[MEDKIT] = Assets.Labels["label_options_medkit"]->Text;
}

// Clear all mappings
void _Actions::ClearMappings(int InputType) {
	for(int i = 0; i < ACTIONS_MAXINPUTS; i++)
		InputMap[InputType][i].clear();
}

// Remove a mapping for an action
void _Actions::ClearMappingsForAction(int InputType, int Action) {
	for(int i = 0; i < ACTIONS_MAXINPUTS; i++) {
		for(auto MapIterator = InputMap[InputType][i].begin(); MapIterator != InputMap[InputType][i].end(); ) {
			if(*MapIterator == Action) {
				MapIterator = InputMap[InputType][i].erase(MapIterator);
			}
			else
				++MapIterator;
		}
	}
}

// Remove all input mappings for an action
void _Actions::ClearAllMappingsForAction(int Action) {
	for(int i = 0; i < _Input::INPUT_COUNT; i++) {
		ClearMappingsForAction(i, Action);
	}
}

// Get action
int _Actions::GetState(int Action) {
	return !!(State & (1 << Action));
}

// Add an input mapping
void _Actions::AddInputMap(int InputType, int Input, int Action, bool IfNone) {
	if(Action < 0 || Action >= COUNT || Input < 0 || Input >= ACTIONS_MAXINPUTS)
		return;

	if(!IfNone || (IfNone && GetInputForAction(InputType, Action) == -1)) {
		InputMap[InputType][Input].push_back(Action);
	}
}

// Returns the first input for an action
int _Actions::GetInputForAction(int InputType, int Action) {
	for(int i = 0; i < ACTIONS_MAXINPUTS; i++) {
		for(auto &Iterator : InputMap[InputType][i]) {
			if(Iterator == Action) {
				return i;
			}
		}
	}

	return -1;
}

// Get name of input key/button for a given action
std::string _Actions::GetInputNameForAction(int Action) {

	for(int i = 0; i < _Input::INPUT_COUNT; i++) {
		int Input = GetInputForAction(i, Action);

		if(Input != -1) {
			switch(i) {
				case _Input::KEYBOARD:
					return _Input::GetKeyName(Input);
				break;
				case _Input::MOUSE_BUTTON:
					return _Input::GetMouseButtonName(Input);
				break;
			}
		}
	}

	return "";
}

// Inject an input into the action handler
void _Actions::InputEvent(int InputType, int Input, int Value) {
	if(Input < 0 || Input >= ACTIONS_MAXINPUTS)
		return;

	for(auto &Action : InputMap[InputType][Input]) {
		State &= ~(1 << Action);
		State |= Value << Action;

		// If true is returned, stop handling the same key
		if(Framework.GetState()->HandleAction(InputType, Action, Value))
			break;
	}
}
