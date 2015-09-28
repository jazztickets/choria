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
#include "hud.h"
#include "game.h"
#include "graphics.h"
#include "globals.h"
#include "input.h"
#include "stats.h"
#include "constants.h"
#include "../playclient.h"
#include "../network/network.h"
#include "../network/packetstream.h"
#include "../instances/map.h"
#include "../objects/player.h"
#include "../objects/item.h"

struct PositionStruct {
	int X;
	int Y;
};

static PositionStruct EquippedItemPositions[PlayerClass::INVENTORY_BACKPACK] = {
	{66, 20},
	{66, 78},
	{64, 162},
	{25, 106},
	{105, 108},
	{25, 146},
	{105, 146}
};

// Initialize
int HUDClass::Init() {

	State = PlayClientState::Instance()->GetState();
	Vendor = NULL;
	Trader = NULL;
	TooltipItem.Reset();
	CursorItem.Reset();
	TooltipSkill.Reset();
	CursorSkill.Reset();
	CharacterOpen = false;
	Chatting = false;
	TypingGold = false;
	TradeAcceptButton = NULL;
	RewardItemSlot = -1;
	ChatBox = NULL;
	ChatHistory.clear();

	InitButtonBar();

	return 1;
}

// Shutdown
int HUDClass::Close() {

	return 1;
}

// Handles key presses
void HUDClass::HandleKeyPress(EKEY_CODE TKey) {

	if(Chatting) {
		if(TKey == KEY_ESCAPE) {
			CloseChat();
		}
	}
	if(TypingGold) {
		if(TKey == KEY_ESCAPE) {
			irrGUI->removeFocus(TradeGoldBox);
		}
	}
}

// Handles mouse movement
void HUDClass::HandleMouseMotion(int TMouseX, int TMouseY) {
	position2di MousePosition(TMouseX, TMouseY);

	switch(*State) {
		case PlayClientState::STATE_VENDOR:
		case PlayClientState::STATE_TRADER:
		case PlayClientState::STATE_TRADE:
		case PlayClientState::STATE_INVENTORY:
			GetItem(MousePosition, TooltipItem);
		break;
		case PlayClientState::STATE_SKILLS:
			GetSkill(MousePosition, TooltipSkill);
		break;
	}
}

// Handles mouse presses
bool HUDClass::HandleMousePress(int TButton, int TMouseX, int TMouseY) {

	switch(*State) {
		case PlayClientState::STATE_VENDOR:
			switch(TButton) {
				case InputClass::MOUSE_LEFT:
					if(TooltipItem.Item) {
						CursorItem = TooltipItem;
					}
				break;
				case InputClass::MOUSE_RIGHT:
					if(TooltipItem.Item) {
						if(TooltipItem.Window == WINDOW_VENDOR)
							BuyItem(&TooltipItem, -1);
						else if(TooltipItem.Window == WINDOW_INVENTORY && Input::Instance().IsShiftDown())
							SellItem(&TooltipItem, 1);
					}
				break;
			}
		break;
		case PlayClientState::STATE_INVENTORY:
			switch(TButton) {
				case InputClass::MOUSE_LEFT:
					if(TooltipItem.Item) {
						if(Input::Instance().IsControlDown())
							SplitStack(TooltipItem.Slot, 1);
						else
							CursorItem = TooltipItem;
					}
				break;
				case InputClass::MOUSE_RIGHT:
					if(Player->UseInventory(TooltipItem.Slot)) {
						PacketClass Packet(NetworkClass::INVENTORY_USE);
						Packet.WriteChar(TooltipItem.Slot);
						ClientNetwork->SendPacketToHost(&Packet);
					}
				break;
			}
		break;
		case PlayClientState::STATE_TRADE:
			switch(TButton) {
				case InputClass::MOUSE_LEFT:
					if(TooltipItem.Item && TooltipItem.Window != WINDOW_TRADETHEM) {
						if(TooltipItem.Window == WINDOW_INVENTORY && Input::Instance().IsControlDown())
							SplitStack(TooltipItem.Slot, 1);
						else
							CursorItem = TooltipItem;
					}
				break;
			}
		break;
		case PlayClientState::STATE_SKILLS:
			switch(TButton) {
				case InputClass::MOUSE_LEFT:
					if(TooltipSkill.Skill && Player->GetSkillLevel(TooltipSkill.Skill->GetID()) > 0)
						CursorSkill = TooltipSkill;
				break;
			}
		break;
	}

	return false;
}

// Handles mouse release
void HUDClass::HandleMouseRelease(int TButton, int TMouseX, int TMouseY) {

	switch(*State) {
		case PlayClientState::STATE_INVENTORY:
		case PlayClientState::STATE_VENDOR:
		case PlayClientState::STATE_TRADE:
			if(TButton == InputClass::MOUSE_LEFT) {

				// Check for valid slots
				if(CursorItem.Item) {

					switch(CursorItem.Window) {
						// Coming from inventory
						case WINDOW_TRADEYOU:
						case WINDOW_INVENTORY:

							switch(TooltipItem.Window) {
								// Send inventory move packet
								case WINDOW_TRADEYOU:
								case WINDOW_INVENTORY:

									if(TooltipItem.Slot >= 0 && Player->MoveInventory(CursorItem.Slot, TooltipItem.Slot)) {
										PacketClass Packet(NetworkClass::INVENTORY_MOVE);
										Packet.WriteChar(CursorItem.Slot);
										Packet.WriteChar(TooltipItem.Slot);
										ClientNetwork->SendPacketToHost(&Packet);

										ResetAcceptButton();
										Player->CalculatePlayerStats();
									}
								break;
								// Sell an item
								case WINDOW_VENDOR: {
									SellItem(&CursorItem, 1);
								}
								break;
							}
						break;
						// Buy an item
						case WINDOW_VENDOR:
							if(TooltipItem.Window == WINDOW_INVENTORY) {
								BuyItem(&CursorItem, TooltipItem.Slot);
							}
						break;
					}
				}

				CursorItem.Reset();
			}
		break;
		case PlayClientState::STATE_SKILLS:
			if(TButton == InputClass::MOUSE_LEFT) {

				// Check for valid slots
				if(CursorSkill.Skill) {

					if(TooltipSkill.Window == WINDOW_SKILLBAR) {
						if(CursorSkill.Window == WINDOW_SKILLS)
							SetSkillBar(TooltipSkill.Slot, -1, CursorSkill.Skill);
						else if(CursorSkill.Window == WINDOW_SKILLBAR)
							SetSkillBar(TooltipSkill.Slot, CursorSkill.Slot, CursorSkill.Skill);
					}
					else if(CursorSkill.Window == WINDOW_SKILLBAR && TooltipSkill.Slot == -1) {
						SetSkillBar(CursorSkill.Slot, -1, NULL);
					}
				}

				CursorSkill.Reset();
			}
		break;
	}
}

// Handles GUI presses
void HUDClass::HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement) {
	int ID = TElement->getID();

	if(TEventType == EGET_BUTTON_CLICKED && ID == ELEMENT_CHARACTER) {
		InitCharacter();
	}

	switch(*State) {
		case PlayClientState::STATE_WALK:
			switch(TEventType) {
				case EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_TOWNPORTAL:
							ToggleTownPortal();
						break;
						case ELEMENT_INVENTORY:
							InitInventory();
						break;
						case ELEMENT_TRADE:
							InitTrade();
						break;
						case ELEMENT_SKILLS:
							InitSkills();
						break;
						case ELEMENT_MAINMENU:
							InitMenu();
						break;
					}
				break;
			}
		break;
		case PlayClientState::STATE_MAINMENU:
			switch(TEventType) {
				case EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_MAINMENU:
						case ELEMENT_MAINMENURESUME:
							CloseMenu();
						break;
						case ELEMENT_MAINMENUEXIT:
							ClientNetwork->Disconnect();
						break;
					}
				break;
			}
		break;
		case PlayClientState::STATE_TOWNPORTAL:
			switch(TEventType) {
				case EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_TOWNPORTAL:
							ToggleTownPortal();
						break;
					}
				break;
			}
		break;
		case PlayClientState::STATE_INVENTORY:
			switch(TEventType) {
				case EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_INVENTORY:
							CloseWindows();
						break;
					}
				break;
			}
		break;
		case PlayClientState::STATE_TRADER:
			switch(TEventType) {
				case EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_TRADERACCEPT: {
							PacketClass Packet(NetworkClass::TRADER_ACCEPT);
							ClientNetwork->SendPacketToHost(&Packet);
							Player->AcceptTrader(Trader, RequiredItemSlots, RewardItemSlot);
							Player->CalculatePlayerStats();
							CloseWindows();
						}
						break;
						case ELEMENT_TRADERCANCEL:
							CloseWindows();
						break;
					}
				break;
			}
		break;
		case PlayClientState::STATE_TRADE:
			switch(TEventType) {
				case EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_TRADE:
							CloseWindows();
						break;
						case ELEMENT_TRADEACCEPT: {
							PacketClass Packet(NetworkClass::TRADE_ACCEPT);
							Packet.WriteChar(TradeAcceptButton->isPressed());
							ClientNetwork->SendPacketToHost(&Packet);
						}
						break;
					}
				break;
				case EGET_ELEMENT_FOCUSED:
					if(ID == ELEMENT_GOLDTRADEBOX) {
						TypingGold = true;
					}
				break;
				case EGET_ELEMENT_FOCUS_LOST:
				case EGET_EDITBOX_ENTER:
					if(ID == ELEMENT_GOLDTRADEBOX) {
						TypingGold = false;
						if(TEventType == EGET_EDITBOX_ENTER)
							irrGUI->removeFocus(TradeGoldBox);

						// Send amount
						int GoldAmount = ValidateTradeGold();
						PacketClass Packet(NetworkClass::TRADE_GOLD);
						Packet.WriteInt(GoldAmount);
						ClientNetwork->SendPacketToHost(&Packet);

						// Reset agreement
						ResetAcceptButton();
					}
				break;
			}
		break;
		case PlayClientState::STATE_SKILLS:
			switch(TEventType) {
				case EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_SKILLS:
							CloseWindows();
						break;
					}

					// Skills
					if(ID >= ELEMENT_SKILLPLUS0) {
						PacketClass Packet(NetworkClass::SKILLS_SKILLADJUST);

						// Buy or sell skill
						int SkillID;
						if(ID >= ELEMENT_SKILLMINUS0) {
							SkillID = ID - ELEMENT_SKILLMINUS0;
							Packet.WriteBit(0);
							Player->AdjustSkillLevel(SkillID, -1);
						}
						else {
							SkillID = ID - ELEMENT_SKILLPLUS0;
							Packet.WriteBit(1);
							Player->AdjustSkillLevel(SkillID, 1);
							if(Player->GetSkillLevel(SkillID) == 1) {

								// Equip new skills
								const SkillClass *Skill = Stats::Instance().GetSkill(SkillID);
								if(Skill) {
									int Direction, Slot;
									if(Skill->GetType() == SkillClass::TYPE_PASSIVE) {
										Slot = 7;
										Direction = -1;
									}
									else {
										Slot = 0;
										Direction = 1;
									}
									for(int i = 0; i < 8; i++) {
										if(Player->GetSkillBar(Slot) == NULL) {
											SetSkillBar(Slot, -1, Skill);
											break;
										}
										Slot += Direction;
									}
								}
							}
						}
						Packet.WriteChar(SkillID);
						ClientNetwork->SendPacketToHost(&Packet);

						// Update player
						Player->CalculateSkillPoints();
						Player->CalculatePlayerStats();
						RefreshSkillButtons();
					}
				break;
			}
		break;
	}
}

