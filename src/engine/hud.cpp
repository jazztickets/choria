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
#include "hud.h"
#include "game.h"
#include "graphics.h"
#include "globals.h"
#include "input.h"
#include "stats.h"
#include "../client.h"
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

const SColor COLOR_GOLD(255, 196, 187, 44);
const SColor COLOR_GRAY(255, 150, 150, 150);
const SColor COLOR_LIGHTGRAY(255, 200, 200, 200);

const int MESSAGE_TIME = 15000;
const int SKILL_STARX = 35;
const int SKILL_STARY = 40;
const int SKILL_SPACINGX = 40;
const int SKILL_SPACINGY = 40;
const int SKILL_BARX = 272;
const int SKILL_BARY = 568;
const int SKILL_COLUMNS = 14;

const int TRADE_WINDOWX = 142;
const int TRADE_WINDOWYTHEM = 10;
const int TRADE_WINDOWYYOU = 138;

HUDClass HUD;

// Initialize
int HUDClass::Init() {

	SkillCategories = new array<const SkillClass *>[Stats.GetSkillTypeCount()];

	State = ClientState.GetState();
	Vendor = NULL;
	Trader = NULL;
	TooltipItem.Reset();
	CursorItem.Reset();
	TooltipAction.Reset();
	CursorAction.Reset();
	CharacterOpen = false;
	Chatting = false;
	TypingGold = false;
	TradeAcceptButton = NULL;
	RewardItemSlot = -1;
	ChatBox = NULL;
	ChatHistory.clear();

	InitButtonBar();
	InitActionBar();

	return 1;
}

// Shutdown
int HUDClass::Close() {

	delete[] SkillCategories;

	return 1;
}

// Handles key presses
void HUDClass::HandleKeyPress(EKEY_CODE Key) {

	if(Chatting) {
		if(Key == KEY_ESCAPE) {
			CloseChat();
		}
	}
	if(TypingGold) {
		if(Key == KEY_ESCAPE) {
			irrGUI->removeFocus(TradeGoldBox);
		}
	}
}

// Handles mouse movement
void HUDClass::HandleMouseMotion(int MouseX, int MouseY) {
}

