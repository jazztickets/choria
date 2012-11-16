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
#include "mainmenu.h"
#include "engine/game.h"
#include "engine/globals.h"
#include "engine/graphics.h"
#include "engine/constants.h"
#include "network/network.h"
#include "network/packetstream.h"
#include "connect.h"
#include "characters.h"

MainMenuStateClass MainMenuState;

// Initializes the state
int MainMenuStateClass::Init() {
	int DrawX = 400, DrawY = 250, OffsetY = 50;

	// Single Player
	Graphics.SetFont(GraphicsClass::FONT_10);
	IGUIButton *SinglePlayerButton = irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 100, 25), 0, ELEMENT_SINGLEPLAYER, L"Single Player");

	// Multiplayer
	DrawY += OffsetY;
	IGUIButton *MultiplayerButton = irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 100, 25), 0, ELEMENT_MULTIPLAYER, L"Multiplayer");

	// Options
	DrawY += OffsetY;
	IGUIButton *OptionButton = irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 100, 25), 0, ELEMENT_OPTIONS, L"Options");
	OptionButton->setEnabled(false);

	// Exit
	DrawY += OffsetY;
	IGUIButton *ExitButton = irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 100, 25), 0, ELEMENT_EXIT, L"Exit");

	// Game version
	Graphics.AddText(GAME_VERSIONSTR, 10, 580);

	Game.StopLocalServer();

	return 1;
}

// Shuts the state down
int MainMenuStateClass::Close() {

	return 1;
}

// Updates the current state
void MainMenuStateClass::Update(u32 FrameTime) {

}

// Draws the current state
void MainMenuStateClass::Draw() {

	Graphics.DrawImage(GraphicsClass::IMAGE_MENULOGO, 400, 125);

	irrGUI->drawAll();
}

// Key presses
bool MainMenuStateClass::HandleKeyPress(EKEY_CODE Key) {

	switch(Key) {
		case KEY_ESCAPE:
			Game.SetDone(true);
		break;
		case KEY_RETURN:
			StartSinglePlayer();
		break;
	}

	return false;
}

// GUI events
void MainMenuStateClass::HandleGUI(EGUI_EVENT_TYPE EventType, IGUIElement *Element) {
	switch(EventType) {
		case EGET_BUTTON_CLICKED:
			switch(Element->getID()) {
				case ELEMENT_SINGLEPLAYER:
					Game.ChangeState(&ConnectState);
					StartSinglePlayer();
				break;
				case ELEMENT_MULTIPLAYER:
					Game.ChangeState(&ConnectState);
				break;
				case ELEMENT_OPTIONS:
				break;
				case ELEMENT_EXIT:
					Game.SetDone(true);
				break;
			}
		break;
	}
}

// Starts a single player game
void MainMenuStateClass::StartSinglePlayer() {
	Game.StartLocalServer();
	ClientNetwork->Connect("");

	// Send fake account information
	PacketClass Packet(NetworkClass::ACCOUNT_LOGININFO);
	Packet.WriteBit(0);
	Packet.WriteString("singleplayer");
	Packet.WriteString("singleplayer");
	ClientNetwork->SendPacketToHost(&Packet);
	Game.ChangeState(&CharactersState);
}