// Updates the HUD
void HUDClass::Update(u32 TDeltaTime) {

	// Chat messages
	for(list<ChatStruct>::Iterator Iterator = ChatHistory.begin(); Iterator != ChatHistory.end();) {
		ChatStruct &Chat = (*Iterator);
		Chat.TimeOut += TDeltaTime;
		if(Chat.TimeOut > MESSAGE_TIME) {
			Iterator = ChatHistory.erase(Iterator);
		}
		else {
			++Iterator;
		}
	}
}

// Draws the HUD before irrGUI
void HUDClass::PreGUIDraw() {
	switch(*State) {
		case PlayClientState::STATE_MAINMENU:
			Graphics::Instance().DrawBackground(GraphicsClass::IMAGE_BLACK, 0, 0, 800, 600, SColor(100, 255, 255, 255));
		break;
		case PlayClientState::STATE_SKILLS: {
			rect<s32> WindowArea = TabSkill->getAbsolutePosition();

			// Draw background
			Graphics::Instance().DrawBackground(GraphicsClass::IMAGE_BLACK, WindowArea.UpperLeftCorner.X, WindowArea.UpperLeftCorner.Y, WindowArea.getWidth(), WindowArea.getHeight(), SColor(220, 255, 255, 255));
		}
		break;
		case PlayClientState::STATE_TRADE: {
			rect<s32> WindowArea = TabTrade->getAbsolutePosition();

			// Draw background
			Graphics::Instance().DrawBackground(GraphicsClass::IMAGE_BLACK, WindowArea.UpperLeftCorner.X-1, WindowArea.UpperLeftCorner.Y, WindowArea.getWidth()-1, WindowArea.getHeight(), SColor(220, 255, 255, 255));
		}
		break;
	}
}

// Draws the HUD elements
void HUDClass::Draw() {
	DrawTopHUD();

	switch(*State) {
		case PlayClientState::STATE_INVENTORY:
			DrawInventory();
		break;
		case PlayClientState::STATE_VENDOR:
			DrawVendor();
			DrawInventory();
		break;
		case PlayClientState::STATE_TRADER:
			DrawTrader();
		break;
		case PlayClientState::STATE_SKILLS:
			DrawSkills();
		break;
		case PlayClientState::STATE_TRADE:
			DrawTrade();
			DrawInventory();
		break;
		case PlayClientState::STATE_TOWNPORTAL:
			DrawTownPortal();
		break;
	}

	if(CharacterOpen)
		DrawCharacter();

	// Draw item information
	DrawCursorItem();
	DrawItemTooltip();

	// Draw skill information
	DrawCursorSkill();
	DrawSkillTooltip();

	DrawChat();
}

// Starts the chat box
void HUDClass::ToggleChat() {

	if(Chatting) {
		stringc Message = stringc(ChatBox->getText());
		Message.trim();
		if(Message != "") {

			// Add message to history
			ChatStruct Chat;
			Chat.Message = Player->GetName() + stringc(": ") + Message;
			ChatHistory.push_back(Chat);
			printf("%s\n", Chat.Message.c_str());

			// Send message to server
			PacketClass Packet(NetworkClass::CHAT_MESSAGE);
			Packet.WriteString(Message.c_str());
			ClientNetwork->SendPacketToHost(&Packet);
		}

		CloseChat();
	}
	else {
		ChatBox = irrGUI->addEditBox(L"", Graphics::Instance().GetRect(25, 570, 200, 20), true, NULL, ELEMENT_CHATBOX);
		ChatBox->setOverrideFont(Graphics::Instance().GetFont(GraphicsClass::FONT_10));
		ChatBox->setMax(NETWORKING_MESSAGESIZE);
		irrGUI->setFocus(ChatBox);
		Chatting = true;
	}
}

// Closes the chat window
void HUDClass::CloseChat() {
	irrGUI->getRootGUIElement()->removeChild(ChatBox);
	Chatting = false;
}

// Creates the buttons for the HUD bar
void HUDClass::InitButtonBar() {

	int DrawX = 800 - 24 * 6 + 25/2, DrawY = 600 - 25 / 2;
	IGUIButton *Button;
	IGUIStaticText *ButtonText;

	// Town portal
	Button = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX, DrawY, 25, 25), NULL, ELEMENT_TOWNPORTAL, 0, L"Town Portal");
	Button->setImage(irrDriver->getTexture(WorkingDirectory + "textures/interface/hud_spawn.png"));
	ButtonText = Graphics::Instance().AddText("Q", 2, 12, GraphicsClass::ALIGN_LEFT, Button);
	ButtonText->setOverrideFont(Graphics::Instance().GetFont(GraphicsClass::FONT_7));
	DrawX += 24;

	// Inventory
	Button = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX, DrawY, 25, 25), NULL, ELEMENT_INVENTORY, 0, L"Inventory");
	Button->setImage(irrDriver->getTexture(WorkingDirectory + "textures/interface/hud_inventory.png"));
	ButtonText = Graphics::Instance().AddText("C", 2, 12, GraphicsClass::ALIGN_LEFT, Button);
	ButtonText->setOverrideFont(Graphics::Instance().GetFont(GraphicsClass::FONT_7));
	DrawX += 24;

	// Trade
	Button = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX, DrawY, 25, 25), NULL, ELEMENT_TRADE, 0, L"Trade");
	Button->setImage(irrDriver->getTexture(WorkingDirectory + "textures/interface/hud_trade.png"));
	ButtonText = Graphics::Instance().AddText("T", 2, 12, GraphicsClass::ALIGN_LEFT, Button);
	ButtonText->setOverrideFont(Graphics::Instance().GetFont(GraphicsClass::FONT_7));
	DrawX += 24;

	// Character
	Button = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX, DrawY, 25, 25), NULL, ELEMENT_CHARACTER, 0, L"Character");
	Button->setImage(irrDriver->getTexture(WorkingDirectory + "textures/interface/hud_character.png"));
	ButtonText = Graphics::Instance().AddText("B", 2, 12, GraphicsClass::ALIGN_LEFT, Button);
	ButtonText->setOverrideFont(Graphics::Instance().GetFont(GraphicsClass::FONT_7));
	DrawX += 24;

	// Skills
	Button = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX, DrawY, 25, 25), NULL, ELEMENT_SKILLS, 0, L"Skills");
	Button->setImage(irrDriver->getTexture(WorkingDirectory + "textures/interface/hud_skills.png"));
	ButtonText = Graphics::Instance().AddText("S", 2, 12, GraphicsClass::ALIGN_LEFT, Button);
	ButtonText->setOverrideFont(Graphics::Instance().GetFont(GraphicsClass::FONT_7));
	DrawX += 24;

	// Menu
	Button = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX, DrawY, 25, 25), NULL, ELEMENT_MAINMENU, 0, L"Menu");
	Button->setImage(irrDriver->getTexture(WorkingDirectory + "textures/interface/hud_menu.png"));
	ButtonText = Graphics::Instance().AddText("Esc", 2, 12, GraphicsClass::ALIGN_LEFT, Button);
	ButtonText->setOverrideFont(Graphics::Instance().GetFont(GraphicsClass::FONT_7));
	DrawX += 24;
}

// Initialize the main menu
void HUDClass::InitMenu() {

	// Add window
	TabMenu = irrGUI->addTab(Graphics::Instance().GetRect(0, 0, 800, 500));

	// Add menu buttons
	irrGUI->addButton(Graphics::Instance().GetCenteredRect(400, 275, 100, 25), TabMenu, ELEMENT_MAINMENURESUME, L"Resume");
	irrGUI->addButton(Graphics::Instance().GetCenteredRect(400, 325, 100, 25), TabMenu, ELEMENT_MAINMENUEXIT, L"Exit");

	*State = PlayClientState::STATE_MAINMENU;
}