// Handles mouse presses
bool HUDClass::HandleMousePress(int Button, int MouseX, int MouseY) {

	switch(*State) {
		case ClientStateClass::STATE_VENDOR:
			switch(Button) {
				case InputClass::MOUSE_LEFT:
					if(TooltipItem.Item) {
						CursorItem = TooltipItem;
					}
				break;
				case InputClass::MOUSE_RIGHT:
					if(TooltipItem.Item) {
						if(TooltipItem.Window == WINDOW_VENDOR)
							BuyItem(&TooltipItem, -1);
						else if(TooltipItem.Window == WINDOW_INVENTORY && Input.IsShiftDown())
							SellItem(&TooltipItem, 1);
					}
				break;
			}
		break;
		case ClientStateClass::STATE_INVENTORY:
			switch(Button) {
				case InputClass::MOUSE_LEFT:
					if(TooltipItem.Item) {
						if(Input.IsControlDown())
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
		case ClientStateClass::STATE_TRADE:
			switch(Button) {
				case InputClass::MOUSE_LEFT:
					if(TooltipItem.Item && TooltipItem.Window != WINDOW_TRADETHEM) {
						if(TooltipItem.Window == WINDOW_INVENTORY && Input.IsControlDown())
							SplitStack(TooltipItem.Slot, 1);
						else
							CursorItem = TooltipItem;
					}
				break;
			}
		break;
		case ClientStateClass::STATE_SKILLS:
			switch(Button) {
				case InputClass::MOUSE_LEFT:
					if(TooltipAction.Skill)
						CursorAction = TooltipAction;
				break;
			}
		break;
	}

	return false;
}

// Handles mouse release
void HUDClass::HandleMouseRelease(int Button, int MouseX, int MouseY) {

	switch(*State) {
		case ClientStateClass::STATE_INVENTORY:
		case ClientStateClass::STATE_VENDOR:
		case ClientStateClass::STATE_TRADE:
			if(Button == InputClass::MOUSE_LEFT) {

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
		case ClientStateClass::STATE_SKILLS:
			if(Button == InputClass::MOUSE_LEFT) {

				// Check for valid slots
				if(CursorAction.Skill) {

					if(TooltipAction.Window == WINDOW_ACTIONBAR) {
						if(CursorAction.Window == WINDOW_SKILLS)
							SetActionBar(TooltipAction.Slot, -1, CursorAction.Skill);
						else if(CursorAction.Window == WINDOW_ACTIONBAR)
							SetActionBar(TooltipAction.Slot, CursorAction.Slot, CursorAction.Skill);
					}
					else if(CursorAction.Window == WINDOW_ACTIONBAR && TooltipAction.Slot == -1) {
						SetActionBar(CursorAction.Slot, -1, NULL);
					}
				}

				CursorAction.Reset();
			}
		break;
	}
}

// Handles GUI presses
void HUDClass::HandleGUI(EGUI_EVENT_TYPE EventType, IGUIElement *Element) {
	int ID = Element->getID();

	if(EventType == EGET_BUTTON_CLICKED && ID == ELEMENT_CHARACTER) {
		InitCharacter();
	}

	switch(*State) {
		case ClientStateClass::STATE_WALK:
			switch(EventType) {
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
						case ELEMENT_ACTIONS:
							InitSkills();
						break;
						case ELEMENT_MAINMENU:
							InitMenu();
						break;
						case ELEMENT_ACTION1:
						case ELEMENT_ACTION2:
						case ELEMENT_ACTION3:
						case ELEMENT_ACTION4:
						case ELEMENT_ACTION5:
						case ELEMENT_ACTION6:
						case ELEMENT_ACTION7:
						case ELEMENT_ACTION8:
							Player->UseActionWorld(ID - ELEMENT_ACTION1);
						break;
					}
				break;
			}
		break;
		case ClientStateClass::STATE_BATTLE:
			switch(EventType) {
				case EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_ACTION1:
						case ELEMENT_ACTION2:
						case ELEMENT_ACTION3:
						case ELEMENT_ACTION4:
						case ELEMENT_ACTION5:
						case ELEMENT_ACTION6:
						case ELEMENT_ACTION7:
						case ELEMENT_ACTION8:
							//Player->UseActionWorld(ID - ELEMENT_ACTION1);
						break;
					}
				break;
			}
		break;
		case ClientStateClass::STATE_MAINMENU:
			switch(EventType) {
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
		case ClientStateClass::STATE_TOWNPORTAL:
			switch(EventType) {
				case EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_TOWNPORTAL:
							ToggleTownPortal();
						break;
					}
				break;
			}
		break;
		case ClientStateClass::STATE_INVENTORY:
			switch(EventType) {
				case EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_INVENTORY:
							CloseWindows();
						break;
					}
				break;
			}
		break;
		case ClientStateClass::STATE_TRADER:
			switch(EventType) {
				case EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_TRADERACCEPT: {
							PacketClass Packet(NetworkClass::TRADER_ACCEPT);
							ClientNetwork->SendPacketToHost(&Packet);
							Player->AcceptTrader(Trader, RequiredItemSlots, RewardItemSlot);
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
		case ClientStateClass::STATE_TRADE:
			switch(EventType) {
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
						if(EventType == EGET_EDITBOX_ENTER)
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
		case ClientStateClass::STATE_SKILLS:
			switch(EventType) {
				case EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_ACTIONS:
							CloseWindows();
						break;
					}
				break;
			}
		break;
	}
}

// Updates the HUD
void HUDClass::Update(u32 FrameTime) {
	TooltipAction.Reset();
	TooltipItem.Reset();

	// Update action bar
	GetActionBarTooltip();

	// Chat messages
	for(list<ChatStruct>::Iterator Iterator = ChatHistory.begin(); Iterator != ChatHistory.end();) {
		ChatStruct &Chat = (*Iterator);
		Chat.TimeOut += FrameTime;
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
		case ClientStateClass::STATE_MAINMENU:
			Graphics.DrawBackground(GraphicsClass::IMAGE_BLACK, 0, 0, 800, 600, SColor(100, 255, 255, 255));
		break;
		case ClientStateClass::STATE_SKILLS: {
			rect<s32> WindowArea = TabSkill->getAbsolutePosition();

			// Draw background
			Graphics.DrawBackground(GraphicsClass::IMAGE_BLACK, WindowArea.UpperLeftCorner.X, WindowArea.UpperLeftCorner.Y, WindowArea.getWidth(), WindowArea.getHeight(), SColor(220, 255, 255, 255));
		}
		break;
		case ClientStateClass::STATE_TRADE: {
			rect<s32> WindowArea = TabTrade->getAbsolutePosition();

			// Draw background
			Graphics.DrawBackground(GraphicsClass::IMAGE_BLACK, WindowArea.UpperLeftCorner.X-1, WindowArea.UpperLeftCorner.Y, WindowArea.getWidth()-1, WindowArea.getHeight(), SColor(220, 255, 255, 255));
		}
		break;
	}
}

// Draws the HUD elements
void HUDClass::Draw() {
	DrawTopHUD();

	switch(*State) {
		case ClientStateClass::STATE_INVENTORY:
			DrawInventory();
		break;
		case ClientStateClass::STATE_VENDOR:
			DrawVendor();
			DrawInventory();
		break;
		case ClientStateClass::STATE_TRADER:
			DrawTrader();
		break;
		case ClientStateClass::STATE_SKILLS:
			DrawSkills();
		break;
		case ClientStateClass::STATE_TRADE:
			DrawTrade();
			DrawInventory();
		break;
		case ClientStateClass::STATE_TOWNPORTAL:
			DrawTownPortal();
		break;
	}

	if(CharacterOpen)
		DrawCharacter();

	// Draw item information
	DrawCursorItem();
	DrawItemTooltip();

	// Draw skill information
	DrawCursorAction();
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
		ChatBox = irrGUI->addEditBox(L"", Graphics.GetRect(25, 570, 200, 20), true, NULL, ELEMENT_CHATBOX);
		ChatBox->setOverrideFont(Graphics.GetFont(GraphicsClass::FONT_10));
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
	Button = irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 25, 25), NULL, ELEMENT_TOWNPORTAL, 0, L"Town Portal");
	Button->setImage(irrDriver->getTexture("textures/interface/hud_spawn.png"));
	ButtonText = Graphics.AddText("S", 2, 12, GraphicsClass::ALIGN_LEFT, Button);
	ButtonText->setOverrideFont(Graphics.GetFont(GraphicsClass::FONT_7));
	DrawX += 24;

	// Inventory
	Button = irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 25, 25), NULL, ELEMENT_INVENTORY, 0, L"Inventory");
	Button->setImage(irrDriver->getTexture("textures/interface/hud_inventory.png"));
	ButtonText = Graphics.AddText("I", 2, 12, GraphicsClass::ALIGN_LEFT, Button);
	ButtonText->setOverrideFont(Graphics.GetFont(GraphicsClass::FONT_7));
	DrawX += 24;

	// Trade
	Button = irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 25, 25), NULL, ELEMENT_TRADE, 0, L"Trade");
	Button->setImage(irrDriver->getTexture("textures/interface/hud_trade.png"));
	ButtonText = Graphics.AddText("T", 2, 12, GraphicsClass::ALIGN_LEFT, Button);
	ButtonText->setOverrideFont(Graphics.GetFont(GraphicsClass::FONT_7));
	DrawX += 24;

	// Character
	Button = irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 25, 25), NULL, ELEMENT_CHARACTER, 0, L"Character");
	Button->setImage(irrDriver->getTexture("textures/interface/hud_character.png"));
	ButtonText = Graphics.AddText("C", 2, 12, GraphicsClass::ALIGN_LEFT, Button);
	ButtonText->setOverrideFont(Graphics.GetFont(GraphicsClass::FONT_7));
	DrawX += 24;

	// Skills
	Button = irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 25, 25), NULL, ELEMENT_ACTIONS, 0, L"Skills");
	Button->setImage(irrDriver->getTexture("textures/interface/hud_skills.png"));
	ButtonText = Graphics.AddText("K", 2, 12, GraphicsClass::ALIGN_LEFT, Button);
	ButtonText->setOverrideFont(Graphics.GetFont(GraphicsClass::FONT_7));
	DrawX += 24;

	// Menu
	Button = irrGUI->addButton(Graphics.GetCenteredRect(DrawX, DrawY, 25, 25), NULL, ELEMENT_MAINMENU, 0, L"Menu");
	Button->setImage(irrDriver->getTexture("textures/interface/hud_menu.png"));
	ButtonText = Graphics.AddText("Esc", 2, 12, GraphicsClass::ALIGN_LEFT, Button);
	ButtonText->setOverrideFont(Graphics.GetFont(GraphicsClass::FONT_7));
	DrawX += 24;
}

// Creates the buttons for the action bar
void HUDClass::InitActionBar() {

	// Add skill buttons
	for(int i = 0; i < GAME_MAXACTIONS; i++) {
		
		// Add button
		ActionButtons[i] = irrGUI->addButton(Graphics.GetRect(SKILL_BARX + i * 32, SKILL_BARY, 32, 32), 0, ELEMENT_ACTION1 + i, 0);
		IGUIStaticText *Text = Graphics.AddText(stringc(i+1).c_str(), 3, 1, GraphicsClass::ALIGN_LEFT, ActionButtons[i]);
		Text->setOverrideFont(Graphics.GetFont(GraphicsClass::FONT_8));		

		ActionButtons[i]->setImage(Graphics.GetImage(GraphicsClass::IMAGE_EMPYSLOT));
	}
}

// Initialize the main menu
void HUDClass::InitMenu() {

	// Add window
	TabMenu = irrGUI->addTab(Graphics.GetRect(0, 0, 800, 500));

	// Add menu buttons
	irrGUI->addButton(Graphics.GetCenteredRect(400, 275, 100, 25), TabMenu, ELEMENT_MAINMENURESUME, L"Resume");
	irrGUI->addButton(Graphics.GetCenteredRect(400, 325, 100, 25), TabMenu, ELEMENT_MAINMENUEXIT, L"Exit");

	*State = ClientStateClass::STATE_MAINMENU;
}

// Close main menu
void HUDClass::CloseMenu() {

	irrGUI->getRootGUIElement()->removeChild(TabMenu);

	*State = ClientStateClass::STATE_WALK;
}

