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
#include <states/client.h>
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
#include <menu.h>
#include <packet.h>
#include <ui/element.h>
#include <ui/button.h>
#include <ui/label.h>
#include <ui/textbox.h>
#include <ui/image.h>
#include <ui/style.h>
#include <objects/object.h>
#include <network/oldnetwork.h>
#include <instances/map.h>
#include <objects/item.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

// Initialize
_HUD::_HUD() {
	Player = nullptr;
	Tooltip.Reset();
	Cursor.Reset();
	RewardItemSlot = -1;
	ChatHistory.clear();

	ChatTextBox = Assets.TextBoxes["textbox_chat"];
	ChatTextBox->TextOffset = glm::vec2(5, 15);

	Assets.Labels["label_buttonbar_teleport"]->Text = Actions.GetInputNameForAction(_Actions::TELEPORT).substr(0, HUD_KEYNAME_LENGTH);
	Assets.Labels["label_buttonbar_inventory"]->Text = Actions.GetInputNameForAction(_Actions::INVENTORY).substr(0, HUD_KEYNAME_LENGTH);
	Assets.Labels["label_buttonbar_trade"]->Text = Actions.GetInputNameForAction(_Actions::TRADE).substr(0, HUD_KEYNAME_LENGTH);
	Assets.Labels["label_buttonbar_skills"]->Text = Actions.GetInputNameForAction(_Actions::SKILLS).substr(0, HUD_KEYNAME_LENGTH);
	Assets.Labels["label_buttonbar_menu"]->Text = Actions.GetInputNameForAction(_Actions::MENU).substr(0, HUD_KEYNAME_LENGTH);

	ActionBarElement = Assets.Elements["element_actionbar"];
	ButtonBarElement = Assets.Elements["element_buttonbar"];
	InventoryElement = Assets.Elements["element_inventory"];
	CharacterElement = Assets.Elements["element_character"];
	VendorElement = Assets.Elements["element_vendor"];
	TradeElement = Assets.Elements["element_trade"];
	TradeTheirsElement = Assets.Elements["element_trade_theirs"];
	TraderElement = Assets.Elements["element_trader"];
	SkillsElement = Assets.Elements["element_skills"];
	TeleportElement = Assets.Elements["element_teleport"];
	ChatElement = Assets.Elements["element_chat"];

	ActionBarElement->SetVisible(true);
	ButtonBarElement->SetVisible(true);
	InventoryElement->SetVisible(false);
	CharacterElement->SetVisible(false);
	VendorElement->SetVisible(false);
	TradeElement->SetVisible(false);
	TradeTheirsElement->SetVisible(false);
	TraderElement->SetVisible(false);
	SkillsElement->SetVisible(false);
	TeleportElement->SetVisible(false);
	ChatElement->SetVisible(false);

	Assets.Elements["element_hud"]->SetVisible(true);
	Assets.Elements["element_hud_health"]->SetVisible(true);
	Assets.Elements["element_hud_mana"]->SetVisible(true);
	Assets.Elements["element_hud_experience"]->SetVisible(true);

	CloseChat();
}

// Shutdown
_HUD::~_HUD() {
	ChatTextBox = nullptr;
	ClearSkills();
}

// Handle the enter key
void _HUD::HandleEnter() {

	if(IsTypingGold()) {
		FocusedElement = nullptr;
		ValidateTradeGold();
	}
	else {
		ToggleChat();
	}
}