// Close main menu
void HUDClass::CloseMenu() {

	irrGUI->getRootGUIElement()->removeChild(TabMenu);

	*State = PlayClientState::STATE_WALK;
}

// Initialize the inventory system
void HUDClass::InitInventory(int TX, int TY, bool TSendBusy) {
	if(TSendBusy)
		SendBusy(true);

	CursorItem.Reset();
	TooltipItem.Reset();

	// Add window
	TabInventory = irrGUI->addTab(Graphics::Instance().GetCenteredRect(TX, TY, 265, 200));

	// Add background
	IGUIImage *Image = irrGUI->addImage(Graphics::Instance().GetImage(GraphicsClass::IMAGE_INVENTORY), position2di(0, 0), true, TabInventory);

	*State = PlayClientState::STATE_INVENTORY;
}

// Close the inventory system
void HUDClass::CloseInventory() {

	irrGUI->getRootGUIElement()->removeChild(TabInventory);
	CursorItem.Reset();
	TooltipItem.Reset();

	// No longer busy
	SendBusy(false);

	*State = PlayClientState::STATE_WALK;
}

// Initialize the vendor
void HUDClass::InitVendor(int TVendorID) {
	if(*State == PlayClientState::STATE_VENDOR)
		return;

	// Get vendor stats
	Vendor = Stats::Instance().GetVendor(TVendorID);

	// Add window
	TabVendor = irrGUI->addTab(Graphics::Instance().GetCenteredRect(400, 180, 262, 246));

	// Add background
	IGUIImage *Image = irrGUI->addImage(Graphics::Instance().GetImage(GraphicsClass::IMAGE_VENDOR), position2di(0, 0), true, TabVendor);

	// Open inventory
	InitInventory(400, 420, false);
}

// Close the vendor
void HUDClass::CloseVendor() {
	if(*State != PlayClientState::STATE_VENDOR)
		return;

	irrGUI->getRootGUIElement()->removeChild(TabVendor);
	CursorItem.Reset();

	// Close inventory
	CloseInventory();

	// Notify server
	PacketClass Packet(NetworkClass::EVENT_END);
	ClientNetwork->SendPacketToHost(&Packet);

	*State = PlayClientState::STATE_WALK;
	Vendor = NULL;
}

// Initialize the trader
void HUDClass::InitTrader(int TTraderID) {
	if(*State == PlayClientState::STATE_TRADER)
		return;

	// Get trader stats
	Trader = Stats::Instance().GetTrader(TTraderID);

	// Check for required items
	RewardItemSlot = Player->GetRequiredItemSlots(Trader, RequiredItemSlots);

	// Add window
	TabTrader = irrGUI->addTab(Graphics::Instance().GetCenteredRect(400, 250, 166, 272));

	// Add background
	IGUIImage *Image = irrGUI->addImage(Graphics::Instance().GetImage(GraphicsClass::IMAGE_TRADER), position2di(0, 0), true, TabTrader);

	// Add buttons
	IGUIButton *TradeButton = irrGUI->addButton(Graphics::Instance().GetCenteredRect(166/2 - 38, 245, 60, 25), TabTrader, ELEMENT_TRADERACCEPT, L"Trade");
	IGUIButton *CancelButton = irrGUI->addButton(Graphics::Instance().GetCenteredRect(166/2 + 38, 245, 60, 25), TabTrader, ELEMENT_TRADERCANCEL, L"Cancel");

	// Can't trade
	if(RewardItemSlot == -1)
		TradeButton->setEnabled(false);

	*State = PlayClientState::STATE_TRADER;
}

// Close the trader
void HUDClass::CloseTrader() {
	if(*State != PlayClientState::STATE_TRADER)
		return;

	irrGUI->getRootGUIElement()->removeChild(TabTrader);
	TooltipItem.Reset();
	CursorItem.Reset();

	// Notify server
	PacketClass Packet(NetworkClass::EVENT_END);
	ClientNetwork->SendPacketToHost(&Packet);

	*State = PlayClientState::STATE_WALK;
	Trader = NULL;
}

// Initialize the character screen
void HUDClass::InitCharacter() {
	if(!CharacterOpen) {
		CharacterOpen = true;
	}
	else {
		CloseCharacter();
	}
}

// Closes the character screen
void HUDClass::CloseCharacter() {

	if(CharacterOpen) {
		CharacterOpen = false;
	}
}

// Initialize the skills screen
void HUDClass::InitSkills() {
	*State = PlayClientState::STATE_SKILLS;
	SendBusy(true);

	// Main window
	TabSkill = irrGUI->addTab(Graphics::Instance().GetCenteredRect(400, 300, 450, 400), NULL, 0);

	// Add +/- buttons
	const array<SkillClass> &Skills = Stats::Instance().GetSkillList();
	int X = 0, Y = 0;
	for(u32 i = 0; i < Skills.size(); i++) {
		int DrawX = X * SKILL_SPACINGX + SKILL_STARTX + 16;
		int DrawY = Y * SKILL_SPACINGY + SKILL_STARTY + 45;

		// Add buy/sell button
		IGUIButton *BuyButton = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX - 8, DrawY, 12, 12), TabSkill, ELEMENT_SKILLPLUS0 + i);
		BuyButton->setImage(Graphics::Instance().GetImage(GraphicsClass::IMAGE_PLUS));
		IGUIButton *SellButton = irrGUI->addButton(Graphics::Instance().GetCenteredRect(DrawX + 8, DrawY, 12, 12), TabSkill, ELEMENT_SKILLMINUS0 + i);
		SellButton->setImage(Graphics::Instance().GetImage(GraphicsClass::IMAGE_MINUS));

		X++;
		if(X >= SKILL_COLUMNS) {
			X = 0;
			Y++;
		}
	}

	RefreshSkillButtons();
	CursorSkill.Reset();
	TooltipSkill.Reset();
	SkillBarChanged = false;
}

// Close the skills screen
void HUDClass::CloseSkills() {

	irrGUI->getRootGUIElement()->removeChild(TabSkill);
	CursorSkill.Reset();
	TooltipSkill.Reset();

	// Send new skill bar to server
	if(SkillBarChanged) {
		PacketClass Packet(NetworkClass::SKILLS_SKILLBAR);
		for(int i = 0; i < 8; i++) {
			Packet.WriteChar(Player->GetSkillBarID(i));
		}
		ClientNetwork->SendPacketToHost(&Packet);
	}

	// No longer busy
	SendBusy(false);

	*State = PlayClientState::STATE_WALK;
}

// Initialize the trade system
void HUDClass::InitTrade() {

	// Send request to server
	SendTradeRequest();

	CursorItem.Reset();
	TooltipItem.Reset();

	// Add window
	TabTrade = irrGUI->addTab(Graphics::Instance().GetCenteredRect(400, 300, 280, 470));

	// Add background
	TraderWindow = irrGUI->addImage(Graphics::Instance().GetImage(GraphicsClass::IMAGE_TRADE), position2di(TRADE_WINDOWX, TRADE_WINDOWYTHEM), true, TabTrade);
	TraderWindow->setVisible(false);
	irrGUI->addImage(Graphics::Instance().GetImage(GraphicsClass::IMAGE_TRADE), position2di(TRADE_WINDOWX, TRADE_WINDOWYYOU), true, TabTrade);

	// Add gold input box
	TradeGoldBox = irrGUI->addEditBox(L"0", Graphics::Instance().GetRect(TRADE_WINDOWX + 33, TRADE_WINDOWYYOU + 68, 89, 20), true, TabTrade, ELEMENT_GOLDTRADEBOX);
	TradeGoldBox->setMax(10);

	// Add accept button
	TradeAcceptButton = irrGUI->addButton(Graphics::Instance().GetCenteredRect(140, 249, 100, 25), TabTrade, ELEMENT_TRADEACCEPT, L"Accept Trade");
	TradeAcceptButton->setIsPushButton(true);
	TradeAcceptButton->setEnabled(false);

	// Add inventory
	InitInventory(400, 432, false);

	*State = PlayClientState::STATE_TRADE;
}

// Closes the trade system
void HUDClass::CloseTrade(bool TSendNotify) {

	// Remove trade window
	irrGUI->getRootGUIElement()->removeChild(TabTrade);
	TradeAcceptButton = NULL;

	// Close inventory
	CloseInventory();

	// Notify server
	if(TSendNotify)
		SendTradeCancel();

	Player->SetTradePlayer(NULL);
	*State = PlayClientState::STATE_WALK;
}

// Closes all windows
void HUDClass::CloseWindows() {

	switch(*State) {
		case PlayClientState::STATE_MAINMENU:
			CloseMenu();
		break;
		case PlayClientState::STATE_VENDOR:
			CloseVendor();
		break;
		case PlayClientState::STATE_TRADER:
			CloseTrader();
		break;
		case PlayClientState::STATE_INVENTORY:
			CloseInventory();
		break;
		case PlayClientState::STATE_SKILLS:
			CloseSkills();
		break;
		case PlayClientState::STATE_TRADE:
			CloseTrade();
		break;
	}
}

// Draws chat messages
void HUDClass::DrawChat() {
	if(ChatHistory.getSize() == 0)
		return;

	// Set font
	IGUIFont *TextFont = Graphics::Instance().GetFont(GraphicsClass::FONT_10);
	Graphics::Instance().SetFont(GraphicsClass::FONT_10);

	int Index = 0;
	for(list<ChatStruct>::Iterator Iterator = ChatHistory.getLast(); Iterator != ChatHistory.end(); --Iterator) {
		ChatStruct &Chat = (*Iterator);

		int DrawX = 15;
		int DrawY = 550 - Index * 20;

		// Draw background
		dimension2du TextArea = TextFont->getDimension(stringw(Chat.Message.c_str()).c_str());
		Graphics::Instance().DrawBackground(GraphicsClass::IMAGE_BLACK, DrawX - 1, DrawY, TextArea.Width + 2, TextArea.Height, SColor(100, 255, 255, 255));

		// Draw text
		Graphics::Instance().RenderText(Chat.Message.c_str(), DrawX, DrawY, GraphicsClass::ALIGN_LEFT, SColor(255, 255, 255, 255));
		Index++;
		if(Index > 20)
			break;
	}
}