// Initialize the inventory system
void HUDClass::InitInventory(int X, int Y, bool SendBusy) {
	if(SendBusy)
		this->SendBusy(true);

	CursorItem.Reset();

	// Add window
	TabInventory = irrGUI->addTab(Graphics.GetCenteredRect(X, Y, 265, 200));

	// Add background
	IGUIImage *Image = irrGUI->addImage(Graphics.GetImage(GraphicsClass::IMAGE_INVENTORY), position2di(0, 0), true, TabInventory);

	*State = ClientStateClass::STATE_INVENTORY;
}

// Close the inventory system
void HUDClass::CloseInventory() {

	irrGUI->getRootGUIElement()->removeChild(TabInventory);
	CursorItem.Reset();

	// No longer busy
	SendBusy(false);

	*State = ClientStateClass::STATE_WALK;
}

// Initialize the vendor
void HUDClass::InitVendor(int VendorID) {
	if(*State == ClientStateClass::STATE_VENDOR)
		return;

	// Get vendor stats
	Vendor = Stats.GetVendor(VendorID);

	// Add window
	TabVendor = irrGUI->addTab(Graphics.GetCenteredRect(400, 180, 262, 246));

	// Add background
	IGUIImage *Image = irrGUI->addImage(Graphics.GetImage(GraphicsClass::IMAGE_VENDOR), position2di(0, 0), true, TabVendor);

	// Open inventory
	InitInventory(400, 420, false);
}

// Close the vendor
void HUDClass::CloseVendor() {
	if(*State != ClientStateClass::STATE_VENDOR)
		return;

	irrGUI->getRootGUIElement()->removeChild(TabVendor);
	CursorItem.Reset();

	// Close inventory
	CloseInventory();

	// Notify server
	PacketClass Packet(NetworkClass::EVENT_END);
	ClientNetwork->SendPacketToHost(&Packet);

	*State = ClientStateClass::STATE_WALK;
	Vendor = NULL;
}

// Initialize the trader
void HUDClass::InitTrader(int TraderID) {
	if(*State == ClientStateClass::STATE_TRADER)
		return;

	// Get trader stats
	Trader = Stats.GetTrader(TraderID);

	// Check for required items
	RewardItemSlot = Player->GetRequiredItemSlots(Trader, RequiredItemSlots);

	// Add window
	TabTrader = irrGUI->addTab(Graphics.GetCenteredRect(400, 250, 166, 272));

	// Add background
	IGUIImage *Image = irrGUI->addImage(Graphics.GetImage(GraphicsClass::IMAGE_TRADER), position2di(0, 0), true, TabTrader);

	// Add buttons
	IGUIButton *TradeButton = irrGUI->addButton(Graphics.GetCenteredRect(166/2 - 38, 245, 60, 25), TabTrader, ELEMENT_TRADERACCEPT, L"Trade");
	IGUIButton *CancelButton = irrGUI->addButton(Graphics.GetCenteredRect(166/2 + 38, 245, 60, 25), TabTrader, ELEMENT_TRADERCANCEL, L"Cancel");

	// Can't trade
	if(RewardItemSlot == -1)
		TradeButton->setEnabled(false);

	*State = ClientStateClass::STATE_TRADER;
}

// Close the trader
void HUDClass::CloseTrader() {
	if(*State != ClientStateClass::STATE_TRADER)
		return;

	irrGUI->getRootGUIElement()->removeChild(TabTrader);
	CursorItem.Reset();

	// Notify server
	PacketClass Packet(NetworkClass::EVENT_END);
	ClientNetwork->SendPacketToHost(&Packet);

	*State = ClientStateClass::STATE_WALK;
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
	*State = ClientStateClass::STATE_SKILLS;
	SendBusy(true);

	// Clear old skill categories
	for(int i = 0; i < Stats.GetSkillTypeCount(); i++)
		SkillCategories[i].clear();

	// Generate skill categories
	const array<const SkillClass *> &SkillsUnlocked = Player->GetSkillsUnlocked();
	for(u32 i = 0; i < SkillsUnlocked.size(); i++) {
		const SkillClass *Skill = SkillsUnlocked[i];
		SkillCategories[Skill->GetType()].push_back(Skill);
	}

	// Main window
	TabSkill = irrGUI->addTab(Graphics.GetCenteredRect(400, 300, 450, 400), NULL, 0);
	
	CursorAction.Reset();
	ActionBarChanged = false;
}

// Close the skills screen
void HUDClass::CloseSkills() {

	irrGUI->getRootGUIElement()->removeChild(TabSkill);
	CursorAction.Reset();

	// Send new action bar to server
	if(ActionBarChanged) {
		PacketClass Packet(NetworkClass::ACTIONS_ACTIONBAR);
		for(int i = 0; i < Player->GetActionBarLength(); i++) {
			const ActionClass *Action = Player->GetActionBar(i);
			if(Action) {
				Packet.WriteChar(Action->GetActionType());
				Packet.WriteChar(Action->GetID());
			}
			else
				Packet.WriteChar(-1);
		}
		ClientNetwork->SendPacketToHost(&Packet);
	}

	// No longer busy
	SendBusy(false);

	*State = ClientStateClass::STATE_WALK;
}

// Initialize the trade system
void HUDClass::InitTrade() {

	// Send request to server
	SendTradeRequest();

	CursorItem.Reset();

	// Add window
	TabTrade = irrGUI->addTab(Graphics.GetCenteredRect(400, 300, 280, 470));

	// Add background
	TraderWindow = irrGUI->addImage(Graphics.GetImage(GraphicsClass::IMAGE_TRADE), position2di(TRADE_WINDOWX, TRADE_WINDOWYTHEM), true, TabTrade);
	TraderWindow->setVisible(false);
	irrGUI->addImage(Graphics.GetImage(GraphicsClass::IMAGE_TRADE), position2di(TRADE_WINDOWX, TRADE_WINDOWYYOU), true, TabTrade);

	// Add gold input box
	TradeGoldBox = irrGUI->addEditBox(L"0", Graphics.GetRect(TRADE_WINDOWX + 33, TRADE_WINDOWYYOU + 68, 89, 20), true, TabTrade, ELEMENT_GOLDTRADEBOX);
	TradeGoldBox->setMax(10);

	// Add accept button
	TradeAcceptButton = irrGUI->addButton(Graphics.GetCenteredRect(140, 249, 100, 25), TabTrade, ELEMENT_TRADEACCEPT, L"Accept Trade");
	TradeAcceptButton->setIsPushButton(true);
	TradeAcceptButton->setEnabled(false);

	// Add inventory
	InitInventory(400, 432, false);

	*State = ClientStateClass::STATE_TRADE;
}

// Closes the trade system
void HUDClass::CloseTrade(bool SendNotify) {

	// Remove trade window
	irrGUI->getRootGUIElement()->removeChild(TabTrade);
	TradeAcceptButton = NULL;

	// Close inventory
	CloseInventory();

	// Notify server
	if(SendNotify)
		SendTradeCancel();

	Player->SetTradePlayer(NULL);
	*State = ClientStateClass::STATE_WALK;
}

// Closes all windows
void HUDClass::CloseWindows() {

	switch(*State) {
		case ClientStateClass::STATE_MAINMENU:
			CloseMenu();
		break;
		case ClientStateClass::STATE_VENDOR:
			CloseVendor();
		break;
		case ClientStateClass::STATE_TRADER:
			CloseTrader();
		break;
		case ClientStateClass::STATE_INVENTORY:
			CloseInventory();
		break;
		case ClientStateClass::STATE_SKILLS:
			CloseSkills();
		break;
		case ClientStateClass::STATE_TRADE:
			CloseTrade();
		break;
	}
}

