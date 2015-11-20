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
#include <actions.h>
#include <ui/element.h>
#include <ui/button.h>
#include <ui/label.h>
#include <ui/textbox.h>
#include <ui/image.h>
#include <ui/style.h>
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
	ClearSkills();
}

// Mouse events
void _HUD::MouseEvent(const _MouseEvent &MouseEvent) {

	// Press
	if(MouseEvent.Pressed) {
		switch(*State) {
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
			case _ClientState::STATE_VENDOR: {
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
						if(TooltipSkill.Skill && Player->GetSkillLevel(TooltipSkill.Skill->ID) > 0)
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
			case _ClientState::STATE_TRADER: {
				if(MouseEvent.Button == SDL_BUTTON_LEFT) {
					_Element *TraderElement = Assets.Elements["element_trader"];
					if(TraderElement->HitElement) {
						if(TraderElement->HitElement->Identifier == "button_trader_accept") {
							_Buffer Packet;
							Packet.Write<char>(_Network::TRADER_ACCEPT);
							ClientNetwork->SendPacketToHost(&Packet);
							Player->AcceptTrader(Trader, RequiredItemSlots, RewardItemSlot);
							Player->CalculatePlayerStats();
							CloseWindows();
						}
						else if(TraderElement->HitElement->Identifier == "button_trader_cancel") {
							CloseWindows();
						}
					}
				}
			} break;
			case _ClientState::STATE_SKILLS:
				if(MouseEvent.Button == SDL_BUTTON_LEFT) {
					_Element *SkillsElement = Assets.Elements["element_skills"];
					if(SkillsElement->HitElement) {
						if(SkillsElement->HitElement->Identifier == "button_skills_plus") {
							AdjustSkillLevel((intptr_t)SkillsElement->HitElement->Parent->UserData, 1);
						}
						else if(SkillsElement->HitElement->Identifier == "button_skills_minus") {
							AdjustSkillLevel((intptr_t)SkillsElement->HitElement->Parent->UserData, -1);
						}
					}

					// Check for assigning skills to actionbar
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
void _HUD::HandleGUI(gui::EGUI_EVENT_TYPE EventType, gui::IGUIElement *TElement) {
	switch(*State) {
		case _PlayClientState::STATE_MAINMENU:
			switch(EventType) {
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
		case _PlayClientState::STATE_TRADE:
			switch(EventType) {
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
						//if(EventType == gui::EGET_EDITBOX_ENTER)
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
	}
}
*/

// Updates the HUD
void _HUD::Update(double FrameTime) {
	Assets.Elements["element_hud"]->Update(FrameTime, Input.GetMouse());
	Assets.Elements["element_buttonbar"]->Update(FrameTime, Input.GetMouse());
	TooltipItem.Reset();
	TooltipSkill.Reset();

	switch(*State) {
		case _ClientState::STATE_VENDOR:
		case _ClientState::STATE_INVENTORY: {
			_Element *InventoryElement = Assets.Elements["element_inventory"];
			InventoryElement->Update(FrameTime, Input.GetMouse());
			_Element *HoverSlot = InventoryElement->HitElement;
			if(HoverSlot)
				TooltipItem.Window = WINDOW_INVENTORY;

			// Get vendor hover item
			_Element *VendorElement = Assets.Elements["element_vendor"];
			if(!HoverSlot && *State == _ClientState::STATE_VENDOR) {
				VendorElement->Update(FrameTime, Input.GetMouse());
				HoverSlot = VendorElement->HitElement;
				TooltipItem.Window = WINDOW_VENDOR;
			}

			// Check for valid slot
			if(!HoverSlot)
				break;

			// Set tooltip item
			if((intptr_t)HoverSlot->UserData >= 0) {
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
		case _ClientState::STATE_TRADER: {
			_Element *TraderElement = Assets.Elements["element_trader"];
			TraderElement->Update(FrameTime, Input.GetMouse());
			_Element *HoverSlot = TraderElement->HitElement;
			if(HoverSlot && HoverSlot != TraderElement) {
				size_t InventoryIndex = (intptr_t)HoverSlot->UserData;
				TooltipItem.Window = WINDOW_TRADER;
				if(InventoryIndex < Trader->TraderItems.size())
					TooltipItem.Set(Trader->TraderItems[InventoryIndex].Item, 0, 1, InventoryIndex);
				else if(InventoryIndex == 8)
					TooltipItem.Set(Trader->RewardItem, 0, 1, InventoryIndex);
			}
		} break;
		case _ClientState::STATE_SKILLS: {

			// Check skill page
			_Element *SkillsElement = Assets.Elements["element_skills"];
			SkillsElement->Update(FrameTime, Input.GetMouse());
			_Element *HoverSlot = SkillsElement->HitElement;
			if(HoverSlot) {
				TooltipSkill.Window = WINDOW_SKILLS;
				TooltipSkill.Skill = Stats.GetSkill((intptr_t)HoverSlot->UserData);
			}

			// Check actionbar
			if(!HoverSlot && *State == _ClientState::STATE_SKILLS) {
				_Element *ActionbarElement = Assets.Elements["element_actionbar"];
				ActionbarElement->Update(FrameTime, Input.GetMouse());
				HoverSlot = ActionbarElement->HitElement;
				if(HoverSlot) {
					TooltipSkill.Window = WINDOW_SKILLBAR;
					TooltipSkill.Slot = (intptr_t)HoverSlot->UserData;
					TooltipSkill.Skill = Player->GetSkillBar(TooltipSkill.Slot);
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
	Assets.Elements["element_buttonbar"]->Render();
	Assets.Elements["element_actionbar"]->Render();
	DrawActionBar();

	std::stringstream Buffer;

	// Set hud values
	Buffer << "Level " << Player->GetLevel();
	Assets.Labels["label_hud_level"]->Text = Buffer.str();
	Buffer.str("");

	Buffer << Player->Gold << " Gold";
	Assets.Labels["label_hud_gold"]->Text = Buffer.str();
	Buffer.str("");

	// Draw experience bar
	Buffer << Player->ExperienceNextLevel - Player->ExperienceNeeded << " / " << Player->ExperienceNextLevel << " XP";
	Assets.Labels["label_hud_experience"]->Text = Buffer.str();
	Buffer.str("");
	Assets.Images["image_hud_experience_bar_full"]->SetWidth(Assets.Elements["element_hud_experience"]->Size.x * Player->GetNextLevelPercent());
	Assets.Images["image_hud_experience_bar_empty"]->SetWidth(Assets.Elements["element_hud_experience"]->Size.x);
	Assets.Elements["element_hud_experience"]->Render();

	// Draw health bar
	float HealthPercent = Player->MaxHealth > 0 ? Player->Health / (float)Player->MaxHealth : 0;
	Buffer << Player->Health << " / " << Player->MaxHealth;
	Assets.Labels["label_hud_health"]->Text = Buffer.str();
	Buffer.str("");
	Assets.Images["image_hud_health_bar_full"]->SetWidth(Assets.Elements["element_hud_health"]->Size.x * HealthPercent);
	Assets.Images["image_hud_health_bar_empty"]->SetWidth(Assets.Elements["element_hud_health"]->Size.x);
	Assets.Elements["element_hud_health"]->Render();

	// Draw mana bar
	float ManaPercent = Player->MaxMana > 0 ? Player->Mana / (float)Player->MaxMana : 0;
	Buffer << Player->Mana << " / " << Player->MaxMana;
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
		sprintf(String, "%d", Player->Gold);
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
		case _ClientState::STATE_TELEPORT:
			DrawTeleport();
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

	Assets.Labels["label_hud_name"]->Text = Player->Name;
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
			Chat.Message = Player->Name + std::string(": ") + Message;
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
void _HUD::InitTrader(int TraderID) {
	if(*State == _ClientState::STATE_TRADER)
		return;

	// Get trader stats
	Trader = Stats.GetTrader(TraderID);

	// Check for required items
	RewardItemSlot = Player->GetRequiredItemSlots(Trader, RequiredItemSlots);

	// Disable accept button if requirements not met
	if(RewardItemSlot == -1)
		Assets.Buttons["button_trader_accept"]->Enabled = false;
	else
		Assets.Buttons["button_trader_accept"]->Enabled = true;

	*State = _ClientState::STATE_TRADER;
}

// Close the trader
void _HUD::CloseTrader() {
	if(*State != _ClientState::STATE_TRADER)
		return;

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

// Delete memory used by skill page
void _HUD::ClearSkills() {
	_Element *SkillsElement = Assets.Elements["element_skills"];
	std::vector<_Element *> &Children = SkillsElement->Children;
	for(size_t i = 0; i < Children.size(); i++) {
		if(Children[i]->Style && Children[i]->Style->UserCreated)
			delete Children[i]->Style;

		// Delete labels
		for(size_t j = 0; j < Children[i]->Children.size(); j++) {
			if(Children[i]->Children[j]->UserCreated)
				delete Children[i]->Children[j];
		}

		if(Children[i]->UserCreated)
			delete Children[i];
	}
	Children.clear();
}

// Initialize the skills screen
void _HUD::InitSkills() {
	*State = _ClientState::STATE_SKILLS;
	SendBusySignal(true);

	// Clear old children
	_Element *SkillsElement = Assets.Elements["element_skills"];
	ClearSkills();

	glm::ivec2 Start(10, 25);
	glm::ivec2 Offset(Start);
	glm::ivec2 LevelOffset(0, -4);
	glm::ivec2 Spacing(10, 50);
	glm::ivec2 PlusOffset(-12, 37);
	glm::ivec2 MinusOffset(12, 37);
	glm::ivec2 LabelOffset(-1, 3);
	size_t i = 0;

	// Iterate over skills
	for(const auto &Skill : Stats.Skills) {

		// Create style
		_Style *Style = new _Style();
		Style->TextureColor = COLOR_WHITE;
		Style->Program = Assets.Programs["ortho_pos_uv"];
		Style->Texture = Skill.Image;
		Style->UserCreated = true;

		// Add skill icon
		_Button *Button = new _Button();
		Button->Identifier = "button_skills_skill";
		Button->Parent = SkillsElement;
		Button->Offset = Offset;
		Button->Size = Skill.Image->Size;
		Button->Alignment = LEFT_TOP;
		Button->Style = Style;
		Button->UserData = (void *)(intptr_t)Skill.ID;
		Button->UserCreated = true;
		SkillsElement->Children.push_back(Button);

		// Add level label
		_Label *LevelLabel = new _Label();
		LevelLabel->Identifier = "label_skills_level";
		LevelLabel->Parent = Button;
		LevelLabel->Offset = LevelOffset;
		LevelLabel->Alignment = CENTER_BASELINE;
		LevelLabel->Color = COLOR_WHITE;
		LevelLabel->Font = Assets.Fonts["hud_small"];
		LevelLabel->UserData = (void *)(intptr_t)Skill.ID;
		LevelLabel->UserCreated = true;
		SkillsElement->Children.push_back(LevelLabel);

		// Add plus button
		_Button *PlusButton = new _Button();
		PlusButton->Identifier = "button_skills_plus";
		PlusButton->Parent = Button;
		PlusButton->Size = glm::ivec2(16, 16);
		PlusButton->Offset = PlusOffset;
		PlusButton->Alignment = CENTER_MIDDLE;
		PlusButton->Style = Assets.Styles["style_menu_button"];
		PlusButton->HoverStyle = Assets.Styles["style_menu_button_hover"];
		PlusButton->UserCreated = true;
		SkillsElement->Children.push_back(PlusButton);

		// Add minus button
		_Button *MinusButton = new _Button();
		MinusButton->Identifier = "button_skills_minus";
		MinusButton->Parent = Button;
		MinusButton->Size = glm::ivec2(16, 16);
		MinusButton->Offset = MinusOffset;
		MinusButton->Alignment = CENTER_MIDDLE;
		MinusButton->Style = Assets.Styles["style_menu_button"];
		MinusButton->HoverStyle = Assets.Styles["style_menu_button_hover"];
		MinusButton->UserCreated = true;
		SkillsElement->Children.push_back(MinusButton);

		// Add plus label
		_Label *PlusLabel = new _Label();
		PlusLabel->Parent = PlusButton;
		PlusLabel->Text = "+";
		PlusLabel->Offset = LabelOffset;
		PlusLabel->Alignment = CENTER_MIDDLE;
		PlusLabel->Color = COLOR_WHITE;
		PlusLabel->Font = Assets.Fonts["hud_medium"];
		PlusLabel->UserCreated = true;
		PlusButton->Children.push_back(PlusLabel);

		// Add minus label
		_Label *MinusLabel = new _Label();
		MinusLabel->Parent = MinusButton;
		MinusLabel->Text = "-";
		MinusLabel->Offset = LabelOffset;
		MinusLabel->Alignment = CENTER_MIDDLE;
		MinusLabel->Color = COLOR_WHITE;
		MinusLabel->Font = Assets.Fonts["hud_medium"];
		MinusLabel->UserCreated = true;
		MinusButton->Children.push_back(MinusLabel);

		// Update position
		Offset.x += Skill.Image->Size.x + Spacing.x;
		if(Offset.x > SkillsElement->Size.x - Skill.Image->Size.x) {
			Offset.y += Skill.Image->Size.y + Spacing.y;
			Offset.x = Start.x;
		}

		i++;
	}
	SkillsElement->CalculateBounds();

	RefreshSkillButtons();
	CursorSkill.Reset();
	TooltipSkill.Reset();
	SkillBarChanged = false;
}

// Shows or hides the plus/minus buttons
void _HUD::RefreshSkillButtons() {

	// Get remaining points
	int SkillPointsRemaining = Player->GetSkillPointsRemaining();

	// Loop through buttons
	_Element *SkillsElement = Assets.Elements["element_skills"];
	for(auto &Element : SkillsElement->Children) {
		if(Element->Identifier == "label_skills_level") {
			int SkillID = (intptr_t)Element->UserData;
			_Label *Label = (_Label *)Element;
			Label->Text = std::to_string(Player->GetSkillLevel(SkillID));
		}
		else if(Element->Identifier == "button_skills_plus") {
			_Button *Button = (_Button *)Element;

			// Get skill
			int SkillID = (intptr_t)Button->Parent->UserData;
			const _Skill *Skill = Stats.GetSkill(SkillID);
			if(Skill->SkillCost > SkillPointsRemaining || Player->GetSkillLevel(SkillID) >= 255)
				Button->SetVisible(false);
			else
				Button->SetVisible(true);
		}
		else if(Element->Identifier == "button_skills_minus") {
			_Button *Button = (_Button *)Element;

			// Get skill
			int SkillID = (intptr_t)Button->Parent->UserData;
			if(Player->GetSkillLevel(SkillID) == 0)
				Button->SetVisible(false);
			else
				Button->SetVisible(true);
		}
	}
}

// Close the skills screen
void _HUD::CloseSkills() {
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
void _HUD::CloseTrade(bool SendNotify) {

	// Close inventory
	CloseInventory();

	// Notify server
	if(SendNotify)
		SendTradeCancel();

	Player->TradePlayer = nullptr;
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

// Draw the teleport sequence
void _HUD::DrawTeleport() {
	double Timeleft = GAME_TELEPORT_TIME - Player->GetTeleportTime();
	if(Timeleft > 0) {
		Assets.Elements["element_teleport"]->Render();

		std::stringstream Buffer;
		Buffer << "Teleport in " << std::fixed << std::setprecision(1) << Timeleft;
		Assets.Labels["label_teleport_timeleft"]->Text = Buffer.str();
	}
}

// Draws the player's inventory
void _HUD::DrawInventory() {
	Assets.Elements["element_inventory"]->Render();

	// Draw player's items
	for(int i = 0; i < _Player::INVENTORY_TRADE; i++) {

		// Get inventory slot
		_InventorySlot *Item = Player->GetInventory(i);
		if(Item->Item && !CursorItem.IsEqual(i, WINDOW_INVENTORY)) {

			// Get bag button
			std::stringstream Buffer;
			Buffer << "button_inventory_bag_" << i;
			_Button *Button = Assets.Buttons[Buffer.str()];

			// Get position of slot
			glm::ivec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2;

			// Draw item
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.DrawCenteredImage(DrawPosition, Item->Item->GetImage());

			// Draw price if using vendor
			DrawItemPrice(Item->Item, Item->Count, DrawPosition, false);

			// Draw count
			if(Item->Count > 1)
				Assets.Fonts["hud_small"]->DrawText(std::to_string(Item->Count).c_str(), DrawPosition + glm::ivec2(20, 20), glm::vec4(1.0f), RIGHT_BASELINE);
		}
	}
}

// Draw the vendor
void _HUD::DrawVendor() {
	Assets.Elements["element_vendor"]->Render();

	// Draw vendor items
	for(size_t i = 0; i < Vendor->Items.size(); i++) {
		const _Item *Item = Vendor->Items[i];
		if(Item && !CursorItem.IsEqual(i, WINDOW_VENDOR)) {

			// Get bag button
			std::stringstream Buffer;
			Buffer << "button_vendor_bag_" << i;
			_Button *Button = Assets.Buttons[Buffer.str()];

			// Get position of slot
			glm::ivec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2;

			// Draw item
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.DrawCenteredImage(DrawPosition, Item->GetImage());

			// Draw price
			DrawItemPrice(Item, 1, DrawPosition, true);
		}
	}
}

// Draw the trader screen
void _HUD::DrawTrader() {
	Assets.Elements["element_trader"]->Render();

	// Draw trader items
	for(size_t i = 0; i < Trader->TraderItems.size(); i++) {

		// Get button position
		std::stringstream Buffer;
		Buffer << "button_trader_bag_" << i;
		_Button *Button = Assets.Buttons[Buffer.str()];
		glm::ivec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2;

		// Draw item
		const _Item *Item = Trader->TraderItems[i].Item;
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(DrawPosition, Item->GetImage());

		glm::vec4 Color;
		if(RequiredItemSlots[i] == -1)
			Color = COLOR_RED;
		else
			Color = COLOR_WHITE;

		Assets.Fonts["hud_small"]->DrawText(std::to_string(Trader->TraderItems[i].Count).c_str(), DrawPosition + glm::ivec2(0, -32), Color, CENTER_BASELINE);
	}

	// Get reward button
	_Button *RewardButton = Assets.Buttons["button_trader_bag_reward"];
	glm::ivec2 DrawPosition = (RewardButton->Bounds.Start + RewardButton->Bounds.End) / 2;

	// Draw item
	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	Graphics.DrawCenteredImage(DrawPosition, Trader->RewardItem->GetImage());
	Assets.Fonts["hud_small"]->DrawText(std::to_string(Trader->Count).c_str(), DrawPosition + glm::ivec2(0, -32), COLOR_WHITE, CENTER_BASELINE);
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
	_Player *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {
		TraderWindow->setVisible(true);

		// Draw player information
		//Graphics.RenderText(TradePlayer->Name.c_str(), OffsetX + 72, OffsetY + 70 - 55, _Graphics::ALIGN_CENTER);
		Graphics.DrawCenteredImage(Stats.GetPortrait(TradePlayer->GetPortraitID())->Image, OffsetX + 72, OffsetY + 70);

		// Draw items
		DrawTradeItems(TradePlayer, TRADE_WINDOWX, TRADE_WINDOWYTHEM, true);

		// Draw gold
		//Graphics.RenderText(std::string(TradePlayer->TradeGold).c_str(), OffsetX + TRADE_WINDOWX + 35, OffsetY + TRADE_WINDOWYTHEM + 70);

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
	//Graphics.RenderText(Player->Name.c_str(), OffsetX + 72, OffsetY + 198 - 55, _Graphics::ALIGN_CENTER);
	Graphics.DrawCenteredImage(Stats.GetPortrait(Player->GetPortraitID())->Image, OffsetX + 72, OffsetY + 198);
	*/
}

// Draw the action bar
void _HUD::DrawActionBar() {
	Assets.Elements["element_actionbar"]->Render();

	// Draw trader items
	for(size_t i = 0; i < BATTLE_MAXSKILLS; i++) {

		// Get button position
		std::stringstream Buffer;
		Buffer << "button_actionbar_" << i;
		_Button *Button = Assets.Buttons[Buffer.str()];
		glm::ivec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2;

		// Draw skill icon
		const _Skill *Skill = Player->GetSkillBar(i);
		if(Skill) {
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.DrawCenteredImage(DrawPosition, Skill->Image);
		}

		// Draw hotkey
		Assets.Fonts["hud_small"]->DrawText(Actions.GetInputNameForAction(_Actions::SKILL1 + i), DrawPosition + glm::ivec2(-16, 19), COLOR_WHITE, CENTER_BASELINE);
	}
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
	sprintf(Buffer, "%d", Player->Experience);
	//Graphics.RenderText(Buffer, RightDrawX, DrawY, _Graphics::ALIGN_RIGHT);

	// Experience needed
	DrawY += 15;
	//Graphics.RenderText("EXP needed", DrawX, DrawY);
	sprintf(Buffer, "%d", Player->ExperienceNeeded);
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
	_Element *Element = Assets.Elements["element_skills"];
	Element->Render();

	// Show remaining skill points
	std::string SkillPointsText;
	if(Player->GetSkillPointsRemaining() != 1)
		SkillPointsText	= std::to_string(Player->GetSkillPointsRemaining()) + " Skill Points";
	else
		SkillPointsText	= std::to_string(Player->GetSkillPointsRemaining()) + " Skill Point";

	glm::ivec2 DrawPosition = glm::ivec2((Element->Bounds.End.x + Element->Bounds.Start.x) / 2, Element->Bounds.End.y - 30);
	Assets.Fonts["hud_medium"]->DrawText(SkillPointsText.c_str(), DrawPosition, COLOR_WHITE, CENTER_BASELINE);
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
		WindowOffset.x += INVENTORY_TOOLTIP_OFFSET;
		WindowOffset.y += -(TooltipElement->Bounds.End.y - TooltipElement->Bounds.Start.y) / 2;

		// Reposition window if out of bounds
		if(WindowOffset.x + Width > Graphics.Element->Bounds.End.x - INVENTORY_TOOLTIP_PADDING)
			WindowOffset.x -= Width + INVENTORY_TOOLTIP_OFFSET + INVENTORY_TOOLTIP_PADDING;

		TooltipElement->SetOffset(WindowOffset);
		TooltipElement->SetWidth(Width);

		// Render tooltip
		TooltipElement->Render();

		// Set draw position to center of window
		glm::ivec2 DrawPosition(TooltipElement->Size.x / 2 + WindowOffset.x, TooltipType->Bounds.End.y);
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
		if(Item->MaxHealth > 0) {
			std::stringstream Buffer;
			Buffer << "+" << Item->MaxHealth;
			Assets.Fonts["hud_medium"]->DrawText("HP", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		}
		if(Item->MaxMana > 0) {
			std::stringstream Buffer;
			Buffer << "+" << Item->MaxMana;
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
		glm::ivec2 DrawPosition = Input.GetMouse();
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(DrawPosition, CursorSkill.Skill->Image);
	}
}

// Draws the skill tooltip window
void _HUD::DrawSkillTooltip() {
	const _Skill *Skill = TooltipSkill.Skill;
	if(Skill) {
		_Element *TooltipElement = Assets.Elements["element_skills_tooltip"];
		_Label *TooltipName = Assets.Labels["label_skills_tooltip_name"];

		// Set label values
		TooltipName->Text = Skill->Name;

		// Get window width
		glm::ivec2 Size = TooltipElement->Size;

		// Position window
		glm::ivec2 WindowOffset = Input.GetMouse();
		WindowOffset.x += INVENTORY_TOOLTIP_OFFSET;
		WindowOffset.y += -Size.y / 2;

		// Reposition window if out of bounds
		if(WindowOffset.y < Graphics.Element->Bounds.Start.x + INVENTORY_TOOLTIP_PADDING)
			WindowOffset.y = Graphics.Element->Bounds.Start.x + INVENTORY_TOOLTIP_PADDING;
		if(WindowOffset.x + Size.x > Graphics.Element->Bounds.End.x - INVENTORY_TOOLTIP_PADDING)
			WindowOffset.x -= Size.x + INVENTORY_TOOLTIP_OFFSET + INVENTORY_TOOLTIP_PADDING;
		if(WindowOffset.y + Size.y > Graphics.Element->Bounds.End.y - INVENTORY_TOOLTIP_PADDING)
			WindowOffset.y -= Size.y + INVENTORY_TOOLTIP_OFFSET - (TooltipElement->Bounds.End.y - TooltipElement->Bounds.Start.y) / 2;

		TooltipElement->SetOffset(WindowOffset);

		// Render tooltip
		TooltipElement->Render();

		// Set draw position to center of window
		glm::ivec2 DrawPosition(WindowOffset.x + 20, TooltipName->Bounds.End.y);
		DrawPosition.y += 30;

		// Get current skill level
		int SkillLevel = Player->GetSkillLevel(Skill->ID);

		// Get current level description
		Assets.Fonts["hud_small"]->DrawText("Level " + std::to_string(std::max(1, SkillLevel)), DrawPosition, COLOR_WHITE, LEFT_BASELINE);
		DrawPosition.y += 25;
		DrawSkillDescription(Skill, SkillLevel, DrawPosition, Size.x);

		// Get next level description
		if(SkillLevel > 0) {
			DrawPosition.y += 25;
			Assets.Fonts["hud_small"]->DrawText("Level " + std::to_string(SkillLevel+1), DrawPosition, COLOR_WHITE, LEFT_BASELINE);
			DrawPosition.y += 25;
			DrawSkillDescription(Skill, SkillLevel+1, DrawPosition, Size.x);
		}

		// Additional information
		switch(Skill->Type) {
			case _Skill::TYPE_PASSIVE:
				DrawPosition.y += 25;
				Assets.Fonts["hud_small"]->DrawText("Passive skills must be equipped to the actionbar", DrawPosition, COLOR_GRAY, LEFT_BASELINE);
			break;
		}
	}
}

// Draw the skill description
void _HUD::DrawSkillDescription(const _Skill *Skill, int SkillLevel, glm::ivec2 &DrawPosition, int Width) {

	// Get power range
	int PowerMin, PowerMax;
	Skill->GetPowerRange(SkillLevel, PowerMin, PowerMax);

	// Get power range rounded
	int PowerMinRound, PowerMaxRound;
	Skill->GetPowerRangeRound(SkillLevel, PowerMinRound, PowerMaxRound);

	// Get floating point range
	float PowerMinFloat, PowerMaxFloat;
	Skill->GetPowerRange(SkillLevel, PowerMinFloat, PowerMaxFloat);

	// Get percent
	int PowerPercent = (int)std::roundf(PowerMaxFloat * 100);

	// Draw description
	int SpacingY = 25;
	char Buffer[512];
	Buffer[0] = 0;
	switch(Skill->Type) {
		case _Skill::TYPE_ATTACK:
			sprintf(Buffer, Skill->Info.c_str(), PowerPercent);
			Assets.Fonts["hud_small"]->DrawText(Buffer, DrawPosition, COLOR_GRAY, LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		break;
		case _Skill::TYPE_SPELL:
			switch(Skill->ID) {
				case 3:
					sprintf(Buffer, Skill->Info.c_str(), PowerMaxRound);
				break;
				case 6:
				case 11:
					sprintf(Buffer, Skill->Info.c_str(), PowerMinRound, PowerMaxRound);
				break;
			}
			Assets.Fonts["hud_small"]->DrawText(Buffer, DrawPosition, COLOR_GRAY, LEFT_BASELINE);
			DrawPosition.y += SpacingY;

			sprintf(Buffer, "%d Mana", Skill->GetManaCost(SkillLevel));
			Assets.Fonts["hud_small"]->DrawText(Buffer, DrawPosition, COLOR_BLUE, LEFT_BASELINE);
			DrawPosition.y += 15;
		break;
		case _Skill::TYPE_USEPOTION:
			sprintf(Buffer, Skill->Info.c_str(), PowerMin);
			Assets.Fonts["hud_small"]->DrawText(Buffer, DrawPosition, COLOR_GRAY, LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		break;
		case _Skill::TYPE_PASSIVE:
			switch(Skill->ID) {
				case 4:
				case 5:
					sprintf(Buffer, Skill->Info.c_str(), PowerMaxRound);
				break;
				case 7:
				case 8:
					sprintf(Buffer, Skill->Info.c_str(), PowerMaxFloat);
				break;
				case 9:
				case 10:
					sprintf(Buffer, Skill->Info.c_str(), PowerMax);
				break;
			}
			Assets.Fonts["hud_small"]->DrawText(Buffer, DrawPosition, COLOR_GRAY, LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		break;
		default:
			Assets.Fonts["hud_small"]->DrawText(Skill->Info.c_str(), DrawPosition, COLOR_GRAY, LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		break;
	}
}

// Draws an item's price
void _HUD::DrawItemPrice(const _Item *Item, int Count, const glm::ivec2 &DrawPosition, bool Buy) {
	if(!Vendor)
		return;

	// Real price
	int Price = Item->GetPrice(Vendor, Count, Buy);

	// Color
	glm::vec4 Color;
	if(Buy && Player->Gold < Price)
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
	_Player *TradePlayer = Player->TradePlayer;
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
	if(Player->Gold >= TCursorItem->Cost && Player->AddItem(TCursorItem->Item, TCursorItem->Count, TTargetSlot)) {

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

// Adjust skill level
void _HUD::AdjustSkillLevel(int SkillID, int Direction) {
	_Buffer Packet;
	Packet.Write<char>(_Network::SKILLS_SKILLADJUST);

	// Sell skill
	if(Direction < 0) {
		Packet.WriteBit(0);
		Player->AdjustSkillLevel(SkillID, -1);
	}
	// Buy skill
	else {
		Packet.WriteBit(1);
		Player->AdjustSkillLevel(SkillID, 1);
		if(Player->GetSkillLevel(SkillID) == 1) {

			// Equip new skills
			const _Skill *Skill = Stats.GetSkill(SkillID);
			if(Skill) {
				int Direction, Slot;
				if(Skill->Type == _Skill::TYPE_PASSIVE) {
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

// Sets the player's skill bar
void _HUD::SetSkillBar(int Slot, int OldSlot, const _Skill *Skill) {
	if(Player->GetSkillBar(Slot) == Skill)
		return;

	// Check for swapping
	if(OldSlot == -1) {

		// Remove duplicate skills
		for(int i = 0; i < 8; i++) {
			if(Player->GetSkillBar(i) == Skill)
				Player->SetSkillBar(i, nullptr);
		}
	}
	else {
		const _Skill *OldSkill = Player->GetSkillBar(Slot);
		Player->SetSkillBar(OldSlot, OldSkill);
	}

	Player->SetSkillBar(Slot, Skill);
	Player->CalculatePlayerStats();
	SkillBarChanged = true;
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

	Player->TradePlayer = nullptr;
}

// Make sure the trade gold box is valid
int _HUD::ValidateTradeGold() {
/*
	// Get gold amount
	int Gold = atoi(std::string(TradeGoldBox->getText()).c_str());
	if(Gold < 0)
		Gold = 0;
	else if(Gold > Player->Gold)
		Gold = Player->Gold;

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

	_Player *TradePlayer = Player->TradePlayer;
	if(TradePlayer)
		TradePlayer->TradeAccepted = false;
}

// Split a stack of items
void _HUD::SplitStack(int Slot, int TCount) {

	// Split only inventory items
	if(!_Player::IsSlotInventory(Slot))
		return;

	// Build packet
	_Buffer Packet;
	Packet.Write<char>(_Network::INVENTORY_SPLIT);
	Packet.Write<char>(Slot);
	Packet.Write<char>(TCount);

	ClientNetwork->SendPacketToHost(&Packet);
	Player->SplitStack(Slot, TCount);
}

// Toggles the teleport state
void _HUD::ToggleTeleport() {
	_Buffer Packet;
	Packet.Write<char>(_Network::WORLD_TELEPORT);
	ClientNetwork->SendPacketToHost(&Packet);
	Player->StartTeleport();

	if(*State == _ClientState::STATE_TELEPORT)
		*State = _ClientState::STATE_WALK;
	else
		*State = _ClientState::STATE_TELEPORT;
}
