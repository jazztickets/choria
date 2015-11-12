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

		void Init();
		void Close();

		void HandleDisconnect(ENetEvent *TEvent);
		void HandlePacket(ENetEvent *TEvent);
		bool HandleKeyPress(irr::EKEY_CODE TKey);
		void HandleGUI(irr::gui::EGUI_EVENT_TYPE TEventType, irr::gui::IGUIElement *TElement);

		void Update(double FrameTime);
		void Draw();

		void SetLoginInfo(const irr::core::stringc &TAccountName, const irr::core::stringc &TPassword);

	private:

		void ChangeState(int TState);
		bool ValidateForm();

		// States
		int State;

		// GUI
		irr::gui::IGUITab *Form;
		irr::gui::IGUIStaticText *TextAccountName, *TextPassword;
		irr::gui::IGUIEditBox *EditAccountName, *EditPassword;
		irr::gui::IGUIButton *ButtonLogin, *ButtonCancel, *ButtonCreateAccount;
		irr::core::stringc Message, AccountName, Password;
		bool CreateAccount;

};

extern _AccountState AccountState;