// Draws chat messages
void HUDClass::DrawChat() {
	if(ChatHistory.getSize() == 0)
		return;

	// Set font
	IGUIFont *TextFont = Graphics.GetFont(GraphicsClass::FONT_10);
	Graphics.SetFont(GraphicsClass::FONT_10);

	int Index = 0;
	for(list<ChatStruct>::Iterator Iterator = ChatHistory.getLast(); Iterator != ChatHistory.end(); --Iterator) {
		ChatStruct &Chat = (*Iterator);

		int DrawX = 15;
		int DrawY = 550 - Index * 20;

		// Draw background
		dimension2du TextArea = TextFont->getDimension(stringw(Chat.Message.c_str()).c_str());
		Graphics.DrawBackground(GraphicsClass::IMAGE_BLACK, DrawX - 1, DrawY, TextArea.Width + 2, TextArea.Height, SColor(100, 255, 255, 255));

		// Draw text
		Graphics.RenderText(Chat.Message.c_str(), DrawX, DrawY, GraphicsClass::ALIGN_LEFT, SColor(255, 255, 255, 255));
		Index++;
		if(Index > 20)
			break;
	}
}

// Draws the top HUD area
void HUDClass::DrawTopHUD() {
	int StartX = 125, StartY = 15, Width = 100, Height = 16, Spacing = 20;

	char String[256];
	Graphics.SetFont(GraphicsClass::FONT_14);
	Graphics.RenderText(Player->GetName().c_str(), StartX / 2, 11, GraphicsClass::ALIGN_CENTER);
	Graphics.SetFont(GraphicsClass::FONT_10);

	// Draw experience bar
	sprintf(String, "Level %d", Player->GetLevel());
	Graphics.DrawBar(GraphicsClass::IMAGE_EXPERIENCE, StartX, StartY, Player->GetNextLevelPercent(), Width, Height);
	Graphics.RenderText(String, StartX + Width/2, StartY, GraphicsClass::ALIGN_CENTER);

	// Draw health
	StartX += Width + Spacing;
	float HealthPercent = Player->GetMaxHealth() > 0 ? Player->GetHealth() / (float)Player->GetMaxHealth() : 0;
	sprintf(String, "%d / %d", Player->GetHealth(), Player->GetMaxHealth());
	Graphics.DrawBar(GraphicsClass::IMAGE_HEALTH, StartX, StartY, HealthPercent, Width, Height);
	Graphics.RenderText(String, StartX + Width/2, StartY, GraphicsClass::ALIGN_CENTER);

	// Draw mana
	StartX += Width + Spacing;
	float ManaPercent = Player->GetMaxMana() > 0 ? Player->GetMana() / (float)Player->GetMaxMana() : 0;
	sprintf(String, "%d / %d", Player->GetMana(), Player->GetMaxMana());
	Graphics.DrawBar(GraphicsClass::IMAGE_MANA, StartX, StartY, ManaPercent, Width, Height);
	Graphics.RenderText(String, StartX + Width/2, StartY, GraphicsClass::ALIGN_CENTER);

	// Draw gold
	StartX += Width + Spacing + 14;
	Graphics.DrawImage(GraphicsClass::IMAGE_GOLD, StartX, StartY + 8);

	StartX += 20;
	sprintf(String, "%d", Player->GetGold());
	Graphics.RenderText(String, StartX, StartY);

	// Draw PVP icon
	if(Player->GetTile()->PVP) {
		StartX += 80;
		SColor PVPColor(255, 255, 255, 255);
		if(!Player->CanAttackPlayer())
			PVPColor.set(150, 150, 150, 150);
		Graphics.DrawImage(GraphicsClass::IMAGE_PVP, StartX, StartY + 8, PVPColor);
		Graphics.SetFont(GraphicsClass::FONT_7);
		Graphics.RenderText("A", StartX - 10, StartY + 8, GraphicsClass::ALIGN_LEFT, PVPColor);
		Graphics.SetFont(GraphicsClass::FONT_10);
	}
}

// Draws the player's inventory
void HUDClass::DrawInventory() {
	
	// Get window offsets
	rect<s32> WindowArea = TabInventory->getAbsolutePosition();
	int OffsetX = WindowArea.UpperLeftCorner.X;
	int OffsetY = WindowArea.UpperLeftCorner.Y;

	// Normalize mouse position to window
	position2di Mouse(Input.GetMousePosition());
	Mouse.X -= OffsetX;
	Mouse.Y -= OffsetY;

	// Set default font
	Graphics.SetFont(GraphicsClass::FONT_7);

	// Draw equipped items
	for(int i = 0; i < PlayerClass::INVENTORY_BACKPACK; i++) {
		InventoryStruct *InventoryItem = Player->GetInventory(i);
		if(!CursorItem.IsEqual(i, WINDOW_INVENTORY)) {
		
			// Get render positions
			PositionStruct *Position = &EquippedItemPositions[i];
			int DrawX = OffsetX + Position->X;
			int DrawY = OffsetY + Position->Y;

			// Draw item and price
			if(InventoryItem->Item) {
				Graphics.DrawCenteredImage(InventoryItem->Item->GetImage(), DrawX, DrawY);
				DrawItemPrice(InventoryItem->Item, InventoryItem->Count, DrawX - 16, DrawY - 16, false);
			}
			
			// Set tooltip item
			rect<s32> ItemArea(Position->X - 16, Position->Y - 16, Position->X + 16, Position->Y + 16);
			if(ItemArea.isPointInside(Mouse)) {
				SetTooltipItem(InventoryItem, i);
			}
		}
	}

	// Draw inventory items
	int PositionX = 0, PositionY = 0;
	for(int i = PlayerClass::INVENTORY_BACKPACK; i < PlayerClass::INVENTORY_TRADE; i++) {
		InventoryStruct *InventoryItem = Player->GetInventory(i);
		if(!CursorItem.IsEqual(i, WINDOW_INVENTORY)) {
			int DrawX = OffsetX + 132 + PositionX * 32;
			int DrawY = OffsetY + 1 + PositionY * 32;

			if(InventoryItem->Item) {
				Graphics.DrawCenteredImage(InventoryItem->Item->GetImage(), DrawX + 16, DrawY + 16);
				DrawItemPrice(InventoryItem->Item, InventoryItem->Count, DrawX, DrawY, false);
				if(InventoryItem->Count > 1)
					Graphics.RenderText(stringc(InventoryItem->Count).c_str(), DrawX + 2, DrawY + 20);
			}
		}

		PositionX++;
		if(PositionX > 3) {
			PositionX = 0;
			PositionY++;
		}
	}

	// Get inventory index of mouse
	rect<s32> InventoryArea(132, 1, 259, 192);
	if(InventoryArea.isPointInside(Mouse)) {
		int InventoryIndex = (Mouse.X - 132) / 32 + (Mouse.Y - 1) / 32 * 4 + PlayerClass::INVENTORY_BACKPACK;
		SetTooltipItem(Player->GetInventory(InventoryIndex), InventoryIndex);
	}

	Graphics.SetFont(GraphicsClass::FONT_10);
}

