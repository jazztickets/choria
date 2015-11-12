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
#include <states/mainmenu.h>
#include <framework.h>
#include <globals.h>
#include <graphics.h>
#include <constants.h>
#include <network/network.h>
#include <buffer.h>
#include <states/connect.h>
#include <states/mapeditor.h>
#include <states/characters.h>
#include <IGUIEnvironment.h>

_MainMenuState MainMenuState;

using namespace irr;

// Initializes the state
int _MainMenuState::Init() {
	int DrawX = 400, DrawY = 250, OffsetY = 50;

	// Single Player
	Graphics.SetFont(_Graphics::FONT_10);
	irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 100, 25), 0, ELEMENT_SINGLEPLAYER, L"Single Player");

	// Multiplayer
	DrawY += OffsetY;
	irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 100, 25), 0, ELEMENT_MULTIPLAYER, L"Multiplayer");

	// Editor
	DrawY += OffsetY;
	irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 100, 25), 0, ELEMENT_EDITOR, L"Map Editor");

	// Exit
	DrawY += OffsetY;
	irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 100, 25), 0, ELEMENT_EXIT, L"Exit");

	// Game version
	Graphics.AddText(GAME_VERSION, 10, 580);

	Framework.StopLocalServer();

	return 1;
}

// Shuts the state down
int _MainMenuState::Close() {

	return 1;
}

// Updates the current state
void _MainMenuState::Update(double FrameTime) {

}

// Draws the current state
void _MainMenuState::Draw() {

	Graphics.DrawImage(_Graphics::IMAGE_MENULOGO, 400, 125);

	irrGUI->drawAll();
}

// Key presses
bool _MainMenuState::HandleKeyPress(EKEY_CODE TKey) {

	switch(TKey) {
		case KEY_ESCAPE:
			Framework.SetDone(true);
		break;
		case KEY_RETURN:
			StartSinglePlayer();
		break;
		default:
		break;
	}

	return false;
}

// GUI events
void _MainMenuState::HandleGUI(gui::EGUI_EVENT_TYPE TEventType, gui::IGUIElement *TElement) {
	switch(TEventType) {
		case gui::EGET_BUTTON_CLICKED:
			switch(TElement->getID()) {
				case ELEMENT_SINGLEPLAYER:
					Framework.ChangeState(&ConnectState);
					StartSinglePlayer();
				break;
				case ELEMENT_MULTIPLAYER:
					Framework.ChangeState(&ConnectState);
				break;
				case ELEMENT_EDITOR:
					Framework.ChangeState(&MapEditorState);
				break;
				case ELEMENT_OPTIONS:
				break;
				case ELEMENT_EXIT:
					Framework.SetDone(true);
				break;
				default:
				break;
			}
		break;
		default:
		break;
	}
}

// Starts a single player game
void _MainMenuState::StartSinglePlayer() {
	Framework.StartLocalServer();
	ClientNetwork->Connect("");

	// Send fake account information
	_Buffer Packet;
	Packet.Write<char>(_Network::ACCOUNT_LOGININFO);
	Packet.WriteBit(0);
	Packet.WriteString("singleplayer");
	Packet.WriteString("singleplayer");
	ClientNetwork->SendPacketToHost(&Packet);
	Framework.ChangeState(&CharactersState);
}
