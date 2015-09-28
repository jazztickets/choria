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
#include <mainmenu.h>
#include <engine/game.h>
#include <engine/globals.h>
#include <engine/graphics.h>
#include <engine/constants.h>
#include <network/network.h>
#include <network/packetstream.h>
#include <connect.h>
#include <mapeditor.h>
#include <characters.h>

// Initializes the state
int MainMenuState::Init() {
	int DrawX = 400, DrawY = 250, OffsetY = 50;

	// Single Player
	Graphics.SetFont(GraphicsClass::FONT_10);
	irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 100, 25), 0, ELEMENT_SINGLEPLAYER, L"Single Player");

	// Multiplayer
	DrawY += OffsetY;
	irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 100, 25), 0, ELEMENT_MULTIPLAYER, L"Multiplayer");

	// Editor
	DrawY += OffsetY;
	irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 100, 25), 0, ELEMENT_EDITOR, L"Map Editor");

	// Options
	//DrawY += OffsetY;
	//IGUIButton *OptionButton = irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 100, 25), 0, ELEMENT_OPTIONS, L"Options");
	//OptionButton->setEnabled(false);

	// Exit
	DrawY += OffsetY;
	irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 100, 25), 0, ELEMENT_EXIT, L"Exit");

	// Game version
	Graphics.AddText(GAME_VERSION, 10, 580);

	Game.StopLocalServer();

	return 1;
}

// Shuts the state down
int MainMenuState::Close() {

	return 1;
}

// Updates the current state
void MainMenuState::Update(u32 TDeltaTime) {

}

// Draws the current state
void MainMenuState::Draw() {

	Graphics.DrawImage(GraphicsClass::IMAGE_MENULOGO, 400, 125);

	irrGUI->drawAll();
}

// Key presses
bool MainMenuState::HandleKeyPress(EKEY_CODE TKey) {

	switch(TKey) {
		case KEY_ESCAPE:
			Game.SetDone(true);
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
void MainMenuState::HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement) {
	switch(TEventType) {
		case EGET_BUTTON_CLICKED:
			switch(TElement->getID()) {
				case ELEMENT_SINGLEPLAYER:
					Game.ChangeState(ConnectState::Instance());
					StartSinglePlayer();
				break;
				case ELEMENT_MULTIPLAYER:
					Game.ChangeState(ConnectState::Instance());
				break;
				case ELEMENT_EDITOR:
					Game.ChangeState(MapEditorState::Instance());
				break;
				case ELEMENT_OPTIONS:
				break;
				case ELEMENT_EXIT:
					Game.SetDone(true);
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
void MainMenuState::StartSinglePlayer() {
	Game.StartLocalServer();
	ClientNetwork->Connect("");

	// Send fake account information
	PacketClass Packet(NetworkClass::ACCOUNT_LOGININFO);
	Packet.WriteBit(0);
	Packet.WriteString("singleplayer");
	Packet.WriteString("singleplayer");
	ClientNetwork->SendPacketToHost(&Packet);
	Game.ChangeState(CharactersState::Instance());
}