// Mouse events
void _HUD::MouseEvent(const _MouseEvent &MouseEvent) {
	if(!Player)
		return;

	// Press
	if(MouseEvent.Pressed) {
		if(Tooltip.Item) {
			switch(Tooltip.Window) {
				case WINDOW_TRADEYOURS:
				case WINDOW_INVENTORY:

					// Pickup item
					if(MouseEvent.Button == SDL_BUTTON_LEFT) {
						if(Input.ModKeyDown(KMOD_CTRL))
							SplitStack(Tooltip.Slot, 1);
						else
							Cursor = Tooltip;
					}
					// Use an item
					else if(MouseEvent.Button == SDL_BUTTON_RIGHT) {
						if(Input.ModKeyDown(KMOD_SHIFT)) {
							SellItem(&Tooltip, 1);
						}
						else if(Player->UseInventory(Tooltip.Slot)) {
							_Buffer Packet;
							Packet.Write<char>(Packet::INVENTORY_USE);
							Packet.Write<char>(Tooltip.Slot);
							OldClientNetwork->SendPacketToHost(&Packet);
						}
					}
				break;
				case WINDOW_VENDOR:
					if(MouseEvent.Button == SDL_BUTTON_LEFT) {
						Cursor = Tooltip;
					}
					else if(MouseEvent.Button == SDL_BUTTON_RIGHT) {
						if(Tooltip.Window == WINDOW_VENDOR)
							BuyItem(&Tooltip, -1);
						else if(Tooltip.Window == WINDOW_INVENTORY && Input.ModKeyDown(KMOD_SHIFT))
							SellItem(&Tooltip, 1);
					}
				break;
			}
		}
		else if(Tooltip.Skill && SkillsElement->Visible) {
			if(MouseEvent.Button == SDL_BUTTON_LEFT) {
				if(Tooltip.Skill && Player->SkillLevels[Tooltip.Skill->ID] > 0)
					Cursor = Tooltip;
			}
		}
	}
	// Release left mouse button
	else if(MouseEvent.Button == SDL_BUTTON_LEFT) {

		// Check button bar
		if(ButtonBarElement->GetClickedElement()) {
			if(ButtonBarElement->GetClickedElement()->Identifier == "button_buttonbar_teleport") {
				ToggleTeleport();
			}
			else if(ButtonBarElement->GetClickedElement()->Identifier == "button_buttonbar_inventory") {
				ToggleInventory();
			}
			else if(ButtonBarElement->GetClickedElement()->Identifier == "button_buttonbar_trade") {
				ToggleTrade();
			}
			else if(ButtonBarElement->GetClickedElement()->Identifier == "button_buttonbar_skills") {
				ToggleSkills();
			}
			else if(ButtonBarElement->GetClickedElement()->Identifier == "button_buttonbar_menu") {
				ToggleMenu();
			}
		}
		// Check skill level up/down
		else if(SkillsElement->GetClickedElement()) {
			if(SkillsElement->GetClickedElement()->Identifier == "button_skills_plus") {
				AdjustSkillLevel((intptr_t)SkillsElement->GetClickedElement()->Parent->UserData, 1);
			}
			else if(SkillsElement->GetClickedElement()->Identifier == "button_skills_minus") {
				AdjustSkillLevel((intptr_t)SkillsElement->GetClickedElement()->Parent->UserData, -1);
			}
		}
		// Accept trader button
		else if(TraderElement->GetClickedElement() == Assets.Buttons["button_trader_accept"]) {
			_Buffer Packet;
			Packet.Write<char>(Packet::TRADER_ACCEPT);
			OldClientNetwork->SendPacketToHost(&Packet);
			Player->AcceptTrader(RequiredItemSlots, RewardItemSlot);
			Player->CalculatePlayerStats();
			CloseWindows();
		}
		// Cancel trader button
		else if(TraderElement->GetClickedElement() == Assets.Buttons["button_trader_cancel"]) {
			CloseWindows();
		}
		// Released an item
		else if(Cursor.Item) {
			switch(Cursor.Window) {

				// Coming from inventory
				case WINDOW_TRADEYOURS:
				case WINDOW_INVENTORY:
					switch(Tooltip.Window) {

						// Send inventory move packet
						case WINDOW_TRADEYOURS:
						case WINDOW_INVENTORY:

							if(Tooltip.Slot >= 0 && Player->MoveInventory(Cursor.Slot, Tooltip.Slot)) {
								_Buffer Packet;
								Packet.Write<char>(Packet::INVENTORY_MOVE);
								Packet.Write<char>(Cursor.Slot);
								Packet.Write<char>(Tooltip.Slot);
								OldClientNetwork->SendPacketToHost(&Packet);

								ResetAcceptButton();
								Player->CalculatePlayerStats();
							}
						break;
						// Sell an item
						case WINDOW_VENDOR:
							SellItem(&Cursor, 1);
						break;
					}
				break;
				// Buy an item
				case WINDOW_VENDOR:
					if(Tooltip.Window == WINDOW_INVENTORY) {
						BuyItem(&Cursor, Tooltip.Slot);
					}
				break;
			}
		}
		// Released a skill
		else if(Cursor.Skill) {
			if(Tooltip.Window == WINDOW_ACTIONBAR) {
				if(Cursor.Window == WINDOW_SKILLS)
					SetActionBar(Tooltip.Slot, -1, Cursor.Skill);
				else if(Cursor.Window == WINDOW_ACTIONBAR)
					SetActionBar(Tooltip.Slot, Cursor.Slot, Cursor.Skill);
			}
			else if(Cursor.Window == WINDOW_ACTIONBAR && (Tooltip.Slot == -1 || Tooltip.Window == -1)) {
				SetActionBar(Cursor.Slot, -1, nullptr);
			}
		}

		switch(Player->State) {
			case _Object::STATE_VENDOR:
			case _Object::STATE_TRADE:
				if(MouseEvent.Button == SDL_BUTTON_LEFT) {
					if(!Cursor.Item) {

						// Check for accept button
						_Button *AcceptButton = Assets.Buttons["button_trade_accept_yours"];
						if(TradeElement->GetClickedElement() == AcceptButton) {
							AcceptButton->Checked = !AcceptButton->Checked;
							UpdateAcceptButton();

							//_Buffer Packet;
							_Buffer Packet;
							Packet.Write<char>(Packet::TRADE_ACCEPT);
							Packet.Write<char>(AcceptButton->Checked);
							OldClientNetwork->SendPacketToHost(&Packet);
						}
					}
				}
			break;
		}

		Cursor.Reset();
	}
}

