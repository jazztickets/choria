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
#include <ui/element.h>
#include <ui/button.h>
#include <ui/label.h>
#include <ui/textbox.h>
#include <ui/image.h>
#include <ui/style.h>
#include <states/client.h>
#include <network/clientnetwork.h>
#include <objects/object.h>
#include <objects/skill.h>
#include <objects/item.h>
#include <objects/inventory.h>
#include <objects/statuseffect.h>
#include <objects/buff.h>
#include <objects/battle.h>
#include <objects/map.h>
#include <framework.h>
#include <graphics.h>
#include <input.h>
#include <stats.h>
#include <font.h>
#include <scripting.h>
#include <constants.h>
#include <buffer.h>
#include <assets.h>
#include <actions.h>
#include <menu.h>
#include <packet.h>
#include <database.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <SDL_keyboard.h>

// Initialize
_HUD::_HUD() {
	ShowStats = false;
	Player = nullptr;
	ActionBarChanged = false;
	Tooltip.Reset();
	Cursor.Reset();
	RewardItemSlot = (size_t)-1;
	ChatHistory.clear();

	ChatTextBox = Assets.TextBoxes["textbox_chat"];
	ChatTextBox->ParentOffset = glm::vec2(5, 15);

	Assets.Labels["label_buttonbar_teleport"]->Text = Actions.GetInputNameForAction(_Actions::TELEPORT).substr(0, HUD_KEYNAME_LENGTH);
	Assets.Labels["label_buttonbar_inventory"]->Text = Actions.GetInputNameForAction(_Actions::INVENTORY).substr(0, HUD_KEYNAME_LENGTH);
	Assets.Labels["label_buttonbar_trade"]->Text = Actions.GetInputNameForAction(_Actions::TRADE).substr(0, HUD_KEYNAME_LENGTH);
	Assets.Labels["label_buttonbar_skills"]->Text = Actions.GetInputNameForAction(_Actions::SKILLS).substr(0, HUD_KEYNAME_LENGTH);
	Assets.Labels["label_buttonbar_menu"]->Text = Actions.GetInputNameForAction(_Actions::MENU).substr(0, HUD_KEYNAME_LENGTH);

	DiedElement = Assets.Elements["element_died"];
	StatusEffectsElement = Assets.Elements["element_hud_statuseffects"];
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
	HealthElement = Assets.Elements["element_hud_health"];
	ManaElement = Assets.Elements["element_hud_mana"];

	Assets.Labels["label_hud_gold"]->Size.x = ButtonBarElement->Size.x;
	Assets.Labels["label_hud_gold"]->CalculateBounds();

	DiedElement->SetVisible(false);
	StatusEffectsElement->SetVisible(true);
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
	HealthElement->SetVisible(true);
	ManaElement->SetVisible(true);

	Assets.Elements["element_hud"]->SetVisible(true);
	Assets.Elements["element_hud_experience"]->SetVisible(true);
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
							SplitStack((uint8_t)Tooltip.Slot, 1);
						else
							Cursor = Tooltip;
					}
					// Use an item
					else if(MouseEvent.Button == SDL_BUTTON_RIGHT) {
						if(Input.ModKeyDown(KMOD_SHIFT)) {
							SellItem(&Tooltip, 1);
						}
						else {
							_Buffer Packet;
							Packet.Write<PacketType>(PacketType::INVENTORY_USE);
							Packet.Write<uint8_t>((uint8_t)Tooltip.Slot);
							ClientState.Network->SendPacket(Packet);
						}
					}
				break;
				case WINDOW_VENDOR:
					if(MouseEvent.Button == SDL_BUTTON_LEFT) {
						Cursor = Tooltip;
					}
					else if(MouseEvent.Button == SDL_BUTTON_RIGHT) {
						if(Tooltip.Window == WINDOW_VENDOR) {
							if(Input.ModKeyDown(KMOD_SHIFT))
								BuyItem(&Tooltip, Player->Inventory->Slots.size());
							else
								BuyItem(&Tooltip, Player->Inventory->Slots.size());
						}
						else if(Tooltip.Window == WINDOW_INVENTORY && Input.ModKeyDown(KMOD_SHIFT))
							SellItem(&Tooltip, 1);
					}
				break;
				case WINDOW_ACTIONBAR:
					if(SkillsElement->Visible || InventoryElement->Visible) {
						if(MouseEvent.Button == SDL_BUTTON_LEFT) {
							Cursor = Tooltip;
						}
					}
				break;
			}
		}
		else if(Tooltip.Skill && (SkillsElement->Visible || InventoryElement->Visible)) {
			if(MouseEvent.Button == SDL_BUTTON_LEFT) {
				if(Player->SkillLevels[Tooltip.Skill->ID] > 0)
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
				ToggleInGameMenu();
			}
		}
		// Check skill level up/down
		else if(SkillsElement->GetClickedElement()) {
			if(SkillsElement->GetClickedElement()->Identifier == "button_skills_plus") {
				AdjustSkillLevel((uint32_t)(intptr_t)SkillsElement->GetClickedElement()->Parent->UserData, 1);
			}
			else if(SkillsElement->GetClickedElement()->Identifier == "button_skills_minus") {
				AdjustSkillLevel((uint32_t)(intptr_t)SkillsElement->GetClickedElement()->Parent->UserData, -1);
			}
		}
		// Accept trader button
		else if(TraderElement->GetClickedElement() == Assets.Buttons["button_trader_accept"]) {
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::TRADER_ACCEPT);
			ClientState.Network->SendPacket(Packet);
			CloseWindows();
		}
		// Cancel trader button
		else if(TraderElement->GetClickedElement() == Assets.Buttons["button_trader_cancel"]) {
			CloseWindows();
		}
		// Released an item
		else if(Cursor.Item) {

			// Check source window
			switch(Cursor.Window) {
				case WINDOW_TRADEYOURS:
				case WINDOW_INVENTORY:

					// Check destination window
					switch(Tooltip.Window) {

						// Send inventory move packet
						case WINDOW_TRADEYOURS:
						case WINDOW_INVENTORY:

							if(Tooltip.Slot < Player->Inventory->Slots.size()) {
								_Buffer Packet;
								Packet.Write<PacketType>(PacketType::INVENTORY_MOVE);
								Packet.Write<uint8_t>((uint8_t)Cursor.Slot);
								Packet.Write<uint8_t>((uint8_t)Tooltip.Slot);
								ClientState.Network->SendPacket(Packet);
							}
						break;
						// Sell an item
						case WINDOW_VENDOR:
							SellItem(&Cursor, 1);
						break;
						case WINDOW_ACTIONBAR:
							if(Cursor.Window == WINDOW_INVENTORY)
								SetActionBar(Tooltip.Slot, Player->ActionBar.size(), Cursor.Item);
							else if(Cursor.Window == WINDOW_ACTIONBAR)
								SetActionBar(Tooltip.Slot, Cursor.Slot, Cursor.Item);
						break;
					}
				break;
				// Buy an item
				case WINDOW_VENDOR:
					if(Tooltip.Window == WINDOW_INVENTORY) {
						BuyItem(&Cursor, Tooltip.Slot);
					}
				break;
				// Drag item from actionbar
				case WINDOW_ACTIONBAR:
					switch(Tooltip.Window) {
						// Onto inventory
						case WINDOW_INVENTORY:
							SetActionBar(Cursor.Slot, Player->ActionBar.size(), Tooltip.Item);
						break;
						// Swap action
						case WINDOW_ACTIONBAR:
							SetActionBar(Tooltip.Slot, Cursor.Slot, Cursor.Item);
						break;
						default:
							// Remove action
							if(Tooltip.Slot >= Player->ActionBar.size() || Tooltip.Window == -1) {
								_Action Action;
								SetActionBar(Cursor.Slot, Player->ActionBar.size(), Action);
							}
						break;
					}
				break;
			}
		}
		// Released a skill
		else if(Cursor.Skill) {
			if(Tooltip.Window == WINDOW_ACTIONBAR) {
				if(Cursor.Window == WINDOW_SKILLS)
					SetActionBar(Tooltip.Slot, Player->ActionBar.size(), Cursor.Skill);
				else if(Cursor.Window == WINDOW_ACTIONBAR)
					SetActionBar(Tooltip.Slot, Cursor.Slot, Cursor.Skill);
			}
			else if(Cursor.Window == WINDOW_ACTIONBAR && (Tooltip.Slot >= Player->ActionBar.size() || Tooltip.Window == -1)) {
				_Action Action;
				SetActionBar(Cursor.Slot, Player->ActionBar.size(), Action);
			}
		}
		// Use action
		else if(ActionBarElement->GetClickedElement()) {
			uint8_t Slot = (uint8_t)(intptr_t)ActionBarElement->GetClickedElement()->UserData;
			if(Player->Battle)
				Player->Battle->ClientHandleInput(_Actions::SKILL1 + Slot);
			else
				ClientState.SendActionUse(Slot);
		}

		if(Player->WaitingForTrade) {
			if(MouseEvent.Button == SDL_BUTTON_LEFT) {
				if(!Cursor.Item) {

					// Check for accept button
					_Button *AcceptButton = Assets.Buttons["button_trade_accept_yours"];
					if(TradeElement->GetClickedElement() == AcceptButton) {
						AcceptButton->Checked = !AcceptButton->Checked;
						UpdateAcceptButton();

						//_Buffer Packet;
						_Buffer Packet;
						Packet.Write<PacketType>(PacketType::TRADE_ACCEPT);
						Packet.Write<char>(AcceptButton->Checked);
						ClientState.Network->SendPacket(Packet);
					}
				}
			}
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
		Tooltip.Slot = (size_t)(intptr_t)HitElement->UserData;

		// Get window id, stored in parent's userdata field
		if(HitElement->Parent && Tooltip.Slot != (size_t)-1) {
			Tooltip.Window = (int)(intptr_t)HitElement->Parent->UserData;
		}

		switch(Tooltip.Window) {
			case WINDOW_INVENTORY:
			case WINDOW_TRADEYOURS: {
				if(Tooltip.Slot < Player->Inventory->Slots.size()) {
					_InventorySlot *InventorySlot = &Player->Inventory->Slots[Tooltip.Slot];
					Tooltip.Item = InventorySlot->Item;
					Tooltip.Count = InventorySlot->Count;
					if(Tooltip.Item && Player->Vendor)
						Tooltip.Cost = Tooltip.Item->GetPrice(Player->Vendor, Tooltip.Count, false);
				}
			} break;
			case WINDOW_TRADETHEIRS: {
				if(Player->TradePlayer && Tooltip.Slot < Player->Inventory->Slots.size()) {
					_InventorySlot *InventorySlot = &Player->TradePlayer->Inventory->Slots[Tooltip.Slot];
					Tooltip.Item = InventorySlot->Item;
					Tooltip.Count = InventorySlot->Count;
				}
			}
			case WINDOW_VENDOR: {
				if(Player->Vendor && Tooltip.Slot < Player->Vendor->Items.size()) {
					Tooltip.Item = Player->Vendor->Items[Tooltip.Slot];
					if(Input.ModKeyDown(KMOD_SHIFT))
						Tooltip.Count = 5;
					else
						Tooltip.Count = 1;

					if(Tooltip.Item)
						Tooltip.Cost = Tooltip.Item->GetPrice(Player->Vendor, Tooltip.Count, true);
				}
			} break;
			case WINDOW_TRADER: {
				if(Player->Trader) {
					if(Tooltip.Slot < Player->Trader->TraderItems.size())
						Tooltip.Item = Player->Trader->TraderItems[Tooltip.Slot].Item;
					else if(Tooltip.Slot == 8)
						Tooltip.Item = Player->Trader->RewardItem;
				}
			} break;
			case WINDOW_SKILLS: {
				Tooltip.Skill = ClientState.Stats->Skills[(uint32_t)Tooltip.Slot];
			} break;
			case WINDOW_ACTIONBAR: {
				Tooltip.Skill = Player->ActionBar[Tooltip.Slot].Skill;
				Tooltip.Item = Player->ActionBar[Tooltip.Slot].Item;
			} break;
			case WINDOW_BATTLE:
			case WINDOW_HUD_EFFECTS: {
				Tooltip.StatusEffect = (_StatusEffect *)HitElement->UserData;
			} break;
		}
	}

	// Get trade items
	if(Player->WaitingForTrade) {
		TradeTheirsElement->SetVisible(false);
		if(Player->TradePlayer) {
			TradeTheirsElement->SetVisible(true);
			Assets.Labels["label_trade_status"]->SetVisible(false);

			Assets.TextBoxes["textbox_trade_gold_theirs"]->Text = std::to_string(Player->TradePlayer->TradeGold);
			Assets.Labels["label_trade_name_theirs"]->Text = Player->TradePlayer->Name;
			Assets.Images["image_trade_portrait_theirs"]->Texture = Player->TradePlayer->Portrait;
		}
	}

	if(IsChatting()) {
		if(ChatTextBox != FocusedElement)
			CloseChat();
	}

	// Update stat changes
	for(auto Iterator = StatChanges.begin(); Iterator != StatChanges.end(); ) {
		_StatChange &StatChange = *Iterator;

		// Find start position
		glm::vec2 StartPosition;
		if(StatChange.Object->Battle)
			StartPosition = StatChange.Object->StatPosition;
		else if(StatChange.Object == Player)
			StartPosition = HealthElement->Bounds.Start + HealthElement->Size / 2.0f;
		StatChange.LastPosition = StatChange.Position;

		// Interpolate between start and end position
		StatChange.Position = glm::mix(StartPosition, StartPosition + glm::vec2(0, -20), StatChange.Time / STATCHANGE_TIMEOUT);
		if(StatChange.Time == 0.0)
			StatChange.LastPosition = StatChange.Position;

		// Update timer
		StatChange.Time += FrameTime;
		if(StatChange.Time >= STATCHANGE_TIMEOUT) {
			Iterator = StatChanges.erase(Iterator);
		}
		else
			++Iterator;
	}

}

