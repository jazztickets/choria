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
#include "connect.h"
#include "engine/game.h"
#include "engine/globals.h"
#include "engine/config.h"
#include "engine/graphics.h"
#include "network/network.h"
#include "network/packetstream.h"
#include "mainmenu.h"
#include "account.h"

ConnectStateClass ConnectState;

// Initializes the state
int ConnectStateClass::Init() {
	int DrawX = 400, DrawY = 250, ButtonWidth = 80;
	Form = irrGUI->addTab(Graphics.GetRect(0, 0, 800, 600));

	IPAddress = Config.GetLastIPAddress();

	// Text
	Graphics.SetFont(GraphicsClass::FONT_10);
	TextIP = Graphics.AddText("IP Address", DrawX, DrawY, GraphicsClass::ALIGN_CENTER, Form);

	// IP Address
	DrawY += 35;
	EditIP = irrGUI->addEditBox(stringw(IPAddress.c_str()).c_str(), Graphics.GetCenteredRect(DrawX, DrawY, 190, 25), true, Form, ELEMENT_IP);
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
int ConnectStateClass::Close() {

	return 1;
}

// Handles a connection to the server
void ConnectStateClass::HandleConnect(ENetEvent *Event) {

	Config.SetLastIPAddress(IPAddress);
	Config.SaveSettings();
}

// Handles a disconnection from the server
void ConnectStateClass::HandleDisconnect(ENetEvent *Event) {

	if(State == STATE_CONNECT)
		Message = "Unable to connect to server";
	ChangeState(STATE_MAIN);
}

// Handles a server packet
void ConnectStateClass::HandlePacket(ENetEvent *Event) {
	PacketClass Packet(Event->packet);
	switch(Packet.ReadChar()) {
		case NetworkClass::GAME_VERSION: {
			int Version = Packet.ReadInt();
			if(Version != GAME_VERSION) {
				Message = "Game version differs from server's";
				ChangeState(STATE_MAIN);
			}
			else {
				Game.ChangeState(&AccountState);
			}
		}
		break;
	}
}

// Updates the current state
void ConnectStateClass::Update(u32 FrameTime) {

	switch(State) {
		case STATE_MAIN:
		break;
		case STATE_CONNECT:
		break;
	}
}

// Draws the current state
void ConnectStateClass::Draw() {

	Graphics.DrawImage(GraphicsClass::IMAGE_MENULOGO, 400, 125);

	// Server message
	if(Message.size() > 0) {
		Graphics.SetFont(GraphicsClass::FONT_10);
		Graphics.RenderText(Message.c_str(), 400, 200, GraphicsClass::ALIGN_CENTER, SColor(255, 255, 0, 0));
	}

	switch(State) {
		case STATE_MAIN:
		break;
		case STATE_CONNECT:
			Graphics.SetFont(GraphicsClass::FONT_10);
			Graphics.RenderText("Connecting...", 400, 250, GraphicsClass::ALIGN_CENTER);
		break;
	}

	irrGUI->drawAll();
}

// Key presses
bool ConnectStateClass::HandleKeyPress(EKEY_CODE Key) {

	switch(State) {
		case STATE_MAIN:
			switch(Key) {
				case KEY_ESCAPE:
					Game.ChangeState(&MainMenuState);
				break;
				case KEY_RETURN:
					ChangeState(STATE_CONNECT);
				break;
			}				
		break;
		case STATE_CONNECT:
			switch(Key) {
				case KEY_ESCAPE:
					ChangeState(STATE_MAIN);
				break;
			}		
		break;
	}
	
	return false;
}

// GUI events
void ConnectStateClass::HandleGUI(EGUI_EVENT_TYPE EventType, IGUIElement *Element) {
	switch(State) {
		case STATE_MAIN:
			switch(EventType) {
				case EGET_BUTTON_CLICKED:
					switch(Element->getID()) {
						case ELEMENT_CONNECT:
							ChangeState(STATE_CONNECT);
						break;
						case ELEMENT_CANCEL:
							Game.ChangeState(&MainMenuState);
						break;
					}
				break;
			}
		break;
	}
}

// Changes the internal state
void ConnectStateClass::ChangeState(int State) {

	switch(State) {
		case STATE_MAIN:
			this->State = State;
			Form->setVisible(true);
			ClientNetwork->Disconnect();
		break;
		case STATE_CONNECT: {
		
			if(ValidateForm() && ClientNetwork->Connect(IPAddress.c_str())) {
				Form->setVisible(false);
				Message = "";
				this->State = State;
			}
		}
		break;
	}	
}

// Validates the login form
bool ConnectStateClass::ValidateForm() {
	IPAddress = EditIP->getText();
	IPAddress.trim();

	if(IPAddress.size() == 0) {
		Message = "Invalid IP address";
		irrGUI->setFocus(EditIP);
		return false;
	}

	return true;
}