// Updates the HUD
void _HUD::Update(double FrameTime) {
	if(!Player)
		return;

	Tooltip.Reset();

	_Element *HitElement = Graphics.Element->HitElement;
	if(HitElement) {
		Tooltip.Slot = (intptr_t)HitElement->UserData;
		if(HitElement->Parent)
			Tooltip.Window = (intptr_t)HitElement->Parent->UserData;

		switch(Tooltip.Window) {
			case WINDOW_INVENTORY:
			case WINDOW_TRADETHEIRS:
			case WINDOW_TRADEYOURS: {
				if(Tooltip.Slot >= 0) {
					_InventorySlot *InventorySlot = &Player->Inventory[Tooltip.Slot];
					Tooltip.Item = InventorySlot->Item;
					Tooltip.Count = InventorySlot->Count;
				}
			} break;
			case WINDOW_VENDOR: {
				if(Player->Vendor && (size_t)Tooltip.Slot < Player->Vendor->Items.size()) {
					Tooltip.Item = Player->Vendor->Items[Tooltip.Slot];
					if(Tooltip.Item) {
						Tooltip.Cost = Tooltip.Item->GetPrice(Player->Vendor, 1, true);
						Tooltip.Count = 1;
					}
				}
			} break;
			case WINDOW_TRADER: {
				if((size_t)Tooltip.Slot < Player->Trader->TraderItems.size())
					Tooltip.Item = Player->Trader->TraderItems[Tooltip.Slot].Item;
				else if(Tooltip.Slot == 8)
					Tooltip.Item = Player->Trader->RewardItem;
			} break;
			case WINDOW_SKILLS: {
				Tooltip.Skill = ClientState.Stats->Skills[Tooltip.Slot];
			} break;
			case WINDOW_ACTIONBAR: {
				Tooltip.Skill = Player->GetActionBar(Tooltip.Slot);
			} break;
		}
	}

	switch(Player->State) {
		case _Object::STATE_TRADE: {

			// Get trade items
			if(Player->State == _Object::STATE_TRADE) {
				TradeTheirsElement->SetVisible(false);
				if(Player->TradePlayer) {
					TradeTheirsElement->SetVisible(true);
					Assets.Labels["label_trade_status"]->SetVisible(false);

					Assets.TextBoxes["textbox_trade_gold_theirs"]->Text = std::to_string(Player->TradePlayer->TradeGold);
					Assets.Labels["label_trade_name_theirs"]->Text = Player->TradePlayer->Name;
					Assets.Images["image_trade_portrait_theirs"]->Texture = Player->TradePlayer->Portrait;
				}
			}
		} break;
	}

	if(IsChatting()) {
		if(ChatTextBox != FocusedElement)
			CloseChat();
	}
}

// Draws the HUD elements
void _HUD::Render() {

	// Draw chat messages
	DrawChat(IsChatting());

	Assets.Elements["element_hud"]->Render();
	ButtonBarElement->Render();
	DrawActionBar();

	std::stringstream Buffer;

	// Set hud values
	Buffer << "Level " << Player->Level;
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
		// Draw PVP icon
		if(Player->GetTile()->PVP) {
			if(!Player->CanAttackPlayer())
				Graphics.DrawImage(_Graphics::IMAGE_PVP, StartX, StartY + 8, PVPColor);
		}
	*/

	DrawInventory();
	DrawCharacter();
	DrawVendor();
	DrawTrade();
	DrawTrader();
	DrawSkills();
	DrawTeleport();

	// Draw item information
	DrawCursorItem();
	if(Tooltip.Item)
		Tooltip.Item->DrawTooltip(Player, Tooltip);

	// Draw skill information
	DrawCursorSkill();
	if(Tooltip.Skill)
		Tooltip.Skill->DrawTooltip(Player, Tooltip, SkillsElement->Visible);
}

// Starts the chat box
void _HUD::ToggleChat() {
	if(IsTypingGold())
		return;

	if(IsChatting()) {
		if(ChatTextBox->Text != "") {

			// Send message to server
			_Buffer Packet;
			Packet.Write<char>(Packet::CHAT_MESSAGE);
			Packet.WriteString(ChatTextBox->Text.c_str());
			OldClientNetwork->SendPacketToHost(&Packet);
		}

		CloseChat();
	}
	else {
		ChatElement->SetVisible(true);
		ChatTextBox->ResetCursor();
		FocusedElement = ChatTextBox;
	}
}

// Toggles the teleport state
void _HUD::ToggleTeleport() {
	_Buffer Packet;
	Packet.Write<char>(Packet::WORLD_TELEPORT);
	OldClientNetwork->SendPacketToHost(&Packet);
	Player->StartTeleport();

	if(Player->State == _Object::STATE_TELEPORT) {
		TeleportElement->SetVisible(false);
	}
	else {
		TeleportElement->SetVisible(true);
	}
}

// Open/close inventory
void _HUD::ToggleInventory() {
	Cursor.Reset();

	if(!InventoryElement->Visible) {
		InventoryElement->SetVisible(true);
		CharacterElement->SetVisible(true);
		//OldClientState.SendBusy(true);
	}
	else {
		CloseInventory();
	}
}

// Open/close trade
void _HUD::ToggleTrade() {
	if(!TradeElement->Visible) {
		InitTrade();
	}
	else {
		CloseTrade();
	}
}

