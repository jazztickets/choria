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
#include <hud.h>
#include <framework.h>
#include <graphics.h>
#include <globals.h>
#include <input.h>
#include <stats.h>
#include <font.h>
#include <constants.h>
#include <buffer.h>
#include <assets.h>
#include <ui/element.h>
#include <ui/button.h>
#include <ui/label.h>
#include <ui/textbox.h>
#include <ui/image.h>
#include <states/client.h>
#include <network/network.h>
#include <instances/map.h>
#include <objects/player.h>
#include <objects/item.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

_HUD HUD;

static glm::ivec2 EquippedItemPositions[_Player::INVENTORY_BACKPACK] = {
	{ 61,  4   },
	{ 61,  67  },
	{ 64,  234 },
	{ 12,  125 },
	{ 111, 125 },
	{ 12,  176 },
	{ 111, 176 }
};

// Initialize
void _HUD::Init() {

	State = ClientState.GetState();
	Vendor = nullptr;
	Trader = nullptr;
	TooltipItem.Reset();
	CursorItem.Reset();
	TooltipSkill.Reset();
	CursorSkill.Reset();
	CharacterOpen = false;
	Chatting = false;
	TypingGold = false;
	RewardItemSlot = -1;
	ChatHistory.clear();
}

// Shutdown
void _HUD::Close() {
}

