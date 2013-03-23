/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2010  Alan Witkowski
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
**************************************************************************************/
#include "connect.h"
#include "engine/game.h"
#include "engine/constants.h"
#include "engine/globals.h"
#include "engine/config.h"
#include "engine/graphics.h"
#include "network/network.h"
#include "network/packetstream.h"
#include "mainmenu.h"
#include "account.h"

// Initializes the state
int ConnectState::Init() {
	int DrawX = 400, DrawY = 250, ButtonWidth = 80;
	Form = irrGUI->addTab(Graphics::Instance().GetRect(0, 0, 800, 600));

	IPAddress = Config::Instance().GetLastIPAddress();

	// Text
	Graphics::Instance().SetFont(GraphicsClass::FONT_10);
	TextIP = Graphics::Instance().AddText("IP Address", DrawX, DrawY, GraphicsClass::ALIGN_CENTER, Form);

	// IP Address
	DrawY += 35;
	EditIP = irrGUI->addEditBox(stringw(IPAddress.c_str()).c_str(), Graphics::Instance().GetCenteredRect(DrawX, DrawY, 190, 25), true, Form, ELEMENT_IP);
	irrGUI->setFocus(EditIP);

	// Buttons
	DrawY += 50;
	ButtonConnect = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX - 55, DrawY, ButtonWidth, 25), Form, ELEMENT_CONNECT, L"Connect");
	ButtonCancel = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX + 55, DrawY, ButtonWidth, 25), Form, ELEMENT_CANCEL, L"Cancel");

	Message = "";
	ChangeState(STATE_MAIN);

	return 1;
}

// Shuts the state down
int ConnectState::Close() {

	return 1;
}

// Handles a connection to the server
void ConnectState::HandleConnect(ENetEvent *TEvent) {

	Config::Instance().SetLastIPAddress(IPAddress);
	Config::Instance().SaveSettings();
}

// Handles a disconnection from the server
void ConnectState::HandleDisconnect(ENetEvent *TEvent) {

	if(State == STATE_CONNECT)
		Message = "Unable to connect to server";
	ChangeState(STATE_MAIN);
}

// Handles a server packet
void ConnectState::HandlePacket(ENetEvent *TEvent) {
	PacketClass Packet(TEvent->packet);
	switch(Packet.ReadChar()) {
		case NetworkClass::GAME_VERSION: {
			int Version = Packet.ReadInt();
			if(Version != GAME_VERSION_NUM) {
				Message = "Game version differs from server's";
				ChangeState(STATE_MAIN);
			}
			else {
				Game::Instance().ChangeState(AccountState::Instance());
			}
		}
		break;
	}
}

// Updates the current state
void ConnectState::Update(u32 TDeltaTime) {

	switch(State) {
		case STATE_MAIN:
		break;
		case STATE_CONNECT:
		break;
	}
}

// Draws the current state
void ConnectState::Draw() {

	Graphics::Instance().DrawImage(GraphicsClass::IMAGE_MENULOGO, 400, 125);

	// Server message
	if(Message.size() > 0) {
		Graphics::Instance().SetFont(GraphicsClass::FONT_10);
		Graphics::Instance().RenderText(Message.c_str(), 400, 200, GraphicsClass::ALIGN_CENTER, SColor(255, 255, 0, 0));
	}

	switch(State) {
		case STATE_MAIN:
		break;
		case STATE_CONNECT:
			Graphics::Instance().SetFont(GraphicsClass::FONT_10);
			Graphics::Instance().RenderText("Connecting...", 400, 250, GraphicsClass::ALIGN_CENTER);
		break;
	}

	irrGUI->drawAll();
}

// Key presses
bool ConnectState::HandleKeyPress(EKEY_CODE TKey) {

	switch(State) {
		case STATE_MAIN:
			switch(TKey) {
				case KEY_ESCAPE:
					Game::Instance().ChangeState(MainMenuState::Instance());
				break;
				case KEY_RETURN:
					ChangeState(STATE_CONNECT);
				break;
			}				
		break;
		case STATE_CONNECT:
			switch(TKey) {
				case KEY_ESCAPE:
					ChangeState(STATE_MAIN);
				break;
			}		
		break;
	}
	
	return false;
}

// GUI events
void ConnectState::HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement) {
	switch(State) {
		case STATE_MAIN:
			switch(TEventType) {
				case EGET_BUTTON_CLICKED:
					switch(TElement->getID()) {
						case ELEMENT_CONNECT:
							ChangeState(STATE_CONNECT);
						break;
						case ELEMENT_CANCEL:
							Game::Instance().ChangeState(MainMenuState::Instance());
						break;
					}
				break;
			}
		break;
	}
}

// Changes the internal state
void ConnectState::ChangeState(int TState) {

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
bool ConnectState::ValidateForm() {
	IPAddress = EditIP->getText();
	IPAddress.trim();

	if(IPAddress.size() == 0) {
		Message = "Invalid IP address";
		irrGUI->setFocus(EditIP);
		return false;
	}

	return true;
}