// Draws the top HUD area
void HUDClass::DrawTopHUD() {
	int StartX = 125, StartY = 15, Width = 100, Height = 16, Spacing = 20;

	char String[256];
	Graphics::Instance().SetFont(GraphicsClass::FONT_14);
	Graphics::Instance().RenderText(Player->GetName().c_str(), StartX / 2, 11, GraphicsClass::ALIGN_CENTER);
	Graphics::Instance().SetFont(GraphicsClass::FONT_10);

	// Draw experience bar
	sprintf(String, "Level %d", Player->GetLevel());
	Graphics::Instance().DrawBar(GraphicsClass::IMAGE_EXPERIENCE, StartX, StartY, Player->GetNextLevelPercent(), Width, Height);
	Graphics::Instance().RenderText(String, StartX + Width/2, StartY, GraphicsClass::ALIGN_CENTER);

	// Draw health
	StartX += Width + Spacing;
	float HealthPercent = Player->GetMaxHealth() > 0 ? Player->GetHealth() / (float)Player->GetMaxHealth() : 0;
	sprintf(String, "%d / %d", Player->GetHealth(), Player->GetMaxHealth());
	Graphics::Instance().DrawBar(GraphicsClass::IMAGE_HEALTH, StartX, StartY, HealthPercent, Width, Height);
	Graphics::Instance().RenderText(String, StartX + Width/2, StartY, GraphicsClass::ALIGN_CENTER);

	// Draw mana
	StartX += Width + Spacing;
	float ManaPercent = Player->GetMaxMana() > 0 ? Player->GetMana() / (float)Player->GetMaxMana() : 0;
	sprintf(String, "%d / %d", Player->GetMana(), Player->GetMaxMana());
	Graphics::Instance().DrawBar(GraphicsClass::IMAGE_MANA, StartX, StartY, ManaPercent, Width, Height);
	Graphics::Instance().RenderText(String, StartX + Width/2, StartY, GraphicsClass::ALIGN_CENTER);

	// Draw gold
	StartX += Width + Spacing + 14;
	Graphics::Instance().DrawImage(GraphicsClass::IMAGE_GOLD, StartX, StartY + 8);

	StartX += 20;
	sprintf(String, "%d", Player->GetGold());
	Graphics::Instance().RenderText(String, StartX, StartY);

	// Draw PVP icon
	if(Player->GetTile()->PVP) {
		StartX += 80;
		SColor PVPColor(255, 255, 255, 255);
		if(!Player->CanAttackPlayer())
			PVPColor.set(150, 150, 150, 150);
		Graphics::Instance().DrawImage(GraphicsClass::IMAGE_PVP, StartX, StartY + 8, PVPColor);
		Graphics::Instance().SetFont(GraphicsClass::FONT_7);
		Graphics::Instance().RenderText("A", StartX - 10, StartY + 8, GraphicsClass::ALIGN_LEFT, PVPColor);
		Graphics::Instance().SetFont(GraphicsClass::FONT_10);
	}

	// Draw RTT
	if(ClientNetwork->GetRTT()) {
		sprintf(String, "%d ms", ClientNetwork->GetRTT());
		Graphics::Instance().RenderText(String, 10, 600 - 25);
	}
}

// Draws the player's inventory
void HUDClass::DrawInventory() {
	rect<s32> WindowArea = TabInventory->getAbsolutePosition();
	int OffsetX = WindowArea.UpperLeftCorner.X;
	int OffsetY = WindowArea.UpperLeftCorner.Y;

	Graphics::Instance().SetFont(GraphicsClass::FONT_7);

	// Draw equipped items
	InventoryStruct *Item;
	PositionStruct *Position;
	for(int i = 0; i < PlayerClass::INVENTORY_BACKPACK; i++) {
		Item = Player->GetInventory(i);
		Position = &EquippedItemPositions[i];
		if(Item->Item && !CursorItem.IsEqual(i, WINDOW_INVENTORY)) {
			int DrawX = OffsetX + Position->X - 16;
			int DrawY = OffsetY + Position->Y - 16;
			Graphics::Instance().DrawCenteredImage(Item->Item->GetImage(), DrawX + 16, DrawY + 16);
			DrawItemPrice(Item->Item, Item->Count, DrawX, DrawY, false);
		}
	}

	// Draw inventory items
	int PositionX = 0, PositionY = 0;
	for(int i = PlayerClass::INVENTORY_BACKPACK; i < PlayerClass::INVENTORY_TRADE; i++) {
		Item = Player->GetInventory(i);
		if(Item->Item && !CursorItem.IsEqual(i, WINDOW_INVENTORY)) {
			int DrawX = OffsetX + 132 + PositionX * 32;
			int DrawY = OffsetY + 1 + PositionY * 32;
			Graphics::Instance().DrawCenteredImage(Item->Item->GetImage(), DrawX + 16, DrawY + 16);
			DrawItemPrice(Item->Item, Item->Count, DrawX, DrawY, false);
			if(Item->Count > 1)
				Graphics::Instance().RenderText(stringc(Item->Count).c_str(), DrawX + 2, DrawY + 20);
		}

		PositionX++;
		if(PositionX > 3) {
			PositionX = 0;
			PositionY++;
		}
	}

	Graphics::Instance().SetFont(GraphicsClass::FONT_10);
}

// Draw the town portal sequence
void HUDClass::DrawTownPortal() {

	Graphics::Instance().SetFont(GraphicsClass::FONT_14);

	// Get text
	char String[256];
	int TimeLeft = GAME_PORTALTIME - (int)Player->GetTownPortalTime();
	sprintf(String, "Portal in %d", max_(TimeLeft / 1000, 0) + 1);

	int DrawX = 400;
	int DrawY = 200;
	IGUIFont *TextFont = Graphics::Instance().GetFont(GraphicsClass::FONT_14);
	dimension2du TextArea = TextFont->getDimension(stringw(String).c_str());
	TextArea.Width += 5;
	TextArea.Height += 5;

	// Draw text
	Graphics::Instance().DrawBackground(GraphicsClass::IMAGE_BLACK, DrawX - 1 - TextArea.Width/2, DrawY-2, TextArea.Width, TextArea.Height, SColor(100, 255, 255, 255));
	Graphics::Instance().RenderText(String, DrawX, DrawY, GraphicsClass::ALIGN_CENTER);

	Graphics::Instance().SetFont(GraphicsClass::FONT_10);
}

// Draw the vendor
void HUDClass::DrawVendor() {
	rect<s32> WindowArea = TabVendor->getAbsolutePosition();
	int OffsetX = WindowArea.UpperLeftCorner.X;
	int OffsetY = WindowArea.UpperLeftCorner.Y;
	int CenterX = WindowArea.getCenter().X;

	// Draw inventory items
	const ItemClass *Item;
	int PositionX = 0, PositionY = 0;
	for(u32 i = 0; i < Vendor->Items.size(); i++) {
		Item = Vendor->Items[i];
		if(!CursorItem.IsEqual(i, WINDOW_VENDOR)) {
			int DrawX = OffsetX + 1 + PositionX * 32;
			int DrawY = OffsetY + 47 + PositionY * 32;
			Graphics::Instance().DrawCenteredImage(Item->GetImage(), DrawX + 16, DrawY + 16);
			DrawItemPrice(Item, 1, DrawX, DrawY, true);
		}

		PositionX++;
		if(PositionX > 7) {
			PositionX = 0;
			PositionY++;
		}
	}

	// Draw name
	Graphics::Instance().SetFont(GraphicsClass::FONT_14);
	Graphics::Instance().RenderText(Vendor->Name.c_str(), CenterX, OffsetY + 4, GraphicsClass::ALIGN_CENTER);

	// Draw info
	Graphics::Instance().SetFont(GraphicsClass::FONT_10);
	Graphics::Instance().RenderText(Vendor->Info.c_str(), CenterX, OffsetY + 27, GraphicsClass::ALIGN_CENTER);

	Graphics::Instance().SetFont(GraphicsClass::FONT_10);
}

// Draw the trader screen
void HUDClass::DrawTrader() {
	rect<s32> WindowArea = TabTrader->getAbsolutePosition();
	int OffsetX = WindowArea.UpperLeftCorner.X;
	int OffsetY = WindowArea.UpperLeftCorner.Y;
	int CenterX = WindowArea.getCenter().X;

	Graphics::Instance().SetFont(GraphicsClass::FONT_8);

	// Draw items
	int PositionX = 0, PositionY = 0;
	char Buffer[256];
	SColor Color;
	for(u32 i = 0; i < Trader->TraderItems.size(); i++) {
		int DrawX = OffsetX + 19 + PositionX * 32;
		int DrawY = OffsetY + 52 + PositionY * 32 + PositionY * 14;
		Graphics::Instance().DrawCenteredImage(Trader->TraderItems[i].Item->GetImage(), DrawX + 16, DrawY + 16);

		if(RequiredItemSlots[i] == -1)
			Color.set(255, 255, 0, 0);
		else
			Color.set(255, 255, 255, 255);
		sprintf(Buffer, "%d", Trader->TraderItems[i].Count);
		Graphics::Instance().RenderText(Buffer, DrawX + 16, DrawY - 14, GraphicsClass::ALIGN_CENTER, Color);
	
		PositionX++;
		if(PositionX > 3) {
			PositionX = 0;
			PositionY++;
		}
	}

	// Draw reward item
	Graphics::Instance().DrawCenteredImage(Trader->RewardItem->GetImage(), OffsetX + 82, OffsetY + 198);
	sprintf(Buffer, "%d", Trader->Count);
	Graphics::Instance().RenderText(Buffer, OffsetX + 82, OffsetY + 166, GraphicsClass::ALIGN_CENTER);
	
	// Draw text
	Graphics::Instance().SetFont(GraphicsClass::FONT_10);
	Graphics::Instance().RenderText("Looking for", CenterX, OffsetY + 13, GraphicsClass::ALIGN_CENTER);
	Graphics::Instance().RenderText("Reward", CenterX, OffsetY + 143, GraphicsClass::ALIGN_CENTER);
}

