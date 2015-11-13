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
#include <states/createcharacter.h>
#include <framework.h>
#include <globals.h>
#include <stats.h>
#include <graphics.h>
#include <network/network.h>
#include <buffer.h>
#include <states/connect.h>
#include <states/characters.h>
#include <IGUIEnvironment.h>
#include <IGUIButton.h>

_CreateCharacterState CreateCharacterState;

using namespace irr;

// Initializes the state
void _CreateCharacterState::Init() {

	// Get portrait list
	std::list<_Portrait> PortraitList;
	Stats.GetPortraits(PortraitList);

	// Create portraits
	int SlotX = 0, SlotY = 0, i = 0;
	for(std::list<_Portrait>::iterator Iterator = PortraitList.begin(); Iterator != PortraitList.end(); ++Iterator) {
		Portraits.push_back(Iterator->ID);
		gui::IGUIButton *Button = irrGUI->addButton(Graphics.GetCenteredRect(SlotX * 100 + 50, SlotY * 100 + 150, 64, 64), 0, ELEMENT_PORTRAITS + i);
		//Button->setImage(Iterator->Image);

		i++;
		SlotX++;
		if(SlotX > 7) {
			SlotX = 0;
			SlotY++;
		}
	}

	// Buttons
	int DrawX = 400, DrawY = 450, ButtonWidth = 80;
	Graphics.SetFont(_Graphics::FONT_10);
	Graphics.AddText("Name", DrawX, DrawY, _Graphics::ALIGN_CENTER);

	DrawY += 35;
	EditName = irrGUI->addEditBox(L"", Graphics.GetCenteredRect(DrawX, DrawY, 180, 25), true, 0, ELEMENT_NAME);
	EditName->setMax(10);

	DrawY += 50;
	ButtonCreate = irrGUI->addButton(Graphics.GetCenteredRect(DrawX - 50, DrawY, ButtonWidth, 25), 0, ELEMENT_CREATE, L"Create");
	ButtonBack = irrGUI->addButton(Graphics.GetCenteredRect(DrawX + 50, DrawY, ButtonWidth, 25), 0, ELEMENT_BACK, L"Back");

	Message = "";
	State = STATE_MAIN;
	SelectedIndex = -1;
	SelectedButton = nullptr;
}

// Shuts the state down
void _CreateCharacterState::Close() {

	Portraits.clear();
}

// Handles a disconnection from the server
void _CreateCharacterState::HandleDisconnect(ENetEvent *TEvent) {

	Framework.ChangeState(&ConnectState);
}

// Handles a server packet
void _CreateCharacterState::HandlePacket(ENetEvent *TEvent) {
	_Buffer Packet((char *)TEvent->packet->data, TEvent->packet->dataLength);
	switch(Packet.Read<char>()) {
		case _Network::CREATECHARACTER_SUCCESS:
			Framework.ChangeState(&CharactersState);
		break;
		case _Network::CREATECHARACTER_INUSE:
			Message = "Character name already in use";
			ButtonCreate->setEnabled(true);
			irrGUI->setFocus(EditName);
		break;
	}
}

// Updates the current state
void _CreateCharacterState::Update(double FrameTime) {

}

// Draws the current state
void _CreateCharacterState::Draw() {

	// Top text
	Graphics.SetFont(_Graphics::FONT_14);
	Graphics.RenderText("Create your character", 400, 10, _Graphics::ALIGN_CENTER);

	Graphics.SetFont(_Graphics::FONT_10);
	Graphics.RenderText("Select a picture", 400, 80, _Graphics::ALIGN_CENTER);

	// Message
	if(Message.size() > 0) {
		Graphics.SetFont(_Graphics::FONT_10);
		Graphics.RenderText(Message.c_str(), 400, 400, _Graphics::ALIGN_CENTER, video::SColor(255, 255, 0, 0));
	}

	// Draw GUI
	irrGUI->drawAll();

	// Draw selected box
	if(SelectedIndex != -1) {
		core::position2di ButtonPosition = SelectedButton->getAbsolutePosition().getCenter();
		Graphics.DrawImage(_Graphics::IMAGE_MENUSELECTED, ButtonPosition.X, ButtonPosition.Y);
	}
}

// Key presses
bool _CreateCharacterState::HandleKeyPress(EKEY_CODE TKey) {

	switch(TKey) {
		case KEY_ESCAPE:
			Back();
		break;
		case KEY_RETURN:
			CreateCharacter();
		break;
		default:
		break;
	}

	return false;
}

// GUI events
void _CreateCharacterState::HandleGUI(gui::EGUI_EVENT_TYPE TEventType, gui::IGUIElement *TElement) {

	int ID = TElement->getID();
	switch(TEventType) {
		case gui::EGET_BUTTON_CLICKED:
			switch(ID) {
				case ELEMENT_CREATE:
					CreateCharacter();
				break;
				case ELEMENT_BACK:
					Back();
				break;
				default:
					if(ID >= ELEMENT_PORTRAITS) {
						SelectedButton = static_cast<gui::IGUIButton *>(TElement);
						UpdateSelection(ID - ELEMENT_PORTRAITS);
					}
				break;
			}
		break;
		default:
		break;
	}
}

// Handles the selection of a portrait
void _CreateCharacterState::UpdateSelection(int TSelectedIndex) {
	SelectedIndex = TSelectedIndex;
}

// Send character info to the server
void _CreateCharacterState::CreateCharacter() {

	// Get picture
	if(SelectedIndex == -1) {
		Message = "Select a picture first";
		return;
	}

	// Get name
	core::stringc Name = EditName->getText();
	Name.trim();
	if(Name.size() == 0) {
		Message = "Enter your name";
		return;
	}

	ButtonCreate->setEnabled(false);

	// Send information
	_Buffer Packet;
	Packet.Write<char>(_Network::CREATECHARACTER_INFO);
	Packet.WriteString(Name.c_str());
	Packet.Write<int32_t>(Portraits[SelectedIndex]);
	ClientNetwork->SendPacketToHost(&Packet);
}

// Back to character select
void _CreateCharacterState::Back() {
	Framework.ChangeState(&CharactersState);
}