// Open/close skills
void _HUD::ToggleSkills() {
	SkillsElement->SetVisible(!SkillsElement->Visible);
	if(SkillsElement->Visible) {
		InitSkills();
	}
	else {
		CloseSkills();
	}
}

// Open/close menu
void _HUD::ToggleMenu() {
	Menu.InitInGame();
}

// Initialize the vendor
void _HUD::InitVendor(int VendorID) {
	if(Player->State == _Object::STATE_VENDOR)
		return;

	Cursor.Reset();

	// Get vendor stats
	Player->Vendor = ClientState.Stats->GetVendor(VendorID);

	// Open inventory
	InventoryElement->SetVisible(true);
	VendorElement->SetVisible(true);
	//OldClientState.SendBusy(true);
}

// Initialize the trade system
void _HUD::InitTrade() {
	if(Player->State != _Object::STATE_WALK)
		return;

	InventoryElement->SetVisible(true);
	TradeElement->SetVisible(true);

	// Send request to server
	SendTradeRequest();

	// Reset UI
	ResetAcceptButton();

	TradeTheirsElement->SetVisible(false);
	Assets.Labels["label_trade_status"]->SetVisible(true);
	Assets.TextBoxes["textbox_trade_gold_theirs"]->Enabled = false;
	Assets.TextBoxes["textbox_trade_gold_theirs"]->Text = "0";
	Assets.TextBoxes["textbox_trade_gold_yours"]->Text = "0";
	Assets.Labels["label_trade_name_yours"]->Text = Player->Name;
	Assets.Images["image_trade_portrait_yours"]->Texture = Player->Portrait;
}

// Initialize the trader
void _HUD::InitTrader(int TraderID) {
	if(Player->State == _Object::STATE_TRADER)
		return;

	// Get trader stats
	Player->Trader = ClientState.Stats->GetTrader(TraderID);

	// Check for required items
	RewardItemSlot = Player->GetRequiredItemSlots(RequiredItemSlots);

	// Disable accept button if requirements not met
	if(RewardItemSlot == -1)
		Assets.Buttons["button_trader_accept"]->Enabled = false;
	else
		Assets.Buttons["button_trader_accept"]->Enabled = true;

	TraderElement->SetVisible(true);
}

// Initialize the skills screen
void _HUD::InitSkills() {
	//OldClientState.SendBusy(true);

	// Clear old children
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
	for(const auto &SkillIterator : ClientState.Stats->Skills) {
		const _Skill *Skill = SkillIterator.second;
		if(!Skill)
			continue;

		// Create style
		_Style *Style = new _Style();
		Style->TextureColor = COLOR_WHITE;
		Style->Program = Assets.Programs["ortho_pos_uv"];
		Style->Texture = Skill->Image;
		Style->UserCreated = true;

		// Add skill icon
		_Button *Button = new _Button();
		Button->Identifier = "button_skills_skill";
		Button->Parent = SkillsElement;
		Button->Offset = Offset;
		Button->Size = Skill->Image->Size;
		Button->Alignment = LEFT_TOP;
		Button->Style = Style;
		Button->UserData = (void *)(intptr_t)Skill->ID;
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
		LevelLabel->UserData = (void *)(intptr_t)Skill->ID;
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
		Offset.x += Skill->Image->Size.x + Spacing.x;
		if(Offset.x > SkillsElement->Size.x - Skill->Image->Size.x) {
			Offset.y += Skill->Image->Size.y + Spacing.y;
			Offset.x = Start.x;
		}

		i++;
	}
	SkillsElement->CalculateBounds();
	SkillsElement->SetVisible(true);

	RefreshSkillButtons();
	Cursor.Reset();
	Tooltip.Reset();
	ActionBarChanged = false;
}

// Closes the chat window
void _HUD::CloseChat() {
	ChatElement->SetVisible(false);
	ChatTextBox->Text = "";
}

// Close inventory screen
void _HUD::CloseInventory() {
	if(InventoryElement->Visible)
		//OldClientState.SendBusy(false);
	InventoryElement->SetVisible(false);
	CharacterElement->SetVisible(false);
}

// Close the vendor
void _HUD::CloseVendor() {
	if(Player->State != _Object::STATE_VENDOR)
		return;

	Cursor.Reset();

	// Close inventory
	CloseInventory();
	VendorElement->SetVisible(false);

	// Notify server
	_Buffer Packet;
	Packet.Write<char>(Packet::EVENT_END);
	OldClientNetwork->SendPacketToHost(&Packet);

	Player->Vendor = nullptr;
}

// Close the skills screen
void _HUD::CloseSkills() {
	Cursor.Reset();
	Tooltip.Reset();

	// Send new skill bar to server
	if(ActionBarChanged) {
		_Buffer Packet;
		Packet.Write<char>(Packet::HUD_ACTIONBAR);
		for(int i = 0; i < ACTIONBAR_SIZE; i++)
			Packet.Write<char>(Player->GetActionBarID(i));

		OldClientNetwork->SendPacketToHost(&Packet);
	}

	// No longer busy
	//OldClientState.SendBusy(false);
	SkillsElement->SetVisible(false);
}

