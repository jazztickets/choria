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
#include "characters.h"
#include "engine/game.h"
#include "engine/globals.h"
#include "engine/stats.h"
#include "engine/graphics.h"
#include "network/network.h"
#include "network/packetstream.h"
#include "mainmenu.h"
#include "connect.h"
#include "account.h"
#include "createcharacter.h"
#include "playclient.h"

// Initializes the state
int CharactersState::Init() {

	// Create character slots
	int SlotX = 0, SlotY = 0;
	for(int i = 0; i < CHARACTERS_MAX; i++) {
		Slots[i].Button = irrGUI->addButton(Graphics::Instance().GetCenteredRect(SlotX * 200 + 200, SlotY * 150 + 150, 64, 64), 0, ELEMENT_SLOT0 + i);
		Slots[i].Button->setImage(Graphics::Instance().GetImage(GraphicsClass::IMAGE_MENUBLANKSLOT));
		Slots[i].Used = false;
		Slots[i].Name = "";
		Slots[i].Level = 0;

		SlotX++;
		if(SlotX > 2) {
			SlotX = 0;
			SlotY++;
		}
	}

	// Buttons
	int DrawX = 400, DrawY = 500, ButtonWidth = 80;
	Graphics::Instance().SetFont(GraphicsClass::FONT_10);
	ButtonPlay = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX - 180, DrawY, ButtonWidth, 25), 0, ELEMENT_PLAY, L"Play");
	ButtonCreate = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX - 60, DrawY, ButtonWidth, 25), 0, ELEMENT_CREATE, L"Create");
	ButtonDelete = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX + 60, DrawY, ButtonWidth, 25), 0, ELEMENT_DELETE, L"Delete");
	ButtonLogout = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX + 180, DrawY, ButtonWidth, 25), 0, ELEMENT_LOGOUT, L"Logout");
	ButtonCreate->setEnabled(false);
	ButtonPlay->setEnabled(false);
	ButtonDelete->setEnabled(false);

	// Get character list
	RequestCharacterList();

	SelectedIndex = -1;

	return 1;
}

// Shuts the state down
int CharactersState::Close() {

	return 1;
}

// Handles a disconnection from the server
void CharactersState::HandleDisconnect(ENetEvent *TEvent) {

	Game::Instance().ChangeState(ConnectState::Instance());
}

// Handles a server packet
void CharactersState::HandlePacket(ENetEvent *TEvent) {
	PacketClass Packet(TEvent->packet);
	switch(Packet.ReadChar()) {
		case NetworkClass::CHARACTERS_LIST:
			HandleCharacterList(&Packet);
		break;
	}
}

// Updates the current state
void CharactersState::Update(u32 TDeltaTime) {
	ClickTimer += TDeltaTime;
}

// Draws the current state
void CharactersState::Draw() {

	// Top text
	Graphics::Instance().SetFont(GraphicsClass::FONT_14);
	Graphics::Instance().RenderText("Select a slot", 400, 10, GraphicsClass::ALIGN_CENTER);
	Graphics::Instance().SetFont(GraphicsClass::FONT_10);

	// Draw character text
	position2di TextPosition;
	char Buffer[256];
	for(int i = 0; i < CHARACTERS_MAX; i++) {
		TextPosition = Slots[i].Button->getAbsolutePosition().getCenter();

		if(Slots[i].Used) {
			Graphics::Instance().SetFont(GraphicsClass::FONT_14);
			Graphics::Instance().RenderText(Slots[i].Name.c_str(), TextPosition.X, TextPosition.Y + 35, GraphicsClass::ALIGN_CENTER);

			sprintf(Buffer, "Level %d", Slots[i].Level);
			Graphics::Instance().SetFont(GraphicsClass::FONT_10);
			Graphics::Instance().RenderText(Buffer, TextPosition.X, TextPosition.Y + 57, GraphicsClass::ALIGN_CENTER);
		}
		else {
			Graphics::Instance().RenderText("Empty", TextPosition.X, TextPosition.Y + 35, GraphicsClass::ALIGN_CENTER);
		}
	}

	// Draw GUI
	irrGUI->drawAll();

	// Draw selected box
	if(SelectedIndex != -1) {
		position2di ButtonPosition = Slots[SelectedIndex].Button->getAbsolutePosition().getCenter();
		Graphics::Instance().DrawImage(GraphicsClass::IMAGE_MENUSELECTED, ButtonPosition.X, ButtonPosition.Y);
	}
}

