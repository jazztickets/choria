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
#pragma once

// Libraries
#include <state.h>

// Forward Declarations
class PacketClass;

// Classes
class _CreateCharacterState : public _State {

	public:

		enum StateType {
			STATE_MAIN,
			STATE_SEND,
		};

		enum ElementType {
			ELEMENT_NAME,
			ELEMENT_CREATE,
			ELEMENT_BACK,
			ELEMENT_PORTRAITS,
		};

		int Init();
		int Close();

		void HandleDisconnect(ENetEvent *TEvent);
		void HandlePacket(ENetEvent *TEvent);
		bool HandleKeyPress(EKEY_CODE TKey);
		void HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement);

		void Update(u32 TDeltaTime);
		void Draw();

	private:

		void UpdateSelection(int TSelectedIndex);
		void CreateCharacter();
		void Back();

		// States
		int State;

		// GUI
		IGUIEditBox *EditName;
		IGUIButton *ButtonCreate, *ButtonBack, *SelectedButton;
		stringc Message;
		int SelectedIndex;

		array<int> Portraits;
};

extern _CreateCharacterState CreateCharacterState;