// Closes the trade system
void _HUD::CloseTrade(bool SendNotify) {
	if(!(Player->State == _Object::STATE_TRADE || Player->State == _Object::STATE_TRADE))
		return;

	FocusedElement = nullptr;

	// Close inventory
	CloseInventory();
	TradeElement->SetVisible(false);

	// Notify server
	if(SendNotify)
		SendTradeCancel();

	Player->TradePlayer = nullptr;
}

// Close the trader
void _HUD::CloseTrader() {
	if(Player->State != _Object::STATE_TRADER)
		return;

	Cursor.Reset();
	TraderElement->SetVisible(false);

	// Notify server
	_Buffer Packet;
	Packet.Write<char>(Packet::EVENT_END);
	OldClientNetwork->SendPacketToHost(&Packet);

	Player->Trader = nullptr;
}

// Closes all windows
void _HUD::CloseWindows() {
	CloseInventory();
	CloseVendor();
	CloseSkills();
	CloseTrade();
	CloseTrader();
}

// Draws chat messages
void _HUD::DrawChat(bool IgnoreTimeout) {
/*
	// Draw window
	ChatElement->Render();

	// Set up UI position
	int SpacingY = -20;
	glm::ivec2 DrawPosition = glm::ivec2(ChatElement->Bounds.Start.x + 10, ChatElement->Bounds.End.y);
	DrawPosition.y += SpacingY + -20;

	// Draw messages
	int Index = 0;
	for(auto Iterator = ChatHistory.rbegin(); Iterator != ChatHistory.rend(); ++Iterator) {
		_ChatMessage &ChatMessage = (*Iterator);

		float TimeLeft = ChatMessage.Time - OldClientState.GetTime() + CHAT_MESSAGE_TIMEOUT;
		if(Index >= CHAT_MESSAGES || (!IgnoreTimeout && TimeLeft <= 0))
			break;

		// Set color
		glm::vec4 Color = ChatMessage.Color;
		if(!IgnoreTimeout && TimeLeft <= CHAT_MESSAGE_FADETIME)
			Color.a = TimeLeft;

		// Draw text
		Assets.Fonts["hud_small"]->DrawText(ChatMessage.Message.c_str(), DrawPosition, Color);
		DrawPosition.y += SpacingY;

		Index++;
	}*/
}

// Draw the teleport sequence
void _HUD::DrawTeleport() {
	double Timeleft = GAME_TELEPORT_TIME - Player->TeleportTime;
	if(Timeleft > 0) {
		Assets.Elements["element_teleport"]->Render();

		std::stringstream Buffer;
		Buffer << "Teleport in " << std::fixed << std::setprecision(1) << Timeleft;
		Assets.Labels["label_teleport_timeleft"]->Text = Buffer.str();
	}
}

