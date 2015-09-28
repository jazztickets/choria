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

// Classes
class _AccountState : public _State {

	public:

		enum StateType {
			STATE_MAIN,
			STATE_LOGIN,
		};

		enum ElementType {
			ELEMENT_ACCOUNT,
			ELEMENT_PASSWORD,
			ELEMENT_LOGIN,
			ELEMENT_CANCEL,
			ELEMENT_CREATEACCOUNT,
		};

		int Init();
		int Close();

		void HandleDisconnect(ENetEvent *TEvent);
		void HandlePacket(ENetEvent *TEvent);
		bool HandleKeyPress(EKEY_CODE TKey);
		void HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement);

		void Update(u32 TDeltaTime);
		void Draw();

		void SetLoginInfo(const stringc &TAccountName, const stringc &TPassword);

	private:

		void ChangeState(int TState);
		bool ValidateForm();

		// States
		int State;

		// GUI
		IGUITab *Form;
		IGUIStaticText *TextAccountName, *TextPassword;
		IGUIEditBox *EditAccountName, *EditPassword;
		IGUIButton *ButtonLogin, *ButtonCancel, *ButtonCreateAccount;
		stringc Message, AccountName, Password;
		bool CreateAccount;

};

extern _AccountState AccountState;
