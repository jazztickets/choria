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
#ifndef CONNECT_H
#define CONNECT_H

// Libraries
#include "engine/state.h"

// Classes
class ConnectState : public StateClass {

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
		bool HandleKeyPress(EKEY_CODE TKey);
		void HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement);

		void Update(u32 TDeltaTime);
		void Draw();

		static ConnectState *Instance() {
			static ConnectState ClassInstance;
			return &ClassInstance;
		}

	private:

		void ChangeState(int TState);
		bool ValidateForm();

		// States
		int State;

		// GUI
		IGUITab *Form;
		IGUIStaticText *TextIP;
		IGUIEditBox *EditIP;
		IGUIButton *ButtonConnect, *ButtonCancel;
		stringc Message, IPAddress;
		
};

#endif
