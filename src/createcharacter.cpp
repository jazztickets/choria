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
#include "createcharacter.h"
#include "engine/game.h"
#include "engine/globals.h"
#include "engine/stats.h"
#include "engine/graphics.h"
#include "network/network.h"
#include "network/packetstream.h"
#include "connect.h"
#include "characters.h"

// Initializes the state
int CreateCharacterState::Init() {

	// Get portrait list
	list<PortraitStruct> PortraitList;
	Stats::Instance().GetPortraitList(PortraitList);

	// Create portraits
	int SlotX = 0, SlotY = 0, i = 0;
	for(list<PortraitStruct>::Iterator Iterator = PortraitList.begin(); Iterator != PortraitList.end(); ++Iterator) {
		Portraits.push_back(Iterator->ID);
		IGUIButton *Button = irrGUI->addButton(Graphics::Instance().GetCenteredRect(SlotX * 100 + 50, SlotY * 100 + 150, 64, 64), 0, ELEMENT_PORTRAITS + i);
		Button->setImage(Iterator->Image);

		i++;
		SlotX++;
		if(SlotX > 7) {
			SlotX = 0;
			SlotY++;
		}
	}

	// Buttons
	int DrawX = 400, DrawY = 450, ButtonWidth = 80;
	Graphics::Instance().SetFont(GraphicsClass::FONT_10);
	Graphics::Instance().AddText("Name", DrawX, DrawY, GraphicsClass::ALIGN_CENTER);

	DrawY += 35;
	EditName = irrGUI->addEditBox(L"", Graphics::Instance().GetCenteredRect(DrawX, DrawY, 180, 25), true, 0, ELEMENT_NAME);
	EditName->setMax(10);

	DrawY += 50;
	ButtonCreate = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX - 50, DrawY, ButtonWidth, 25), 0, ELEMENT_CREATE, L"Create");
	ButtonBack = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX + 50, DrawY, ButtonWidth, 25), 0, ELEMENT_BACK, L"Back");

	Message = "";
	State = STATE_MAIN;
	SelectedIndex = -1;
	SelectedButton = NULL;

	return 1;
}

// Shuts the state down
int CreateCharacterState::Close() {

	Portraits.clear();

	return 1;
}

// Handles a disconnection from the server
void CreateCharacterState::HandleDisconnect(ENetEvent *TEvent) {

	Game::Instance().ChangeState(ConnectState::Instance());
}

// Handles a server packet
void CreateCharacterState::HandlePacket(ENetEvent *TEvent) {
	PacketClass Packet(TEvent->packet);
	switch(Packet.ReadChar()) {
		case NetworkClass::CREATECHARACTER_SUCCESS:
			Game::Instance().ChangeState(CharactersState::Instance());
		break;
		case NetworkClass::CREATECHARACTER_INUSE:
			Message = "Character name already in use";
			ButtonCreate->setEnabled(true);
			irrGUI->setFocus(EditName);
		break;
	}
}

// Updates the current state
void CreateCharacterState::Update(u32 TDeltaTime) {

}

// Draws the current state
void CreateCharacterState::Draw() {

	// Top text
	Graphics::Instance().SetFont(GraphicsClass::FONT_14);
	Graphics::Instance().RenderText("Create your character", 400, 10, GraphicsClass::ALIGN_CENTER);

	Graphics::Instance().SetFont(GraphicsClass::FONT_10);
	Graphics::Instance().RenderText("Select a picture", 400, 80, GraphicsClass::ALIGN_CENTER);

	// Message
	if(Message.size() > 0) {
		Graphics::Instance().SetFont(GraphicsClass::FONT_10);
		Graphics::Instance().RenderText(Message.c_str(), 400, 400, GraphicsClass::ALIGN_CENTER, SColor(255, 255, 0, 0));
	}

	// Draw GUI
	irrGUI->drawAll();

	// Draw selected box
	if(SelectedIndex != -1) {
		position2di ButtonPosition = SelectedButton->getAbsolutePosition().getCenter();
		Graphics::Instance().DrawImage(GraphicsClass::IMAGE_MENUSELECTED, ButtonPosition.X, ButtonPosition.Y);
	}
}

// Key presses
bool CreateCharacterState::HandleKeyPress(EKEY_CODE TKey) {

	switch(TKey) {
		case KEY_ESCAPE:
			Back();
		break;
		case KEY_RETURN:
			CreateCharacter();
		break;
	}				

	return false;
}

// GUI events
void CreateCharacterState::HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement) {

	int ID = TElement->getID();
	switch(TEventType) {
		case EGET_BUTTON_CLICKED:
			switch(ID) {
				case ELEMENT_CREATE:
					CreateCharacter();
				break;
				case ELEMENT_BACK:
					Back();					
				break;
				default:
					if(ID >= ELEMENT_PORTRAITS) {
						SelectedButton = static_cast<IGUIButton *>(TElement);
						UpdateSelection(ID - ELEMENT_PORTRAITS);
					}
				break;
			}
		break;
	}
}

// Handles the selection of a portrait
void CreateCharacterState::UpdateSelection(int TSelectedIndex) {
	SelectedIndex = TSelectedIndex;
}

// Send character info to the server
void CreateCharacterState::CreateCharacter() {

	// Get picture
	if(SelectedIndex == -1) {
		Message = "Select a picture first";
		return;
	}

	// Get name
	stringc Name = EditName->getText();
	Name.trim();
	if(Name.size() == 0) {
		Message = "Enter your name";
		return;
	}

	ButtonCreate->setEnabled(false);

	// Send information
	PacketClass Packet(NetworkClass::CREATECHARACTER_INFO);
	Packet.WriteString(Name.c_str());
	Packet.WriteInt(Portraits[SelectedIndex]);
	ClientNetwork->SendPacketToHost(&Packet);
}

// Back to character select
void CreateCharacterState::Back() {
	Game::Instance().ChangeState(CharactersState::Instance());
}
