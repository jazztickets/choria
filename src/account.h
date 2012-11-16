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
#ifndef ACCOUNT_H
#define ACCOUNT_H

// Libraries
#include "engine/state.h"

// Classes
class AccountStateClass : public StateClass {

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
		
		void HandleDisconnect(ENetEvent *Event);
		void HandlePacket(ENetEvent *Event);
		bool HandleKeyPress(EKEY_CODE Key);
		void HandleGUI(EGUI_EVENT_TYPE EventType, IGUIElement *Element);

		void Update(u32 FrameTime);
		void Draw();

		void SetLoginInfo(const stringc &AccountName, const stringc &Password);

	private:

		void ChangeState(int State);
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

extern AccountStateClass AccountState;

#endif