// Draws the player's inventory
void _HUD::DrawInventory() {
	if(!InventoryElement->Visible)
		return;

	InventoryElement->Render();

	// Draw player's items
	for(int i = 0; i < _Object::INVENTORY_TRADE; i++) {

		// Get inventory slot
		_InventorySlot *Item = &Player->Inventory[i];
		if(Item->Item && !Cursor.IsEqual(i, WINDOW_INVENTORY)) {

			// Get bag button
			std::stringstream Buffer;
			Buffer << "button_inventory_bag_" << i;
			_Button *Button = Assets.Buttons[Buffer.str()];

			// Get position of slot
			glm::ivec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2;

			// Draw item
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.DrawCenteredImage(DrawPosition, Item->Item->Image);

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
	if(!VendorElement->Visible)
		return;

	VendorElement->Render();

	// Draw vendor items
	for(size_t i = 0; i < Player->Vendor->Items.size(); i++) {
		const _Item *Item = Player->Vendor->Items[i];
		if(Item && !Cursor.IsEqual(i, WINDOW_VENDOR)) {

			// Get bag button
			std::stringstream Buffer;
			Buffer << "button_vendor_bag_" << i;
			_Button *Button = Assets.Buttons[Buffer.str()];

			// Get position of slot
			glm::ivec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2;

			// Draw item
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.DrawCenteredImage(DrawPosition, Item->Image);

			// Draw price
			DrawItemPrice(Item, 1, DrawPosition, true);
		}
	}
}

// Draw the trade screen
void _HUD::DrawTrade() {
	if(!TradeElement->Visible)
		return;

	TradeElement->Render();

	// Draw items
	DrawTradeItems(Player, "button_trade_yourbag_", WINDOW_TRADEYOURS);
	DrawTradeItems(Player->TradePlayer, "button_trade_theirbag_", WINDOW_TRADETHEIRS);
}

// Draws trading items
void _HUD::DrawTradeItems(_Object *Player, const std::string &ElementPrefix, int Window) {
	if(!Player)
		return;

	// Draw offered items
	int BagIndex = 0;
	for(int i = _Object::INVENTORY_TRADE; i < _Object::INVENTORY_COUNT; i++) {

		// Get inventory slot
		_InventorySlot *Item = &Player->Inventory[i];
		if(Item->Item && !Cursor.IsEqual(i, Window)) {

			// Get bag button
			std::stringstream Buffer;
			Buffer << ElementPrefix << BagIndex;
			_Button *Button = Assets.Buttons[Buffer.str()];

			// Get position of slot
			glm::ivec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2;

			// Draw item
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.DrawCenteredImage(DrawPosition, Item->Item->Image);

			// Draw count
			if(Item->Count > 1)
				Assets.Fonts["hud_small"]->DrawText(std::to_string(Item->Count).c_str(), DrawPosition + glm::ivec2(20, 20), glm::vec4(1.0f), RIGHT_BASELINE);
		}

		BagIndex++;
	}
}

// Draw the trader screen
void _HUD::DrawTrader() {
	if(!TraderElement->Visible)
		return;

	TraderElement->Render();

	// Draw trader items
	for(size_t i = 0; i < Player->Trader->TraderItems.size(); i++) {

		// Get button position
		std::stringstream Buffer;
		Buffer << "button_trader_bag_" << i;
		_Button *Button = Assets.Buttons[Buffer.str()];
		glm::ivec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2;

		// Draw item
		const _Item *Item = Player->Trader->TraderItems[i].Item;
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(DrawPosition, Item->Image);

		glm::vec4 Color;
		if(RequiredItemSlots[i] == -1)
			Color = COLOR_RED;
		else
			Color = COLOR_WHITE;

		Assets.Fonts["hud_small"]->DrawText(std::to_string(Player->Trader->TraderItems[i].Count).c_str(), DrawPosition + glm::ivec2(0, -32), Color, CENTER_BASELINE);
	}

	// Get reward button
	_Button *RewardButton = Assets.Buttons["button_trader_bag_reward"];
	glm::ivec2 DrawPosition = (RewardButton->Bounds.Start + RewardButton->Bounds.End) / 2;

	// Draw item
	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	Graphics.DrawCenteredImage(DrawPosition, Player->Trader->RewardItem->Image);
	Assets.Fonts["hud_small"]->DrawText(std::to_string(Player->Trader->Count).c_str(), DrawPosition + glm::ivec2(0, -32), COLOR_WHITE, CENTER_BASELINE);
}

// Draw the action bar
void _HUD::DrawActionBar() {
	if(!ActionBarElement->Visible)
		return;

	ActionBarElement->Render();

	// Draw trader items
	for(size_t i = 0; i < ACTIONBAR_SIZE; i++) {

		// Get button position
		std::stringstream Buffer;
		Buffer << "button_actionbar_" << i;
		_Button *Button = Assets.Buttons[Buffer.str()];
		glm::ivec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2;

		// Draw skill icon
		const _Skill *Skill = Player->GetActionBar(i);
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
	if(!CharacterElement->Visible)
		return;

	CharacterElement->Render();

	// Set up UI
	int SpacingY = 20;
	glm::ivec2 Spacing(15, 0);
	glm::ivec2 DrawPosition = CharacterElement->Bounds.Start;
	DrawPosition.x += CharacterElement->Size.x/2;
	DrawPosition.y += 20 + SpacingY;
	std::stringstream Buffer;

	// Damage
	Buffer << Player->MinDamage << " - " << Player->MaxDamage;
	Assets.Fonts["hud_small"]->DrawText("Damage", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Defense
	Buffer << Player->MinDefense << " - " << Player->MaxDefense;
	Assets.Fonts["hud_small"]->DrawText("Defense", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// HP Regen
	Buffer << std::setprecision(3) << Player->HealthRegen;
	Assets.Fonts["hud_small"]->DrawText("HP regen", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// MP Regen
	Buffer << std::setprecision(3) << Player->ManaRegen;
	Assets.Fonts["hud_small"]->DrawText("MP regen", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Separator
	DrawPosition.y += SpacingY;

	// Playtime
	if(Player->PlayTime < 60)
		Buffer << Player->PlayTime << "s";
	else if(Player->PlayTime < 3600)
		Buffer << Player->PlayTime / 60 << "m";
	else
		Buffer << Player->PlayTime / 3600 << "h" << (Player->PlayTime / 60 % 60) << "m";
	Assets.Fonts["hud_small"]->DrawText("Play time", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Deaths
	Buffer << Player->Deaths;
	Assets.Fonts["hud_small"]->DrawText("Deaths", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Monster kills
	Buffer << Player->MonsterKills;
	Assets.Fonts["hud_small"]->DrawText("Monster kills", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Player kills
	Buffer << Player->PlayerKills;
	Assets.Fonts["hud_small"]->DrawText("Player kills", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
	Buffer.str("");
	DrawPosition.y += SpacingY;
}

// Draws the skill page
void _HUD::DrawSkills() {
	if(!SkillsElement->Visible)
		return;

	SkillsElement->Render();

	// Show remaining skill points
	std::string SkillPointsText;
	if(Player->GetSkillPointsRemaining() != 1)
		SkillPointsText	= std::to_string(Player->GetSkillPointsRemaining()) + " Skill Points";
	else
		SkillPointsText	= std::to_string(Player->GetSkillPointsRemaining()) + " Skill Point";

	glm::ivec2 DrawPosition = glm::ivec2((SkillsElement->Bounds.End.x + SkillsElement->Bounds.Start.x) / 2, SkillsElement->Bounds.End.y - 30);
	Assets.Fonts["hud_medium"]->DrawText(SkillPointsText.c_str(), DrawPosition, COLOR_WHITE, CENTER_BASELINE);
}

// Draws the item under the cursor
void _HUD::DrawCursorItem() {
	if(Cursor.Item) {
		glm::ivec2 DrawPosition = Input.GetMouse();
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(DrawPosition, Cursor.Item->Image);
		DrawItemPrice(Cursor.Item, Cursor.Count, DrawPosition, Cursor.Window == WINDOW_VENDOR);
		if(Cursor.Count > 1)
			Assets.Fonts["hud_small"]->DrawText(std::to_string(Cursor.Count).c_str(), DrawPosition + glm::ivec2(20, 20), glm::vec4(1.0f), RIGHT_BASELINE);
	}
}

// Draws the skill under the cursor
void _HUD::DrawCursorSkill() {
	if(Cursor.Skill) {
		glm::ivec2 DrawPosition = Input.GetMouse();
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(DrawPosition, Cursor.Skill->Image);
	}
}

// Draws an item's price
void _HUD::DrawItemPrice(const _Item *Item, int Count, const glm::ivec2 &DrawPosition, bool Buy) {
	if(!Player->Vendor)
		return;

	// Real price
	int Price = Item->GetPrice(Player->Vendor, Count, Buy);

	// Color
	glm::vec4 Color;
	if(Buy && Player->Gold < Price)
		Color = COLOR_RED;
	else
		Color = COLOR_LIGHTGOLD;

	Assets.Fonts["hud_small"]->DrawText(std::to_string(Price).c_str(), DrawPosition + glm::ivec2(20, -8), Color, RIGHT_BASELINE);
}

// Buys an item from the vendor
void _HUD::BuyItem(_Cursor *Item, int TargetSlot) {
	if(Player->Gold >= Item->Cost && Player->AddItem(Item->Item, Item->Count, TargetSlot)) {

		// Update player
		int Price = Item->Item->GetPrice(Player->Vendor, Item->Count, true);
		Player->UpdateGold(-Price);

		// Notify server
		_Buffer Packet;
		Packet.Write<char>(Packet::VENDOR_EXCHANGE);
		Packet.WriteBit(1);
		Packet.Write<char>(Item->Count);
		Packet.Write<char>(Item->Slot);
		Packet.Write<char>(TargetSlot);
		OldClientNetwork->SendPacketToHost(&Packet);

		Player->CalculatePlayerStats();
	}
}

// Sells an item
void _HUD::SellItem(_Cursor *Item, int Amount) {
	if(!Item->Item || !Player->Vendor)
		return;

	// Update player
	int Price = Item->Item->GetPrice(Player->Vendor, Amount, 0);
	Player->UpdateGold(Price);
	bool Deleted = Player->UpdateInventory(Item->Slot, -Amount);

	// Notify server
	_Buffer Packet;
	Packet.Write<char>(Packet::VENDOR_EXCHANGE);
	Packet.WriteBit(0);
	Packet.Write<char>(Amount);
	Packet.Write<char>(Item->Slot);
	OldClientNetwork->SendPacketToHost(&Packet);

	if(Deleted)
		Item->Reset();

	Player->CalculatePlayerStats();
}

// Adjust skill level
void _HUD::AdjustSkillLevel(uint32_t SkillID, int Direction) {
	if(SkillID == 0)
		return;

	_Buffer Packet;
	Packet.Write<char>(Packet::SKILLS_SKILLADJUST);

	// Sell skill
	if(Direction < 0) {
		Packet.WriteBit(0);
		Player->AdjustSkillLevel(SkillID, -1);
	}
	// Buy skill
	else {
		Packet.WriteBit(1);
		Player->AdjustSkillLevel(SkillID, 1);
		if(Player->SkillLevels[SkillID] == 1) {

			// Equip new skills
			const _Skill *Skill = ClientState.Stats->Skills[SkillID];
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
					if(Player->GetActionBar(Slot) == nullptr) {
						SetActionBar(Slot, -1, Skill);
						break;
					}
					Slot += Direction;
				}
			}
		}
	}
	Packet.Write<char>(SkillID);
	OldClientNetwork->SendPacketToHost(&Packet);

	// Update player
	Player->CalculateSkillPoints();
	Player->CalculatePlayerStats();
	RefreshSkillButtons();
}

// Sets the player's skill bar
void _HUD::SetActionBar(int Slot, int OldSlot, const _Skill *Skill) {
	if(Player->GetActionBar(Slot) == Skill)
		return;

	// Check for swapping
	if(OldSlot == -1) {

		// Remove duplicate skills
		for(int i = 0; i < 8; i++) {
			if(Player->GetActionBar(i) == Skill)
				Player->ActionBar[i] = nullptr;
		}
	}
	else {
		const _Skill *OldSkill = Player->GetActionBar(Slot);
		Player->ActionBar[OldSlot] = OldSkill;
	}

	Player->ActionBar[Slot] = Skill;
	Player->CalculatePlayerStats();
	ActionBarChanged = true;
}

// Delete memory used by skill page
void _HUD::ClearSkills() {
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

// Shows or hides the plus/minus buttons
void _HUD::RefreshSkillButtons() {

	// Get remaining points
	int SkillPointsRemaining = Player->GetSkillPointsRemaining();

	// Loop through buttons
	for(auto &Element : SkillsElement->Children) {
		if(Element->Identifier == "label_skills_level") {
			int SkillID = (intptr_t)Element->UserData;
			_Label *Label = (_Label *)Element;
			Label->Text = std::to_string(Player->SkillLevels[SkillID]);
		}
		else if(Element->Identifier == "button_skills_plus") {
			_Button *Button = (_Button *)Element;

			// Get skill
			uint32_t SkillID = (intptr_t)Button->Parent->UserData;
			const _Skill *Skill = ClientState.Stats->Skills[SkillID];
			if(Skill->SkillCost > SkillPointsRemaining || Player->SkillLevels[SkillID] >= 255)
				Button->SetVisible(false);
			else
				Button->SetVisible(true);
		}
		else if(Element->Identifier == "button_skills_minus") {
			_Button *Button = (_Button *)Element;

			// Get skill
			int SkillID = (intptr_t)Button->Parent->UserData;
			if(Player->SkillLevels[SkillID] == 0)
				Button->SetVisible(false);
			else
				Button->SetVisible(true);
		}
	}
}

// Trade with another player
void _HUD::SendTradeRequest() {
	_Buffer Packet;
	Packet.Write<char>(Packet::TRADE_REQUEST);
	OldClientNetwork->SendPacketToHost(&Packet);
}

// Cancel a trade
void _HUD::SendTradeCancel() {
	_Buffer Packet;
	Packet.Write<char>(Packet::TRADE_CANCEL);
	OldClientNetwork->SendPacketToHost(&Packet);

	Player->TradePlayer = nullptr;
}

// Make sure the trade gold box is valid and send gold to player
void _HUD::ValidateTradeGold() {
	if(!Player)
		return;

	_TextBox *GoldTextBox = Assets.TextBoxes["textbox_trade_gold_yours"];

	// Get gold amount
	std::stringstream Buffer(GoldTextBox->Text);
	int Gold;
	Buffer >> Gold;
	if(Gold < 0)
		Gold = 0;
	else if(Gold > Player->Gold)
		Gold = Player->Gold;

	// Set text
	GoldTextBox->Text = std::to_string(Gold);

	// Send amount
	if(Player->TradePlayer) {
		_Buffer Packet;
		Packet.Write<char>(Packet::TRADE_GOLD);
		Packet.Write<int32_t>(Gold);
		OldClientNetwork->SendPacketToHost(&Packet);
	}

	// Reset agreement
	ResetAcceptButton();
}

// Update accept button label text
void _HUD::UpdateAcceptButton() {
	_Button *AcceptButton = Assets.Buttons["button_trade_accept_yours"];
	_Label *LabelTradeStatusYours = Assets.Labels["label_trade_status_yours"];
	if(AcceptButton->Checked) {
		LabelTradeStatusYours->Text = "Accepted";
		LabelTradeStatusYours->Color = COLOR_GREEN;
	}
	else {
		LabelTradeStatusYours->Text = "Accept";
		LabelTradeStatusYours->Color = COLOR_WHITE;
	}
}

// Resets the trade agreement
void _HUD::ResetAcceptButton() {
	_Button *AcceptButton = Assets.Buttons["button_trade_accept_yours"];
	AcceptButton->Checked = false;
	UpdateAcceptButton();

	UpdateTradeStatus(false);
}

// Update their status label
void _HUD::UpdateTradeStatus(bool Accepted) {
	_Label *LabelTradeStatusTheirs = Assets.Labels["label_trade_status_theirs"];
	if(Accepted) {
		LabelTradeStatusTheirs->Text = "Accepted";
		LabelTradeStatusTheirs->Color = COLOR_GREEN;
	}
	else {
		LabelTradeStatusTheirs->Text = "Unaccepted";
		LabelTradeStatusTheirs->Color = COLOR_RED;
	}
}

// Split a stack of items
void _HUD::SplitStack(int Slot, int Count) {

	// Split only inventory items
	if(!_Object::IsSlotInventory(Slot))
		return;

	// Build packet
	_Buffer Packet;
	Packet.Write<char>(Packet::INVENTORY_SPLIT);
	Packet.Write<char>(Slot);
	Packet.Write<char>(Count);

	OldClientNetwork->SendPacketToHost(&Packet);
	Player->SplitStack(Slot, Count);
}

// Return true if player is typing gold
bool _HUD::IsTypingGold() {
	return FocusedElement == Assets.TextBoxes["textbox_trade_gold_yours"];
}

// Return true if the chatbox is open
bool _HUD::IsChatting() {
	return ChatTextBox->Visible;
}

// Set player for HUD
void _HUD::SetPlayer(_Object *Player) {
	this->Player = Player;

	Assets.Labels["label_hud_name"]->Text = Player->Name;
}