// Draw the trade screen
void HUDClass::DrawTrade() {
	rect<s32> WindowArea = TabTrade->getAbsolutePosition();
	int OffsetX = WindowArea.UpperLeftCorner.X;
	int OffsetY = WindowArea.UpperLeftCorner.Y;
	int CenterX = WindowArea.getCenter().X;

	// Draw items
	DrawTradeItems(Player, TRADE_WINDOWX, TRADE_WINDOWYYOU, false);

	// Draw trading player information
	PlayerClass *TradePlayer = Player->GetTradePlayer();
	if(TradePlayer) {
		TraderWindow->setVisible(true);

		// Draw player information
		Graphics::Instance().RenderText(TradePlayer->GetName().c_str(), OffsetX + 72, OffsetY + 70 - 55, GraphicsClass::ALIGN_CENTER);
		Graphics::Instance().DrawCenteredImage(Stats::Instance().GetPortrait(TradePlayer->GetPortraitID())->Image, OffsetX + 72, OffsetY + 70);

		// Draw items
		DrawTradeItems(TradePlayer, TRADE_WINDOWX, TRADE_WINDOWYTHEM, true);

		// Draw gold
		Graphics::Instance().RenderText(stringc(TradePlayer->GetTradeGold()).c_str(), OffsetX + TRADE_WINDOWX + 35, OffsetY + TRADE_WINDOWYTHEM + 70);

		// Draw agreement state
		stringc AcceptText;
		SColor AcceptColor;
		if(TradePlayer->GetTradeAccepted()) {
			AcceptText = "Accepted";
			AcceptColor.set(255, 0, 255, 0);
		}
		else {
			AcceptText = "Unaccepted";
			AcceptColor.set(255, 255, 0, 0);
		}

		Graphics::Instance().RenderText(AcceptText.c_str(), CenterX, OffsetY + TRADE_WINDOWYTHEM + 100, GraphicsClass::ALIGN_CENTER, AcceptColor);
		TradeAcceptButton->setEnabled(true);
	}
	else {
		TraderWindow->setVisible(false);
		Graphics::Instance().RenderText("Waiting for other player...", 400, 120, GraphicsClass::ALIGN_CENTER);
		TradeAcceptButton->setEnabled(false);
	}

	// Draw player information
	Graphics::Instance().RenderText(Player->GetName().c_str(), OffsetX + 72, OffsetY + 198 - 55, GraphicsClass::ALIGN_CENTER);
	Graphics::Instance().DrawCenteredImage(Stats::Instance().GetPortrait(Player->GetPortraitID())->Image, OffsetX + 72, OffsetY + 198);
}

// Draw the character stats page
void HUDClass::DrawCharacter() {
	char Buffer[256];
	int Width = 180;
	int Height = 300;

	int DrawX = 800 - Width;
	int DrawY = 300 - Height / 2;
	int RightDrawX = 800 - 10;
	Graphics::Instance().DrawBackground(GraphicsClass::IMAGE_BLACK, DrawX, DrawY, Width, Height, SColor(220, 255, 255, 255));
	DrawX += 10;
	DrawY += 10;

	// Experience
	Graphics::Instance().RenderText("EXP", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetExperience());
	Graphics::Instance().RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	// Experience needed
	DrawY += 15;
	Graphics::Instance().RenderText("EXP needed", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetExperienceNeeded());
	Graphics::Instance().RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	// Damage
	DrawY += 15;
	Graphics::Instance().RenderText("Damage", DrawX, DrawY);
	sprintf(Buffer, "%d-%d", Player->GetMinDamage(), Player->GetMaxDamage());
	Graphics::Instance().RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	// Defense
	DrawY += 15;
	Graphics::Instance().RenderText("Defense", DrawX, DrawY);
	sprintf(Buffer, "%d-%d", Player->GetMinDefense(), Player->GetMaxDefense());
	Graphics::Instance().RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	// Regen
	DrawY += 15;
	Graphics::Instance().RenderText("HP Regen", DrawX, DrawY);
	sprintf(Buffer, "%0.2f%%", Player->GetHealthRegen());
	Graphics::Instance().RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	DrawY += 15;
	Graphics::Instance().RenderText("MP Regen", DrawX, DrawY);
	sprintf(Buffer, "%0.2f%%", Player->GetManaRegen());
	Graphics::Instance().RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	// Stats
	DrawY += 30;
	Graphics::Instance().RenderText("Play Time", DrawX, DrawY);

	int Seconds = Player->GetPlayTime();
	if(Seconds < 60)
		sprintf(Buffer, "%ds", Seconds);
	else if(Seconds < 3600)
		sprintf(Buffer, "%dm", Seconds / 60);
	else
		sprintf(Buffer, "%dh%dm", Seconds / 3600, (Seconds / 60 % 60));
	Graphics::Instance().RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	DrawY += 15;
	Graphics::Instance().RenderText("Deaths", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetDeaths());
	Graphics::Instance().RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	DrawY += 15;
	Graphics::Instance().RenderText("Monster Kills", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetMonsterKills());
	Graphics::Instance().RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	DrawY += 15;
	Graphics::Instance().RenderText("Player Kills", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetPlayerKills());
	Graphics::Instance().RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	//DrawY += 15;
	//Graphics::Instance().RenderText("Bounty", DrawX, DrawY);
	//sprintf(Buffer, "%d", Player->GetBounty());
	//Graphics::Instance().RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);
}

// Draws the skill page
void HUDClass::DrawSkills() {
	char Buffer[256];
	rect<s32> WindowArea = TabSkill->getAbsolutePosition();

	// Remaining points
	sprintf(Buffer, "Skill Points: %d", Player->GetSkillPointsRemaining());
	Graphics::Instance().RenderText(Buffer, 400, SKILL_BARY - 23, GraphicsClass::ALIGN_CENTER);

	// Draw skills
	const array<SkillClass> &Skills = Stats::Instance().GetSkillList();
	int X = 0, Y = 0;
	for(u32 i = 0; i < Skills.size(); i++) {
		int DrawX = WindowArea.UpperLeftCorner.X + X * SKILL_SPACINGX + SKILL_STARTX + 16;
		int DrawY = WindowArea.UpperLeftCorner.Y + Y * SKILL_SPACINGY + SKILL_STARTY + 16;
		Graphics::Instance().DrawCenteredImage(Skills[i].GetImage(), DrawX, DrawY);

		sprintf(Buffer, "%d", Player->GetSkillLevel(i));
		Graphics::Instance().RenderText(Buffer, DrawX, DrawY - 38, GraphicsClass::ALIGN_CENTER);

		X++;
		if(X >= SKILL_COLUMNS) {
			X = 0;
			Y++;
		}
	}

	// Draw skill bar
	ITexture *Image;
	for(int i = 0; i < 8; i++) {

		if(Player->GetSkillBar(i))
			Image = Player->GetSkillBar(i)->GetImage();
		else
			Image = Graphics::Instance().GetImage(GraphicsClass::IMAGE_EMPTYSLOT);

		Graphics::Instance().DrawCenteredImage(Image, SKILL_BARX + i * 32 + 16, SKILL_BARY + 16);
	}
}

// Draws the item under the cursor
void HUDClass::DrawCursorItem() {
	if(CursorItem.Item) {
		int DrawX = Input::Instance().GetMousePosition().X - 16;
		int DrawY = Input::Instance().GetMousePosition().Y - 16;
		Graphics::Instance().SetFont(GraphicsClass::FONT_7);
		Graphics::Instance().DrawCenteredImage(CursorItem.Item->GetImage(), DrawX + 16, DrawY + 16);
		DrawItemPrice(CursorItem.Item, CursorItem.Count, DrawX, DrawY, CursorItem.Window == WINDOW_VENDOR);
		if(CursorItem.Count > 1)
			Graphics::Instance().RenderText(stringc(CursorItem.Count).c_str(), DrawX + 2, DrawY + 20);
		Graphics::Instance().SetFont(GraphicsClass::FONT_10);
	}
}