// Draw the town portal sequence
void HUDClass::DrawTownPortal() {

	Graphics.SetFont(GraphicsClass::FONT_14);

	// Get text
	char String[256];
	sprintf(String, "Portal in %d", max_((int)(5 - Player->GetTownPortalTime() / 1000), 1));

	int DrawX = 400;
	int DrawY = 200;
	IGUIFont *TextFont = Graphics.GetFont(GraphicsClass::FONT_14);
	dimension2du TextArea = TextFont->getDimension(stringw(String).c_str());
	TextArea.Width += 5;
	TextArea.Height += 5;

	// Draw text
	Graphics.DrawBackground(GraphicsClass::IMAGE_BLACK, DrawX - 1 - TextArea.Width/2, DrawY-2, TextArea.Width, TextArea.Height, SColor(100, 255, 255, 255));
	Graphics.RenderText(String, DrawX, DrawY, GraphicsClass::ALIGN_CENTER);

	Graphics.SetFont(GraphicsClass::FONT_10);
}

// Draw the vendor
void HUDClass::DrawVendor() {
	
	// Get window offsets
	rect<s32> WindowArea = TabVendor->getAbsolutePosition();
	int OffsetX = WindowArea.UpperLeftCorner.X;
	int OffsetY = WindowArea.UpperLeftCorner.Y;
	int CenterX = WindowArea.getCenter().X;

	// Normalize mouse position to window
	position2di Mouse(Input.GetMousePosition());
	Mouse.X -= OffsetX;
	Mouse.Y -= OffsetY;

	// Draw inventory items
	const ItemClass *Item;
	int PositionX = 0, PositionY = 0;
	for(u32 i = 0; i < Vendor->Items.size(); i++) {
		Item = Vendor->Items[i];
		if(!CursorItem.IsEqual(i, WINDOW_VENDOR)) {
			int DrawX = OffsetX + 1 + PositionX * 32;
			int DrawY = OffsetY + 47 + PositionY * 32;
			Graphics.DrawCenteredImage(Item->GetImage(), DrawX + 16, DrawY + 16);
			DrawItemPrice(Item, 1, DrawX, DrawY, true);
		}

		PositionX++;
		if(PositionX > 7) {
			PositionX = 0;
			PositionY++;
		}
	}

	// Draw name
	Graphics.SetFont(GraphicsClass::FONT_14);
	Graphics.RenderText(Vendor->Name.c_str(), CenterX, OffsetY + 4, GraphicsClass::ALIGN_CENTER);

	// Draw info
	Graphics.SetFont(GraphicsClass::FONT_10);
	Graphics.RenderText(Vendor->Info.c_str(), CenterX, OffsetY + 27, GraphicsClass::ALIGN_CENTER);

	// Get inventory index of mouse
	rect<s32> InventoryArea(1, 47, 256, 239);
	if(InventoryArea.isPointInside(Mouse)) {
		u32 InventoryIndex = (Mouse.X - 1) / 32 + (Mouse.Y - 47) / 32 * 8;
		if(InventoryIndex < Vendor->Items.size()) {
			const ItemClass *Item = Vendor->Items[InventoryIndex];
			int Price = Item->GetPrice(Vendor, 1, true);
			TooltipItem.Set(WINDOW_VENDOR, Item, Price, 1, InventoryIndex);
		}
	}
}

// Draw the trader screen
void HUDClass::DrawTrader() {

	// Get window offsets
	rect<s32> WindowArea = TabTrader->getAbsolutePosition();
	int OffsetX = WindowArea.UpperLeftCorner.X;
	int OffsetY = WindowArea.UpperLeftCorner.Y;
	int CenterX = WindowArea.getCenter().X;

	// Normalize mouse position to window
	position2di Mouse(Input.GetMousePosition());
	Mouse.X -= OffsetX;
	Mouse.Y -= OffsetY;

	// Set font
	Graphics.SetFont(GraphicsClass::FONT_8);

	// Draw items
	int PositionX = 0, PositionY = 0;
	char Buffer[256];
	SColor Color;
	for(u32 i = 0; i < Trader->TraderItems.size(); i++) {
		int DrawX = OffsetX + 19 + PositionX * 32;
		int DrawY = OffsetY + 52 + PositionY * 32 + PositionY * 14;
		Graphics.DrawCenteredImage(Trader->TraderItems[i].Item->GetImage(), DrawX + 16, DrawY + 16);

		if(RequiredItemSlots[i] == -1)
			Color.set(255, 255, 0, 0);
		else
			Color.set(255, 255, 255, 255);
		sprintf(Buffer, "%d", Trader->TraderItems[i].Count);
		Graphics.RenderText(Buffer, DrawX + 16, DrawY - 14, GraphicsClass::ALIGN_CENTER, Color);
	
		PositionX++;
		if(PositionX > 3) {
			PositionX = 0;
			PositionY++;
		}
	}

	// Draw reward item
	Graphics.DrawCenteredImage(Trader->RewardItem->GetImage(), OffsetX + 82, OffsetY + 198);
	sprintf(Buffer, "%d", Trader->Count);
	Graphics.RenderText(Buffer, OffsetX + 82, OffsetY + 166, GraphicsClass::ALIGN_CENTER);
	
	// Draw text
	Graphics.SetFont(GraphicsClass::FONT_10);
	Graphics.RenderText("Looking for", CenterX, OffsetY + 13, GraphicsClass::ALIGN_CENTER);
	Graphics.RenderText("Reward", CenterX, OffsetY + 143, GraphicsClass::ALIGN_CENTER);

	// Get trader slot
	int InventoryIndex = -1;
	if(Mouse.X >= 19 && Mouse.X <= 146 && Mouse.Y >= 52 && Mouse.Y <= 83)
		InventoryIndex = (Mouse.X - 19) / 32 + (Mouse.Y - 52) / 32;
	else if(Mouse.X >= 19 && Mouse.X <= 146 && Mouse.Y >= 98 && Mouse.Y <= 129)
		InventoryIndex = 4 + (Mouse.X - 19) / 32 + (Mouse.Y - 98) / 32;
	else if(Mouse.X >= 67 && Mouse.X <= 98 && Mouse.Y >= 182 && Mouse.Y <= 213)
		InventoryIndex = 9;

	//printf("%d=%d %d\n", InventoryIndex, Mouse.X, Mouse.Y);

	// Set cursor item
	if(InventoryIndex != -1) {
		if(InventoryIndex < (int)Trader->TraderItems.size())
			TooltipItem.Set(WINDOW_TRADER, Trader->TraderItems[InventoryIndex].Item, 0, 1, InventoryIndex);
		else if(InventoryIndex == 9)
			TooltipItem.Set(WINDOW_TRADER, Trader->RewardItem, 0, 1, InventoryIndex);
	}
}