// Draws the HUD elements
void _HUD::Render(_Map *Map, double BlendFactor, double Time) {
	if(!Player)
		return;

	// Draw chat messages
	DrawChat(Time, IsChatting());

	// Show network stats
	if(ShowStats) {
		std::stringstream Buffer;

		Buffer << Graphics.FramesPerSecond << " FPS";
		Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::vec2(20, 120 + 15 * 0));
		Buffer.str("");

		Buffer << ClientState.Network->GetSentSpeed() / 1024.0f << " KB/s";
		Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::vec2(20, 120 + 15 * 1));
		Buffer.str("");

		Buffer << ClientState.Network->GetReceiveSpeed() / 1024.0f << " KB/s";
		Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::vec2(20, 120 + 15 * 2));
		Buffer.str("");

		Buffer << ClientState.Network->GetRTT() << "ms";
		Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::vec2(20, 120 + 15 * 3));
		Buffer.str("");
	}

	// Draw button bar
	ButtonBarElement->Render();

	// Draw hud elements while alive or in battle
	if(Player->Health > 0 || Player->Battle) {
		std::stringstream Buffer;

		Assets.Elements["element_hud"]->Render();
		DrawActionBar();

		// Set hud values
		Buffer << "Level " << Player->Level;
		Assets.Labels["label_hud_level"]->Text = Buffer.str();
		Buffer.str("");

		Buffer << Player->Gold << " Gold";
		Assets.Labels["label_hud_gold"]->Text = Buffer.str();
		Buffer.str("");

		Map->GetClockAsString(Buffer);
		Assets.Labels["label_hud_clock"]->Text = Buffer.str();
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
		Assets.Images["image_hud_health_bar_full"]->SetWidth(HealthElement->Size.x * HealthPercent);
		Assets.Images["image_hud_health_bar_empty"]->SetWidth(HealthElement->Size.x);
		HealthElement->Render();

		// Draw mana bar
		float ManaPercent = Player->MaxMana > 0 ? Player->Mana / (float)Player->MaxMana : 0;
		Buffer << Player->Mana << " / " << Player->MaxMana;
		Assets.Labels["label_hud_mana"]->Text = Buffer.str();
		Buffer.str("");
		Assets.Images["image_hud_mana_bar_full"]->SetWidth(ManaElement->Size.x * ManaPercent);
		Assets.Images["image_hud_mana_bar_empty"]->SetWidth(ManaElement->Size.x);
		ManaElement->Render();

		DrawHudEffects();
		DrawInventory();
		DrawCharacter();
		DrawVendor();
		DrawTrade();
		DrawTrader();
		DrawSkills();
		DrawTeleport();

		// Draw stat changes
		for(auto &StatChange : StatChanges) {
			StatChange.Render(BlendFactor);
		}

		// Draw item information
		DrawCursorItem();
		if(Tooltip.Item)
			Tooltip.Item->DrawTooltip(Player, Tooltip);

		// Draw skill information
		DrawCursorSkill();
		if(Tooltip.Skill)
			Tooltip.Skill->DrawTooltip(ClientState.Scripting, Player, SkillsElement->Visible);

		// Draw status effects
		if(Tooltip.StatusEffect)
			Tooltip.StatusEffect->Buff->DrawTooltip(Scripting, Tooltip.StatusEffect->Level);
	}
	else {

		// Dead outside of combat

		//Assets.Labels["label_died_gold"]->Text = "You lost " + std::to_string(std::abs(100)) + " gold";
		DiedElement->Size = Graphics.WindowSize;
		DiedElement->CalculateBounds();
		DiedElement->SetVisible(true);
		DiedElement->Render();
	}

}

