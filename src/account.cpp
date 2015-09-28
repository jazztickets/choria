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
#include <account.h>
#include <engine/game.h>
#include <engine/globals.h>
#include <engine/config.h>
#include <engine/graphics.h>
#include <network/network.h>
#include <network/packetstream.h>
#include <connect.h>
#include <characters.h>

_AccountState AccountState;

// Initializes the state
int _AccountState::Init() {
	int DrawX = 400, DrawY = 250, ButtonWidth = 80;
	Form = irrGUI->addTab(Graphics.GetRect(0, 0, 800, 600));

	if(AccountName == "")
		AccountName = Config.GetLastAccountName();

	// Account
	Graphics.SetFont(GraphicsClass::FONT_10);
	TextAccountName = Graphics.AddText("Account", DrawX, DrawY, GraphicsClass::ALIGN_CENTER, Form);

	DrawY += 35;
	EditAccountName = irrGUI->addEditBox(L"", Graphics.GetCenteredRect(DrawX, DrawY, 180, 25), true, Form, ELEMENT_ACCOUNT);
	EditAccountName->setMax(15);
	EditAccountName->setText(stringw(AccountName.c_str()).c_str());

	// Password
	DrawY += 35;
	TextPassword = Graphics.AddText("Password", DrawX, DrawY, GraphicsClass::ALIGN_CENTER, Form);

	DrawY += 35;
	EditPassword = irrGUI->addEditBox(L"", Graphics.GetCenteredRect(DrawX, DrawY, 180, 25), true, Form, ELEMENT_PASSWORD);
	EditPassword->setPasswordBox(true);
	EditPassword->setMax(15);
	EditPassword->setText(stringw(Password.c_str()).c_str());

	// Buttons
	DrawY += 50;
	ButtonLogin = irrGUI->addButton(Graphics.GetCenteredRect(DrawX - 50, DrawY, ButtonWidth, 25), Form, ELEMENT_LOGIN, L"Log in");
	ButtonCancel = irrGUI->addButton(Graphics.GetCenteredRect(DrawX + 50, DrawY, ButtonWidth, 25), Form, ELEMENT_CANCEL, L"Cancel");

	// Create account
	DrawY += 50;
	ButtonCreateAccount = irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 120, 25), Form, ELEMENT_CREATEACCOUNT, L"Create Account");

	Message = "";
	ChangeState(STATE_MAIN);

	return 1;
}

// Shuts the state down
int _AccountState::Close() {

	return 1;
}

// Handles a disconnection from the server
void _AccountState::HandleDisconnect(ENetEvent *TEvent) {

	Game.ChangeState(&ConnectState);
}

// Handles a server packet
void _AccountState::HandlePacket(ENetEvent *TEvent) {
	PacketClass Packet(TEvent->packet);
	switch(Packet.ReadChar()) {
		case NetworkClass::ACCOUNT_NOTFOUND:
			Message = "Account not found";
			ChangeState(STATE_MAIN);
		break;
		case NetworkClass::ACCOUNT_EXISTS:
			Message = "Account exists";
			ChangeState(STATE_MAIN);
		break;
		case NetworkClass::ACCOUNT_ALREADYLOGGEDIN:
			Message = "Account is already logged into";
			ChangeState(STATE_MAIN);
		break;
		case NetworkClass::ACCOUNT_SUCCESS:
			Config.SetLastAccountName(AccountName);
			Config.SaveSettings();
			AccountName = "";
			Password = "";
			Game.ChangeState(&CharactersState);
		break;
	}
}

// Updates the current state
void _AccountState::Update(u32 TDeltaTime) {

}

// Draws the current state
void _AccountState::Draw() {

	Graphics.DrawImage(GraphicsClass::IMAGE_MENULOGO, 400, 125);

	// Server message
	if(Message.size() > 0) {
		Graphics.SetFont(GraphicsClass::FONT_10);
		Graphics.RenderText(Message.c_str(), 400, 200, GraphicsClass::ALIGN_CENTER, SColor(255, 255, 0, 0));
	}

	switch(State) {
		case STATE_MAIN:
		break;
		case STATE_LOGIN:
			Graphics.SetFont(GraphicsClass::FONT_10);
			Graphics.RenderText("Sending information...", 400, 250, GraphicsClass::ALIGN_CENTER);
		break;
	}

	irrGUI->drawAll();
}

// Key presses
bool _AccountState::HandleKeyPress(EKEY_CODE TKey) {

	switch(State) {
		case STATE_MAIN:
			switch(TKey) {
				case KEY_ESCAPE:
					Game.ChangeState(&ConnectState);
				break;
				case KEY_RETURN:
					ChangeState(STATE_LOGIN);
				break;
				default:
				break;
			}
		break;
	}

	return false;
}

// GUI events
void _AccountState::HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement) {
	switch(State) {
		case STATE_MAIN:
			switch(TEventType) {
				case EGET_BUTTON_CLICKED:
					switch(TElement->getID()) {
						case ELEMENT_LOGIN:
							CreateAccount = false;
							ChangeState(STATE_LOGIN);
						break;
						case ELEMENT_CANCEL:
							Game.ChangeState(&ConnectState);
						break;
						case ELEMENT_CREATEACCOUNT:
							CreateAccount = true;
							ChangeState(STATE_LOGIN);
						break;
						default:
						break;
					}
				break;
				default:
				break;
			}
		break;
		default:
		break;
	}
}

// Changes the internal state
void _AccountState::ChangeState(int TState) {

	switch(TState) {
		case STATE_MAIN:
			Form->setVisible(true);
			CreateAccount = false;
			if(AccountName == "")
				irrGUI->setFocus(EditAccountName);
			else
				irrGUI->setFocus(EditPassword);

			State = TState;
		break;
		case STATE_LOGIN:
			if(ValidateForm()) {
				Form->setVisible(false);
				Message = "";
				State = TState;

				// Send information
				PacketClass Packet(NetworkClass::ACCOUNT_LOGININFO);
				Packet.WriteBit(CreateAccount);
				Packet.WriteString(AccountName.c_str());
				Packet.WriteString(Password.c_str());
				ClientNetwork->SendPacketToHost(&Packet);
			}
		break;
	}
}

// Validates the login form
bool _AccountState::ValidateForm() {

	AccountName = EditAccountName->getText();
	AccountName.trim();

	Password = EditPassword->getText();
	Password.trim();

	if(AccountName.size() == 0) {
		Message = "Invalid account name";
		irrGUI->setFocus(EditAccountName);
		return false;
	}
	else if(Password.size() == 0) {
		Message = "Invalid password";
		irrGUI->setFocus(EditPassword);
		return false;
	}

	return true;
}

// Sets the default login info
void _AccountState::SetLoginInfo(const stringc &TAccountName, const stringc &TPassword) {
	AccountName = TAccountName;
	Password = TPassword;
}