// Mouse events
void _HUD::MouseEvent(const _MouseEvent &MouseEvent) {

	// Press
	if(MouseEvent.Pressed) {
		switch(*State) {
			case _ClientState::STATE_VENDOR:
				switch(MouseEvent.Button) {
					case SDL_BUTTON_LEFT:
						if(TooltipItem.Item) {
							CursorItem = TooltipItem;
						}
					break;
					case SDL_BUTTON_RIGHT:
						if(TooltipItem.Item) {
							if(TooltipItem.Window == WINDOW_VENDOR)
								BuyItem(&TooltipItem, -1);
							else if(TooltipItem.Window == WINDOW_INVENTORY && Input.ModKeyDown(KMOD_SHIFT))
								SellItem(&TooltipItem, 1);
						}
					break;
				}
			break;
			case _ClientState::STATE_INVENTORY: {
				switch(MouseEvent.Button) {
					case SDL_BUTTON_LEFT:
						if(TooltipItem.Item) {
							if(Input.ModKeyDown(KMOD_CTRL))
								SplitStack(TooltipItem.Slot, 1);
							else
								CursorItem = TooltipItem;
						}
					break;
					case SDL_BUTTON_RIGHT:
						if(Player->UseInventory(TooltipItem.Slot)) {
							_Buffer Packet;
							Packet.Write<char>(_Network::INVENTORY_USE);
							Packet.Write<char>(TooltipItem.Slot);
							ClientNetwork->SendPacketToHost(&Packet);
						}
					break;
				}
			} break;
			case _ClientState::STATE_TRADE:
				switch(MouseEvent.Button) {
					case SDL_BUTTON_LEFT:
						if(TooltipItem.Item && TooltipItem.Window != WINDOW_TRADETHEM) {
							if(TooltipItem.Window == WINDOW_INVENTORY && Input.ModKeyDown(KMOD_CTRL))
								SplitStack(TooltipItem.Slot, 1);
							else
								CursorItem = TooltipItem;
						}
					break;
				}
			break;
			case _ClientState::STATE_SKILLS:
				switch(MouseEvent.Button) {
					case SDL_BUTTON_LEFT:
						if(TooltipSkill.Skill && Player->GetSkillLevel(TooltipSkill.Skill->GetID()) > 0)
							CursorSkill = TooltipSkill;
					break;
				}
			break;
		}
	}
	// Release
	else {
		switch(*State) {
			case _ClientState::STATE_INVENTORY:
			case _ClientState::STATE_VENDOR:
			case _ClientState::STATE_TRADE:
				if(MouseEvent.Button == SDL_BUTTON_LEFT) {

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
											_Buffer Packet;
											Packet.Write<char>(_Network::INVENTORY_MOVE);
											Packet.Write<char>(CursorItem.Slot);
											Packet.Write<char>(TooltipItem.Slot);
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
			case _ClientState::STATE_SKILLS:
				if(MouseEvent.Button == SDL_BUTTON_LEFT) {

					// Check for valid slots
					if(CursorSkill.Skill) {

						if(TooltipSkill.Window == WINDOW_SKILLBAR) {
							if(CursorSkill.Window == WINDOW_SKILLS)
								SetSkillBar(TooltipSkill.Slot, -1, CursorSkill.Skill);
							else if(CursorSkill.Window == WINDOW_SKILLBAR)
								SetSkillBar(TooltipSkill.Slot, CursorSkill.Slot, CursorSkill.Skill);
						}
						else if(CursorSkill.Window == WINDOW_SKILLBAR && TooltipSkill.Slot == -1) {
							SetSkillBar(CursorSkill.Slot, -1, nullptr);
						}
					}

					CursorSkill.Reset();
				}
			break;
		}
	}
}

// Handles GUI presses
/*
void _HUD::HandleGUI(gui::EGUI_EVENT_TYPE TEventType, gui::IGUIElement *TElement) {
	int ID = TElement->getID();

	if(TEventType == gui::EGET_BUTTON_CLICKED && ID == ELEMENT_CHARACTER) {
		InitCharacter();
	}

	switch(*State) {
		case _PlayClientState::STATE_WALK:
			switch(TEventType) {
				case gui::EGET_BUTTON_CLICKED:
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
						default:
						break;
					}
				break;
				default:
				break;
			}
		break;
		case _PlayClientState::STATE_MAINMENU:
			switch(TEventType) {
				case gui::EGET_BUTTON_CLICKED:
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
				default:
				break;
			}
		break;
		case _PlayClientState::STATE_TOWNPORTAL:
			switch(TEventType) {
				case gui::EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_TOWNPORTAL:
							ToggleTownPortal();
						break;
					}
				break;
				default:
				break;
			}
		break;
		case _PlayClientState::STATE_TRADER:
			switch(TEventType) {
				case gui::EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_TRADERACCEPT: {
							_Buffer Packet;
							Packet.Write<char>(_Network::TRADER_ACCEPT);
							ClientNetwork->SendPacketToHost(&Packet);
							Player->AcceptTrader(Trader, RequiredItemSlots, RewardItemSlot);
							Player->CalculatePlayerStats();
							CloseWindows();
						}
						break;
						case ELEMENT_TRADERCANCEL:
							CloseWindows();
						break;
						default:
						break;
					}
				break;
				default:
				break;
			}
		break;
		case _PlayClientState::STATE_TRADE:
			switch(TEventType) {
				case gui::EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_TRADE:
							CloseWindows();
						break;
						case ELEMENT_TRADEACCEPT: {
							_Buffer Packet;
							Packet.Write<char>(_Network::TRADE_ACCEPT);
							//Packet.Write<char>(TradeAcceptButton->isPressed());
							ClientNetwork->SendPacketToHost(&Packet);
						}
						break;
						default:
						break;
					}
				break;
				case gui::EGET_ELEMENT_FOCUSED:
					if(ID == ELEMENT_GOLDTRADEBOX) {
						TypingGold = true;
					}
				break;
				case gui::EGET_ELEMENT_FOCUS_LOST:
				case gui::EGET_EDITBOX_ENTER:
					if(ID == ELEMENT_GOLDTRADEBOX) {
						TypingGold = false;
						//if(TEventType == gui::EGET_EDITBOX_ENTER)
						//	irrGUI->removeFocus(TradeGoldBox);

						// Send amount
						int GoldAmount = ValidateTradeGold();
						_Buffer Packet;
						Packet.Write<char>(_Network::TRADE_GOLD);
						Packet.Write<int32_t>(GoldAmount);
						ClientNetwork->SendPacketToHost(&Packet);

						// Reset agreement
						ResetAcceptButton();
					}
				break;
				default:
				break;
			}
		break;
		case _PlayClientState::STATE_SKILLS:
			switch(TEventType) {
				case gui::EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_SKILLS:
							CloseWindows();
						break;
					}

					// Skills
					if(ID >= ELEMENT_SKILLPLUS0) {
						_Buffer Packet;
						Packet.Write<char>(_Network::SKILLS_SKILLADJUST);

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
								const _Skill *Skill = Stats.GetSkill(SkillID);
								if(Skill) {
									int Direction, Slot;
									if(Skill->GetType() == _Skill::TYPE_PASSIVE) {
										Slot = 7;
										Direction = -1;
									}
									else {
										Slot = 0;
										Direction = 1;
									}
									for(int i = 0; i < 8; i++) {
										if(Player->GetSkillBar(Slot) == nullptr) {
											SetSkillBar(Slot, -1, Skill);
											break;
										}
										Slot += Direction;
									}
								}
							}
						}
						Packet.Write<char>(SkillID);
						ClientNetwork->SendPacketToHost(&Packet);

						// Update player
						Player->CalculateSkillPoints();
						Player->CalculatePlayerStats();
						RefreshSkillButtons();
					}
				break;
				default:
				break;
			}
		break;
		default:
		break;
	}
}
*/

// Updates the HUD
void _HUD::Update(double FrameTime) {
	Assets.Elements["element_hud"]->Update(FrameTime, Input.GetMouse());
	Assets.Elements["element_hud_buttonbar"]->Update(FrameTime, Input.GetMouse());
	TooltipItem.Reset();

	switch(*State) {
		case _ClientState::STATE_VENDOR:
		case _ClientState::STATE_INVENTORY: {
			_Element *InventoryElement = Assets.Elements["element_inventory"];
			_Element *VendorElement = Assets.Elements["element_vendor"];
			_Element *HoverSlot;
			InventoryElement->Update(FrameTime, Input.GetMouse());
			HoverSlot = InventoryElement->HitElement;
			TooltipItem.Window = WINDOW_INVENTORY;

			// Get vendor hover item
			if(!HoverSlot && *State == _ClientState::STATE_VENDOR) {
				VendorElement->Update(FrameTime, Input.GetMouse());
				HoverSlot = VendorElement->HitElement;
				TooltipItem.Window = WINDOW_VENDOR;
			}

			// Set tooltip item
			if(HoverSlot && HoverSlot != InventoryElement && HoverSlot != VendorElement) {
				size_t InventoryIndex = (intptr_t)HoverSlot->UserData;
				if(TooltipItem.Window == WINDOW_INVENTORY) {
					_InventorySlot *InventorySlot = Player->GetInventory(InventoryIndex);

					// Get price if vendor is open
					int Price = 0;
					if(InventorySlot->Item)
						Price = InventorySlot->Item->GetPrice(Vendor, InventorySlot->Count, false);

					TooltipItem.Set(InventorySlot->Item, Price, InventorySlot->Count, InventoryIndex);
				}
				else if(TooltipItem.Window == WINDOW_VENDOR) {
					if(InventoryIndex < Vendor->Items.size()) {
						const _Item *Item = Vendor->Items[InventoryIndex];
						if(Item) {
							int Price = Item->GetPrice(Vendor, 1, true);
							TooltipItem.Set(Item, Price, 1, InventoryIndex);
						}
					}
				}
			}
		} break;
		default:
		break;
	}

	// Chat messages
	for(std::list<_ChatMessage>::iterator Iterator = ChatHistory.begin(); Iterator != ChatHistory.end();) {
		_ChatMessage &Chat = (*Iterator);
		Chat.TimeOut += FrameTime;
		if(Chat.TimeOut > NETWORKING_CHAT_TIMEOUT) {
			Iterator = ChatHistory.erase(Iterator);
		}
		else {
			++Iterator;
		}
	}
}

// Draws the HUD elements
void _HUD::Render() {
	Assets.Elements["element_hud"]->Render();
	//Assets.Elements["element_hud_bottom"]->Render();
	Assets.Elements["element_hud_buttonbar"]->Render();
	Assets.Elements["element_hud_actionbar"]->Render();
	std::stringstream Buffer;

	// Set hud values
	Buffer << "Level " << Player->GetLevel();
	Assets.Labels["label_hud_level"]->Text = Buffer.str();
	Buffer.str("");

	Buffer << Player->GetGold() << " Gold";
	Assets.Labels["label_hud_gold"]->Text = Buffer.str();
	Buffer.str("");

	// Draw experience bar
	Buffer << Player->GetExperienceNextLevel() - Player->GetExperienceNeeded() << " / " << Player->GetExperienceNextLevel() << " XP";
	Assets.Labels["label_hud_experience"]->Text = Buffer.str();
	Buffer.str("");
	Assets.Images["image_hud_experience_bar_full"]->SetWidth(Assets.Elements["element_hud_experience"]->Size.x * Player->GetNextLevelPercent());
	Assets.Images["image_hud_experience_bar_empty"]->SetWidth(Assets.Elements["element_hud_experience"]->Size.x);
	Assets.Elements["element_hud_experience"]->Render();

	// Draw health bar
	float HealthPercent = Player->GetMaxHealth() > 0 ? Player->GetHealth() / (float)Player->GetMaxHealth() : 0;
	Buffer << Player->GetHealth() << " / " << Player->GetMaxHealth();
	Assets.Labels["label_hud_health"]->Text = Buffer.str();
	Buffer.str("");
	Assets.Images["image_hud_health_bar_full"]->SetWidth(Assets.Elements["element_hud_health"]->Size.x * HealthPercent);
	Assets.Images["image_hud_health_bar_empty"]->SetWidth(Assets.Elements["element_hud_health"]->Size.x);
	Assets.Elements["element_hud_health"]->Render();

	// Draw mana bar
	float ManaPercent = Player->GetMaxMana() > 0 ? Player->GetMana() / (float)Player->GetMaxMana() : 0;
	Buffer << Player->GetMana() << " / " << Player->GetMaxMana();
	Assets.Labels["label_hud_mana"]->Text = Buffer.str();
	Buffer.str("");
	Assets.Images["image_hud_mana_bar_full"]->SetWidth(Assets.Elements["element_hud_mana"]->Size.x * ManaPercent);
	Assets.Images["image_hud_mana_bar_empty"]->SetWidth(Assets.Elements["element_hud_mana"]->Size.x);
	Assets.Elements["element_hud_mana"]->Render();
	/*
		// Draw gold
		StartX += Width + Spacing + 14;
		Graphics.DrawImage(_Graphics::IMAGE_GOLD, StartX, StartY + 8);

		StartX += 20;
		sprintf(String, "%d", Player->GetGold());
		//Graphics.RenderText(String, StartX, StartY);

		// Draw PVP icon
		if(Player->GetTile()->PVP) {
			StartX += 80;
			video::SColor PVPColor(255, 255, 255, 255);
			if(!Player->CanAttackPlayer())
				PVPColor.set(150, 150, 150, 150);
			Graphics.DrawImage(_Graphics::IMAGE_PVP, StartX, StartY + 8, PVPColor);
			Graphics.SetFont(_Graphics::FONT_7);
			//Graphics.RenderText("A", StartX - 10, StartY + 8, _Graphics::ALIGN_LEFT, PVPColor);
			Graphics.SetFont(_Graphics::FONT_10);
		}

		// Draw RTT
		if(ClientNetwork->GetRTT()) {
			sprintf(String, "%d ms", ClientNetwork->GetRTT());
			//Graphics.RenderText(String, 10, 600 - 25);
		}*/

	switch(*State) {
		case _ClientState::STATE_INVENTORY:
			DrawInventory();
		break;
		case _ClientState::STATE_VENDOR:
			DrawVendor();
			DrawInventory();
		break;
		case _ClientState::STATE_TRADER:
			DrawTrader();
		break;
		case _ClientState::STATE_SKILLS:
			DrawSkills();
		break;
		case _ClientState::STATE_TRADE:
			DrawTrade();
			DrawInventory();
		break;
		case _ClientState::STATE_TOWNPORTAL:
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

// Set player for HUD
void _HUD::SetPlayer(_Player *Player) {
	this->Player = Player;

	Assets.Labels["label_hud_name"]->Text = Player->GetName();
}

// Starts the chat box
void _HUD::ToggleChat() {
/*
	if(Chatting) {
		std::string Message = std::string(ChatBox->getText());
		Message.trim();
		if(Message != "") {

			// Add message to history
			_ChatMessage Chat;
			Chat.Message = Player->GetName() + std::string(": ") + Message;
			ChatHistory.push_back(Chat);
			printf("%s\n", Chat.Message.c_str());

			// Send message to server
			_Buffer Packet;
			Packet.Write<char>(_Network::CHAT_MESSAGE);
			Packet.WriteString(Message.c_str());
			ClientNetwork->SendPacketToHost(&Packet);
		}

		CloseChat();
	}
	else {
		ChatBox = irrGUI->addEditBox(L"", Graphics.GetRect(25, 570, 200, 20), true, nullptr, ELEMENT_CHATBOX);
		ChatBox->setOverrideFont(Graphics.GetFont(_Graphics::FONT_10));
		ChatBox->setMax(NETWORKING_CHAT_SIZE);
		irrGUI->setFocus(ChatBox);
		Chatting = true;
	}*/
}

// Closes the chat window
void _HUD::CloseChat() {
	//irrGUI->getRootGUIElement()->removeChild(ChatBox);
	Chatting = false;
}

// Initialize the main menu
void _HUD::InitMenu() {

	// Add window
	//TabMenu = irrGUI->addTab(Graphics.GetRect(0, 0, 800, 500));

	// Add menu buttons
	//irrGUI->addButton(Graphics.GetCenteredRect(400, 275, 100, 25), TabMenu, ELEMENT_MAINMENURESUME, L"Resume");
	//irrGUI->addButton(Graphics.GetCenteredRect(400, 325, 100, 25), TabMenu, ELEMENT_MAINMENUEXIT, L"Exit");

	*State = _ClientState::STATE_MAINMENU;
}

// Close main menu
void _HUD::CloseMenu() {

	//irrGUI->getRootGUIElement()->removeChild(TabMenu);

	*State = _ClientState::STATE_WALK;
}

// Initialize the inventory system
void _HUD::InitInventory(bool SendBusy) {
	if(SendBusy)
		SendBusySignal(true);

	CursorItem.Reset();
	TooltipItem.Reset();

	*State = _ClientState::STATE_INVENTORY;
}

// Close the inventory system
void _HUD::CloseInventory() {

	//irrGUI->getRootGUIElement()->removeChild(TabInventory);
	CursorItem.Reset();
	TooltipItem.Reset();

	// No longer busy
	SendBusySignal(false);

	*State = _ClientState::STATE_WALK;
}

// Initialize the vendor
void _HUD::InitVendor(int VendorID) {
	if(*State == _ClientState::STATE_VENDOR)
		return;

	// Get vendor stats
	Vendor = Stats.GetVendor(VendorID);

	// Open inventory
	InitInventory(false);
}

// Close the vendor
void _HUD::CloseVendor() {
	if(*State != _ClientState::STATE_VENDOR)
		return;

	//irrGUI->getRootGUIElement()->removeChild(TabVendor);
	CursorItem.Reset();

	// Close inventory
	CloseInventory();

	// Notify server
	_Buffer Packet;
	Packet.Write<char>(_Network::EVENT_END);
	ClientNetwork->SendPacketToHost(&Packet);

	*State = _ClientState::STATE_WALK;
	Vendor = nullptr;
}

// Initialize the trader
void _HUD::InitTrader(int TTraderID) {
	if(*State == _ClientState::STATE_TRADER)
		return;

	// Get trader stats
	Trader = Stats.GetTrader(TTraderID);

	// Check for required items
	RewardItemSlot = Player->GetRequiredItemSlots(Trader, RequiredItemSlots);

	// Add window
	//TabTrader = irrGUI->addTab(Graphics.GetCenteredRect(400, 250, 166, 272));

	// Add background
//	irrGUI->addImage(Graphics.GetImage(_Graphics::IMAGE_TRADER), glm::ivec2(0, 0), true, TabTrader);

	// Add buttons
	//gui::IGUIButton *TradeButton = irrGUI->addButton(Graphics.GetCenteredRect(166/2 - 38, 245, 60, 25), TabTrader, ELEMENT_TRADERACCEPT, L"Trade");
	//irrGUI->addButton(Graphics.GetCenteredRect(166/2 + 38, 245, 60, 25), TabTrader, ELEMENT_TRADERCANCEL, L"Cancel");

	// Can't trade
	//if(RewardItemSlot == -1)
	//	TradeButton->setEnabled(false);

	*State = _ClientState::STATE_TRADER;
}

// Close the trader
void _HUD::CloseTrader() {
	if(*State != _ClientState::STATE_TRADER)
		return;

	//irrGUI->getRootGUIElement()->removeChild(TabTrader);
	TooltipItem.Reset();
	CursorItem.Reset();

	// Notify server
	_Buffer Packet;
	Packet.Write<char>(_Network::EVENT_END);
	ClientNetwork->SendPacketToHost(&Packet);

	*State = _ClientState::STATE_WALK;
	Trader = nullptr;
}

// Initialize the character screen
void _HUD::InitCharacter() {
	if(!CharacterOpen) {
		CharacterOpen = true;
	}
	else {
		CloseCharacter();
	}
}

// Closes the character screen
void _HUD::CloseCharacter() {

	if(CharacterOpen) {
		CharacterOpen = false;
	}
}

// Initialize the skills screen
void _HUD::InitSkills() {
	*State = _ClientState::STATE_SKILLS;
	SendBusySignal(true);

	// Main window
	//TabSkill = irrGUI->addTab(Graphics.GetCenteredRect(400, 300, 450, 400), nullptr, 0);

	// Add +/- buttons
	const std::vector<_Skill> &Skills = Stats.GetSkillList();
	int X = 0, Y = 0;
	for(size_t i = 0; i < Skills.size(); i++) {
		//int DrawX = X * SKILL_SPACINGX + SKILL_STARTX + 16;
		//int DrawY = Y * SKILL_SPACINGY + SKILL_STARTY + 45;

		// Add buy/sell button
		//gui::IGUIButton *BuyButton = irrGUI->addButton(Graphics.GetCenteredRect(DrawX - 8, DrawY, 12, 12), TabSkill, ELEMENT_SKILLPLUS0 + i);
		//BuyButton->setImage(Graphics.GetImage(_Graphics::IMAGE_PLUS));
		//gui::IGUIButton *SellButton = irrGUI->addButton(Graphics.GetCenteredRect(DrawX + 8, DrawY, 12, 12), TabSkill, ELEMENT_SKILLMINUS0 + i);
		//SellButton->setImage(Graphics.GetImage(_Graphics::IMAGE_MINUS));

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
void _HUD::CloseSkills() {

	//irrGUI->getRootGUIElement()->removeChild(TabSkill);
	CursorSkill.Reset();
	TooltipSkill.Reset();

	// Send new skill bar to server
	if(SkillBarChanged) {
		_Buffer Packet;
		Packet.Write<char>(_Network::SKILLS_SKILLBAR);
		for(int i = 0; i < BATTLE_MAXSKILLS; i++)
			Packet.Write<char>(Player->GetSkillBarID(i));

		ClientNetwork->SendPacketToHost(&Packet);
	}

	// No longer busy
	SendBusySignal(false);

	*State = _ClientState::STATE_WALK;
}

// Initialize the trade system
void _HUD::InitTrade() {
/*
	// Send request to server
	SendTradeRequest();

	CursorItem.Reset();
	TooltipItem.Reset();

	// Add window
	TabTrade = irrGUI->addTab(Graphics.GetCenteredRect(400, 300, 280, 470));

	// Add background
	TraderWindow = irrGUI->addImage(Graphics.GetImage(_Graphics::IMAGE_TRADE), glm::ivec2(TRADE_WINDOWX, TRADE_WINDOWYTHEM), true, TabTrade);
	TraderWindow->setVisible(false);
	irrGUI->addImage(Graphics.GetImage(_Graphics::IMAGE_TRADE), glm::ivec2(TRADE_WINDOWX, TRADE_WINDOWYYOU), true, TabTrade);

	// Add gold input box
	TradeGoldBox = irrGUI->addEditBox(L"0", Graphics.GetRect(TRADE_WINDOWX + 33, TRADE_WINDOWYYOU + 68, 89, 20), true, TabTrade, ELEMENT_GOLDTRADEBOX);
	TradeGoldBox->setMax(10);

	// Add accept button
	TradeAcceptButton = irrGUI->addButton(Graphics.GetCenteredRect(140, 249, 100, 25), TabTrade, ELEMENT_TRADEACCEPT, L"Accept Trade");
	TradeAcceptButton->setIsPushButton(true);
	TradeAcceptButton->setEnabled(false);

	// Add inventory
	InitInventory(400, 432, false);
*/
	*State = _ClientState::STATE_TRADE;
}

// Closes the trade system
void _HUD::CloseTrade(bool TSendNotify) {

	// Remove trade window
	//irrGUI->getRootGUIElement()->removeChild(TabTrade);
	//TradeAcceptButton = nullptr;

	// Close inventory
	CloseInventory();

	// Notify server
	if(TSendNotify)
		SendTradeCancel();

	Player->SetTradePlayer(nullptr);
	*State = _ClientState::STATE_WALK;
}

// Closes all windows
void _HUD::CloseWindows() {

	switch(*State) {
		case _ClientState::STATE_MAINMENU:
			CloseMenu();
		break;
		case _ClientState::STATE_VENDOR:
			CloseVendor();
		break;
		case _ClientState::STATE_TRADER:
			CloseTrader();
		break;
		case _ClientState::STATE_INVENTORY:
			CloseInventory();
		break;
		case _ClientState::STATE_SKILLS:
			CloseSkills();
		break;
		case _ClientState::STATE_TRADE:
			CloseTrade();
		break;
	}
}

// Draws chat messages
void _HUD::DrawChat() {
	/*
	if(ChatHistory.size() == 0)
		return;

	int Index = 0;
	for(std::list<_ChatMessage>::reverse_iterator Iterator = ChatHistory.rbegin(); Iterator != ChatHistory.rend(); ++Iterator) {
		_ChatMessage &Chat = (*Iterator);

		int DrawX = 15;
		int DrawY = 550 - Index * 20;

		// Draw background
		//core::dimension2du TextArea = TextFont->getDimension(core::stringw(Chat.Message.c_str()).c_str());
		//Graphics.DrawBackground(_Graphics::IMAGE_BLACK, DrawX - 1, DrawY, TextArea.Width + 2, TextArea.Height, video::SColor(100, 255, 255, 255));

		// Draw text
		//Graphics.RenderText(Chat.Message.c_str(), DrawX, DrawY, _Graphics::ALIGN_LEFT, video::SColor(255, 255, 255, 255));
		Index++;
		if(Index > 20)
			break;
	}
	*/
}

// Draw the town portal sequence
void _HUD::DrawTownPortal() {

	/*

	Graphics.SetFont(_Graphics::FONT_14);

	// Get text
	char String[256];
	double TimeLeft = GAME_PORTALTIME - Player->GetTownPortalTime();
	sprintf(String, "Portal in %.2f", std::max(TimeLeft, 0.0));

	int DrawX = 400;
	int DrawY = 200;
	gui::IGUIFont *TextFont = Graphics.GetFont(_Graphics::FONT_14);
	core::dimension2du TextArea = TextFont->getDimension(core::stringw(String).c_str());
	TextArea.Width += 5;
	TextArea.Height += 5;

	// Draw text
	Graphics.DrawBackground(_Graphics::IMAGE_BLACK, DrawX - 1 - TextArea.Width/2, DrawY-2, TextArea.Width, TextArea.Height, video::SColor(100, 255, 255, 255));
	//Graphics.RenderText(String, DrawX, DrawY, _Graphics::ALIGN_CENTER);

	Graphics.SetFont(_Graphics::FONT_10);
	*/
}

// Draws the player's inventory
void _HUD::DrawInventory() {
	Assets.Elements["element_inventory"]->Render();

	// Get position of window
	glm::ivec2 Offset = Assets.Elements["element_inventory"]->Bounds.Start;

	// Draw equipped items
	for(int i = 0; i < _Player::INVENTORY_BACKPACK; i++) {
		_InventorySlot *Item = Player->GetInventory(i);
		glm::ivec2 &Position = EquippedItemPositions[i];
		if(Item->Item && !CursorItem.IsEqual(i, WINDOW_INVENTORY)) {
			glm::ivec2 DrawPosition = Offset + Position + glm::ivec2(24, 24);
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.DrawCenteredImage(DrawPosition, Item->Item->GetImage());
			DrawItemPrice(Item->Item, Item->Count, DrawPosition, false);
		}
	}

	// Draw inventory items
	glm::ivec2 GridPosition(0, 0);
	for(int i = _Player::INVENTORY_BACKPACK; i < _Player::INVENTORY_TRADE; i++) {
		_InventorySlot *Item = Player->GetInventory(i);
		if(Item->Item && !CursorItem.IsEqual(i, WINDOW_INVENTORY)) {
			glm::ivec2 DrawPosition(Offset.x + INVENTORY_BAG_OFFSET + GridPosition.x * 48 + 24, Offset.y + GridPosition.y * 48 + 24);
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.DrawCenteredImage(DrawPosition, Item->Item->GetImage());
			DrawItemPrice(Item->Item, Item->Count, DrawPosition, false);
			if(Item->Count > 1) {
				Assets.Fonts["hud_small"]->DrawText(std::to_string(Item->Count).c_str(), DrawPosition + glm::ivec2(20, 20), glm::vec4(1.0f), RIGHT_BASELINE);
			}
		}

		GridPosition.x++;
		if(GridPosition.x >= INVENTORY_BAG_COLUMNS) {
			GridPosition.x = 0;
			GridPosition.y++;
		}
	}
}

// Draw the vendor
void _HUD::DrawVendor() {
	Assets.Elements["element_vendor"]->Render();

	// Get position of window
	glm::ivec2 Offset = Assets.Elements["element_vendor"]->Bounds.Start;

	// Draw inventory items
	glm::ivec2 GridPosition(0, 0);
	for(size_t i = 0; i < Vendor->Items.size(); i++) {
		const _Item *Item = Vendor->Items[i];
		if(Item && !CursorItem.IsEqual(i, WINDOW_VENDOR)) {
			glm::ivec2 DrawPosition(Offset.x + GridPosition.x * 48 + 24, Offset.y + GridPosition.y * 48 + 24);
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.DrawCenteredImage(DrawPosition, Item->GetImage());
			DrawItemPrice(Item, 1, DrawPosition, true);
		}

		GridPosition.x++;
		if(GridPosition.x >= VENDOR_BAG_COLUMNS) {
			GridPosition.x = 0;
			GridPosition.y++;
		}
	}
	/*

	// Draw name
	Graphics.SetFont(_Graphics::FONT_14);
	//Graphics.RenderText(Vendor->Name.c_str(), CenterX, OffsetY + 4, _Graphics::ALIGN_CENTER);

	// Draw info
	Graphics.SetFont(_Graphics::FONT_10);
	//Graphics.RenderText(Vendor->Info.c_str(), CenterX, OffsetY + 27, _Graphics::ALIGN_CENTER);

	Graphics.SetFont(_Graphics::FONT_10);
	*/
}

// Draw the trader screen
void _HUD::DrawTrader() {
	/*
	core::recti WindowArea = TabTrader->getAbsolutePosition();
	int OffsetX = WindowArea.UpperLeftCorner.x;
	int OffsetY = WindowArea.UpperLeftCorner.y;
	int CenterX = WindowArea.getCenter().x;

	Graphics.SetFont(_Graphics::FONT_8);

	// Draw items
	int PositionX = 0, PositionY = 0;
	char Buffer[256];
	video::SColor Color;
	for(size_t i = 0; i < Trader->TraderItems.size(); i++) {
		int DrawX = OffsetX + 19 + PositionX * 32;
		int DrawY = OffsetY + 52 + PositionY * 32 + PositionY * 14;
		Graphics.DrawCenteredImage(Trader->TraderItems[i].Item->GetImage(), DrawX + 16, DrawY + 16);

		if(RequiredItemSlots[i] == -1)
			Color.set(255, 255, 0, 0);
		else
			Color.set(255, 255, 255, 255);
		sprintf(Buffer, "%d", Trader->TraderItems[i].Count);
		//Graphics.RenderText(Buffer, DrawX + 16, DrawY - 14, _Graphics::ALIGN_CENTER, Color);

		PositionX++;
		if(PositionX > 3) {
			PositionX = 0;
			PositionY++;
		}
	}

	// Draw reward item
	Graphics.DrawCenteredImage(Trader->RewardItem->GetImage(), OffsetX + 82, OffsetY + 198);
	sprintf(Buffer, "%d", Trader->Count);
	//Graphics.RenderText(Buffer, OffsetX + 82, OffsetY + 166, _Graphics::ALIGN_CENTER);

	// Draw text
	Graphics.SetFont(_Graphics::FONT_10);
	//Graphics.RenderText("Looking for", CenterX, OffsetY + 13, _Graphics::ALIGN_CENTER);
	//Graphics.RenderText("Reward", CenterX, OffsetY + 143, _Graphics::ALIGN_CENTER);
	*/
}

// Draw the trade screen
void _HUD::DrawTrade() {
	/*
	core::recti WindowArea = TabTrade->getAbsolutePosition();
	int OffsetX = WindowArea.UpperLeftCorner.x;
	int OffsetY = WindowArea.UpperLeftCorner.y;
	int CenterX = WindowArea.getCenter().x;

	// Draw items
	DrawTradeItems(Player, TRADE_WINDOWX, TRADE_WINDOWYYOU, false);

	// Draw trading player information
	_Player *TradePlayer = Player->GetTradePlayer();
	if(TradePlayer) {
		TraderWindow->setVisible(true);

		// Draw player information
		//Graphics.RenderText(TradePlayer->GetName().c_str(), OffsetX + 72, OffsetY + 70 - 55, _Graphics::ALIGN_CENTER);
		Graphics.DrawCenteredImage(Stats.GetPortrait(TradePlayer->GetPortraitID())->Image, OffsetX + 72, OffsetY + 70);

		// Draw items
		DrawTradeItems(TradePlayer, TRADE_WINDOWX, TRADE_WINDOWYTHEM, true);

		// Draw gold
		//Graphics.RenderText(std::string(TradePlayer->GetTradeGold()).c_str(), OffsetX + TRADE_WINDOWX + 35, OffsetY + TRADE_WINDOWYTHEM + 70);

		// Draw agreement state
		std::string AcceptText;
		video::SColor AcceptColor;
		if(TradePlayer->GetTradeAccepted()) {
			AcceptText = "Accepted";
			AcceptColor.set(255, 0, 255, 0);
		}
		else {
			AcceptText = "Unaccepted";
			AcceptColor.set(255, 255, 0, 0);
		}

		//Graphics.RenderText(AcceptText.c_str(), CenterX, OffsetY + TRADE_WINDOWYTHEM + 100, _Graphics::ALIGN_CENTER, AcceptColor);
		TradeAcceptButton->setEnabled(true);
	}
	else {
		TraderWindow->setVisible(false);
		//Graphics.RenderText("Waiting for other player...", 400, 120, _Graphics::ALIGN_CENTER);
		TradeAcceptButton->setEnabled(false);
	}

	// Draw player information
	//Graphics.RenderText(Player->GetName().c_str(), OffsetX + 72, OffsetY + 198 - 55, _Graphics::ALIGN_CENTER);
	Graphics.DrawCenteredImage(Stats.GetPortrait(Player->GetPortraitID())->Image, OffsetX + 72, OffsetY + 198);
	*/
}

// Draw the character stats page
void _HUD::DrawCharacter() {
	/*
	char Buffer[256];
	int Width = 180;
	int Height = 300;

	int DrawX = 800 - Width;
	int DrawY = 300 - Height / 2;
	int RightDrawX = 800 - 10;
	//Graphics.DrawBackground(_Graphics::IMAGE_BLACK, DrawX, DrawY, Width, Height, video::SColor(220, 255, 255, 255));
	DrawX += 10;
	DrawY += 10;

	// Experience
	//Graphics.RenderText("EXP", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetExperience());
	//Graphics.RenderText(Buffer, RightDrawX, DrawY, _Graphics::ALIGN_RIGHT);

	// Experience needed
	DrawY += 15;
	//Graphics.RenderText("EXP needed", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetExperienceNeeded());
	//Graphics.RenderText(Buffer, RightDrawX, DrawY, _Graphics::ALIGN_RIGHT);

	// Damage
	DrawY += 15;
	//Graphics.RenderText("Damage", DrawX, DrawY);
	sprintf(Buffer, "%d-%d", Player->GetMinDamage(), Player->GetMaxDamage());
	//Graphics.RenderText(Buffer, RightDrawX, DrawY, _Graphics::ALIGN_RIGHT);

	// Defense
	DrawY += 15;
	//Graphics.RenderText("Defense", DrawX, DrawY);
	sprintf(Buffer, "%d-%d", Player->GetMinDefense(), Player->GetMaxDefense());
	//Graphics.RenderText(Buffer, RightDrawX, DrawY, _Graphics::ALIGN_RIGHT);

	// Regen
	DrawY += 15;
	//Graphics.RenderText("HP Regen", DrawX, DrawY);
	sprintf(Buffer, "%0.2f%%", Player->GetHealthRegen());
	//Graphics.RenderText(Buffer, RightDrawX, DrawY, _Graphics::ALIGN_RIGHT);

	DrawY += 15;
	//Graphics.RenderText("MP Regen", DrawX, DrawY);
	sprintf(Buffer, "%0.2f%%", Player->GetManaRegen());
	//Graphics.RenderText(Buffer, RightDrawX, DrawY, _Graphics::ALIGN_RIGHT);

	// Stats
	DrawY += 30;
	//Graphics.RenderText("Play Time", DrawX, DrawY);

	int Seconds = Player->GetPlayTime();
	if(Seconds < 60)
		sprintf(Buffer, "%ds", Seconds);
	else if(Seconds < 3600)
		sprintf(Buffer, "%dm", Seconds / 60);
	else
		sprintf(Buffer, "%dh%dm", Seconds / 3600, (Seconds / 60 % 60));
	//Graphics.RenderText(Buffer, RightDrawX, DrawY, _Graphics::ALIGN_RIGHT);

	DrawY += 15;
	//Graphics.RenderText("Deaths", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetDeaths());
	//Graphics.RenderText(Buffer, RightDrawX, DrawY, _Graphics::ALIGN_RIGHT);

	DrawY += 15;
	//Graphics.RenderText("Monster Kills", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetMonsterKills());
	//Graphics.RenderText(Buffer, RightDrawX, DrawY, _Graphics::ALIGN_RIGHT);

	DrawY += 15;
	//Graphics.RenderText("Player Kills", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->GetPlayerKills());
	//Graphics.RenderText(Buffer, RightDrawX, DrawY, _Graphics::ALIGN_RIGHT);
*/
}

// Draws the skill page
void _HUD::DrawSkills() {

	/*
	char Buffer[256];
	core::recti WindowArea = TabSkill->getAbsolutePosition();

	// Remaining points
	sprintf(Buffer, "Skill Points: %d", Player->GetSkillPointsRemaining());
	//Graphics.RenderText(Buffer, 400, SKILL_BARY - 23, _Graphics::ALIGN_CENTER);

	// Draw skills
	const std::vector<_Skill> &Skills = Stats.GetSkillList();
	int X = 0, Y = 0;
	for(size_t i = 0; i < Skills.size(); i++) {
		int DrawX = WindowArea.UpperLeftCorner.x + X * SKILL_SPACINGX + SKILL_STARTX + 16;
		int DrawY = WindowArea.UpperLeftCorner.y + Y * SKILL_SPACINGY + SKILL_STARTY + 16;
		Graphics.DrawCenteredImage(Skills[i].GetImage(), DrawX, DrawY);

		sprintf(Buffer, "%d", Player->GetSkillLevel(i));
		//Graphics.RenderText(Buffer, DrawX, DrawY - 38, _Graphics::ALIGN_CENTER);

		X++;
		if(X >= SKILL_COLUMNS) {
			X = 0;
			Y++;
		}
	}

	// Draw skill bar
	const _Texture *Image;
	for(int i = 0; i < 8; i++) {

		if(Player->GetSkillBar(i))
			Image = Player->GetSkillBar(i)->GetImage();
		else
			Image = Graphics.GetImage(_Graphics::IMAGE_EMPTYSLOT);

		Graphics.DrawCenteredImage(Image, SKILL_BARX + i * 32 + 16, SKILL_BARY + 16);
	}
	*/
}

// Draws the item under the cursor
void _HUD::DrawCursorItem() {
	if(CursorItem.Item) {
		glm::ivec2 DrawPosition = Input.GetMouse();
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(DrawPosition, CursorItem.Item->GetImage());
		DrawItemPrice(CursorItem.Item, CursorItem.Count, DrawPosition, CursorItem.Window == WINDOW_VENDOR);
		if(CursorItem.Count > 1)
			Assets.Fonts["hud_small"]->DrawText(std::to_string(CursorItem.Count).c_str(), DrawPosition + glm::ivec2(20, 20), glm::vec4(1.0f), RIGHT_BASELINE);
	}
}

// Draws information about an item
void _HUD::DrawItemTooltip() {
	const _Item *Item = TooltipItem.Item;
	if(Item) {
		_Element *TooltipElement = Assets.Elements["element_item_tooltip"];
		_Label *TooltipName = Assets.Labels["label_item_tooltip_name"];
		_Label *TooltipType = Assets.Labels["label_item_tooltip_type"];

		// Set label values
		TooltipName->Text = Item->GetName();
		Item->GetType(TooltipType->Text);

		// Set window width
		_TextBounds TextBounds;
		Assets.Fonts["hud_medium"]->GetStringDimensions(TooltipName->Text, TextBounds);
		int Width = 250;
		Width = std::max(Width, TextBounds.Width) + 20;

		// Position window
		glm::ivec2 WindowOffset = Input.GetMouse();
		WindowOffset.x += INVENTORY_TOOLTIP_OFFSETX;
		WindowOffset.y += -(TooltipElement->Bounds.End.y - TooltipElement->Bounds.Start.y) / 2;

		// Reposition window if out of bounds
		if(WindowOffset.x + Width > Graphics.Element->Bounds.End.x - INVENTORY_TOOLTIP_PADDING)
			WindowOffset.x -= Width + INVENTORY_TOOLTIP_OFFSETX + INVENTORY_TOOLTIP_PADDING;

		TooltipElement->SetOffset(WindowOffset);
		TooltipElement->SetWidth(Width);

		// Render tooltip
		TooltipElement->Render();

		// Set draw position to center of window
		glm::ivec2 DrawPosition((TooltipElement->Bounds.End.x - TooltipElement->Bounds.Start.x) / 2 + WindowOffset.x, TooltipType->Bounds.End.y);
		DrawPosition.y += 40;

		glm::ivec2 Spacing(10, 0);
		int SpacingY = 25;

		// Render damage
		int Min, Max;
		Item->GetDamageRange(Min, Max);
		if(Min != 0 || Max != 0) {
			std::stringstream Buffer;
			if(Min != Max)
				Buffer << Min << " - " << Max;
			else
				Buffer << Min;

			Assets.Fonts["hud_medium"]->DrawText("Damage", DrawPosition + -Spacing, glm::vec4(1.0f), RIGHT_BASELINE);
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, glm::vec4(1.0f), LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		}

		// Render defense
		Item->GetDefenseRange(Min, Max);
		if(Min != 0 || Max != 0) {
			std::stringstream Buffer;
			if(Min != Max)
				Buffer << Min << " - " << Max;
			else
				Buffer << Min;

			Assets.Fonts["hud_medium"]->DrawText("Defense", DrawPosition + -Spacing, glm::vec4(1.0f), RIGHT_BASELINE);
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, glm::vec4(1.0f), LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		}

		switch(Item->GetType()) {
			case _Item::TYPE_WEAPON1HAND:
			case _Item::TYPE_WEAPON2HAND:
			case _Item::TYPE_HEAD:
			case _Item::TYPE_BODY:
			case _Item::TYPE_LEGS:
			case _Item::TYPE_SHIELD:
			break;
			case _Item::TYPE_POTION:
				if(TooltipItem.Window == WINDOW_INVENTORY) {
					DrawPosition.y -= 20;
					Assets.Fonts["hud_small"]->DrawText("Right-click to use", DrawPosition, COLOR_GRAY, CENTER_BASELINE);
					DrawPosition.y += 40;
				}
				if(Item->GetHealthRestore() > 0) {
					std::stringstream Buffer;
					Buffer << "+" << Item->GetHealthRestore() << " HP";
					Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition, COLOR_GREEN, CENTER_BASELINE);
					DrawPosition.y += SpacingY;
				}
				if(Item->GetManaRestore() > 0) {
					std::stringstream Buffer;
					Buffer << "+" << Item->GetManaRestore() << " MP";
					Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition, COLOR_BLUE, CENTER_BASELINE);
					DrawPosition.y += SpacingY;
				}
				if(Item->GetInvisPower() > 0) {
					std::stringstream Buffer;
					Buffer << "+" << Item->GetInvisPower() << " Invisibility Time";
					Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition, COLOR_TWHITE, CENTER_BASELINE);
					DrawPosition.y += SpacingY;
				}
			break;
		}

		// Boosts
		if(Item->GetMaxHealth() > 0) {
			std::stringstream Buffer;
			Buffer << "+" << Item->GetMaxHealth() << "%";
			Assets.Fonts["hud_medium"]->DrawText("HP", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		}
		if(Item->GetMaxMana() > 0) {
			std::stringstream Buffer;
			Buffer << "+" << Item->GetMaxMana() << "%";
			Assets.Fonts["hud_medium"]->DrawText("MP", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		}
		if(Item->GetHealthRegen() > 0) {
			std::stringstream Buffer;
			Buffer << "+" << std::setprecision(2) << Item->GetHealthRegen() << "%";
			Assets.Fonts["hud_medium"]->DrawText("HP Regen", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		}
		if(Item->GetManaRegen() > 0) {
			std::stringstream Buffer;
			Buffer << "+" << std::setprecision(2) << Item->GetManaRegen() << "%";
			Assets.Fonts["hud_medium"]->DrawText("MP Regen", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		}

		// Vendors
		if(Vendor) {
			DrawPosition.y += SpacingY;
			if(TooltipItem.Window == WINDOW_VENDOR) {
				std::stringstream Buffer;
				Buffer << "Buy for " << TooltipItem.Cost << " gold";
				Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition, COLOR_GOLD, CENTER_BASELINE);
				DrawPosition.y += SpacingY;
				Assets.Fonts["hud_small"]->DrawText("Right-click to buy", DrawPosition, COLOR_GRAY, CENTER_BASELINE);
				DrawPosition.y += SpacingY;
			}
			else if(TooltipItem.Window == WINDOW_INVENTORY) {
				std::stringstream Buffer;
				Buffer << "Sell for " << TooltipItem.Cost << " gold";
				Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition, COLOR_GOLD, CENTER_BASELINE);
				DrawPosition.y += SpacingY;
				Assets.Fonts["hud_small"]->DrawText("Shift+Right-click to sell", DrawPosition, COLOR_GRAY, CENTER_BASELINE);
				DrawPosition.y += SpacingY;
			}
		}

		switch(*State) {
			case _ClientState::STATE_INVENTORY:
			case _ClientState::STATE_TRADE:
				if(TooltipItem.Window == WINDOW_INVENTORY && TooltipItem.Count > 1) {
					Assets.Fonts["hud_small"]->DrawText("Ctrl+click to split", DrawPosition, COLOR_GRAY, CENTER_BASELINE);
					DrawPosition.y += SpacingY;
				}
			break;
		}
	}
}

// Draws the skill under the cursor
void _HUD::DrawCursorSkill() {
	if(CursorSkill.Skill) {
		glm::ivec2 DrawPosition = Input.GetMouse() - 16;
		Graphics.DrawCenteredImage(DrawPosition + 16, CursorSkill.Skill->GetImage());
	}
}

// Draws the skill tooltip window
void _HUD::DrawSkillTooltip() {
	/*
	const _Skill *Skill = TooltipSkill.Skill;
	if(Skill) {
		int SkillLevel = Player->GetSkillLevel(Skill->GetID());
		int DrawX = OldInput.GetMousePosition().x + 16;
		int DrawY = OldInput.GetMousePosition().y - 100;
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
		std::string String;
		Graphics.SetFont(_Graphics::FONT_10);
		Graphics.DrawBackground(_Graphics::IMAGE_BLACK, DrawX, DrawY, Width, Height);
		DrawX += 10;
		DrawY += 10;

		// Draw name
		//Graphics.RenderText(Skill->GetName().c_str(), DrawX, DrawY);
		DrawY += 15;

		// Draw description
		DrawSkillDescription(Skill, SkillLevel, DrawX, DrawY);

		// Draw next level description
		if(SkillLevel > 0) {
			DrawY += 30;
			//Graphics.RenderText("Next level", DrawX, DrawY, _Graphics::ALIGN_LEFT);
			DrawY += 15;
			DrawSkillDescription(Skill, SkillLevel+1, DrawX, DrawY);
		}

		// Sell cost
		DrawY += 30;
		sprintf(Buffer, "Sell cost: $%d", Skill->GetSellCost(Player->GetLevel()));
		//Graphics.RenderText(Buffer, DrawX, DrawY, _Graphics::ALIGN_LEFT, COLOR_GOLD);

		// Additional information
		switch(Skill->GetType()) {
			case _Skill::TYPE_PASSIVE:
				DrawY += 15;
				//Graphics.RenderText("Passive skills must be equipped to skillbar", DrawX, DrawY, _Graphics::ALIGN_LEFT, COLOR_GRAY);
			break;
		}
	}*/
}

// Draw the skill description
void _HUD::DrawSkillDescription(const _Skill *Skill, int TLevel, int TDrawX, int &TDrawY) {

	/*
	// Get skill data
	int PowerMin, PowerMax, PowerMinRound, PowerMaxRound, PowerPercent;
	float PowerMinFloat, PowerMaxFloat;
	Skill->GetPowerRange(TLevel, PowerMin, PowerMax);
	Skill->GetPowerRangeRound(TLevel, PowerMinRound, PowerMaxRound);
	Skill->GetPowerRange(TLevel, PowerMinFloat, PowerMaxFloat);
	PowerPercent = (int)std::roundf(PowerMaxFloat * 100);

	// Draw description
	char Buffer[256];
	switch(Skill->GetType()) {
		case _Skill::TYPE_ATTACK:
			sprintf(Buffer, Skill->GetInfo().c_str(), PowerPercent);
			//Graphics.RenderText(Buffer, TDrawX, TDrawY, _Graphics::ALIGN_LEFT, COLOR_LIGHTGRAY);
		break;
		case _Skill::TYPE_SPELL:
			switch(Skill->GetID()) {
				case 3:
					sprintf(Buffer, Skill->GetInfo().c_str(), PowerMaxRound);
				break;
				case 6:
				case 11:
					sprintf(Buffer, Skill->GetInfo().c_str(), PowerMinRound, PowerMaxRound);
				break;
			}
			//Graphics.RenderText(Buffer, TDrawX, TDrawY, _Graphics::ALIGN_LEFT, COLOR_LIGHTGRAY);

			TDrawY += 15;
			sprintf(Buffer, "%d Mana", Skill->GetManaCost(TLevel));
			//Graphics.RenderText(Buffer, TDrawX, TDrawY, _Graphics::ALIGN_LEFT, video::SColor(255, 0, 0, 255));
		break;
		case _Skill::TYPE_USEPOTION:
			sprintf(Buffer, Skill->GetInfo().c_str(), PowerMin);
			//Graphics.RenderText(Buffer, TDrawX, TDrawY, _Graphics::ALIGN_LEFT, COLOR_LIGHTGRAY);
		break;
		case _Skill::TYPE_PASSIVE:
			switch(Skill->GetID()) {
				case 4:
				case 5:
					sprintf(Buffer, Skill->GetInfo().c_str(), PowerMaxRound);
					//Graphics.RenderText(Buffer, TDrawX, TDrawY, _Graphics::ALIGN_LEFT, COLOR_LIGHTGRAY);
				break;
				case 7:
				case 8:
					sprintf(Buffer, Skill->GetInfo().c_str(), PowerMaxFloat);
					//Graphics.RenderText(Buffer, TDrawX, TDrawY, _Graphics::ALIGN_LEFT, COLOR_LIGHTGRAY);
				break;
				case 9:
				case 10:
					sprintf(Buffer, Skill->GetInfo().c_str(), PowerMax);
					//Graphics.RenderText(Buffer, TDrawX, TDrawY, _Graphics::ALIGN_LEFT, COLOR_LIGHTGRAY);
				break;
			}
		break;
		default:
			//Graphics.RenderText(Skill->GetInfo().c_str(), TDrawX, TDrawY, _Graphics::ALIGN_LEFT, COLOR_LIGHTGRAY);
		break;
	}*/
}

// Draws an item's price
void _HUD::DrawItemPrice(const _Item *Item, int Count, const glm::ivec2 &DrawPosition, bool Buy) {
	if(!Vendor)
		return;

	// Real price
	int Price = Item->GetPrice(Vendor, Count, Buy);

	// Color
	glm::vec4 Color;
	if(Buy && Player->GetGold() < Price)
		Color = COLOR_RED;
	else
		Color = COLOR_LIGHTGOLD;

	Assets.Fonts["hud_small"]->DrawText(std::to_string(Price).c_str(), DrawPosition + glm::ivec2(20, -8), Color, RIGHT_BASELINE);
}

// Draws trading items
void _HUD::DrawTradeItems(_Player *TPlayer, int TDrawX, int TDrawY, bool TDrawAll) {
	/*
	core::recti WindowArea = TabTrade->getAbsolutePosition();
	int OffsetX = WindowArea.UpperLeftCorner.x;
	int OffsetY = WindowArea.UpperLeftCorner.y;

	Graphics.SetFont(_Graphics::FONT_7);

	// Draw trade items
	_InventorySlot *Item;
	int PositionX = 0, PositionY = 0;
	for(int i = _Player::INVENTORY_TRADE; i < _Player::INVENTORY_COUNT; i++) {
		Item = TPlayer->GetInventory(i);
		if(Item->Item && (TDrawAll || !CursorItem.IsEqual(i, WINDOW_TRADEYOU))) {
			int DrawX = OffsetX + TDrawX + PositionX * 32;
			int DrawY = OffsetY + TDrawY + PositionY * 32;
			Graphics.DrawCenteredImage(Item->Item->GetImage(), DrawX + 16, DrawY + 16);
			if(Item->Count > 1) {
				//Graphics.RenderText(std::string(Item->Count).c_str(), DrawX + 2, DrawY + 20);
			}
		}

		PositionX++;
		if(PositionX > 3) {
			PositionX = 0;
			PositionY++;
		}
	}
	Graphics.SetFont(_Graphics::FONT_10);
	*/
}

// Returns an item that's on the screen
void _HUD::GetItem(const glm::ivec2 &Position, _CursorItem &TCursorItem) {
	/*
	TCursorItem.Reset();

	switch(*State) {
		case _PlayClientState::STATE_INVENTORY:
			GetInventoryItem(Position, TCursorItem);
		break;
		case _PlayClientState::STATE_VENDOR:
			GetInventoryItem(Position, TCursorItem);
			GetVendorItem(Position, TCursorItem);
		break;
		case _PlayClientState::STATE_TRADER:
			GetTraderItem(Position, TCursorItem);
		break;
		case _PlayClientState::STATE_TRADE:
			GetInventoryItem(Position, TCursorItem);
			GetTradeItem(Position, TCursorItem);
		break;
	}*/
}

// Returns a vendor item from a mouse position
void _HUD::GetVendorItem(const glm::ivec2 &Position, _CursorItem &TCursorItem) {
	/*
	core::recti WindowArea = TabVendor->getAbsolutePosition();
	if(!WindowArea.isPointInside(TPoint))
		return;

	TCursorItem.Window = WINDOW_VENDOR;

	// Adjust mouse position
	TPoint.x -= WindowArea.UpperLeftCorner.x;
	TPoint.y -= WindowArea.UpperLeftCorner.y;

	// Get vendor slot
	if(TPoint.x >= 1 && TPoint.x <= 256 && TPoint.y >= 47 && TPoint.y < 47 + 192) {
		size_t InventoryIndex = (TPoint.x - 1) / 32 + (TPoint.y - 47) / 32 * 8;
		//printf("%d=%d %d\n", InventoryIndex, TPoint.x, TPoint.y);
		if(InventoryIndex < Vendor->Items.size()) {
			int Price = Vendor->Items[InventoryIndex]->GetPrice(Vendor, 1, true);
			TCursorItem.Set(Vendor->Items[InventoryIndex], Price, 1, InventoryIndex);
		}
	}
	*/
}

// Returns a trader item from a mouse position
void _HUD::GetTraderItem(const glm::ivec2 &Position, _CursorItem &TCursorItem) {
	/*
	core::recti WindowArea = TabTrader->getAbsolutePosition();
	if(!WindowArea.isPointInside(TPoint))
		return;

	// Adjust mouse position
	TPoint.x -= WindowArea.UpperLeftCorner.x;
	TPoint.y -= WindowArea.UpperLeftCorner.y;

	// Get trader slot
	int InventoryIndex = -1;
	if(TPoint.x >= 19 && TPoint.x <= 146 && TPoint.y >= 52 && TPoint.y <= 83)
		InventoryIndex = (TPoint.x - 19) / 32 + (TPoint.y - 52) / 32;
	else if(TPoint.x >= 19 && TPoint.x <= 146 && TPoint.y >= 98 && TPoint.y <= 129)
		InventoryIndex = 4 + (TPoint.x - 19) / 32 + (TPoint.y - 98) / 32;
	else if(TPoint.x >= 67 && TPoint.x <= 98 && TPoint.y >= 182 && TPoint.y <= 213)
		InventoryIndex = 9;

	//printf("%d=%d %d\n", InventoryIndex, TPoint.x, TPoint.y);

	// Set cursor item
	if(InventoryIndex != -1) {
		TCursorItem.Window = WINDOW_TRADER;
		if(InventoryIndex < (int)Trader->TraderItems.size())
			TCursorItem.Set(Trader->TraderItems[InventoryIndex].Item, 0, 1, InventoryIndex);
		else if(InventoryIndex == 9)
			TCursorItem.Set(Trader->RewardItem, 0, 1, InventoryIndex);
	}
	*/
}

// Returns a trade item from a mouse position
void _HUD::GetTradeItem(const glm::ivec2 &Position, _CursorItem &TCursorItem) {
	/*
	core::recti WindowArea = TabTrade->getAbsolutePosition();
	if(!WindowArea.isPointInside(TPoint))
		return;

	// Adjust mouse position
	TPoint.x -= WindowArea.UpperLeftCorner.x;
	TPoint.y -= WindowArea.UpperLeftCorner.y;

	// Get trade slot
	_Player *TradePlayer = Player->GetTradePlayer();
	if(TradePlayer && TPoint.x >= TRADE_WINDOWX && TPoint.x < TRADE_WINDOWX + 128 && TPoint.y >= TRADE_WINDOWYTHEM && TPoint.y < TRADE_WINDOWYTHEM + 64) {
		size_t InventoryIndex = (TPoint.x - TRADE_WINDOWX) / 32 + (TPoint.y - TRADE_WINDOWYTHEM) / 32 * 4 + _Player::INVENTORY_TRADE;
		//printf("them: %d=%d %d\n", InventoryIndex, TPoint.x, TPoint.y);
		const _Item *Item = TradePlayer->GetInventory(InventoryIndex)->Item;
		TCursorItem.Window = WINDOW_TRADETHEM;
		TCursorItem.Set(Item, 0, TradePlayer->GetInventory(InventoryIndex)->Count, InventoryIndex);
	}
	else if(TPoint.x >= TRADE_WINDOWX && TPoint.x < TRADE_WINDOWX + 128 && TPoint.y >= TRADE_WINDOWYYOU && TPoint.y < TRADE_WINDOWYYOU + 64) {
		size_t InventoryIndex = (TPoint.x - TRADE_WINDOWX) / 32 + (TPoint.y - TRADE_WINDOWYYOU) / 32 * 4 + _Player::INVENTORY_TRADE;
		//printf("you: %d=%d %d\n", InventoryIndex, TPoint.x, TPoint.y);
		const _Item *Item = Player->GetInventory(InventoryIndex)->Item;
		TCursorItem.Window = WINDOW_TRADEYOU;
		TCursorItem.Set(Item, 0, Player->GetInventory(InventoryIndex)->Count, InventoryIndex);
	}
	*/
}

// Buys an item from the vendor
void _HUD::BuyItem(_CursorItem *TCursorItem, int TTargetSlot) {
	if(Player->GetGold() >= TCursorItem->Cost && Player->AddItem(TCursorItem->Item, TCursorItem->Count, TTargetSlot)) {

		// Update player
		int Price = TCursorItem->Item->GetPrice(Vendor, TCursorItem->Count, true);
		Player->UpdateGold(-Price);

		// Notify server
		_Buffer Packet;
		Packet.Write<char>(_Network::VENDOR_EXCHANGE);
		Packet.WriteBit(1);
		Packet.Write<char>(TCursorItem->Count);
		Packet.Write<char>(TCursorItem->Slot);
		Packet.Write<char>(TTargetSlot);
		ClientNetwork->SendPacketToHost(&Packet);

		Player->CalculatePlayerStats();
	}
}

// Sells an item
void _HUD::SellItem(_CursorItem *TCursorItem, int TAmount) {
	if(!TCursorItem->Item)
		return;

	// Update player
	int Price = TCursorItem->Item->GetPrice(Vendor, TAmount, 0);
	Player->UpdateGold(Price);
	bool Deleted = Player->UpdateInventory(TCursorItem->Slot, -TAmount);

	// Notify server
	_Buffer Packet;
	Packet.Write<char>(_Network::VENDOR_EXCHANGE);
	Packet.WriteBit(0);
	Packet.Write<char>(TAmount);
	Packet.Write<char>(TCursorItem->Slot);
	ClientNetwork->SendPacketToHost(&Packet);

	if(Deleted)
		TCursorItem->Reset();

	Player->CalculatePlayerStats();
}

// Returns a skill that's on the screen
void _HUD::GetSkill(const glm::ivec2 &Position, _CursorSkill &TCursorSkill) {
	TCursorSkill.Reset();

	switch(*State) {
		case _ClientState::STATE_SKILLS:
			GetSkillPageSkill(Position, TCursorSkill);
		break;
		case _ClientState::STATE_BATTLE:
		break;
	}
}

// Gets a skill from the skill page
void _HUD::GetSkillPageSkill(const glm::ivec2 &Position, _CursorSkill &TCursorSkill) {
	/*
	core::recti WindowArea = TabSkill->getAbsolutePosition();
	if(!WindowArea.isPointInside(TPoint))
		return;

	// Get skill from page
	int X = 0, Y = 0;
	for(size_t i = 0; i < Stats.GetSkillList().size(); i++) {
		int SkillX = WindowArea.UpperLeftCorner.x + SKILL_STARTX + X * SKILL_SPACINGX;
		int SkillY = WindowArea.UpperLeftCorner.y + SKILL_STARTY + Y * SKILL_SPACINGY;
		core::recti SkillArea(SkillX, SkillY, SkillX + 32, SkillY + 32);
		if(SkillArea.isPointInside(TPoint)) {
			TCursorSkill.Skill = Stats.GetSkill(i);
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
	if(TPoint.x >= SKILL_BARX && TPoint.x < SKILL_BARX + 32 * 8 && TPoint.y >= SKILL_BARY && TPoint.y <= SKILL_BARY + 32) {
		int Index = (TPoint.x - SKILL_BARX) / 32;
		TCursorSkill.Skill = Player->GetSkillBar(Index);
		TCursorSkill.Slot = Index;
		TCursorSkill.Window = WINDOW_SKILLBAR;
	}
	*/
}

// Sets the player's skill bar
void _HUD::SetSkillBar(int TSlot, int TOldSlot, const _Skill *TSkill) {
	if(Player->GetSkillBar(TSlot) == TSkill)
		return;

	// Check for swapping
	if(TOldSlot == -1) {

		// Remove duplicate skills
		for(int i = 0; i < 8; i++) {
			if(Player->GetSkillBar(i) == TSkill)
				Player->SetSkillBar(i, nullptr);
		}
	}
	else {
		const _Skill *OldSkill = Player->GetSkillBar(TSlot);
		Player->SetSkillBar(TOldSlot, OldSkill);
	}

	Player->SetSkillBar(TSlot, TSkill);
	Player->CalculatePlayerStats();
	SkillBarChanged = true;
}

// Shows or hides the plus/minus buttons
void _HUD::RefreshSkillButtons() {
/*
	// Get remaining points
	int SkillPointsRemaining = Player->GetSkillPointsRemaining();

	// Loop through buttons
	irr::core::list<gui::IGUIElement *> Buttons = TabSkill->getChildren();
	for(irr::core::list<gui::IGUIElement *>::Iterator Iterator = Buttons.begin(); Iterator != Buttons.end(); ++Iterator) {
		gui::IGUIButton *Button = static_cast<gui::IGUIButton *>(*Iterator);
		Button->setVisible(true);

		// Toggle buttons
		int SkillID;
		if(Button->getID() < ELEMENT_SKILLMINUS0) {
			SkillID = Button->getID() - ELEMENT_SKILLPLUS0;

			// Hide locked skills
			const _Skill *Skill = Stats.GetSkill(SkillID);
			if(Skill->GetSkillCost() > SkillPointsRemaining || Player->GetSkillLevel(SkillID) >= 255)
				Button->setVisible(false);
		}
		else {
			SkillID = Button->getID() - ELEMENT_SKILLMINUS0;
			if(Player->GetSkillLevel(SkillID) == 0)
				Button->setVisible(false);
		}
	}
	*/
}

// Sends the busy signal to the server
void _HUD::SendBusySignal(bool Value) {
	_Buffer Packet;
	Packet.Write<char>(_Network::WORLD_BUSY);
	Packet.Write<char>(Value);
	ClientNetwork->SendPacketToHost(&Packet);
}

// Trade with another player
void _HUD::SendTradeRequest() {
	_Buffer Packet;
	Packet.Write<char>(_Network::TRADE_REQUEST);
	ClientNetwork->SendPacketToHost(&Packet);
}

// Cancel a trade
void _HUD::SendTradeCancel() {
	_Buffer Packet;
	Packet.Write<char>(_Network::TRADE_CANCEL);
	ClientNetwork->SendPacketToHost(&Packet);

	Player->SetTradePlayer(nullptr);
}

// Make sure the trade gold box is valid
int _HUD::ValidateTradeGold() {
/*
	// Get gold amount
	int Gold = atoi(std::string(TradeGoldBox->getText()).c_str());
	if(Gold < 0)
		Gold = 0;
	else if(Gold > Player->GetGold())
		Gold = Player->GetGold();

	// Set text
	TradeGoldBox->setText(core::stringw(Gold).c_str());

	return Gold;
	*/
	return 0;
}

// Resets the trade agreement
void _HUD::ResetAcceptButton() {
	//if(TradeAcceptButton)
	//	TradeAcceptButton->setPressed(false);

	_Player *TradePlayer = Player->GetTradePlayer();
	if(TradePlayer)
		TradePlayer->SetTradeAccepted(false);
}

// Split a stack of items
void _HUD::SplitStack(int TSlot, int TCount) {

	// Split only inventory items
	if(!_Player::IsSlotInventory(TSlot))
		return;

	// Build packet
	_Buffer Packet;
	Packet.Write<char>(_Network::INVENTORY_SPLIT);
	Packet.Write<char>(TSlot);
	Packet.Write<char>(TCount);

	ClientNetwork->SendPacketToHost(&Packet);
	Player->SplitStack(TSlot, TCount);
}

// Toggles the town portal state
void _HUD::ToggleTownPortal() {
	_Buffer Packet;
	Packet.Write<char>(_Network::WORLD_TOWNPORTAL);
	ClientNetwork->SendPacketToHost(&Packet);
	Player->StartTownPortal();

	if(*State == _ClientState::STATE_TOWNPORTAL)
		*State = _ClientState::STATE_WALK;
	else
		*State = _ClientState::STATE_TOWNPORTAL;
}