// Starts the chat box
void _HUD::ToggleChat() {
	if(IsTypingGold())
		return;

	if(IsChatting()) {
		if(ChatTextBox->Text != "") {

			// Send message to server
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::CHAT_MESSAGE);
			Packet.WriteString(ChatTextBox->Text.c_str());
			ClientState.Network->SendPacket(Packet);
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
	if(Player->WaitForServer || Player->Health <= 0)
		return;

	if(!TeleportElement->Visible) {
		CloseWindows();
		ClientState.SendStatus(_Object::STATUS_TELEPORT);
		Player->WaitForServer = true;
	}
	else {
		CloseWindows();
	}
}

// Open/close inventory
void _HUD::ToggleInventory() {
	if(Player->WaitForServer || Player->Health <= 0)
		return;

	if(!InventoryElement->Visible) {
		CloseWindows();

		InventoryElement->SetVisible(true);
		CharacterElement->SetVisible(true);
		ClientState.SendStatus(_Object::STATUS_INVENTORY);
	}
	else {
		CloseWindows();
	}
}

// Open/close trade
void _HUD::ToggleTrade() {
	if(Player->WaitForServer || Player->Health <= 0)
		return;

	if(!TradeElement->Visible) {
		CloseWindows();
		InitTrade();
	}
	else {
		CloseWindows();
	}
}

// Open/close skills
void _HUD::ToggleSkills() {
	if(Player->WaitForServer || Player->Health <= 0)
		return;

	if(!SkillsElement->Visible) {
		CloseWindows();
		InitSkills();
	}
	else {
		CloseWindows();
	}
}

// Open/close menu
void _HUD::ToggleInGameMenu() {
	if(Player->WaitForServer)
		return;

	// Close windows if open
	if(CloseWindows())
		return;

	if(ClientState.IsTesting)
		ClientState.Network->Disconnect();
	else {
		Menu.InitInGame();
	}
}

// Initialize the vendor
void _HUD::InitVendor() {
	Cursor.Reset();

	// Open inventory
	InventoryElement->SetVisible(true);
	VendorElement->SetVisible(true);
}

// Initialize the trade system
void _HUD::InitTrade() {
	if(Player->WaitingForTrade)
		return;

	Player->WaitingForTrade = true;
	InventoryElement->SetVisible(true);
	TradeElement->SetVisible(true);

	// Send request to server
	SendTradeRequest();

	// Reset UI
	ResetAcceptButton();

	// Reset their trade UI
	ResetTradeTheirsWindow();
}

// Initialize the trader
void _HUD::InitTrader() {

	// Check for required items
	RequiredItemSlots.resize(Player->Trader->TraderItems.size(), Player->Inventory->Slots.size());
	RewardItemSlot = Player->Inventory->GetRequiredItemSlots(Player->Trader, RequiredItemSlots);

	// Disable accept button if requirements not met
	if(RewardItemSlot >= Player->Inventory->Slots.size())
		Assets.Buttons["button_trader_accept"]->Enabled = false;
	else
		Assets.Buttons["button_trader_accept"]->Enabled = true;

	TraderElement->SetVisible(true);
}

// Initialize the skills screen
void _HUD::InitSkills() {

	// Clear old children
	ClearSkills();

	glm::vec2 Start(10, 25);
	glm::vec2 Offset(Start);
	glm::vec2 LevelOffset(0, -4);
	glm::vec2 Spacing(10, 50);
	glm::vec2 PlusOffset(-12, 37);
	glm::vec2 MinusOffset(12, 37);
	glm::vec2 LabelOffset(0, 3);
	size_t i = 0;

	// Iterate over skills
	ClientState.Stats->Database->PrepareQuery("SELECT id FROM skill ORDER BY rank");
	while(ClientState.Stats->Database->FetchRow()) {
		const _Skill *Skill = ClientState.Stats->Skills[ClientState.Stats->Database->GetInt<uint32_t>(0)];
		if(!Skill)
			continue;

		// Create style
		_Style *Style = new _Style();
		Style->TextureColor = COLOR_WHITE;
		Style->Program = Assets.Programs["ortho_pos_uv"];
		Style->Texture = Skill->Texture;
		Style->UserCreated = true;

		// Add skill icon
		_Button *Button = new _Button();
		Button->Identifier = "button_skills_skill";
		Button->Parent = SkillsElement;
		Button->Offset = Offset;
		Button->Size = Skill->Texture->Size;
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
		PlusButton->Size = glm::vec2(16, 16);
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
		MinusButton->Size = glm::vec2(16, 16);
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
		Offset.x += Skill->Texture->Size.x + Spacing.x;
		if(Offset.x > SkillsElement->Size.x - Skill->Texture->Size.x) {
			Offset.y += Skill->Texture->Size.y + Spacing.y;
			Offset.x = Start.x;
		}

		i++;
	}
	ClientState.Stats->Database->CloseQuery();

	SkillsElement->CalculateBounds();
	SkillsElement->SetVisible(true);

	RefreshSkillButtons();
	Cursor.Reset();
	ActionBarChanged = false;

	ClientState.SendStatus(_Object::STATUS_SKILLS);
}

// Closes the chat window
void _HUD::CloseChat() {
	ChatElement->SetVisible(false);
	ChatTextBox->Clear();
	FocusedElement = nullptr;
}

// Close inventory screen
bool _HUD::CloseInventory() {
	bool WasOpen = InventoryElement->Visible;
	Cursor.Reset();
	InventoryElement->SetVisible(false);
	CharacterElement->SetVisible(false);

	return WasOpen;
}

// Close the vendor
bool _HUD::CloseVendor() {
	if(!Player->Vendor)
		return false;

	bool WasOpen = VendorElement->Visible;
	CloseInventory();
	Player->Vendor = nullptr;
	VendorElement->SetVisible(false);
	Cursor.Reset();

	return WasOpen;
}

// Close the skills screen
bool _HUD::CloseSkills() {

	// Send new skill bar to server
	if(ActionBarChanged) {
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::ACTIONBAR_CHANGED);
		for(size_t i = 0; i < Player->ActionBar.size(); i++) {
			Player->ActionBar[i].Serialize(Packet);
		}

		ClientState.Network->SendPacket(Packet);
	}

	bool WasOpen = SkillsElement->Visible;

	SkillsElement->SetVisible(false);
	Cursor.Reset();

	return WasOpen;
}

