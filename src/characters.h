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
#ifndef CHARACTERS_H
#define CHARACTERS_H

// Libraries
#include "engine/state.h"

// Constants
const int CHARACTERS_MAX = 6;

// Forward Declarations
class PacketClass;

// Structures
struct SlotStruct {
	IGUIButton *Button;
	stringc Name;
	int Level;
	bool Used;
};

// Classes
class CharactersStateClass : public StateClass {

	public:

		enum ElementType {
			ELEMENT_PLAY,
			ELEMENT_LOGOUT,
			ELEMENT_CREATE,
			ELEMENT_DELETE,
			ELEMENT_DELETECONFIRM,
			ELEMENT_SLOT0,
			ELEMENT_SLOT1,
			ELEMENT_SLOT2,
			ELEMENT_SLOT3,
			ELEMENT_SLOT4,
			ELEMENT_SLOT5,
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

		void HandleCharacterList(PacketClass *Packet);
		void UpdateSelection(int Index);
		void RequestCharacterList();
		void PlayCharacter();
		void Delete();
		void Logout();

		// GUI
		IGUIButton *ButtonPlay, *ButtonLogout, *ButtonCreate, *ButtonDelete;
		SlotStruct Slots[CHARACTERS_MAX];
		int SelectedIndex;
		u32 ClickTimer;
		
};

extern CharactersStateClass CharactersState;

#endif
