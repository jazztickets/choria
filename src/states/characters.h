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
#include <IGUIButton.h>

// Constants
const int CHARACTERS_MAX = 6;

// Forward Declarations
class _Buffer;

// Structures
struct _CharacterSlot {
	irr::gui::IGUIButton *Button;
	irr::core::stringc Name;
	int Level;
	bool Used;
};

// Classes
class _CharactersState : public _State {

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

		void HandleDisconnect(ENetEvent *TEvent);
		void HandlePacket(ENetEvent *TEvent);
		bool HandleKeyPress(irr::EKEY_CODE TKey);
		void HandleGUI(irr::gui::EGUI_EVENT_TYPE TEventType, irr::gui::IGUIElement *TElement);

		void Update(double FrameTime);
		void Draw();

	private:

		void HandleCharacterList(_Buffer *TPacket);
		void UpdateSelection(int TSelectedIndex);
		void RequestCharacterList();
		void PlayCharacter();
		void Delete();
		void Logout();

		// GUI
		irr::gui::IGUIButton *ButtonPlay, *ButtonLogout, *ButtonCreate, *ButtonDelete;
		_CharacterSlot Slots[CHARACTERS_MAX];
		int SelectedIndex;
		double ClickTimer;

};

extern _CharactersState CharactersState;