// Closes the trade system
bool _HUD::CloseTrade(bool SendNotify) {

	bool WasOpen = TradeElement->Visible;

	// Close inventory
	CloseInventory();
	TradeElement->SetVisible(false);
	FocusedElement = nullptr;

	// Notify server
	if(SendNotify)
		SendTradeCancel();

	Player->WaitingForTrade = false;
	Player->TradePlayer = nullptr;

	return WasOpen;
}

// Close the trader
bool _HUD::CloseTrader() {
	if(!Player->Trader)
		return false;

	bool WasOpen = TraderElement->Visible;
	TraderElement->SetVisible(false);
	Player->Trader = nullptr;
	Cursor.Reset();

	return WasOpen;
}

// Closes all windows
bool _HUD::CloseWindows() {
	Cursor.Reset();

	bool WasOpen = false;
	WasOpen |= CloseInventory();
	WasOpen |= CloseVendor();
	WasOpen |= CloseSkills();
	WasOpen |= CloseTrade();
	WasOpen |= CloseTrader();

	if(WasOpen)
		ClientState.SendStatus(_Object::STATUS_NONE);

	return WasOpen;
}

// Draws chat messages
void _HUD::DrawChat(double Time, bool IgnoreTimeout) {

	// Draw window
	ChatElement->Render();

	// Set up UI position
	int SpacingY = -20;
	glm::vec2 DrawPosition = glm::vec2(ChatElement->Bounds.Start.x + 10, ChatElement->Bounds.End.y);
	DrawPosition.y += SpacingY + -20;

	// Draw messages
	int Index = 0;
	for(auto Iterator = ChatHistory.rbegin(); Iterator != ChatHistory.rend(); ++Iterator) {
		_ChatMessage &ChatMessage = (*Iterator);

		double TimeLeft = ChatMessage.Time - Time + CHAT_MESSAGE_TIMEOUT;
		if(Index >= CHAT_MESSAGES || (!IgnoreTimeout && TimeLeft <= 0))
			break;

		// Set color
		glm::vec4 Color = ChatMessage.Color;
		if(!IgnoreTimeout && TimeLeft <= CHAT_MESSAGE_FADETIME)
			Color.a = (float)TimeLeft;

		// Draw text
		Assets.Fonts["hud_small"]->DrawText(ChatMessage.Message.c_str(), DrawPosition, Color);
		DrawPosition.y += SpacingY;

		Index++;
	}
}

