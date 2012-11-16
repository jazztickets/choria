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
#include "account.h"
#include "engine/game.h"
#include "engine/globals.h"
#include "engine/config.h"
#include "engine/graphics.h"
#include "network/network.h"
#include "network/packetstream.h"
#include "connect.h"
#include "characters.h"

AccountStateClass AccountState;

// Initializes the state
int AccountStateClass::Init() {
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
int AccountStateClass::Close() {

	return 1;
}

// Handles a disconnection from the server
void AccountStateClass::HandleDisconnect(ENetEvent *Event) {

	Game.ChangeState(&ConnectState);
}

// Handles a server packet
void AccountStateClass::HandlePacket(ENetEvent *Event) {
	PacketClass Packet(Event->packet);
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
void AccountStateClass::Update(u32 FrameTime) {

}

// Draws the current state
void AccountStateClass::Draw() {

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
bool AccountStateClass::HandleKeyPress(EKEY_CODE Key) {

	switch(State) {
		case STATE_MAIN:
			switch(Key) {
				case KEY_ESCAPE:
					Game.ChangeState(&ConnectState);
				break;
				case KEY_RETURN:
					ChangeState(STATE_LOGIN);
				break;
			}				
		break;
	}
	
	return false;
}

// GUI events
void AccountStateClass::HandleGUI(EGUI_EVENT_TYPE EventType, IGUIElement *Element) {
	switch(State) {
		case STATE_MAIN:
			switch(EventType) {
				case EGET_BUTTON_CLICKED:
					switch(Element->getID()) {
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
					}
				break;
			}
		break;
	}
}

// Changes the internal state
void AccountStateClass::ChangeState(int State) {

	switch(State) {
		case STATE_MAIN:
			Form->setVisible(true);
			CreateAccount = false;
			if(AccountName == "")
				irrGUI->setFocus(EditAccountName);
			else
				irrGUI->setFocus(EditPassword);

			this->State = State;
		break;
		case STATE_LOGIN:
			if(ValidateForm()) {
				Form->setVisible(false);
				Message = "";
				this->State = State;

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
bool AccountStateClass::ValidateForm() {

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
void AccountStateClass::SetLoginInfo(const stringc &AccountName, const stringc &Password) {
	this->AccountName = AccountName;
	this->Password = Password;
}
