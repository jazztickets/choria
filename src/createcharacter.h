/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef CREATECHARACTER_H
#define CREATECHARACTER_H

// Libraries
#include "engine/state.h"

// Forward Declarations
class PacketClass;

// Classes
class CreateCharacterStateClass : public StateClass {

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
		
		void HandleDisconnect(ENetEvent *Event);
		void HandlePacket(ENetEvent *Event);
		bool HandleKeyPress(EKEY_CODE Key);
		void HandleGUI(EGUI_EVENT_TYPE EventType, IGUIElement *Element);

		void Update(u32 FrameTime);
		void Draw();

	private:

		void UpdateSelection(int Index);
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

extern CreateCharacterStateClass CreateCharacterState;

#endif