// Draws player's status effects
void _HUD::DrawHudEffects() {
	StatusEffectsElement->Render();

	// Draw status effects
	glm::vec2 Offset(0, 0);
	for(auto &StatusEffect : Player->StatusEffects) {
		if(StatusEffect->HUDElement) {
			StatusEffect->HUDElement->Offset = Offset;
			StatusEffect->HUDElement->CalculateBounds();
			StatusEffect->Render(StatusEffect->HUDElement, COLOR_WHITE);
			Offset.x += StatusEffect->Buff->Texture->Size.x + 2;
		}
	}
}

// Draw the teleport sequence
void _HUD::DrawTeleport() {
	if(Player->TeleportTime <= 0.0)
		return;

	TeleportElement->SetVisible(true);
	TeleportElement->Render();
	TeleportElement->SetVisible(false);

	std::stringstream Buffer;
	Buffer << "Teleport in " << std::fixed << std::setprecision(1) << Player->TeleportTime;
	Assets.Labels["label_teleport_timeleft"]->Text = Buffer.str();
}

// Draws the player's inventory
void _HUD::DrawInventory() {
	if(!InventoryElement->Visible)
		return;

	InventoryElement->Render();

	// Draw player's items
	for(size_t i = 0; i < InventoryType::TRADE; i++) {

		// Get inventory slot
		_InventorySlot *Item = &Player->Inventory->Slots[i];
		if(Item->Item && !Cursor.IsEqual(i, WINDOW_INVENTORY)) {

			// Get bag button
			std::stringstream Buffer;
			Buffer << "button_inventory_bag_" << i;
			_Button *Button = Assets.Buttons[Buffer.str()];

			// Get position of slot
			glm::vec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2.0f;

			// Draw item
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.DrawCenteredImage(DrawPosition, Item->Item->Texture);

			// Draw price if using vendor
			DrawItemPrice(Item->Item, Item->Count, DrawPosition, false);

			// Draw count
			if(Item->Count > 1)
				Assets.Fonts["hud_tiny"]->DrawText(std::to_string(Item->Count).c_str(), DrawPosition + glm::vec2(20, 20), glm::vec4(1.0f), RIGHT_BASELINE);
		}
	}
}