// Draw the trade screen
void HUDClass::DrawTrade() {

	// Get window offsets
	rect<s32> WindowArea = TabTrade->getAbsolutePosition();
	int OffsetX = WindowArea.UpperLeftCorner.X;
	int OffsetY = WindowArea.UpperLeftCorner.Y;
	int CenterX = WindowArea.getCenter().X;

	// Normalize mouse position to window
	position2di Mouse(Input.GetMousePosition());
	Mouse.X -= OffsetX;
	Mouse.Y -= OffsetY;

	// Draw items
	DrawTradeItems(Player, TRADE_WINDOWX, TRADE_WINDOWYYOU, false);

	// Draw trading player information
	PlayerClass *TradePlayer = Player->GetTradePlayer();
	if(TradePlayer) {
		TraderWindow->setVisible(true);

		// Draw player information
		Graphics.RenderText(TradePlayer->GetName().c_str(), OffsetX + 72, OffsetY + 70 - 55, GraphicsClass::ALIGN_CENTER);
		Graphics.DrawCenteredImage(Stats.GetPortrait(TradePlayer->GetPortraitID())->Image, OffsetX + 72, OffsetY + 70);

		// Draw items
		DrawTradeItems(TradePlayer, TRADE_WINDOWX, TRADE_WINDOWYTHEM, true);

		// Draw gold
		Graphics.RenderText(stringc(TradePlayer->GetTradeGold()).c_str(), OffsetX + TRADE_WINDOWX + 35, OffsetY + TRADE_WINDOWYTHEM + 70);

		// Draw agreement state
		stringc AcceptText;
		SColor AccepColor;
		if(TradePlayer->GetTradeAccepted()) {
			AcceptText = "Accepted";
			AccepColor.set(255, 0, 255, 0);
		}
		else {
			AcceptText = "Unaccepted";
			AccepColor.set(255, 255, 0, 0);
		}

		Graphics.RenderText(AcceptText.c_str(), CenterX, OffsetY + TRADE_WINDOWYTHEM + 100, GraphicsClass::ALIGN_CENTER, AccepColor);
		TradeAcceptButton->setEnabled(true);
	}
	else {
		TraderWindow->setVisible(false);
		Graphics.RenderText("Waiting for other player...", 400, 120, GraphicsClass::ALIGN_CENTER);
		TradeAcceptButton->setEnabled(false);
	}

	// Draw player information
	Graphics.RenderText(Player->GetName().c_str(), OffsetX + 72, OffsetY + 198 - 55, GraphicsClass::ALIGN_CENTER);
	Graphics.DrawCenteredImage(Stats.GetPortrait(Player->GetPortraitID())->Image, OffsetX + 72, OffsetY + 198);

	// Get trade slot
	if(TradePlayer && Mouse.X >= TRADE_WINDOWX && Mouse.X < TRADE_WINDOWX + 128 && Mouse.Y >= TRADE_WINDOWYTHEM && Mouse.Y < TRADE_WINDOWYTHEM + 64) {
		u32 InventoryIndex = (Mouse.X - TRADE_WINDOWX) / 32 + (Mouse.Y - TRADE_WINDOWYTHEM) / 32 * 4 + PlayerClass::INVENTORY_TRADE;
		//printf("them: %d=%d %d\n", InventoryIndex, Mouse.X, Mouse.Y);
		const ItemClass *Item = TradePlayer->GetInventory(InventoryIndex)->Item;
		TooltipItem.Set(WINDOW_TRADETHEM, Item, 0, TradePlayer->GetInventory(InventoryIndex)->Count, InventoryIndex);
	}
	else if(Mouse.X >= TRADE_WINDOWX && Mouse.X < TRADE_WINDOWX + 128 && Mouse.Y >= TRADE_WINDOWYYOU && Mouse.Y < TRADE_WINDOWYYOU + 64) {
		u32 InventoryIndex = (Mouse.X - TRADE_WINDOWX) / 32 + (Mouse.Y - TRADE_WINDOWYYOU) / 32 * 4 + PlayerClass::INVENTORY_TRADE;
		//printf("you: %d=%d %d\n", InventoryIndex, Mouse.X, Mouse.Y);
		const ItemClass *Item = Player->GetInventory(InventoryIndex)->Item;
		TooltipItem.Set(WINDOW_TRADEYOU, Item, 0, Player->GetInventory(InventoryIndex)->Count, InventoryIndex);
	}
}

// Draw the character stats page
void HUDClass::DrawCharacter() {
	char Buffer[256];
	int Width = 180;
	int Height = 300;

	int DrawX = 800 - Width;
	int DrawY = 300 - Height / 2;
	int RightDrawX = 800 - 10;
	Graphics.DrawBackground(GraphicsClass::IMAGE_BLACK, DrawX, DrawY, Width, Height, SColor(220, 255, 255, 255));
	DrawX += 10;
	DrawY += 10;

	// Experience
	Graphics.RenderText("EXP", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetExperience());
	Graphics.RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	// Experience needed
	DrawY += 15;
	Graphics.RenderText("EXP needed", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetExperienceNeeded());
	Graphics.RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	// Damage
	DrawY += 15;
	Graphics.RenderText("Damage", DrawX, DrawY);
	sprintf(Buffer, "%d-%d", Player->GetMinDamage(), Player->GetMaxDamage());
	Graphics.RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	// Defense
	DrawY += 15;
	Graphics.RenderText("Defense", DrawX, DrawY);
	sprintf(Buffer, "%d-%d", Player->GetMinDefense(), Player->GetMaxDefense());
	Graphics.RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	// Regen
	DrawY += 15;
	Graphics.RenderText("HP Regen", DrawX, DrawY);
	sprintf(Buffer, "%0.2f%%", Player->GetHealthRegen());
	Graphics.RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	DrawY += 15;
	Graphics.RenderText("MP Regen", DrawX, DrawY);
	sprintf(Buffer, "%0.2f%%", Player->GetManaRegen());
	Graphics.RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	// Stats
	DrawY += 30;
	Graphics.RenderText("Play Time", DrawX, DrawY);

	int Seconds = Player->GetPlayTime();
	if(Seconds < 60)
		sprintf(Buffer, "%ds", Seconds);
	else if(Seconds < 3600)
		sprintf(Buffer, "%dm", Seconds / 60);
	else
		sprintf(Buffer, "%dh%dm", Seconds / 3600, (Seconds / 60 % 60));
	Graphics.RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	DrawY += 15;
	Graphics.RenderText("Deaths", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetDeaths());
	Graphics.RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	DrawY += 15;
	Graphics.RenderText("Monster Kills", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetMonsterKills());
	Graphics.RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	DrawY += 15;
	Graphics.RenderText("Player Kills", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetPlayerKills());
	Graphics.RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);

	DrawY += 15;
	Graphics.RenderText("Bounty", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetBounty());
	Graphics.RenderText(Buffer, RightDrawX, DrawY, GraphicsClass::ALIGN_RIGHT);
}

// Draws the skill page
void HUDClass::DrawSkills() {
	rect<s32> WindowArea = TabSkill->getAbsolutePosition();

	// Draw skills
	int DrawX = WindowArea.UpperLeftCorner.X + 20;
	int DrawY = WindowArea.UpperLeftCorner.Y + 10;
	for(int i = 0; i < Stats.GetSkillTypeCount(); i++) {

		if(SkillCategories[i].size() > 0) {

			// Draw type
			Graphics.RenderText(Stats.GetSkillTypeList()[i].Name.c_str(), DrawX, DrawY);
			DrawY += 32;

			// Draw skills
			int X = 0, Y = 0;
			for(u32 j = 0; j < SkillCategories[i].size(); j++) {
				const SkillClass *Skill = SkillCategories[i][j];

				// Draw image
				int SkillDrawX = DrawX + 10 + X * SKILL_SPACINGX + 16;
				int SkillDrawY = DrawY + Y * SKILL_SPACINGY + 16;
				Graphics.DrawCenteredImage(Skill->GetImage(), SkillDrawX, SkillDrawY);

				// Detect mouse collisions
				rect<s32> SkillArea(SkillDrawX - 16, SkillDrawY - 16, SkillDrawX + 16, SkillDrawY + 16);
				if(SkillArea.isPointInside(Input.GetMousePosition())) {
					TooltipAction.Skill = Skill;
					TooltipAction.Window = WINDOW_SKILLS;
				}

				X++;
				if(X >= SKILL_COLUMNS) {
					X = 0;
					Y++;
				}
			}

			DrawY += 32;
		}
	}
}

