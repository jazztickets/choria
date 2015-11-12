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
#include <states/connect.h>
#include <framework.h>
#include <constants.h>
#include <globals.h>
#include <config.h>
#include <graphics.h>
#include <buffer.h>
#include <network/network.h>
#include <states/mainmenu.h>
#include <states/account.h>
#include <string>
#include <IGUIEnvironment.h>

_ConnectState ConnectState;

using namespace irr;

// Initializes the state
int _ConnectState::Init() {
	int DrawX = 400, DrawY = 250, ButtonWidth = 80;
	Form = irrGUI->addTab(Graphics.GetRect(0, 0, 800, 600));

	IPAddress = Config.LastIPAddress;

	// Text
	Graphics.SetFont(_Graphics::FONT_10);
	TextIP = Graphics.AddText("IP Address", DrawX, DrawY, _Graphics::ALIGN_CENTER, Form);

	// IP Address
	DrawY += 35;
	EditIP = irrGUI->addEditBox(core::stringw(IPAddress.c_str()).c_str(), Graphics.GetCenteredRect(DrawX, DrawY, 190, 25), true, Form, ELEMENT_IP);
	irrGUI->setFocus(EditIP);

	// Buttons
	DrawY += 50;
	ButtonConnect = irrGUI->addButton(Graphics.GetCenteredRect(DrawX - 55, DrawY, ButtonWidth, 25), Form, ELEMENT_CONNECT, L"Connect");
	ButtonCancel = irrGUI->addButton(Graphics.GetCenteredRect(DrawX + 55, DrawY, ButtonWidth, 25), Form, ELEMENT_CANCEL, L"Cancel");

	Message = "";
	ChangeState(STATE_MAIN);

	return 1;
}

// Shuts the state down
int _ConnectState::Close() {

	return 1;
}

// Handles a connection to the server
void _ConnectState::HandleConnect(ENetEvent *TEvent) {

	Config.LastIPAddress = IPAddress;
	Config.SaveSettings();
}

// Handles a disconnection from the server
void _ConnectState::HandleDisconnect(ENetEvent *TEvent) {

	if(State == STATE_CONNECT)
		Message = "Unable to connect to server";
	ChangeState(STATE_MAIN);
}

// Handles a server packet
void _ConnectState::HandlePacket(ENetEvent *TEvent) {
	_Buffer Packet((char *)TEvent->packet->data, TEvent->packet->dataLength);
	switch(Packet.Read<char>()) {
		case _Network::VERSION: {
			std::string Version(Packet.ReadString());
			if(Version != GAME_VERSION) {
				Message = "Game version differs from server's";
				ChangeState(STATE_MAIN);
			}
			else {
				Framework.ChangeState(&AccountState);
			}
		}
		break;
	}
}

// Updates the current state
void _ConnectState::Update(double FrameTime) {

	switch(State) {
		case STATE_MAIN:
		break;
		case STATE_CONNECT:
		break;
	}
}

// Draws the current state
void _ConnectState::Draw() {

	Graphics.DrawImage(_Graphics::IMAGE_MENULOGO, 400, 125);

	// Server message
	if(Message.size() > 0) {
		Graphics.SetFont(_Graphics::FONT_10);
		Graphics.RenderText(Message.c_str(), 400, 200, _Graphics::ALIGN_CENTER, video::SColor(255, 255, 0, 0));
	}

	switch(State) {
		case STATE_MAIN:
		break;
		case STATE_CONNECT:
			Graphics.SetFont(_Graphics::FONT_10);
			Graphics.RenderText("Connecting...", 400, 250, _Graphics::ALIGN_CENTER);
		break;
	}

	irrGUI->drawAll();
}

// Key presses
bool _ConnectState::HandleKeyPress(EKEY_CODE TKey) {

	switch(State) {
		case STATE_MAIN:
			switch(TKey) {
				case KEY_ESCAPE:
					Framework.ChangeState(&MainMenuState);
				break;
				case KEY_RETURN:
					ChangeState(STATE_CONNECT);
				break;
				default:
				break;
			}
		break;
		case STATE_CONNECT:
			switch(TKey) {
				case KEY_ESCAPE:
					ChangeState(STATE_MAIN);
				break;
				default:
				break;
			}
		break;
		default:
		break;
	}

	return false;
}

// GUI events
void _ConnectState::HandleGUI(gui::EGUI_EVENT_TYPE TEventType, gui::IGUIElement *TElement) {
	switch(State) {
		case STATE_MAIN:
			switch(TEventType) {
				case gui::EGET_BUTTON_CLICKED:
					switch(TElement->getID()) {
						case ELEMENT_CONNECT:
							ChangeState(STATE_CONNECT);
						break;
						case ELEMENT_CANCEL:
							Framework.ChangeState(&MainMenuState);
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
void _ConnectState::ChangeState(int TState) {

	switch(TState) {
		case STATE_MAIN:
			State = TState;
			Form->setVisible(true);
			ClientNetwork->Disconnect();
		break;
		case STATE_CONNECT: {

			if(ValidateForm() && ClientNetwork->Connect(IPAddress.c_str())) {
				Form->setVisible(false);
				Message = "";
				State = TState;
			}
		}
		break;
	}
}

// Validates the login form
bool _ConnectState::ValidateForm() {
	IPAddress = EditIP->getText();
	IPAddress.trim();

	if(IPAddress.size() == 0) {
		Message = "Invalid IP address";
		irrGUI->setFocus(EditIP);
		return false;
	}

	return true;
}