// Draw the vendor
void _HUD::DrawVendor() {
	if(!Player->Vendor) {
		VendorElement->Visible = false;
		return;
	}

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
			glm::vec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2.0f;

			// Draw item
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.DrawCenteredImage(DrawPosition, Item->Texture);

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
	for(size_t i = InventoryType::TRADE; i < InventoryType::COUNT; i++) {

		// Get inventory slot
		_InventorySlot *Item = &Player->Inventory->Slots[i];
		if(Item->Item && !Cursor.IsEqual(i, Window)) {

			// Get bag button
			std::stringstream Buffer;
			Buffer << ElementPrefix << BagIndex;
			_Button *Button = Assets.Buttons[Buffer.str()];

			// Get position of slot
			glm::vec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2.0f;

			// Draw item
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.DrawCenteredImage(DrawPosition, Item->Item->Texture);

			// Draw count
			if(Item->Count > 1)
				Assets.Fonts["hud_tiny"]->DrawText(std::to_string(Item->Count).c_str(), DrawPosition + glm::vec2(20, 20), glm::vec4(1.0f), RIGHT_BASELINE);
		}

		BagIndex++;
	}
}

// Draw the trader screen
void _HUD::DrawTrader() {
	if(!Player->Trader) {
		TraderElement->Visible = false;
		return;
	}

	TraderElement->Render();

	// Draw trader items
	for(size_t i = 0; i < Player->Trader->TraderItems.size(); i++) {

		// Get button position
		std::stringstream Buffer;
		Buffer << "button_trader_bag_" << i;
		_Button *Button = Assets.Buttons[Buffer.str()];
		glm::vec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2.0f;

		// Draw item
		const _Item *Item = Player->Trader->TraderItems[i].Item;
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(DrawPosition, Item->Texture);

		glm::vec4 Color;
		if(RequiredItemSlots[i] >= Player->Inventory->Slots.size())
			Color = COLOR_RED;
		else
			Color = COLOR_WHITE;

		Assets.Fonts["hud_small"]->DrawText(std::to_string(Player->Trader->TraderItems[i].Count).c_str(), DrawPosition + glm::vec2(0, -32), Color, CENTER_BASELINE);
	}

	// Get reward button
	_Button *RewardButton = Assets.Buttons["button_trader_bag_reward"];
	glm::vec2 DrawPosition = (RewardButton->Bounds.Start + RewardButton->Bounds.End) / 2.0f;

	// Draw item
	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	Graphics.DrawCenteredImage(DrawPosition, Player->Trader->RewardItem->Texture);
	Assets.Fonts["hud_small"]->DrawText(std::to_string(Player->Trader->Count).c_str(), DrawPosition + glm::vec2(0, -32), COLOR_WHITE, CENTER_BASELINE);
}

// Draw the action bar
void _HUD::DrawActionBar() {
	if(!Player || !ActionBarElement->Visible)
		return;

	ActionBarElement->Render();

	// Draw trader items
	for(size_t i = 0; i < Player->ActionBar.size(); i++) {

		// Get button position
		std::stringstream Buffer;
		Buffer << "button_actionbar_" << i;
		_Button *Button = Assets.Buttons[Buffer.str()];
		glm::vec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2.0f;

		// Draw skill icon
		const _Skill *Skill = Player->ActionBar[i].Skill;
		if(Skill) {
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.DrawCenteredImage(DrawPosition, Skill->Texture);
		}

		// Draw item icon
		const _Item *Item = Player->ActionBar[i].Item;
		if(Item) {
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.DrawCenteredImage(DrawPosition, Item->Texture);

			Assets.Fonts["hud_tiny"]->DrawText(std::to_string(Player->ActionBar[i].Count), DrawPosition + glm::vec2(20, 19), COLOR_WHITE, RIGHT_BASELINE);
		}

		// Draw hotkey
		Assets.Fonts["hud_small"]->DrawText(Actions.GetInputNameForAction((int)(_Actions::SKILL1 + i)), DrawPosition + glm::vec2(-16, 19), COLOR_WHITE, CENTER_BASELINE);
	}
}