// Draws information about an item
void HUDClass::DrawItemTooltip() {
	const ItemClass *Item = TooltipItem.Item;
	if(Item) {
		int DrawX = Input::Instance().GetMousePosition().X + 16;
		int DrawY = Input::Instance().GetMousePosition().Y - 200;
		int Width = 175;
		int Height = 200;
		if(DrawY < 20)
			DrawY = 20;

		// Draw background
		char Buffer[256];
		stringc String;
		Graphics::Instance().SetFont(GraphicsClass::FONT_10);
		Graphics::Instance().DrawBackground(GraphicsClass::IMAGE_BLACK, DrawX, DrawY, Width, Height);
		DrawX += 10;

		// Draw name
		DrawY += 10;
		Graphics::Instance().RenderText(Item->GetName().c_str(), DrawX, DrawY);

		// Draw type
		DrawY += 15;
		Item->GetType(String);
		Graphics::Instance().RenderText(String.c_str(), DrawX, DrawY);

		/*
		// Draw level
		DrawY += 15;
		sprintf(Buffer, "Level %d", Item->GetLevel());
		Graphics::Instance().RenderText(Buffer, DrawX, DrawY);
		*/

		DrawY += 15;
		int Min, Max;

		// Damage
		Item->GetDamageRange(Min, Max);
		if(Min != 0 || Max != 0) {
			if(Min != Max)
				sprintf(Buffer, "Damage: %d to %d", Min, Max);
			else
				sprintf(Buffer, "Damage: %d", Min);
			Graphics::Instance().RenderText(Buffer, DrawX, DrawY);
			DrawY += 15;
		}

		// Defense
		Item->GetDefenseRange(Min, Max);
		if(Min != 0 || Max != 0) {
			if(Min != Max)
				sprintf(Buffer, "Defense: %d to %d", Min, Max);
			else
				sprintf(Buffer, "Defense: %d", Min);
			Graphics::Instance().RenderText(Buffer, DrawX, DrawY);
			DrawY += 15;
		}

		switch(Item->GetType()) {
			case ItemClass::TYPE_WEAPON1HAND:
			case ItemClass::TYPE_WEAPON2HAND:
			case ItemClass::TYPE_HEAD:
			case ItemClass::TYPE_BODY:
			case ItemClass::TYPE_LEGS:
			case ItemClass::TYPE_SHIELD:
			break;
			case ItemClass::TYPE_POTION:
				if(Item->GetHealthRestore() > 0) {
					sprintf(Buffer, "HP Restored: %d", Item->GetHealthRestore());
					Graphics::Instance().RenderText(Buffer, DrawX, DrawY);
					DrawY += 15;
				}
				if(Item->GetManaRestore() > 0) {
					sprintf(Buffer, "MP Restored: %d", Item->GetManaRestore());
					Graphics::Instance().RenderText(Buffer, DrawX, DrawY);
					DrawY += 15;
				}
				if(Item->GetInvisPower() > 0) {
					sprintf(Buffer, "Invisibility Length: %d", Item->GetInvisPower());
					Graphics::Instance().RenderText(Buffer, DrawX, DrawY);
					DrawY += 15;
				}

				if(TooltipItem.Window == WINDOW_INVENTORY) {
					Graphics::Instance().RenderText("Right-click to use", DrawX, DrawY, GraphicsClass::ALIGN_LEFT, COLOR_GRAY);
					DrawY += 15;
				}
			break;
		}

		// Boosts
		if(Item->GetMaxHealth() != 0) {
			sprintf(Buffer, "%+d HP", Item->GetMaxHealth());
			Graphics::Instance().RenderText(Buffer, DrawX, DrawY);
			DrawY += 15;
		}
		if(Item->GetMaxMana() != 0) {
			sprintf(Buffer, "%+d MP", Item->GetMaxMana());
			Graphics::Instance().RenderText(Buffer, DrawX, DrawY);
			DrawY += 15;
		}
		if(Item->GetHealthRegen() != 0) {
			sprintf(Buffer, "%+0.2f%% HP Regen", Item->GetHealthRegen());
			Graphics::Instance().RenderText(Buffer, DrawX, DrawY);
			DrawY += 15;
		}
		if(Item->GetManaRegen() != 0) {
			sprintf(Buffer, "%+0.2f%% MP Regen", Item->GetManaRegen());
			Graphics::Instance().RenderText(Buffer, DrawX, DrawY);
			DrawY += 15;
		}

		// Vendors
		if(Vendor) {
			DrawY += 15;
			if(TooltipItem.Window == WINDOW_VENDOR) {
				sprintf(Buffer, "$%d", TooltipItem.Cost);
				Graphics::Instance().RenderText(Buffer, DrawX, DrawY, GraphicsClass::ALIGN_LEFT, COLOR_GOLD);
				DrawY += 15;
				Graphics::Instance().RenderText("Right-click to buy", DrawX, DrawY, GraphicsClass::ALIGN_LEFT, COLOR_GRAY);
			}
			else if(TooltipItem.Window == WINDOW_INVENTORY) {
				sprintf(Buffer, "$%d", TooltipItem.Cost);
				Graphics::Instance().RenderText(Buffer, DrawX, DrawY, GraphicsClass::ALIGN_LEFT, COLOR_GOLD);
				DrawY += 15;
				Graphics::Instance().RenderText("Shift+Right-click to sell", DrawX, DrawY, GraphicsClass::ALIGN_LEFT, COLOR_GRAY);
			}
		}

		switch(*State) {
			case PlayClientState::STATE_INVENTORY:
			case PlayClientState::STATE_TRADE:
				if(TooltipItem.Window == WINDOW_INVENTORY && TooltipItem.Count > 1) {
					Graphics::Instance().RenderText("Ctrl+click to split", DrawX, DrawY, GraphicsClass::ALIGN_LEFT, COLOR_GRAY);
					DrawY += 15;
				}
			break;
		}
	}
}

// Draws the skill under the cursor
void HUDClass::DrawCursorSkill() {
	if(CursorSkill.Skill) {
		int DrawX = Input::Instance().GetMousePosition().X - 16;
		int DrawY = Input::Instance().GetMousePosition().Y - 16;
		Graphics::Instance().DrawCenteredImage(CursorSkill.Skill->GetImage(), DrawX + 16, DrawY + 16);
	}
}

// Draws the skill tooltip window
void HUDClass::DrawSkillTooltip() {
	const SkillClass *Skill = TooltipSkill.Skill;
	if(Skill) {
		int SkillLevel = Player->GetSkillLevel(Skill->GetID());
		int DrawX = Input::Instance().GetMousePosition().X + 16;
		int DrawY = Input::Instance().GetMousePosition().Y - 100;
		int Width = 300;
		int Height;
		if(SkillLevel > 0)
			Height = 150;
		else
			Height = 100;

		if(DrawY < 20)
			DrawY = 20;
		if(DrawX + Width > 800)
			DrawX = 800 - Width;

		// Draw background
		char Buffer[256];
		stringc String;
		Graphics::Instance().SetFont(GraphicsClass::FONT_10);
		Graphics::Instance().DrawBackground(GraphicsClass::IMAGE_BLACK, DrawX, DrawY, Width, Height);
		DrawX += 10;
		DrawY += 10;

		// Draw name
		Graphics::Instance().RenderText(Skill->GetName().c_str(), DrawX, DrawY);
		DrawY += 15;

		// Draw description
		DrawSkillDescription(Skill, SkillLevel, DrawX, DrawY);

		// Draw next level description
		if(SkillLevel > 0) {
			DrawY += 30;
			Graphics::Instance().RenderText("Next level", DrawX, DrawY, GraphicsClass::ALIGN_LEFT);
			DrawY += 15;
			DrawSkillDescription(Skill, SkillLevel+1, DrawX, DrawY);
		}

		// Sell cost
		DrawY += 30;
		sprintf(Buffer, "Sell cost: $%d", Skill->GetSellCost(Player->GetLevel()));
		Graphics::Instance().RenderText(Buffer, DrawX, DrawY, GraphicsClass::ALIGN_LEFT, COLOR_GOLD);

		// Additional information
		switch(Skill->GetType()) {
			case SkillClass::TYPE_PASSIVE:
				DrawY += 15;
				Graphics::Instance().RenderText("Passive skills must be equipped to skillbar", DrawX, DrawY, GraphicsClass::ALIGN_LEFT, COLOR_GRAY);
			break;
		}
	}
}

// Draw the skill description
void HUDClass::DrawSkillDescription(const SkillClass *Skill, int TLevel, int TDrawX, int &TDrawY) {
	
	// Get skill data
	int PowerMin, PowerMax, PowerMinRound, PowerMaxRound, PowerPercent;
	float PowerMinFloat, PowerMaxFloat;
	Skill->GetPowerRange(TLevel, PowerMin, PowerMax);
	Skill->GetPowerRangeRound(TLevel, PowerMinRound, PowerMaxRound);
	Skill->GetPowerRange(TLevel, PowerMinFloat, PowerMaxFloat);
	PowerPercent = (int)round_(PowerMaxFloat * 100);

	// Draw description
	char Buffer[256];
	switch(Skill->GetType()) {
		case SkillClass::TYPE_ATTACK:
			sprintf(Buffer, Skill->GetInfo().c_str(), PowerPercent);
			Graphics::Instance().RenderText(Buffer, TDrawX, TDrawY, GraphicsClass::ALIGN_LEFT, COLOR_LIGHTGRAY);
		break;
		case SkillClass::TYPE_SPELL:
			switch(Skill->GetID()) {
				case 3:
					sprintf(Buffer, Skill->GetInfo().c_str(), PowerMaxRound);
				break;
				case 6:
				case 11:
					sprintf(Buffer, Skill->GetInfo().c_str(), PowerMinRound, PowerMaxRound);
				break;
			}
			Graphics::Instance().RenderText(Buffer, TDrawX, TDrawY, GraphicsClass::ALIGN_LEFT, COLOR_LIGHTGRAY);

			TDrawY += 15;
			sprintf(Buffer, "%d Mana", Skill->GetManaCost(TLevel));
			Graphics::Instance().RenderText(Buffer, TDrawX, TDrawY, GraphicsClass::ALIGN_LEFT, SColor(255, 0, 0, 255));
		break;
		case SkillClass::TYPE_USEPOTION:
			sprintf(Buffer, Skill->GetInfo().c_str(), PowerMin);
			Graphics::Instance().RenderText(Buffer, TDrawX, TDrawY, GraphicsClass::ALIGN_LEFT, COLOR_LIGHTGRAY);
		break;
		case SkillClass::TYPE_PASSIVE:
			switch(Skill->GetID()) {
				case 4:
				case 5:
					sprintf(Buffer, Skill->GetInfo().c_str(), PowerMaxRound);
					Graphics::Instance().RenderText(Buffer, TDrawX, TDrawY, GraphicsClass::ALIGN_LEFT, COLOR_LIGHTGRAY);
				break;
				case 7:
				case 8:
					sprintf(Buffer, Skill->GetInfo().c_str(), PowerMaxFloat);
					Graphics::Instance().RenderText(Buffer, TDrawX, TDrawY, GraphicsClass::ALIGN_LEFT, COLOR_LIGHTGRAY);
				break;
				case 9:
				case 10:
					sprintf(Buffer, Skill->GetInfo().c_str(), PowerMax);
					Graphics::Instance().RenderText(Buffer, TDrawX, TDrawY, GraphicsClass::ALIGN_LEFT, COLOR_LIGHTGRAY);
				break;
			}
		break;
		default:
			Graphics::Instance().RenderText(Skill->GetInfo().c_str(), TDrawX, TDrawY, GraphicsClass::ALIGN_LEFT, COLOR_LIGHTGRAY);
		break;
	}
}