// Draws the item under the cursor
void HUDClass::DrawCursorItem() {
	if(CursorItem.Item) {
		int DrawX = Input.GetMousePosition().X - 16;
		int DrawY = Input.GetMousePosition().Y - 16;
		Graphics.SetFont(GraphicsClass::FONT_7);
		Graphics.DrawCenteredImage(CursorItem.Item->GetImage(), DrawX + 16, DrawY + 16);
		DrawItemPrice(CursorItem.Item, CursorItem.Count, DrawX, DrawY, CursorItem.Window == WINDOW_VENDOR);
		if(CursorItem.Count > 1)
			Graphics.RenderText(stringc(CursorItem.Count).c_str(), DrawX + 2, DrawY + 20);
		Graphics.SetFont(GraphicsClass::FONT_10);
	}
}

// Draws information about an item
void HUDClass::DrawItemTooltip() {
	const ItemClass *Item = TooltipItem.Item;
	if(Item) {
		int DrawX = Input.GetMousePosition().X + 16;
		int DrawY = Input.GetMousePosition().Y - 200;
		int Width = 150;
		int Height = 200;
		if(DrawY < 20)
			DrawY = 20;

		// Draw background
		char Buffer[256];
		stringc String;
		Graphics.SetFont(GraphicsClass::FONT_10);
		Graphics.DrawBackground(GraphicsClass::IMAGE_BLACK, DrawX, DrawY, Width, Height);
		DrawX += 10;

		// Draw name
		DrawY += 10;
		Graphics.RenderText(Item->GetName().c_str(), DrawX, DrawY);

		// Draw type
		DrawY += 15;
		Item->GetTypeAsString(String);
		Graphics.RenderText(String.c_str(), DrawX, DrawY);

		/*
		// Draw level
		DrawY += 15;
		sprintf(Buffer, "Level %d", Item->GetLevel());
		Graphics.RenderText(Buffer, DrawX, DrawY);
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
			Graphics.RenderText(Buffer, DrawX, DrawY);
			DrawY += 15;
		}

		// Defense
		Item->GetDefenseRange(Min, Max);
		if(Min != 0 || Max != 0) {
			if(Min != Max)
				sprintf(Buffer, "Defense: %d to %d", Min, Max);
			else
				sprintf(Buffer, "Defense: %d", Min);
			Graphics.RenderText(Buffer, DrawX, DrawY);
			DrawY += 15;
		}

		switch(Item->GetType()) {
			case ItemClass::YPE_WEAPON1HAND:
			case ItemClass::YPE_WEAPON2HAND:
			case ItemClass::YPE_HEAD:
			case ItemClass::YPE_BODY:
			case ItemClass::YPE_LEGS:
			case ItemClass::YPE_SHIELD:
			break;
			case ItemClass::YPE_USEABLE:
				if(Item->GetHealthRestore() > 0) {
					sprintf(Buffer, "HP Restored: %d", Item->GetHealthRestore());
					Graphics.RenderText(Buffer, DrawX, DrawY);
					DrawY += 15;
				}
				if(Item->GetManaRestore() > 0) {
					sprintf(Buffer, "MP Restored: %d", Item->GetManaRestore());
					Graphics.RenderText(Buffer, DrawX, DrawY);
					DrawY += 15;
				}

				if(TooltipItem.Window == WINDOW_INVENTORY) {
					Graphics.RenderText("Right-click to use", DrawX, DrawY, GraphicsClass::ALIGN_LEFT, COLOR_GRAY);
					DrawY += 15;
				}
			break;
		}

		// Boosts
		if(Item->GetMaxHealth() != 0) {
			sprintf(Buffer, "+%d HP", Item->GetMaxHealth());
			Graphics.RenderText(Buffer, DrawX, DrawY);
			DrawY += 15;
		}
		if(Item->GetMaxMana() != 0) {
			sprintf(Buffer, "+%d MP", Item->GetMaxMana());
			Graphics.RenderText(Buffer, DrawX, DrawY);
			DrawY += 15;
		}
		if(Item->GetHealthRegen() != 0) {
			sprintf(Buffer, "+%0.2f%% HP Regen", Item->GetHealthRegen());
			Graphics.RenderText(Buffer, DrawX, DrawY);
			DrawY += 15;
		}
		if(Item->GetManaRegen() != 0) {
			sprintf(Buffer, "+%0.2f%% MP Regen", Item->GetManaRegen());
			Graphics.RenderText(Buffer, DrawX, DrawY);
			DrawY += 15;
		}

		// Vendors
		if(Vendor) {
			DrawY += 15;
			if(TooltipItem.Window == WINDOW_VENDOR) {
				sprintf(Buffer, "$%d", TooltipItem.Cost);
				Graphics.RenderText(Buffer, DrawX, DrawY, GraphicsClass::ALIGN_LEFT, COLOR_GOLD);
				DrawY += 15;
				Graphics.RenderText("Right-click to buy", DrawX, DrawY, GraphicsClass::ALIGN_LEFT, COLOR_GRAY);
			}
			else if(TooltipItem.Window == WINDOW_INVENTORY) {
				sprintf(Buffer, "$%d", TooltipItem.Cost);
				Graphics.RenderText(Buffer, DrawX, DrawY, GraphicsClass::ALIGN_LEFT, COLOR_GOLD);
				DrawY += 15;
				Graphics.RenderText("Shift+Right-click to sell", DrawX, DrawY, GraphicsClass::ALIGN_LEFT, COLOR_GRAY);
			}
		}

		switch(*State) {
			case ClientStateClass::STATE_INVENTORY:
			case ClientStateClass::STATE_TRADE:
				if(TooltipItem.Window == WINDOW_INVENTORY && TooltipItem.Count > 1) {
					Graphics.RenderText("Ctrl+click to split", DrawX, DrawY, GraphicsClass::ALIGN_LEFT, COLOR_GRAY);
					DrawY += 15;
				}
			break;
		}
	}
}

// Draws the skill under the cursor
void HUDClass::DrawCursorAction() {
	if(CursorAction.Skill) {
		int DrawX = Input.GetMousePosition().X - 16;
		int DrawY = Input.GetMousePosition().Y - 16;
		Graphics.DrawCenteredImage(CursorAction.Skill->GetImage(), DrawX + 16, DrawY + 16);
	}
}

// Draws the skill tooltip window
void HUDClass::DrawSkillTooltip() {
	const SkillClass *Skill = TooltipAction.Skill;
	if(Skill) {
		int DrawX = Input.GetMousePosition().X + 16;
		int DrawY = Input.GetMousePosition().Y - 100;
		int Width = 300;
		int Height = 100;

		if(DrawY < 20)
			DrawY = 20;
		if(DrawX + Width > 800)
			DrawX = 800 - Width;

		// Draw background
		stringc String;
		Graphics.SetFont(GraphicsClass::FONT_10);
		Graphics.DrawBackground(GraphicsClass::IMAGE_BLACK, DrawX, DrawY, Width, Height);
		DrawX += 10;
		DrawY += 10;

		// Draw name
		Graphics.RenderText(Skill->GetName().c_str(), DrawX, DrawY);
		DrawY += 15;

		/*
		DrawY += 15;
		sprintf(Buffer, "%d Mana", Skill->GetManaCost(Level));
		Graphics.RenderText(Buffer, DrawX, DrawY, GraphicsClass::ALIGN_LEFT, SColor(255, 0, 0, 255));
		*/
	}
}

