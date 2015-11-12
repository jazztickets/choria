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
#pragma once

// Libraries
#include <state.h>
#include <IGUITabControl.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>

// Classes
class _ConnectState : public _State {

	public:

		enum StateType {
			STATE_MAIN,
			STATE_CONNECT,
		};

		enum ElementType {
			ELEMENT_IP,
			ELEMENT_CONNECT,
			ELEMENT_CANCEL,
		};

		int Init();
		int Close();

		void HandleConnect(ENetEvent *TEvent);
		void HandleDisconnect(ENetEvent *TEvent);
		void HandlePacket(ENetEvent *TEvent);
		bool HandleKeyPress(irr::EKEY_CODE TKey);
		void HandleGUI(irr::gui::EGUI_EVENT_TYPE TEventType, irr::gui::IGUIElement *TElement);

		void Update(double FrameTime);
		void Draw();

	private:

		void ChangeState(int TState);
		bool ValidateForm();

		// States
		int State;

		// GUI
		irr::gui::IGUITab *Form;
		irr::gui::IGUIStaticText *TextIP;
		irr::gui::IGUIEditBox *EditIP;
		irr::gui::IGUIButton *ButtonConnect, *ButtonCancel;
		irr::core::stringc Message, IPAddress;

};

extern _ConnectState ConnectState;