// Draws an item's price
void HUDClass::DrawItemPrice(const ItemClass *TItem, int TCount, int TDrawX, int TDrawY, bool TBuy) {
	if(!Vendor)
		return;

	// Real price
	int Price = TItem->GetPrice(Vendor, TCount, TBuy);

	// Color
	SColor Color;
	if(TBuy && Player->GetGold() < Price)
		Color.set(255, 255, 0, 0);
	else
		Color.set(255, 224, 216, 84);

	Graphics::Instance().SetFont(GraphicsClass::FONT_7);
	Graphics::Instance().RenderText(stringc(Price).c_str(), TDrawX + 2, TDrawY + 0, GraphicsClass::ALIGN_LEFT, Color);
}

// Draws trading items
void HUDClass::DrawTradeItems(PlayerClass *TPlayer, int TDrawX, int TDrawY, bool TDrawAll) {
	rect<s32> WindowArea = TabTrade->getAbsolutePosition();
	int OffsetX = WindowArea.UpperLeftCorner.X;
	int OffsetY = WindowArea.UpperLeftCorner.Y;
	int CenterX = WindowArea.getCenter().X;

	Graphics::Instance().SetFont(GraphicsClass::FONT_7);

	// Draw trade items
	InventoryStruct *Item;
	int PositionX = 0, PositionY = 0;
	for(int i = PlayerClass::INVENTORY_TRADE; i < PlayerClass::INVENTORY_COUNT; i++) {
		Item = TPlayer->GetInventory(i);
		if(Item->Item && (TDrawAll || !CursorItem.IsEqual(i, WINDOW_TRADEYOU))) {
			int DrawX = OffsetX + TDrawX + PositionX * 32;
			int DrawY = OffsetY + TDrawY + PositionY * 32;
			Graphics::Instance().DrawCenteredImage(Item->Item->GetImage(), DrawX + 16, DrawY + 16);
			if(Item->Count > 1)
				Graphics::Instance().RenderText(stringc(Item->Count).c_str(), DrawX + 2, DrawY + 20);
		}

		PositionX++;
		if(PositionX > 3) {
			PositionX = 0;
			PositionY++;
		}
	}
	Graphics::Instance().SetFont(GraphicsClass::FONT_10);
}

// Returns an item that's on the screen
void HUDClass::GetItem(position2di &TPoint, CursorItemStruct &TCursorItem) {
	TCursorItem.Reset();

	switch(*State) {
		case PlayClientState::STATE_INVENTORY:
			GetInventoryItem(TPoint, TCursorItem);
		break;
		case PlayClientState::STATE_VENDOR:
			GetInventoryItem(TPoint, TCursorItem);
			GetVendorItem(TPoint, TCursorItem);
		break;
		case PlayClientState::STATE_TRADER:
			GetTraderItem(TPoint, TCursorItem);
		break;
		case PlayClientState::STATE_TRADE:
			GetInventoryItem(TPoint, TCursorItem);
			GetTradeItem(TPoint, TCursorItem);
		break;
	}
}

// Returns an inventory item from a mouse position
void HUDClass::GetInventoryItem(position2di &TPoint, CursorItemStruct &TCursorItem) {
	rect<s32> WindowArea = TabInventory->getAbsolutePosition();
	if(!WindowArea.isPointInside(TPoint))
		return;

	const ItemClass *Item;
	int Price = 0;
	TCursorItem.Window = WINDOW_INVENTORY;

	// Adjust mouse position
	TPoint.X -= WindowArea.UpperLeftCorner.X;
	TPoint.Y -= WindowArea.UpperLeftCorner.Y;

	// Search equipped item area
	if(TPoint.X < 122) {

		rect<s32> ItemArea;
		for(int i = 0; i < PlayerClass::INVENTORY_BACKPACK; i++) {
			ItemArea.UpperLeftCorner.X = EquippedItemPositions[i].X - 16;
			ItemArea.UpperLeftCorner.Y = EquippedItemPositions[i].Y - 16;
			ItemArea.LowerRightCorner.X = EquippedItemPositions[i].X + 16;
			ItemArea.LowerRightCorner.Y = EquippedItemPositions[i].Y + 16;
			if(ItemArea.isPointInside(TPoint)) {
				//printf("%d=%d %d\n", i, TPoint.X, TPoint.Y);
				Item = Player->GetInventory(i)->Item;
				if(Item)
					Price = Item->GetPrice(Vendor, Player->GetInventory(i)->Count, false);
				TCursorItem.Set(Item, Price, Player->GetInventory(i)->Count, i);
				return;
			}
		}
	}
	else if(TPoint.X >= 132 && TPoint.X <= 259 && TPoint.Y >= 1 && TPoint.Y <= 192) {
		int InventoryIndex = (TPoint.X - 132) / 32 + (TPoint.Y - 1) / 32 * 4 + PlayerClass::INVENTORY_BACKPACK;
		//printf("%d=%d %d\n", InventoryIndex, TPoint.X, TPoint.Y);
		Item = Player->GetInventory(InventoryIndex)->Item;
		if(Item)
			Price = Item->GetPrice(Vendor, Player->GetInventory(InventoryIndex)->Count, false);
		TCursorItem.Set(Item, Price, Player->GetInventory(InventoryIndex)->Count, InventoryIndex);
	}
}

// Returns a vendor item from a mouse position
void HUDClass::GetVendorItem(position2di &TPoint, CursorItemStruct &TCursorItem) {
	rect<s32> WindowArea = TabVendor->getAbsolutePosition();
	if(!WindowArea.isPointInside(TPoint))
		return;

	TCursorItem.Window = WINDOW_VENDOR;

	// Adjust mouse position
	TPoint.X -= WindowArea.UpperLeftCorner.X;
	TPoint.Y -= WindowArea.UpperLeftCorner.Y;

	// Get vendor slot
	if(TPoint.X >= 1 && TPoint.X <= 256 && TPoint.Y >= 47 && TPoint.Y < 47 + 192) {
		u32 InventoryIndex = (TPoint.X - 1) / 32 + (TPoint.Y - 47) / 32 * 8;
		//printf("%d=%d %d\n", InventoryIndex, TPoint.X, TPoint.Y);
		if(InventoryIndex < Vendor->Items.size()) {
			int Price = Vendor->Items[InventoryIndex]->GetPrice(Vendor, 1, true);
			TCursorItem.Set(Vendor->Items[InventoryIndex], Price, 1, InventoryIndex);
		}
	}
}

// Returns a trader item from a mouse position
void HUDClass::GetTraderItem(position2di &TPoint, CursorItemStruct &TCursorItem) {
	rect<s32> WindowArea = TabTrader->getAbsolutePosition();
	if(!WindowArea.isPointInside(TPoint))
		return;

	// Adjust mouse position
	TPoint.X -= WindowArea.UpperLeftCorner.X;
	TPoint.Y -= WindowArea.UpperLeftCorner.Y;

	// Get trader slot
	int InventoryIndex = -1;
	if(TPoint.X >= 19 && TPoint.X <= 146 && TPoint.Y >= 52 && TPoint.Y <= 83)
		InventoryIndex = (TPoint.X - 19) / 32 + (TPoint.Y - 52) / 32;
	else if(TPoint.X >= 19 && TPoint.X <= 146 && TPoint.Y >= 98 && TPoint.Y <= 129)
		InventoryIndex = 4 + (TPoint.X - 19) / 32 + (TPoint.Y - 98) / 32;
	else if(TPoint.X >= 67 && TPoint.X <= 98 && TPoint.Y >= 182 && TPoint.Y <= 213)
		InventoryIndex = 9;

	//printf("%d=%d %d\n", InventoryIndex, TPoint.X, TPoint.Y);

	// Set cursor item
	if(InventoryIndex != -1) {
		TCursorItem.Window = WINDOW_TRADER;
		if(InventoryIndex < (int)Trader->TraderItems.size())
			TCursorItem.Set(Trader->TraderItems[InventoryIndex].Item, 0, 1, InventoryIndex);
		else if(InventoryIndex == 9)
			TCursorItem.Set(Trader->RewardItem, 0, 1, InventoryIndex);
	}
}

