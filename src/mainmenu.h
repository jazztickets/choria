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
#ifndef MAINMENU_H
#define MAINMENU_H

// Libraries
#include "engine/state.h"

// Classes
class MainMenuState : public StateClass {

	public:

		enum ElementType {
			ELEMENT_SINGLEPLAYER,
			ELEMENT_MULTIPLAYER,
			ELEMENT_EDITOR,
			ELEMENT_OPTIONS,
			ELEMENT_EXIT,
		};

		int Init();
		int Close();

		bool HandleKeyPress(EKEY_CODE TKey);
		void HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement);

		void Update(u32 TDeltaTime);
		void Draw();

		static MainMenuState *Instance() {
			static MainMenuState ClassInstance;
			return &ClassInstance;
		}

	private:

		void StartSinglePlayer();

};

#endif