// Draws an item's price
void HUDClass::DrawItemPrice(const ItemClass *Item, int Count, int DrawX, int DrawY, bool Buy) {
	if(!Vendor)
		return;

	// Real price
	int Price = Item->GetPrice(Vendor, Count, Buy);

	// Color
	SColor Color;
	if(Buy && Player->GetGold() < Price)
		Color.set(255, 255, 0, 0);
	else
		Color.set(255, 224, 216, 84);

	Graphics.SetFont(GraphicsClass::FONT_7);
	Graphics.RenderText(stringc(Price).c_str(), DrawX + 2, DrawY + 0, GraphicsClass::ALIGN_LEFT, Color);
}

// Draws trading items
void HUDClass::DrawTradeItems(PlayerClass *TradePlayer, int DrawX, int DrawY, bool DrawAll) {
	rect<s32> WindowArea = TabTrade->getAbsolutePosition();
	int OffsetX = WindowArea.UpperLeftCorner.X;
	int OffsetY = WindowArea.UpperLeftCorner.Y;
	int CenterX = WindowArea.getCenter().X;

	Graphics.SetFont(GraphicsClass::FONT_7);

	// Draw trade items
	InventoryStruct *Item;
	int PositionX = 0, PositionY = 0;
	for(int i = PlayerClass::INVENTORY_TRADE; i < PlayerClass::INVENTORY_COUNT; i++) {
		Item = TradePlayer->GetInventory(i);
		if(Item->Item && (DrawAll || !CursorItem.IsEqual(i, WINDOW_TRADEYOU))) {
			int X = OffsetX + DrawX + PositionX * 32;
			int Y = OffsetY + DrawY + PositionY * 32;
			Graphics.DrawCenteredImage(Item->Item->GetImage(), X + 16, Y + 16);
			if(Item->Count > 1)
				Graphics.RenderText(stringc(Item->Count).c_str(), X + 2, Y + 20);
		}

		PositionX++;
		if(PositionX > 3) {
			PositionX = 0;
			PositionY++;
		}
	}
	Graphics.SetFont(GraphicsClass::FONT_10);
}

// Sets the tooltip item for the inventory
void HUDClass::SetTooltipItem(const InventoryStruct *InventoryItem, int Slot) {
	
	// Get price
	int Price = 0;
	if(InventoryItem->Item)
		Price = InventoryItem->Item->GetPrice(Vendor, InventoryItem->Count, false);
	
	// Set tooltip item
	TooltipItem.Set(WINDOW_INVENTORY, InventoryItem->Item, Price, InventoryItem->Count, Slot);
}

// Buys an item from the vendor
void HUDClass::BuyItem(CursorItemStruct *CursorItem, int TargetSlot) {
	if(Player->GetGold() >= CursorItem->Cost && Player->AddItem(CursorItem->Item, CursorItem->Count, TargetSlot)) {

		// Update player
		int Price = CursorItem->Item->GetPrice(Vendor, CursorItem->Count, true);
		Player->UpdateGold(-Price);

		// Notify server
		PacketClass Packet(NetworkClass::VENDOR_EXCHANGE);
		Packet.WriteBit(1);
		Packet.WriteChar(CursorItem->Count);
		Packet.WriteChar(CursorItem->Slot);
		Packet.WriteChar(TargetSlot);
		ClientNetwork->SendPacketToHost(&Packet);

		Player->CalculatePlayerStats();
	}
}

// Sells an item
void HUDClass::SellItem(CursorItemStruct *CursorItem, int Amount) {
	if(!CursorItem->Item)
		return;

	// Update player
	int Price = CursorItem->Item->GetPrice(Vendor, Amount, 0);
	Player->UpdateGold(Price);
	bool Deleted = Player->UpdateInventory(CursorItem->Slot, -Amount);

	// Notify server
	PacketClass Packet(NetworkClass::VENDOR_EXCHANGE);
	Packet.WriteBit(0);
	Packet.WriteChar(Amount);
	Packet.WriteChar(CursorItem->Slot);
	ClientNetwork->SendPacketToHost(&Packet);

	if(Deleted)
		CursorItem->Reset();

	Player->CalculatePlayerStats();
}

// Sets the player's action bar
void HUDClass::SetActionBar(int Index, int OldIndex, const SkillClass *Skill) {
	if(Player->GetActionBar(Index) == Skill)
		return;

	// Check for swapping
	if(OldIndex == -1) {

		// Remove duplicate skills
		for(int i = 0; i < Player->GetActionBarLength(); i++) {
			if(Player->GetActionBar(i) == Skill)
				Player->SetActionBar(i, NULL);
		}
	}
	else {
		const ActionClass *OldAction = Player->GetActionBar(Index);
		Player->SetActionBar(OldIndex, OldAction);
	}

	Player->SetActionBar(Index, Skill);
	Player->CalculatePlayerStats();

	ActionBarChanged = true;
	RefreshActionBar();
}

// Gets an action from the action bar
void HUDClass::GetActionBarTooltip() {

	position2di Mouse(Input.GetMousePosition());
	if(Mouse.X >= SKILL_BARX && Mouse.X < SKILL_BARX + 32 * 8 && Mouse.Y >= SKILL_BARY && Mouse.Y <= SKILL_BARY + 32) {
		int Index = (Mouse.X - SKILL_BARX) / 32;
		TooltipAction.Skill = (const SkillClass *)(Player->GetActionBar(Index));
		TooltipAction.Slot = Index;
		TooltipAction.Window = WINDOW_ACTIONBAR;
	}
}

// Refreshes the action bar images
void HUDClass::RefreshActionBar() {

	for(int i = 0; i < Player->GetActionBarLength(); i++) {
		const ActionClass *Action = Player->GetActionBar(i);
		if(Action) {
			ActionButtons[i]->setImage(Action->GetImage());
			ActionButtons[i]->setPressedImage(Action->GetImage());
		}
		else {
			ActionButtons[i]->setImage(Graphics.GetImage(GraphicsClass::IMAGE_EMPYSLOT));
			ActionButtons[i]->setPressedImage(Graphics.GetImage(GraphicsClass::IMAGE_EMPYSLOT));
		}
	}
}

// Sends the busy signal to the server
void HUDClass::SendBusy(bool Value) {

	PacketClass Packet(NetworkClass::WORLD_BUSY);
	Packet.WriteChar(Value);
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
void HUDClass::SplitStack(int Slot, int Count) {

	// Split only inventory items
	if(!PlayerClass::IsSlotInventory(Slot))
		return;

	// Build packet
	PacketClass Packet(NetworkClass::INVENTORY_SPLIT);
	Packet.WriteChar(Slot);
	Packet.WriteChar(Count);

	ClientNetwork->SendPacketToHost(&Packet);
	Player->SplitStack(Slot, Count);
}

// Toggles the town portal state
void HUDClass::ToggleTownPortal() {
	PacketClass Packet(NetworkClass::WORLD_TOWNPORTAL);
	ClientNetwork->SendPacketToHost(&Packet);
	Player->StartTownPortal();

	if(*State == ClientStateClass::STATE_TOWNPORTAL)
		*State = ClientStateClass::STATE_WALK;
	else
		*State = ClientStateClass::STATE_TOWNPORTAL;
}