// Key presses
bool CharactersState::HandleKeyPress(EKEY_CODE TKey) {

	switch(TKey) {
		case KEY_ESCAPE:
			Logout();
		break;
		case KEY_RETURN:
			if(Slots[0].Used) {
				SelectedIndex = 0;
				PlayCharacter();
			}
		break;
	}

	return false;
}

// GUI events
void CharactersState::HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement) {

	switch(TEventType) {
		case EGET_BUTTON_CLICKED:
			switch(TElement->getID()) {
				case ELEMENT_PLAY:
					PlayCharacter();
				break;
				case ELEMENT_LOGOUT:
					Logout();
				break;
				case ELEMENT_CREATE:
					Game::Instance().ChangeState(CreateCharacterState::Instance());
				break;
				case ELEMENT_DELETE:
					irrGUI->addMessageBox(L"", L"Are you sure you want to delete this character?", true, EMBF_YES | EMBF_NO, 0, ELEMENT_DELETECONFIRM);
				break;
				case ELEMENT_SLOT0:
				case ELEMENT_SLOT1:
				case ELEMENT_SLOT2:
				case ELEMENT_SLOT3:
				case ELEMENT_SLOT4:
				case ELEMENT_SLOT5:
					UpdateSelection(TElement->getID() - ELEMENT_SLOT0);
				break;
			}
		break;
		case EGET_MESSAGEBOX_YES:
			switch(TElement->getID()) {
				case ELEMENT_DELETECONFIRM:
					Delete();
				break;
			}
		break;
	}

}

// Process the character list packet
void CharactersState::HandleCharacterList(PacketClass *TPacket) {

	// Get count
	int CharacterCount = TPacket->ReadChar();
	if(CharacterCount < CHARACTERS_MAX)
		ButtonCreate->setEnabled(true);

	// Get characters
	ITexture *PortraitImage;
	int i;
	for(i = 0; i < CharacterCount; i++) {
		Slots[i].Used = true;
		Slots[i].Name = TPacket->ReadString();
		PortraitImage = Stats::Instance().GetPortrait(TPacket->ReadInt())->Image;
		Slots[i].Button->setImage(PortraitImage);
		Slots[i].Button->setPressedImage(PortraitImage);
		Slots[i].Level = Stats::Instance().FindLevel(TPacket->ReadInt())->Level;
	}
	for(; i < CHARACTERS_MAX; i++) {
		Slots[i].Used = false;
		PortraitImage = Graphics::Instance().GetImage(GraphicsClass::IMAGE_MENUBLANKSLOT);
		Slots[i].Button->setImage(PortraitImage);
		Slots[i].Button->setPressedImage(PortraitImage);
	}
}

// Requests a character list from the server
void CharactersState::RequestCharacterList() {

	PacketClass Packet(NetworkClass::CHARACTERS_REQUEST);
	ClientNetwork->SendPacketToHost(&Packet);
}

// Handles the selection of a slot
void CharactersState::UpdateSelection(int TSelectedIndex) {

	int OldIndex = SelectedIndex;
	SelectedIndex = TSelectedIndex;

	// Check for double clicks
	if(SelectedIndex == OldIndex && ClickTimer < 500) {
		PlayCharacter();
	}

	if(Slots[SelectedIndex].Used) {
		ButtonPlay->setEnabled(true);
		ButtonDelete->setEnabled(true);
	}
	else {
		ButtonPlay->setEnabled(false);
		ButtonDelete->setEnabled(false);
	}

	ClickTimer = 0;
}

// Plays a character
void CharactersState::PlayCharacter() {
	if(SelectedIndex == -1 || !Slots[SelectedIndex].Used)
		return;

	PlayClientState::Instance()->SetCharacterSlot(SelectedIndex);
	Game::Instance().ChangeState(PlayClientState::Instance());
}

// Delete a character
void CharactersState::Delete() {
	if(SelectedIndex == -1 || !Slots[SelectedIndex].Used)
		return;

	PacketClass Packet(NetworkClass::CHARACTERS_DELETE);
	Packet.WriteChar(SelectedIndex);
	ClientNetwork->SendPacketToHost(&Packet);
}

// Logout
void CharactersState::Logout() {
	if(Game::Instance().IsLocalServerRunning()) {
		ClientNetwork->Disconnect();
		Game::Instance().ChangeState(MainMenuState::Instance());
	}
	else
		Game::Instance().ChangeState(AccountState::Instance());
}