// Returns a trade item from a mouse position
void HUDClass::GetTradeItem(position2di &TPoint, CursorItemStruct &TCursorItem) {
	rect<s32> WindowArea = TabTrade->getAbsolutePosition();
	if(!WindowArea.isPointInside(TPoint))
		return;

	// Adjust mouse position
	TPoint.X -= WindowArea.UpperLeftCorner.X;
	TPoint.Y -= WindowArea.UpperLeftCorner.Y;

	// Get trade slot
	PlayerClass *TradePlayer = Player->GetTradePlayer();
	if(TradePlayer && TPoint.X >= TRADE_WINDOWX && TPoint.X < TRADE_WINDOWX + 128 && TPoint.Y >= TRADE_WINDOWYTHEM && TPoint.Y < TRADE_WINDOWYTHEM + 64) {
		u32 InventoryIndex = (TPoint.X - TRADE_WINDOWX) / 32 + (TPoint.Y - TRADE_WINDOWYTHEM) / 32 * 4 + PlayerClass::INVENTORY_TRADE;
		//printf("them: %d=%d %d\n", InventoryIndex, TPoint.X, TPoint.Y);
		const ItemClass *Item = TradePlayer->GetInventory(InventoryIndex)->Item;
		TCursorItem.Window = WINDOW_TRADETHEM;
		TCursorItem.Set(Item, 0, TradePlayer->GetInventory(InventoryIndex)->Count, InventoryIndex);
	}
	else if(TPoint.X >= TRADE_WINDOWX && TPoint.X < TRADE_WINDOWX + 128 && TPoint.Y >= TRADE_WINDOWYYOU && TPoint.Y < TRADE_WINDOWYYOU + 64) {
		u32 InventoryIndex = (TPoint.X - TRADE_WINDOWX) / 32 + (TPoint.Y - TRADE_WINDOWYYOU) / 32 * 4 + PlayerClass::INVENTORY_TRADE;
		//printf("you: %d=%d %d\n", InventoryIndex, TPoint.X, TPoint.Y);
		const ItemClass *Item = Player->GetInventory(InventoryIndex)->Item;
		TCursorItem.Window = WINDOW_TRADEYOU;
		TCursorItem.Set(Item, 0, Player->GetInventory(InventoryIndex)->Count, InventoryIndex);
	}
}

// Buys an item from the vendor
void HUDClass::BuyItem(CursorItemStruct *TCursorItem, int TTargetSlot) {
	if(Player->GetGold() >= TCursorItem->Cost && Player->AddItem(TCursorItem->Item, TCursorItem->Count, TTargetSlot)) {

		// Update player
		int Price = TCursorItem->Item->GetPrice(Vendor, TCursorItem->Count, true);
		Player->UpdateGold(-Price);

		// Notify server
		PacketClass Packet(NetworkClass::VENDOR_EXCHANGE);
		Packet.WriteBit(1);
		Packet.WriteChar(TCursorItem->Count);
		Packet.WriteChar(TCursorItem->Slot);
		Packet.WriteChar(TTargetSlot);
		ClientNetwork->SendPacketToHost(&Packet);

		Player->CalculatePlayerStats();
	}
}

// Sells an item
void HUDClass::SellItem(CursorItemStruct *TCursorItem, int TAmount) {
	if(!TCursorItem->Item)
		return;

	// Update player
	int Price = TCursorItem->Item->GetPrice(Vendor, TAmount, 0);
	Player->UpdateGold(Price);
	bool Deleted = Player->UpdateInventory(TCursorItem->Slot, -TAmount);

	// Notify server
	PacketClass Packet(NetworkClass::VENDOR_EXCHANGE);
	Packet.WriteBit(0);
	Packet.WriteChar(TAmount);
	Packet.WriteChar(TCursorItem->Slot);
	ClientNetwork->SendPacketToHost(&Packet);

	if(Deleted)
		TCursorItem->Reset();

	Player->CalculatePlayerStats();
}

// Returns a skill that's on the screen
void HUDClass::GetSkill(position2di &TPoint, CursorSkillStruct &TCursorSkill) {
	TCursorSkill.Reset();

	switch(*State) {
		case PlayClientState::STATE_SKILLS:
			GetSkillPageSkill(TPoint, TCursorSkill);
		break;
		case PlayClientState::STATE_BATTLE:
		break;
	}
}

// Gets a skill from the skill page
void HUDClass::GetSkillPageSkill(position2di &TPoint, CursorSkillStruct &TCursorSkill) {
	rect<s32> WindowArea = TabSkill->getAbsolutePosition();
	if(!WindowArea.isPointInside(TPoint))
		return;

	// Get skill from page
	u32 SkillCount = Stats::Instance().GetSkillList().size();
	int X = 0, Y = 0;
	for(u32 i = 0; i < SkillCount; i++) {
		int SkillX = WindowArea.UpperLeftCorner.X + SKILL_STARTX + X * SKILL_SPACINGX;
		int SkillY = WindowArea.UpperLeftCorner.Y + SKILL_STARTY + Y * SKILL_SPACINGY;
		rect<s32> SkillArea(SkillX, SkillY, SkillX + 32, SkillY + 32);
		if(SkillArea.isPointInside(TPoint)) {
			TCursorSkill.Skill = Stats::Instance().GetSkill(i);
			TCursorSkill.Slot = i;
			TCursorSkill.Window = WINDOW_SKILLS;
			return;
		}

		X++;
		if(X >= SKILL_COLUMNS) {
			X = 0;
			Y++;
		}
	}

	// Get skill bar
	if(TPoint.X >= SKILL_BARX && TPoint.X < SKILL_BARX + 32 * 8 && TPoint.Y >= SKILL_BARY && TPoint.Y <= SKILL_BARY + 32) {
		int Index = (TPoint.X - SKILL_BARX) / 32;
		TCursorSkill.Skill = Player->GetSkillBar(Index);
		TCursorSkill.Slot = Index;
		TCursorSkill.Window = WINDOW_SKILLBAR;
	}
}

// Sets the player's skill bar
void HUDClass::SetSkillBar(int TSlot, int TOldSlot, const SkillClass *TSkill) {
	if(Player->GetSkillBar(TSlot) == TSkill)
		return;

	// Check for swapping
	if(TOldSlot == -1) {

		// Remove duplicate skills
		for(int i = 0; i < 8; i++) {
			if(Player->GetSkillBar(i) == TSkill)
				Player->SetSkillBar(i, NULL);
		}
	}
	else {
		const SkillClass *OldSkill = Player->GetSkillBar(TSlot);
		Player->SetSkillBar(TOldSlot, OldSkill);
	}

	Player->SetSkillBar(TSlot, TSkill);
	Player->CalculatePlayerStats();
	SkillBarChanged = true;
}

// Shows or hides the plus/minus buttons
void HUDClass::RefreshSkillButtons() {

	// Get remaining points
	int SkillPointsRemaining = Player->GetSkillPointsRemaining();

	// Loop through buttons
	list<IGUIElement *> Buttons = TabSkill->getChildren();
	for(list<IGUIElement *>::Iterator Iterator = Buttons.begin(); Iterator != Buttons.end(); ++Iterator) {
		IGUIButton *Button = static_cast<IGUIButton *>(*Iterator);
		Button->setVisible(true);

		// Toggle buttons
		int SkillID;
		if(Button->getID() < ELEMENT_SKILLMINUS0) {
			SkillID = Button->getID() - ELEMENT_SKILLPLUS0;

			// Hide locked skills
			const SkillClass *Skill = Stats::Instance().GetSkill(SkillID);
			if(Skill->GetSkillCost() > SkillPointsRemaining || Player->GetSkillLevel(SkillID) >= 255)
				Button->setVisible(false);
		}
		else {
			SkillID = Button->getID() - ELEMENT_SKILLMINUS0;
			if(Player->GetSkillLevel(SkillID) == 0)
				Button->setVisible(false);
		}
	}
}

// Sends the busy signal to the server
void HUDClass::SendBusy(bool TValue) {

	PacketClass Packet(NetworkClass::WORLD_BUSY);
	Packet.WriteChar(TValue);
	ClientNetwork->SendPacketToHost(&Packet);
}

// Trade with another player
void HUDClass::SendTradeRequest() {
	PacketClass Packet(NetworkClass::TRADE_REQUEST);
	ClientNetwork->SendPacketToHost(&Packet);
}

// Cancel a trade
void HUDClass::SendTradeCancel() {
	PacketClass Packet(NetworkClass::TRADE_CANCEL);
	ClientNetwork->SendPacketToHost(&Packet);

	Player->SetTradePlayer(NULL);
}

// Make sure the trade gold box is valid
int HUDClass::ValidateTradeGold() {

	// Get gold amount
	int Gold = atoi(stringc(TradeGoldBox->getText()).c_str());
	if(Gold < 0)
		Gold = 0;
	else if(Gold > Player->GetGold())
		Gold = Player->GetGold();

	// Set text
	TradeGoldBox->setText(stringw(Gold).c_str());

	return Gold;
}

// Resets the trade agreement
void HUDClass::ResetAcceptButton() {
	if(TradeAcceptButton)
		TradeAcceptButton->setPressed(false);

	PlayerClass *TradePlayer = Player->GetTradePlayer();
	if(TradePlayer)
		TradePlayer->SetTradeAccepted(false);
}

// Split a stack of items
void HUDClass::SplitStack(int TSlot, int TCount) {

	// Split only inventory items
	if(!PlayerClass::IsSlotInventory(TSlot))
		return;

	// Build packet
	PacketClass Packet(NetworkClass::INVENTORY_SPLIT);
	Packet.WriteChar(TSlot);
	Packet.WriteChar(TCount);

	ClientNetwork->SendPacketToHost(&Packet);
	Player->SplitStack(TSlot, TCount);
}

// Toggles the town portal state
void HUDClass::ToggleTownPortal() {
	PacketClass Packet(NetworkClass::WORLD_TOWNPORTAL);
	ClientNetwork->SendPacketToHost(&Packet);
	Player->StartTownPortal();

	if(*State == PlayClientState::STATE_TOWNPORTAL)
		*State = PlayClientState::STATE_WALK;
	else
		*State = PlayClientState::STATE_TOWNPORTAL;
}