// Draw the character stats page
void _HUD::DrawCharacter() {
	if(!CharacterElement->Visible)
		return;

	CharacterElement->Render();

	// Set up UI
	int SpacingY = 20;
	glm::vec2 Spacing(15, 0);
	glm::vec2 DrawPosition = CharacterElement->Bounds.Start;
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
	if(Player->HealthRegen != 0.0f) {
		Buffer << std::setprecision(3) << Player->HealthRegen;
		Assets.Fonts["hud_small"]->DrawText("HP regen", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
		Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// MP Regen
	if(Player->ManaRegen != 0.0f) {
		Buffer << std::setprecision(3) << Player->ManaRegen;
		Assets.Fonts["hud_small"]->DrawText("MP regen", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
		Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

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
	if(Player->Deaths > 0) {
		Buffer << Player->Deaths;
		Assets.Fonts["hud_small"]->DrawText("Deaths", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
		Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Monster kills
	if(Player->MonsterKills > 0) {
		Buffer << Player->MonsterKills;
		Assets.Fonts["hud_small"]->DrawText("Monster kills", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
		Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Player kills
	if(Player->PlayerKills > 0) {
		Buffer << Player->PlayerKills;
		Assets.Fonts["hud_small"]->DrawText("Player kills", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
		Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}
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

	glm::vec2 DrawPosition = glm::vec2((SkillsElement->Bounds.End.x + SkillsElement->Bounds.Start.x) / 2, SkillsElement->Bounds.End.y - 30);
	Assets.Fonts["hud_medium"]->DrawText(SkillPointsText.c_str(), DrawPosition, COLOR_WHITE, CENTER_BASELINE);
}

// Draws the item under the cursor
void _HUD::DrawCursorItem() {
	if(Cursor.Item) {
		glm::vec2 DrawPosition = Input.GetMouse();
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(DrawPosition, Cursor.Item->Texture);
		if(Cursor.Window != WINDOW_ACTIONBAR)
			DrawItemPrice(Cursor.Item, Cursor.Count, DrawPosition, Cursor.Window == WINDOW_VENDOR);
		if(Cursor.Count > 1)
			Assets.Fonts["hud_tiny"]->DrawText(std::to_string(Cursor.Count).c_str(), DrawPosition + glm::vec2(20, 20), glm::vec4(1.0f), RIGHT_BASELINE);
	}
}

// Draws the skill under the cursor
void _HUD::DrawCursorSkill() {
	if(Cursor.Skill) {
		glm::vec2 DrawPosition = Input.GetMouse();
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(DrawPosition, Cursor.Skill->Texture);
	}
}

// Draws an item's price
void _HUD::DrawItemPrice(const _Item *Item, int Count, const glm::vec2 &DrawPosition, bool Buy) {
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

	Assets.Fonts["hud_tiny"]->DrawText(std::to_string(Price).c_str(), DrawPosition + glm::vec2(20, -11), Color, RIGHT_BASELINE);
}

// Buys an item from the vendor
void _HUD::BuyItem(_Cursor *Item, size_t TargetSlot) {

	// Notify server
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::VENDOR_EXCHANGE);
	Packet.WriteBit(1);
	Packet.Write<uint8_t>((uint8_t)Item->Count);
	Packet.Write<uint8_t>((uint8_t)Item->Slot);
	Packet.Write<uint8_t>((uint8_t)TargetSlot);
	ClientState.Network->SendPacket(Packet);
}

// Sells an item
void _HUD::SellItem(_Cursor *Item, int Amount) {
	if(!Item->Item || !Player->Vendor)
		return;

	// Notify server
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::VENDOR_EXCHANGE);
	Packet.WriteBit(0);
	Packet.Write<uint8_t>((uint8_t)Amount);
	Packet.Write<uint8_t>((uint8_t)Item->Slot);
	ClientState.Network->SendPacket(Packet);
}

// Adjust skill level
void _HUD::AdjustSkillLevel(uint32_t SkillID, int Direction) {
	if(SkillID == 0)
		return;

	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::SKILLS_SKILLADJUST);

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
				size_t Slot = 0;
				for(size_t i = 0; i < Player->ActionBar.size(); i++) {
					if(!Player->ActionBar[Slot].IsSet()) {
						SetActionBar(Slot, Player->ActionBar.size(), Skill);
						break;
					}
					Slot++;
				}
			}
		}
	}
	Packet.Write<uint32_t>(SkillID);
	ClientState.Network->SendPacket(Packet);

	// Update player
	Player->CalculateSkillPoints();
	Player->CalculateStats();
	RefreshSkillButtons();
}

// Sets the player's skill bar
void _HUD::SetActionBar(size_t Slot, size_t OldSlot, const _Action &Action) {
	if(Player->ActionBar[Slot] == Action)
		return;

	// Check for bringing new skill/item onto bar
	if(OldSlot >= Player->ActionBar.size()) {

		// Remove duplicate skills
		for(size_t i = 0; i < Player->ActionBar.size(); i++) {
			if(Player->ActionBar[i] == Action)
				Player->ActionBar[i].Unset();
		}
	}
	// Rearrange action bar
	else {
		Player->ActionBar[OldSlot] = Player->ActionBar[Slot];
	}

	Player->ActionBar[Slot] = Action;
	Player->RefreshActionBarCount();

	Player->CalculateStats();
	ActionBarChanged = true;
}

// Delete memory used by skill page
void _HUD::ClearSkills() {
	for(auto &Child : SkillsElement->Children) {
		if(Child->Style && Child->Style->UserCreated)
			delete Child->Style;

		// Delete labels
		for(auto &LabelChild : Child->Children) {
			if(LabelChild->UserCreated)
				delete LabelChild;
		}

		if(Child->UserCreated)
			delete Child;
	}
	SkillsElement->Children.clear();
}

// Shows or hides the plus/minus buttons
void _HUD::RefreshSkillButtons() {

	// Get remaining points
	int SkillPointsRemaining = Player->GetSkillPointsRemaining();

	// Loop through buttons
	for(auto &Element : SkillsElement->Children) {
		if(Element->Identifier == "label_skills_level") {
			uint32_t SkillID = (uint32_t)(intptr_t)Element->UserData;
			_Label *Label = (_Label *)Element;
			Label->Text = std::to_string(Player->SkillLevels[SkillID]);
		}
		else if(Element->Identifier == "button_skills_plus") {
			_Button *Button = (_Button *)Element;

			// Get skill
			uint32_t SkillID = (uint32_t)(intptr_t)Button->Parent->UserData;
			if(SkillPointsRemaining == 0 || Player->SkillLevels[SkillID] >= SKILL_MAX_LEVEL)
				Button->SetVisible(false);
			else
				Button->SetVisible(true);
		}
		else if(Element->Identifier == "button_skills_minus") {
			_Button *Button = (_Button *)Element;

			// Get skill
			uint32_t SkillID = (uint32_t)(intptr_t)Button->Parent->UserData;
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
	Packet.Write<PacketType>(PacketType::TRADE_REQUEST);
	ClientState.Network->SendPacket(Packet);
}

// Cancel a trade
void _HUD::SendTradeCancel() {
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::TRADE_CANCEL);
	ClientState.Network->SendPacket(Packet);

	Player->TradePlayer = nullptr;
}

// Make sure the trade gold box is valid and send gold to player
void _HUD::ValidateTradeGold() {
	if(!Player || !TradeElement->Visible)
		return;

	_TextBox *GoldTextBox = Assets.TextBoxes["textbox_trade_gold_yours"];

	// Get gold amount
	std::stringstream Buffer(GoldTextBox->Text);
	int Gold = 0;
	Buffer >> Gold;
	if(Gold < 0)
		Gold = 0;
	else if(Gold > Player->Gold)
		Gold = Player->Gold;

	// Set text
	GoldTextBox->SetText(std::to_string(Gold));

	// Send amount
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::TRADE_GOLD);
	Packet.Write<int32_t>(Gold);
	ClientState.Network->SendPacket(Packet);

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

// Resets upper trade window status
void _HUD::ResetTradeTheirsWindow() {
	TradeTheirsElement->SetVisible(false);
	Assets.Labels["label_trade_status"]->SetVisible(true);
	Assets.TextBoxes["textbox_trade_gold_theirs"]->Enabled = false;
	Assets.TextBoxes["textbox_trade_gold_theirs"]->SetText("0");
	Assets.TextBoxes["textbox_trade_gold_yours"]->SetText("0");
	Assets.Labels["label_trade_name_yours"]->Text = Player->Name;
	Assets.Images["image_trade_portrait_yours"]->Texture = Player->Portrait;
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
void _HUD::SplitStack(uint8_t Slot, uint8_t Count) {

	// Don't split trade items
	if(_Inventory::IsSlotTrade(Slot))
		return;

	// Build packet
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::INVENTORY_SPLIT);
	Packet.Write<uint8_t>(Slot);
	Packet.Write<uint8_t>(Count);

	ClientState.Network->SendPacket(Packet);
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

	Tooltip.Reset();
	Cursor.Reset();

	if(Player)
		Assets.Labels["label_hud_name"]->Text = Player->Name;
}

// Resize action bar
void _HUD::SetActionBarSize(size_t Size) {

	// Set all off
	for(size_t i = 0; i < ACTIONBAR_MAX_SIZE; i++)
		Assets.Buttons["button_actionbar_" + std::to_string(i)]->SetVisible(false);

	// Turn on
	for(size_t i = 0; i < Size; i++)
		Assets.Buttons["button_actionbar_" + std::to_string(i)]->SetVisible(true);

	// Center actionbar
	_Button *Button = Assets.Buttons["button_actionbar_0"];
	ActionBarElement->Size.x = Button->Size.x * Size;
	ActionBarElement->CalculateBounds();
}

// Remove stat changes owned by an object
void _HUD::RemoveStatChanges(_Object *Owner) {

	for(auto Iterator = StatChanges.begin(); Iterator != StatChanges.end(); ) {
		_Object *StatObject = Iterator->Object;
		if(StatObject == Owner) {
			Iterator = StatChanges.erase(Iterator);
		}
		else
			++Iterator;
	}
}
