/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2018  Alan Witkowski
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
#include <ae/graphics.h>
#include <ae/input.h>
#include <ae/font.h>
#include <ae/buffer.h>
#include <ae/assets.h>
#include <ae/audio.h>
#include <ae/actions.h>
#include <ae/ui.h>
#include <ae/clientnetwork.h>
#include <ae/util.h>
#include <ae/database.h>
#include <objects/object.h>
#include <objects/item.h>
#include <objects/components/character.h>
#include <objects/components/inventory.h>
#include <objects/components/record.h>
#include <objects/components/fighter.h>
#include <objects/components/controller.h>
#include <objects/statuseffect.h>
#include <objects/buff.h>
#include <objects/battle.h>
#include <objects/map.h>
#include <objects/minigame.h>
#include <states/play.h>
#include <framework.h>
#include <scripting.h>
#include <stats.h>
#include <constants.h>
#include <config.h>
#include <actiontype.h>
#include <menu.h>
#include <packet.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <vector>
#include <algorithm>
#include <sstream>
#include <regex>
#include <iomanip>

// Initialize
_HUD::_HUD() {
	EnableMouseCombat = false;
	ShowDebug = false;
	Player = nullptr;
	Minigame = nullptr;
	LowestRecentItemTime = 0.0;
	Tooltip.Reset();
	Cursor.Reset();

	ChatTextBox = ae::Assets.Elements["textbox_chat"];

	ae::Assets.Elements["label_buttonbar_join"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_JOIN).substr(0, HUD_KEYNAME_LENGTH);
	ae::Assets.Elements["label_buttonbar_inventory"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_INVENTORY).substr(0, HUD_KEYNAME_LENGTH);
	ae::Assets.Elements["label_buttonbar_trade"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_TRADE).substr(0, HUD_KEYNAME_LENGTH);
	ae::Assets.Elements["label_buttonbar_skills"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_SKILLS).substr(0, HUD_KEYNAME_LENGTH);
	ae::Assets.Elements["label_buttonbar_party"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_PARTY).substr(0, HUD_KEYNAME_LENGTH);
	ae::Assets.Elements["label_buttonbar_menu"]->Text = ae::Actions.GetInputNameForAction(Action::MENU_BACK).substr(0, HUD_KEYNAME_LENGTH);
	ae::Assets.Elements["label_hud_pvp"]->Text = "";

	DarkOverlayElement = ae::Assets.Elements["element_dark_overlay"];
	ConfirmElement = ae::Assets.Elements["element_menu_confirm"];
	DiedElement = ae::Assets.Elements["element_died"];
	StatusEffectsElement = ae::Assets.Elements["element_hud_statuseffects"];
	ActionBarElement = ae::Assets.Elements["element_actionbar"];
	ButtonBarElement = ae::Assets.Elements["element_buttonbar"];
	EquipmentElement = ae::Assets.Elements["element_equipment"];
	InventoryElement = ae::Assets.Elements["element_inventory"];
	InventoryTabsElement = ae::Assets.Elements["element_inventory_tabs"];
	KeysElement = ae::Assets.Elements["element_keys"];
	CharacterElement = ae::Assets.Elements["element_character"];
	VendorElement = ae::Assets.Elements["element_vendor"];
	TradeElement = ae::Assets.Elements["element_trade"];
	TradeTheirsElement = ae::Assets.Elements["element_trade_theirs"];
	TraderElement = ae::Assets.Elements["element_trader"];
	BlacksmithElement = ae::Assets.Elements["element_blacksmith"];
	MinigameElement = ae::Assets.Elements["element_minigame"];
	SkillsElement = ae::Assets.Elements["element_skills"];
	PartyElement = ae::Assets.Elements["element_party"];
	TeleportElement = ae::Assets.Elements["element_teleport"];
	ChatElement = ae::Assets.Elements["element_chat"];
	HealthElement = ae::Assets.Elements["element_hud_health"];
	ManaElement = ae::Assets.Elements["element_hud_mana"];
	ExperienceElement = ae::Assets.Elements["element_hud_experience"];
	RecentItemsElement = ae::Assets.Elements["element_hud_recentitems"];
	PartyTextBox = ae::Assets.Elements["textbox_party"];
	GoldElement = ae::Assets.Elements["label_hud_gold"];
	MessageElement = ae::Assets.Elements["element_hud_message"];
	MessageLabel = ae::Assets.Elements["label_hud_message"];
	BlacksmithCost = ae::Assets.Elements["label_blacksmith_cost"];
	RespawnInstructions = ae::Assets.Elements["label_died_respawn"];

	GoldElement->Size.x = ButtonBarElement->Size.x;
	GoldElement->CalculateBounds();

	DarkOverlayElement->SetActive(false);
	ConfirmElement->SetActive(false);
	DiedElement->SetActive(false);
	StatusEffectsElement->SetActive(true);
	ActionBarElement->SetActive(true);
	ButtonBarElement->SetActive(true);
	EquipmentElement->SetActive(false);
	InventoryElement->SetActive(false);
	InventoryTabsElement->SetActive(false);
	KeysElement->SetActive(false);
	CharacterElement->SetActive(false);
	VendorElement->SetActive(false);
	TradeElement->SetActive(false);
	TradeTheirsElement->SetActive(false);
	TraderElement->SetActive(false);
	BlacksmithElement->SetActive(false);
	MinigameElement->SetActive(false);
	SkillsElement->SetActive(false);
	PartyElement->SetActive(false);
	TeleportElement->SetActive(false);
	ChatElement->SetActive(false);
	HealthElement->SetActive(true);
	ManaElement->SetActive(true);
	ExperienceElement->SetActive(true);
	GoldElement->SetActive(true);
	MessageElement->SetActive(false);
	BlacksmithCost->SetActive(false);
	RecentItemsElement->SetActive(false);

	ae::Assets.Elements["element_hud"]->SetActive(true);
}

// Shutdown
_HUD::~_HUD() {
	Reset();
}

// Reset state
void _HUD::Reset() {
	delete Minigame;
	Minigame = nullptr;

	CloseWindows(false);
	ClearSkills();

	SetMessage("");
	ChatHistory.clear();
	SentHistory.clear();
	RecentItems.clear();
}

// Handle the enter key
void _HUD::HandleEnter() {

	if(IsTypingGold()) {
		ae::FocusedElement = nullptr;
		ValidateTradeGold();
	}
	else if(IsTypingParty()) {
		ae::FocusedElement = nullptr;
		SendPartyInfo();
	}
	else {
		ToggleChat();
	}
}

// Mouse events
void _HUD::HandleMouseButton(const ae::_MouseEvent &MouseEvent) {
	if(!Player)
		return;

	// Press
	if(MouseEvent.Pressed) {

		// Update minigame
		if(Player->Character->Minigame && Minigame) {
			if(Minigame->State == _Minigame::StateType::CANDROP && Player->Inventory->CountItem(Player->Character->Minigame->RequiredItem) >= Player->Character->Minigame->Cost) {
				if(ae::Input.GetMouse().y > MinigameElement->Bounds.Start.y && ae::Input.GetMouse().y < MinigameElement->Bounds.End.y)
					Minigame->HandleMouseButton(MouseEvent);

				// Pay
				if(Minigame->State == _Minigame::StateType::DROPPED) {
					ae::_Buffer Packet;
					Packet.Write<PacketType>(PacketType::MINIGAME_PAY);
					PlayState.Network->SendPacket(Packet);

					Player->Record->GamesPlayed++;
					PlayState.PlayCoinSound();
				}
			}
		}

		if(Tooltip.InventorySlot.Item) {
			switch(Tooltip.Window) {
				case WINDOW_TRADEYOURS:
				case WINDOW_EQUIPMENT:
				case WINDOW_INVENTORY:

					// Pickup item
					if(MouseEvent.Button == SDL_BUTTON_LEFT) {
						if(ae::Input.ModKeyDown(KMOD_CTRL))
							SplitStack(Tooltip.Slot, 1 + (INVENTORY_SPLIT_MODIFIER - 1) * ae::Input.ModKeyDown(KMOD_SHIFT));
						else
							Cursor = Tooltip;
					}
					// Use an item
					else if(MouseEvent.Button == SDL_BUTTON_RIGHT) {
						if(ae::Input.ModKeyDown(KMOD_SHIFT)) {
							SellItem(&Tooltip, 1 + (INVENTORY_SPLIT_MODIFIER - 1) * ae::Input.ModKeyDown(KMOD_CTRL));
						}
						else {
							ae::_Buffer Packet;
							Packet.Write<PacketType>(PacketType::INVENTORY_USE);
							Tooltip.Slot.Serialize(Packet);
							PlayState.Network->SendPacket(Packet);
						}
					}
				break;
				case WINDOW_VENDOR:
					if(MouseEvent.Button == SDL_BUTTON_LEFT) {
						Cursor = Tooltip;
					}
					else if(MouseEvent.Button == SDL_BUTTON_RIGHT) {
						if(Tooltip.Window == WINDOW_VENDOR) {
							if(ae::Input.ModKeyDown(KMOD_SHIFT))
								BuyItem(&Tooltip);
							else
								BuyItem(&Tooltip);
						}
						else if((Tooltip.Window == WINDOW_EQUIPMENT || Tooltip.Window == WINDOW_INVENTORY) && ae::Input.ModKeyDown(KMOD_SHIFT))
							SellItem(&Tooltip, 1);
					}
				break;
				case WINDOW_ACTIONBAR:
					if(SkillsElement->Active || InventoryTabsElement->Active) {
						if(MouseEvent.Button == SDL_BUTTON_LEFT) {
							Cursor = Tooltip;
						}
					}
				break;
				case WINDOW_SKILLS:
					if(MouseEvent.Button == SDL_BUTTON_LEFT) {
						if(Player->Character->Skills[Tooltip.InventorySlot.Item->ID] > 0)
							Cursor = Tooltip;
					}
					else if(MouseEvent.Button == SDL_BUTTON_RIGHT) {
						EquipSkill(Tooltip.InventorySlot.Item->ID);
					}
				break;
			}
		}
	}
	// Release left mouse button
	else if(MouseEvent.Button == SDL_BUTTON_LEFT) {

		// Check confirm screen
		if(ConfirmElement->GetClickedElement()) {
			if(ConfirmElement->GetClickedElement()->Name == "button_confirm_ok") {

				// Delete item
				if(Player->Inventory->IsValidSlot(DeleteSlot)) {
					ae::_Buffer Packet;
					Packet.Write<PacketType>(PacketType::INVENTORY_DELETE);
					DeleteSlot.Serialize(Packet);
					PlayState.Network->SendPacket(Packet);

					if(DeleteSlot == UpgradeSlot)
						UpgradeSlot.Reset();

					if(DeleteSlot.BagType == _Bag::BagType::TRADE)
						ResetAcceptButton();
				}

				CloseConfirm();
			}
			else if(ConfirmElement->GetClickedElement()->Name == "button_confirm_cancel") {
				CloseConfirm();
			}
		}
		else if(DarkOverlayElement->GetClickedElement()) {
			return;
		}
		// Check button bar
		else if(ButtonBarElement->GetClickedElement()) {
			if(ButtonBarElement->GetClickedElement()->Name == "button_buttonbar_join") {
				PlayState.SendJoinRequest();
			}
			else if(ButtonBarElement->GetClickedElement()->Name == "button_buttonbar_inventory") {
				ToggleInventory();
			}
			else if(ButtonBarElement->GetClickedElement()->Name == "button_buttonbar_trade") {
				ToggleTrade();
			}
			else if(ButtonBarElement->GetClickedElement()->Name == "button_buttonbar_party") {
				ToggleParty();
			}
			else if(ButtonBarElement->GetClickedElement()->Name == "button_buttonbar_skills") {
				ToggleSkills();
			}
			else if(ButtonBarElement->GetClickedElement()->Name == "button_buttonbar_menu") {
				ToggleInGameMenu(true);
			}
			else if(ButtonBarElement->GetClickedElement()->Name == "button_buttonbar_fullscreen") {
				Menu.SetFullscreen(!Config.Fullscreen);
			}
		}
		// Check inventory tabs
		else if(InventoryTabsElement->GetClickedElement()) {
			InitInventoryTab(InventoryTabsElement->GetClickedElement()->Index);
		}
		// Check skill level up/down
		else if(SkillsElement->GetClickedElement()) {
			if(SkillsElement->GetClickedElement()->Name == "button_skills_plus") {
				AdjustSkillLevel((uint32_t)SkillsElement->GetClickedElement()->Parent->Index, 1 + 4 * ae::Input.ModKeyDown(KMOD_SHIFT));
			}
			else if(SkillsElement->GetClickedElement()->Name == "button_skills_minus") {
				AdjustSkillLevel((uint32_t)SkillsElement->GetClickedElement()->Parent->Index, -(1 + 4 * ae::Input.ModKeyDown(KMOD_SHIFT)));
			}
		}
		// Accept trader button
		else if(TraderElement->GetClickedElement() == ae::Assets.Elements["button_trader_accept"]) {
			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::TRADER_ACCEPT);
			PlayState.Network->SendPacket(Packet);
		}
		// Cancel trader button
		else if(TraderElement->GetClickedElement() == ae::Assets.Elements["button_trader_cancel"]) {
			CloseWindows(true);
		}
		// Upgrade item
		else if(BlacksmithElement->GetClickedElement() == ae::Assets.Elements["button_blacksmith_upgrade"]) {
			if(Player->Inventory->IsValidSlot(UpgradeSlot)) {
				ae::_Buffer Packet;
				Packet.Write<PacketType>(PacketType::BLACKSMITH_UPGRADE);
				UpgradeSlot.Serialize(Packet);
				PlayState.Network->SendPacket(Packet);
			}
		}
		// Released an item
		else if(Cursor.InventorySlot.Item) {

			// Check source window
			switch(Cursor.Window) {
				case WINDOW_TRADEYOURS:
				case WINDOW_EQUIPMENT:
				case WINDOW_INVENTORY:

					// Check destination window
					switch(Tooltip.Window) {

						// Send inventory move packet
						case WINDOW_EQUIPMENT:
						case WINDOW_INVENTORY:
						case WINDOW_TRADEYOURS:
							if(Player->Inventory->IsValidSlot(Tooltip.Slot) && Player->Inventory->IsValidSlot(Cursor.Slot) && Cursor.Slot != Tooltip.Slot) {
								ae::_Buffer Packet;
								Packet.Write<PacketType>(PacketType::INVENTORY_MOVE);
								Cursor.Slot.Serialize(Packet);
								Tooltip.Slot.Serialize(Packet);
								PlayState.Network->SendPacket(Packet);

								// Remove upgrade item from upgrade window
								if(Cursor.Slot == UpgradeSlot || Tooltip.Slot == UpgradeSlot)
									UpgradeSlot.BagType = _Bag::BagType::NONE;
							}
						break;
						// Sell an item
						case WINDOW_VENDOR:
							SellItem(&Cursor, Cursor.InventorySlot.Count);
						break;
						// Upgrade an item
						case WINDOW_BLACKSMITH:

							// Replace item if dragging onto upgrade slot only
							if(Cursor.InventorySlot.Item->IsEquippable() && Tooltip.Slot.Index != 1)
								UpgradeSlot = Cursor.Slot;
						break;
						// Move item to actionbar
						case WINDOW_ACTIONBAR:
							if((Cursor.Window == WINDOW_EQUIPMENT || Cursor.Window == WINDOW_INVENTORY) && !Cursor.InventorySlot.Item->IsSkill())
								SetActionBar(Tooltip.Slot.Index, Player->Character->ActionBar.size(), Cursor.InventorySlot.Item);
							else if(Cursor.Window == WINDOW_ACTIONBAR)
								SetActionBar(Tooltip.Slot.Index, Cursor.Slot.Index, Cursor.InventorySlot.Item);
						break;
						// Delete item
						case -1: {
							InitConfirm("Delete this item?");
							DeleteSlot = Cursor.Slot;
						} break;
					}
				break;
				// Buy an item
				case WINDOW_VENDOR:
					if(Tooltip.Window == WINDOW_EQUIPMENT || Tooltip.Window == WINDOW_INVENTORY) {
						_Bag::BagType BagType = GetBagFromWindow(Tooltip.Window);
						BuyItem(&Cursor, _Slot(BagType, Tooltip.Slot.Index));
					}
				break;
				// Drag item from actionbar
				case WINDOW_ACTIONBAR:
					switch(Tooltip.Window) {
						case WINDOW_EQUIPMENT:
						case WINDOW_INVENTORY:

							// Swap actionbar with inventory
							if(Tooltip.InventorySlot.Item && !Tooltip.InventorySlot.Item->IsSkill())
								SetActionBar(Cursor.Slot.Index, Player->Character->ActionBar.size(), Tooltip.InventorySlot.Item);
							else
								SetActionBar(Cursor.Slot.Index, Player->Character->ActionBar.size(), nullptr);
						break;
						case WINDOW_ACTIONBAR:
							SetActionBar(Tooltip.Slot.Index, Cursor.Slot.Index, Cursor.InventorySlot.Item);
						break;
						default:

							// Remove action
							if(Tooltip.Slot.Index >= Player->Character->ActionBar.size() || Tooltip.Window == -1) {
								_Action Action;
								SetActionBar(Cursor.Slot.Index, Player->Character->ActionBar.size(), Action);
							}
						break;
					}
				break;
				case WINDOW_SKILLS:
					if(Tooltip.Window == WINDOW_ACTIONBAR) {
						SetActionBar(Tooltip.Slot.Index, Player->Character->ActionBar.size(), Cursor.InventorySlot.Item);
					}
				break;
			}
		}
		// Use action
		else if(ActionBarElement->GetClickedElement()) {
			uint8_t Slot = (uint8_t)ActionBarElement->GetClickedElement()->Index;
			if(Player->Character->Battle)
				Player->Character->Battle->ClientHandleInput(Action::GAME_SKILL1 + Slot);
			else
				PlayState.SendActionUse(Slot);
		}
		// Handle mouse click during combat
		else if(EnableMouseCombat && Player->Character->Battle && Player->Fighter->PotentialAction.IsSet()) {
			Player->Character->Battle->ClientSetAction((uint8_t)Player->Fighter->PotentialAction.ActionBarSlot);
		}

		if(Player->Character->WaitingForTrade) {
			if(MouseEvent.Button == SDL_BUTTON_LEFT) {
				if(!Cursor.InventorySlot.Item) {

					// Check for accept button
					ae::_Element *AcceptButton = ae::Assets.Elements["button_trade_accept_yours"];
					if(TradeElement->GetClickedElement() == AcceptButton) {
						AcceptButton->Checked = !AcceptButton->Checked;
						UpdateAcceptButton();

						ae::_Buffer Packet;
						Packet.Write<PacketType>(PacketType::TRADE_ACCEPT);
						Packet.Write<char>(AcceptButton->Checked);
						PlayState.Network->SendPacket(Packet);
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

	ae::_Element *HitElement = ae::Graphics.Element->HitElement;
	if(!DarkOverlayElement->Active && HitElement) {
		Tooltip.Slot.Index = (size_t)HitElement->Index;

		// Get window id, stored in parent's userdata field
		if(HitElement->Parent && Tooltip.Slot.Index != NOSLOT) {
			Tooltip.Window = HitElement->Parent->Index;
			Tooltip.Slot.BagType = GetBagFromWindow(Tooltip.Window);
		}

		switch(Tooltip.Window) {
			case WINDOW_EQUIPMENT:
			case WINDOW_INVENTORY:
			case WINDOW_TRADEYOURS: {
				if(Player->Inventory->IsValidSlot(Tooltip.Slot)) {
					Tooltip.InventorySlot = Player->Inventory->GetSlot(Tooltip.Slot);
					if(Tooltip.InventorySlot.Item && Player->Character->Vendor)
						Tooltip.Cost = Tooltip.InventorySlot.Item->GetPrice(Player->Character->Vendor, Tooltip.InventorySlot.Count, false);
				}
			} break;
			case WINDOW_TRADETHEIRS: {
				if(Player->Character->TradePlayer && Player->Character->TradePlayer->Inventory->IsValidSlot(Tooltip.Slot)) {
					Tooltip.InventorySlot = Player->Character->TradePlayer->Inventory->GetSlot(Tooltip.Slot);
				}
			} break;
			case WINDOW_VENDOR: {
				if(Player->Character->Vendor && Tooltip.Slot.Index < Player->Character->Vendor->Items.size()) {
					Tooltip.InventorySlot.Item = Player->Character->Vendor->Items[Tooltip.Slot.Index];
					if(ae::Input.ModKeyDown(KMOD_SHIFT))
						Tooltip.InventorySlot.Count = INVENTORY_INCREMENT_MODIFIER;
					else
						Tooltip.InventorySlot.Count = 1;

					if(Tooltip.InventorySlot.Item)
						Tooltip.Cost = Tooltip.InventorySlot.Item->GetPrice(Player->Character->Vendor, Tooltip.InventorySlot.Count, true);
				}
			} break;
			case WINDOW_TRADER: {
				if(Player->Character->Trader) {
					if(Tooltip.Slot.Index < Player->Character->Trader->Items.size())
						Tooltip.InventorySlot.Item = Player->Character->Trader->Items[Tooltip.Slot.Index].Item;
					else if(Tooltip.Slot.Index == TRADER_MAXITEMS)
						Tooltip.InventorySlot.Item = Player->Character->Trader->RewardItem;
				}
			} break;
			case WINDOW_BLACKSMITH: {
				if(Player->Character->Blacksmith && Player->Inventory->IsValidSlot(UpgradeSlot)) {
					Tooltip.InventorySlot = Player->Inventory->GetSlot(UpgradeSlot);
					if(Tooltip.InventorySlot.Upgrades < Tooltip.InventorySlot.Item->MaxLevel)
						Tooltip.InventorySlot.Upgrades++;
				}
			} break;
			case WINDOW_SKILLS: {
				Tooltip.InventorySlot.Item = PlayState.Stats->Items.at((uint32_t)Tooltip.Slot.Index);
			} break;
			case WINDOW_ACTIONBAR: {
				if(Tooltip.Slot.Index < Player->Character->ActionBar.size())
					Tooltip.InventorySlot.Item = Player->Character->ActionBar[Tooltip.Slot.Index].Item;
			} break;
			case WINDOW_BATTLE: {
				_Object *MouseObject = (_Object *)HitElement->UserData;
				if(EnableMouseCombat && MouseObject && Player->Character->Battle && Player->Fighter->PotentialAction.IsSet() && Player->Fighter->PotentialAction.Item->UseMouseTargetting() && Player->Fighter->PotentialAction.Item->CanTarget(Player, MouseObject)) {
					Player->Character->Battle->ClientSetTarget(Player->Fighter->PotentialAction.Item, MouseObject->Fighter->BattleSide, MouseObject);
				}
			} break;
			case WINDOW_HUD_EFFECTS: {
				Tooltip.StatusEffect = (_StatusEffect *)HitElement->UserData;
			} break;
		}
	}

	// Get trade items
	if(Player->Character->WaitingForTrade) {
		TradeTheirsElement->SetActive(false);
		if(Player->Character->TradePlayer) {
			TradeTheirsElement->SetActive(true);
			ae::Assets.Elements["label_trade_status"]->SetActive(false);

			ae::Assets.Elements["textbox_trade_gold_theirs"]->Text = std::to_string(Player->Character->TradePlayer->Character->TradeGold);
			ae::Assets.Elements["label_trade_name_theirs"]->Text = Player->Character->TradePlayer->Name;
			ae::Assets.Elements["image_trade_portrait_theirs"]->Texture = Player->Character->TradePlayer->Character->Portrait;
		}
	}

	// Close chat if it loses focus
	if(IsChatting()) {
		if(ChatTextBox != ae::FocusedElement)
			CloseChat();
	}

	// Update stat changes
	for(auto Iterator = StatChanges.begin(); Iterator != StatChanges.end(); ) {
		_StatChangeUI &StatChangeUI = *Iterator;

		// Find start position
		StatChangeUI.LastPosition = StatChangeUI.Position;

		// Interpolate between start and end position
		StatChangeUI.Position = glm::mix(StatChangeUI.StartPosition, StatChangeUI.StartPosition + glm::vec2(0, HUD_STATCHANGE_DISTANCE * StatChangeUI.Direction), StatChangeUI.Time / StatChangeUI.Timeout);
		if(StatChangeUI.Time == 0.0)
			StatChangeUI.LastPosition = StatChangeUI.Position;

		// Update timer
		StatChangeUI.Time += FrameTime;
		if(StatChangeUI.Time >= StatChangeUI.Timeout) {
			Iterator = StatChanges.erase(Iterator);
		}
		else
			++Iterator;
	}

	// Update recent items
	LowestRecentItemTime = std::numeric_limits<double>::infinity();
	for(auto Iterator = RecentItems.begin(); Iterator != RecentItems.end(); ) {
		_RecentItem &RecentItem = *Iterator;

		// Update timer
		RecentItem.Time += FrameTime;
		if(RecentItem.Time < LowestRecentItemTime)
			LowestRecentItemTime = RecentItem.Time;

		if(RecentItem.Time >= HUD_RECENTITEM_TIMEOUT) {
			Iterator = RecentItems.erase(Iterator);
		}
		else
			++Iterator;
	}

	// Update minigame
	if(Minigame) {
		Minigame->Update(FrameTime);
		if(Minigame->State == _Minigame::StateType::DONE) {
			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::MINIGAME_GETPRIZE);
			Packet.Write<float>(Minigame->DropX);
			PlayState.Network->SendPacket(Packet);

			Minigame->State =_Minigame::StateType::NEEDSEED;

			if(Minigame->Prizes[Minigame->Bucket])
				ae::Audio.PlaySound(ae::Assets.Sounds["success1.ogg"]);
			else
				ae::Audio.PlaySound(ae::Assets.Sounds["fail0.ogg"]);
		}
	}

	Message.Time += FrameTime;
}

// Draws the HUD elements
void _HUD::Render(_Map *Map, double BlendFactor, double Time) {
	if(!Player)
		return;

	std::stringstream Buffer;

	// Draw chat messages
	DrawChat(Time, IsChatting());

	// Show network stats
	if(ShowDebug) {
		Buffer << ae::Graphics.FramesPerSecond << " FPS";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::vec2(20, 120 + 15 * 0));
		Buffer.str("");

		Buffer << PlayState.Network->GetSentSpeed() / 1024.0f << " KB/s";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::vec2(20, 120 + 15 * 1));
		Buffer.str("");

		Buffer << PlayState.Network->GetReceiveSpeed() / 1024.0f << " KB/s";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::vec2(20, 120 + 15 * 2));
		Buffer.str("");

		Buffer << PlayState.Network->GetRTT() << "ms";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::vec2(20, 120 + 15 * 3));
		Buffer.str("");
	}

	// Draw button bar
	ButtonBarElement->Render();

	// Draw hud elements while alive or in battle
	if(Player->Character->IsAlive() || Player->Character->Battle) {
		DiedElement->SetActive(false);
		ae::Assets.Elements["element_hud"]->Render();
		DrawActionBar();

		// Update label text
		UpdateLabels();

		// Update clock
		Map->GetClockAsString(Buffer);
		ae::Assets.Elements["label_hud_clock"]->Text = Buffer.str();
		Buffer.str("");

		// Update pvp zone
		if(Map->IsPVPZone(Player->Position))
			ae::Assets.Elements["label_hud_pvp"]->Text = "PVP Zone";
		else
			ae::Assets.Elements["label_hud_pvp"]->Text = "";

		// Draw experience bar
		Buffer << Player->Character->ExperienceNextLevel - Player->Character->ExperienceNeeded << " / " << Player->Character->ExperienceNextLevel << " XP";
		ae::Assets.Elements["label_hud_experience"]->Text = Buffer.str();
		Buffer.str("");
		ae::Assets.Elements["image_hud_experience_bar_full"]->SetWidth(ExperienceElement->Size.x * Player->Character->GetNextLevelPercent());
		ae::Assets.Elements["image_hud_experience_bar_empty"]->SetWidth(ExperienceElement->Size.x);
		ExperienceElement->Render();

		// Draw health bar
		Buffer << Player->Character->Health << " / " << Player->Character->MaxHealth;
		ae::Assets.Elements["label_hud_health"]->Text = Buffer.str();
		Buffer.str("");
		ae::Assets.Elements["image_hud_health_bar_full"]->SetWidth(HealthElement->Size.x * Player->Character->GetHealthPercent());
		ae::Assets.Elements["image_hud_health_bar_empty"]->SetWidth(HealthElement->Size.x);
		HealthElement->Render();

		// Draw mana bar
		Buffer << Player->Character->Mana << " / " << Player->Character->MaxMana;
		ae::Assets.Elements["label_hud_mana"]->Text = Buffer.str();
		Buffer.str("");
		ae::Assets.Elements["image_hud_mana_bar_full"]->SetWidth(ManaElement->Size.x * Player->Character->GetManaPercent());
		ae::Assets.Elements["image_hud_mana_bar_empty"]->SetWidth(ManaElement->Size.x);
		ManaElement->Render();

		DrawMessage();
		DrawHudEffects();
		DrawInventory();
		DrawVendor();
		DrawTrade();
		DrawTrader();
		DrawBlacksmith();
		DrawMinigame(BlendFactor);
		DrawCharacterStats();
		DrawSkills();
		DrawParty();
		DrawTeleport();
		DrawConfirm();

		// Draw stat changes
		for(auto &StatChange : StatChanges) {
			StatChange.Render(BlendFactor);
		}

		// Draw item information
		DrawCursorItem();
		const _Item *Item = Tooltip.InventorySlot.Item;
		if(Item) {

			// Compare items
			_Slot CompareSlot;
			Item->GetEquipmentSlot(CompareSlot);
			if(Item->IsEquippable() && (Tooltip.Window == WINDOW_EQUIPMENT || Tooltip.Window == WINDOW_INVENTORY || Tooltip.Window == WINDOW_VENDOR || Tooltip.Window == WINDOW_TRADETHEIRS || Tooltip.Window == WINDOW_BLACKSMITH)) {

				// Get equipment slot to compare
				switch(Tooltip.Window) {
					case WINDOW_BLACKSMITH:
						CompareSlot = UpgradeSlot;
					break;
					case WINDOW_EQUIPMENT:
						CompareSlot.BagType = _Bag::BagType::NONE;
					break;
					default:
					break;
				}

				// Check for valid slot
				if(Player->Inventory->IsValidSlot(CompareSlot)) {
					float OffsetX = -35;
					if(Tooltip.Window == WINDOW_EQUIPMENT || Tooltip.Window == WINDOW_INVENTORY)
						OffsetX += -80;

					_Cursor EquippedTooltip;
					EquippedTooltip.InventorySlot = Player->Inventory->GetSlot(CompareSlot);
					if(EquippedTooltip.InventorySlot.Item)
						EquippedTooltip.InventorySlot.Item->DrawTooltip(glm::vec2(EquipmentElement->Bounds.Start.x + OffsetX, -1), PlayState.Scripting, Player, EquippedTooltip, _Slot());
				}
			}

			// Draw item tooltip
			Item->DrawTooltip(ae::Input.GetMouse(), PlayState.Scripting, Player, Tooltip, CompareSlot);
		}

		// Draw status effects
		if(Tooltip.StatusEffect)
			Tooltip.StatusEffect->Buff->DrawTooltip(Scripting, Tooltip.StatusEffect->Level, Tooltip.StatusEffect->Duration);
	}
	// Dead outside of combat
	else {
		if(!DiedElement->Active)
			CloseWindows(false);

		// Show respawn instructions
		Buffer << "Hit " << ae::Actions.GetInputNameForAction(Action::MENU_BACK);
		if(Player->Character->Hardcore)
			Buffer << " to exit";
		else
			Buffer << " to respawn";

		RespawnInstructions->Text = Buffer.str();
		Buffer.str("");

		DiedElement->Size = ae::Graphics.CurrentSize;
		DiedElement->CalculateBounds();
		DiedElement->SetActive(true);
		DiedElement->Render();
	}
}

// Starts the chat box
void _HUD::ToggleChat() {
	if(IsTypingGold())
		return;

	if(IsChatting()) {
		if(ChatTextBox->Text != "") {

			// Handle test commands
			if(PlayState.DevMode) {
				if (ChatTextBox->Text.find("-search") == 0) {
					std::smatch Match;
					std::regex Regex("-search (.+) (.+)");
					if(std::regex_search(ChatTextBox->Text, Match, Regex) && Match.size() > 2) {
						std::string Table = Match.str(1);
						std::string Search = "%" + Match.str(2) + "%";

						// Search database for keyword
						ae::_Database *Database = PlayState.Stats->Database;
						try {
							Database->PrepareQuery("SELECT id, name FROM " + Table + " WHERE name like @search");
							Database->BindString(1, Search);
							while(Database->FetchRow()) {
								int ID = Database->GetInt<int>("id");
								std::string Name = Database->GetString("name");

								std::cout << std::setw(3) << ID << " " << Name << std::endl;
							}
							Database->CloseQuery();
						} catch(std::exception &Error) {
							std::cout << Error.what() << std::endl;
						}
					}
				}
			}

			// Handle console commands
			if(ChatTextBox->Text.find("-volume") == 0) {
				std::smatch Match;
				std::regex Regex("-volume (.+)");
				if(std::regex_search(ChatTextBox->Text, Match, Regex) && Match.size() > 1) {
					Config.SoundVolume = Config.MusicVolume = ae::ToNumber<float>(Match.str(1));
					ae::Audio.SetSoundVolume(Config.SoundVolume);
					ae::Audio.SetMusicVolume(Config.MusicVolume);
					Config.Save();
				}
			}

			// Send message to server
			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::CHAT_MESSAGE);
			Packet.WriteString(ChatTextBox->Text.c_str());
			PlayState.Network->SendPacket(Packet);

			// Add message to history if not a repeat
			if(SentHistory.size() == 0 || (SentHistory.size() > 0 && SentHistory.back() != ChatTextBox->Text))
				SentHistory.push_back(ChatTextBox->Text);

			SentHistoryIterator = SentHistory.end();
		}

		CloseChat();
	}
	else {
		ChatElement->SetActive(true);
		ChatTextBox->ResetCursor();
		ae::FocusedElement = ChatTextBox;
	}
}

// Toggles the teleport state
void _HUD::ToggleTeleport() {
	return;

	if(!Player->Character->CanTeleport())
		return;

	if(!Player->Controller->WaitForServer && !TeleportElement->Active) {
		CloseWindows(true);
		PlayState.SendStatus(_Character::STATUS_TELEPORT);
		Player->Controller->WaitForServer = true;
	}
	else {
		Player->Controller->WaitForServer = false;
		CloseWindows(true);
	}
}

// Open/close inventory
void _HUD::ToggleInventory() {
	if(Player->Controller->WaitForServer || !Player->Character->CanOpenInventory())
		return;

	if(Minigame) {
		ToggleCharacterStats();
		return;
	}

	if(!InventoryTabsElement->Active) {
		CloseWindows(true);

		InventoryTabsElement->SetActive(true);
		InitInventoryTab(0);
		CharacterElement->SetActive(true);
		PlayState.SendStatus(_Character::STATUS_INVENTORY);
	}
	else {
		CloseWindows(true);
	}
}

// Open/close trade
void _HUD::ToggleTrade() {
	if(Player->Controller->WaitForServer || !Player->Character->CanOpenTrade())
		return;

	// Restrict trading for new characters
	if(Player->Character->Level < GAME_TRADING_LEVEL) {
		SetMessage("Trading unlocks at level " + std::to_string(GAME_TRADING_LEVEL));
		return;
	}

	if(!TradeElement->Active) {
		CloseWindows(true);
		InitTrade();
	}
	else {
		CloseWindows(true);
	}
}

// Open/close skills
void _HUD::ToggleSkills() {
	if(Player->Controller->WaitForServer || !Player->Character->CanOpenInventory())
		return;

	if(!SkillsElement->Active) {
		CloseWindows(true);
		InitSkills();
	}
	else {
		CloseWindows(true);
	}
}

// Open/close party screen
void _HUD::ToggleParty() {
	if(Player->Controller->WaitForServer || !Player->Character->CanOpenParty())
		return;

	if(!PartyElement->Active) {
		CloseWindows(true);
		InitParty();
	}
	else {
		CloseWindows(true);
	}
}

// Open/close menu
void _HUD::ToggleInGameMenu(bool Force) {
	if(Player->Controller->WaitForServer)
		return;

	// Close windows if open
	if(CloseWindows(true))
		return;

	if(PlayState.DevMode && !Force)
		PlayState.Network->Disconnect();
	else {
		Menu.ShowExitWarning = Player->Character->Battle;
		Menu.ShowRespawn = !Player->Character->IsAlive() && !Player->Character->Hardcore;
		Menu.InitInGame();
	}
}

// Show character stats
void _HUD::ToggleCharacterStats() {
	CharacterElement->SetActive(!CharacterElement->Active);
}

// Initialize the confirm screen
void _HUD::InitConfirm(const std::string &WarningMessage) {
	ae::Assets.Elements["label_menu_confirm_warning"]->Text = WarningMessage;

	ConfirmElement->SetActive(true);
}

// Initialize the vendor
void _HUD::InitVendor() {
	Cursor.Reset();

	// Open inventory
	InitInventoryTab(0);
	VendorElement->SetActive(true);
}

// Initialize the trade system
void _HUD::InitTrade() {
	if(Player->Character->WaitingForTrade)
		return;

	Player->Character->WaitingForTrade = true;
	InitInventoryTab(0);
	TradeElement->SetActive(true);

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
	RequiredItemSlots.resize(Player->Character->Trader->Items.size());
	RewardItemSlot = Player->Inventory->GetRequiredItemSlots(Player->Character->Trader, RequiredItemSlots);

	// Disable accept button if requirements not met
	if(!Player->Inventory->IsValidSlot(RewardItemSlot))
		ae::Assets.Elements["button_trader_accept"]->SetEnabled(false);
	else
		ae::Assets.Elements["button_trader_accept"]->SetEnabled(true);

	TraderElement->SetActive(true);
}

// Initialize the blacksmith
void _HUD::InitBlacksmith() {
	Cursor.Reset();

	InitInventoryTab(0);
	BlacksmithElement->SetActive(true);
	BlacksmithCost->SetActive(false);
	ae::Assets.Elements["button_blacksmith_upgrade"]->SetEnabled(false);
	UpgradeSlot.BagType = _Bag::BagType::NONE;
}

// Initialize minigame
void _HUD::InitMinigame() {
	if(!Player->Character->Minigame)
		return;

	Cursor.Reset();

	// Show UI
	MinigameElement->SetActive(true);
	ae::_Element *NameElement = ae::Assets.Elements["label_minigame_name"];
	ae::_Element *CostElement = ae::Assets.Elements["label_minigame_cost"];
	NameElement->Text = Player->Character->Minigame->Name;
	CostElement->Text = std::to_string(Player->Character->Minigame->Cost) + " token";
	if(Player->Character->Minigame->Cost != 1)
		CostElement->Text += "s";
	CostElement->Text += " to play";

	// Create minigame
	Minigame = new _Minigame(Player->Character->Minigame);
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
	glm::vec2 LabelOffset(0, 2);
	size_t i = 0;

	// Get all player skills
	std::list<const _Item *> SortedSkills;
	for(auto &SkillID : Player->Character->Skills) {
		const _Item *Skill = PlayState.Stats->Items.at(SkillID.first);
		if(!Skill)
			continue;

		SortedSkills.push_back(Skill);
	}

	// Sort skills
	SortedSkills.sort(CompareItems);

	// Iterate over skills
	for(auto &Skill : SortedSkills) {

		// Add skill icon
		ae::_Element *Button = new ae::_Element();
		Button->Name = "button_skills_skill";
		Button->Parent = SkillsElement;
		Button->Offset = Offset;
		Button->Size = Skill->Texture->Size;
		Button->Alignment = ae::LEFT_TOP;
		Button->Texture = Skill->Texture;
		Button->Index = (int)Skill->ID;
		SkillsElement->Children.push_back(Button);

		// Add level label
		ae::_Element *LevelLabel = new ae::_Element();
		LevelLabel->Name = "label_skills_level";
		LevelLabel->Parent = Button;
		LevelLabel->Offset = LevelOffset;
		LevelLabel->Alignment = ae::CENTER_BASELINE;
		LevelLabel->Font = ae::Assets.Fonts["hud_small"];
		LevelLabel->Index = (int)Skill->ID;
		SkillsElement->Children.push_back(LevelLabel);

		// Add plus button
		ae::_Element *PlusButton = new ae::_Element();
		PlusButton->Name = "button_skills_plus";
		PlusButton->Parent = Button;
		PlusButton->Size = glm::vec2(16, 16);
		PlusButton->Offset = PlusOffset;
		PlusButton->Alignment = ae::CENTER_MIDDLE;
		PlusButton->Style = ae::Assets.Styles["style_menu_button"];
		PlusButton->HoverStyle = ae::Assets.Styles["style_menu_button_hover"];
		SkillsElement->Children.push_back(PlusButton);

		// Add minus button
		ae::_Element *MinusButton = new ae::_Element();
		MinusButton->Name = "button_skills_minus";
		MinusButton->Parent = Button;
		MinusButton->Size = glm::vec2(16, 16);
		MinusButton->Offset = MinusOffset;
		MinusButton->Alignment = ae::CENTER_MIDDLE;
		MinusButton->Style = ae::Assets.Styles["style_menu_button"];
		MinusButton->HoverStyle = ae::Assets.Styles["style_menu_button_hover"];
		SkillsElement->Children.push_back(MinusButton);

		// Add plus label
		ae::_Element *PlusLabel = new ae::_Element();
		PlusLabel->Parent = PlusButton;
		PlusLabel->Text = "+";
		PlusLabel->Offset = LabelOffset;
		PlusLabel->Alignment = ae::CENTER_MIDDLE;
		PlusLabel->Font = ae::Assets.Fonts["hud_medium"];
		PlusButton->Children.push_back(PlusLabel);

		// Add minus label
		ae::_Element *MinusLabel = new ae::_Element();
		MinusLabel->Parent = MinusButton;
		MinusLabel->Text = "-";
		MinusLabel->Offset = LabelOffset;
		MinusLabel->Alignment = ae::CENTER_MIDDLE;
		MinusLabel->Font = ae::Assets.Fonts["hud_medium"];
		MinusButton->Children.push_back(MinusLabel);

		// Update position
		Offset.x += Skill->Texture->Size.x + Spacing.x;
		if(Offset.x > SkillsElement->Size.x - Skill->Texture->Size.x) {
			Offset.y += Skill->Texture->Size.y + Spacing.y;
			Offset.x = Start.x;
		}

		i++;
	}
	PlayState.Stats->Database->CloseQuery();

	SkillsElement->CalculateBounds();
	SkillsElement->SetActive(true);
	CharacterElement->SetActive(true);

	RefreshSkillButtons();
	Cursor.Reset();

	PlayState.SendStatus(_Character::STATUS_SKILLS);
}

// Initialize the party screen
void _HUD::InitParty() {
	Cursor.Reset();

	PartyElement->SetActive(true);
	PartyTextBox->Text = Player->Character->PartyName;
	PartyTextBox->CursorPosition = PartyTextBox->Text.size();
	PartyTextBox->ResetCursor();
	ae::FocusedElement = PartyTextBox;
}

// Closes the chat window
void _HUD::CloseChat() {
	ChatElement->SetActive(false);
	ChatTextBox->Clear();
	ae::FocusedElement = nullptr;
}

// Close confirmation screen
bool _HUD::CloseConfirm() {
	bool WasOpen = ConfirmElement->Active;
	ConfirmElement->SetActive(false);
	DarkOverlayElement->SetActive(false);

	DeleteSlot.Reset();

	return WasOpen;
}

// Close inventory screen
bool _HUD::CloseInventory() {
	bool WasOpen = InventoryTabsElement->Active;
	Cursor.Reset();

	InventoryTabsElement->SetActive(false);
	EquipmentElement->SetActive(false);
	InventoryElement->SetActive(false);
	KeysElement->SetActive(false);
	CharacterElement->SetActive(false);

	return WasOpen;
}

// Close the vendor
bool _HUD::CloseVendor() {
	bool WasOpen = VendorElement->Active;
	CloseInventory();
	if(Player)
		Player->Character->Vendor = nullptr;

	VendorElement->SetActive(false);
	Cursor.Reset();

	return WasOpen;
}

// Close the skills screen
bool _HUD::CloseSkills() {
	bool WasOpen = SkillsElement->Active;

	SkillsElement->SetActive(false);
	Cursor.Reset();

	return WasOpen;
}

// Close party screen
bool _HUD::CloseParty() {
	bool WasOpen = PartyElement->Active;

	PartyElement->SetActive(false);
	Cursor.Reset();

	return WasOpen;
}

// Cancel teleport
bool _HUD::CloseTeleport() {
	bool WasOpen = TeleportElement->Active;
	TeleportElement->SetActive(false);

	return WasOpen;
}

// Draw confirm action
void _HUD::DrawConfirm() {
	if(!ConfirmElement->Active)
		return;

	DarkOverlayElement->Size = ae::Graphics.CurrentSize;
	DarkOverlayElement->CalculateBounds();
	DarkOverlayElement->SetActive(true);
	DarkOverlayElement->Render();

	ConfirmElement->Render();
}

// Closes the trade system
bool _HUD::CloseTrade(bool SendNotify) {

	bool WasOpen = TradeElement->Active;

	// Close inventory
	CloseInventory();
	TradeElement->SetActive(false);
	ae::FocusedElement = nullptr;

	// Notify server
	if(SendNotify)
		SendTradeCancel();

	if(Player) {
		Player->Character->WaitingForTrade = false;
		Player->Character->TradePlayer = nullptr;
	}

	return WasOpen;
}

// Close the trader
bool _HUD::CloseTrader() {
	bool WasOpen = TraderElement->Active;
	TraderElement->SetActive(false);
	Cursor.Reset();

	if(Player)
		Player->Character->Trader = nullptr;

	return WasOpen;
}

// Close the blacksmith
bool _HUD::CloseBlacksmith() {
	bool WasOpen = BlacksmithElement->Active;
	CloseInventory();

	BlacksmithElement->SetActive(false);
	Cursor.Reset();

	if(Player)
		Player->Character->Blacksmith = nullptr;

	UpgradeSlot.BagType = _Bag::BagType::NONE;

	return WasOpen;
}

// Close minigame
bool _HUD::CloseMinigame() {
	bool WasOpen = MinigameElement->Active;

	MinigameElement->SetActive(false);
	delete Minigame;
	Minigame = nullptr;

	Cursor.Reset();

	if(Player)
		Player->Character->Minigame = nullptr;

	return WasOpen;
}

// Closes all windows
bool _HUD::CloseWindows(bool SendStatus, bool SendNotify) {
	Cursor.Reset();

	bool WasOpen = false;
	WasOpen |= CloseConfirm();
	WasOpen |= CloseInventory();
	WasOpen |= CloseVendor();
	WasOpen |= CloseSkills();
	WasOpen |= CloseParty();
	WasOpen |= CloseTrade(SendNotify);
	WasOpen |= CloseTrader();
	WasOpen |= CloseBlacksmith();
	WasOpen |= CloseMinigame();
	WasOpen |= CloseTeleport();

	if(WasOpen && SendStatus)
		PlayState.SendStatus(_Character::STATUS_NONE);

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
		_Message &ChatMessage = (*Iterator);

		double TimeLeft = ChatMessage.Time - Time + HUD_CHAT_TIMEOUT;
		if(Index >= HUD_CHAT_MESSAGES || (!IgnoreTimeout && TimeLeft <= 0))
			break;

		// Set color
		glm::vec4 Color = ChatMessage.Color;
		if(!IgnoreTimeout && TimeLeft <= HUD_CHAT_FADETIME)
			Color.a = (float)TimeLeft;

		// Draw text
		ae::Assets.Fonts["hud_small"]->DrawText(ChatMessage.Message, DrawPosition, ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;

		Index++;
	}
}

// Draws player's status effects
void _HUD::DrawHudEffects() {
	StatusEffectsElement->Render();

	// Draw status effects
	glm::vec2 Offset(0, 0);
	for(auto &StatusEffect : Player->Character->StatusEffects) {
		if(StatusEffect->HUDElement) {
			StatusEffect->HUDElement->Offset = Offset;
			StatusEffect->HUDElement->CalculateBounds();
			StatusEffect->Render(StatusEffect->HUDElement, glm::vec4(1.0f));
			Offset.x += StatusEffect->Buff->Texture->Size.x + 2;
		}
	}
}

// Draw the teleport sequence
void _HUD::DrawTeleport() {
	if(!TeleportElement->Active)
		return;

	TeleportElement->Render();

	std::stringstream Buffer;
	Buffer << "Teleport in " << std::fixed << std::setprecision(1) << Player->Character->TeleportTime;
	ae::Assets.Elements["label_teleport_timeleft"]->Text = Buffer.str();
}

// Draws the player's inventory
void _HUD::DrawInventory() {
	if(!InventoryTabsElement->Active)
		return;

	InventoryTabsElement->Render();
	if(InventoryElement->Active) {
		EquipmentElement->Render();
		InventoryElement->Render();
		DrawBag(_Bag::EQUIPMENT);
		DrawBag(_Bag::INVENTORY);
	}
	else if(KeysElement->Active) {
		KeysElement->Render();
		DrawKeys(_Bag::KEYS);
	}
}

// Draw an inventory bag
void _HUD::DrawBag(_Bag::BagType Type) {
	_Bag &Bag = Player->Inventory->Bags[Type];
	for(size_t i = 0; i < Bag.Slots.size(); i++) {

		// Get inventory slot
		_InventorySlot *Slot = &Bag.Slots[i];
		if(Slot->Item) {

			// Get bag button
			std::stringstream Buffer;
			Buffer << "button_" << Bag.Name << "_bag_" << i;
			ae::_Element *Button = ae::Assets.Elements[Buffer.str()];
			Buffer.str("");

			// Get position of slot
			glm::vec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2.0f;

			// Draw item
			ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
			ae::Graphics.DrawCenteredImage(DrawPosition, Slot->Item->Texture);

			// Draw two handed weapon twice in equipment bag
			if(Type == _Bag::BagType::EQUIPMENT && i == EquipmentType::HAND1 && Slot->Item->Type == ItemType::TWOHANDED_WEAPON) {
				Buffer << "button_" << Bag.Name << "_bag_" << EquipmentType::HAND2;
				ae::_Element *Button = ae::Assets.Elements[Buffer.str()];
				ae::Graphics.DrawCenteredImage((Button->Bounds.Start + Button->Bounds.End) / 2.0f, Slot->Item->Texture, ae::Assets.Colors["itemfade"]);
			}

			// Draw price if using vendor
			DrawItemPrice(Slot->Item, Slot->Count, DrawPosition, false);

			// Draw upgrade count if using blacksmith
			if(Player->Character->Blacksmith && Slot->Item->MaxLevel) {
				glm::vec4 Color;
				if(Slot->Upgrades >= Slot->Item->MaxLevel || Slot->Upgrades >= Player->Character->Blacksmith->Level)
					Color = ae::Assets.Colors["red"];
				else
					Color = ae::Assets.Colors["green"];

				ae::Assets.Fonts["hud_tiny"]->DrawText(std::to_string(Slot->Upgrades), DrawPosition + glm::vec2(20, -11), ae::RIGHT_BASELINE, Color);
			}

			// Draw count
			if(Slot->Count > 1)
				ae::Assets.Fonts["hud_tiny"]->DrawText(std::to_string(Slot->Count), DrawPosition + glm::vec2(20, 20), ae::RIGHT_BASELINE);
		}
	}
}

// Draw keys
void _HUD::DrawKeys(_Bag::BagType Type) {

	_Bag &Bag = Player->Inventory->Bags[Type];
	glm::vec2 StartOffset(10, 20);
	glm::vec2 Spacing(120, 20);
	int Column = 0;
	int Row = 0;
	for(size_t i = 0; i < Bag.Slots.size(); i++) {

		// Get inventory slot
		_InventorySlot *Slot = &Bag.Slots[i];
		if(Slot->Item) {

			// Get position
			glm::vec2 DrawPosition = KeysElement->Bounds.Start + StartOffset + glm::vec2(Spacing.x * Column, Spacing.y * Row);
			ae::Assets.Fonts["hud_tiny"]->DrawText(Slot->Item->Name, DrawPosition);

			// Update position
			Row++;
			if(Row >= 14) {
				Column++;
				Row = 0;
			}
		}
	}
}

// Draw the vendor
void _HUD::DrawVendor() {
	if(!Player->Character->Vendor) {
		VendorElement->Active = false;
		return;
	}

	VendorElement->Render();

	// Draw vendor items
	for(size_t i = 0; i < Player->Character->Vendor->Items.size(); i++) {
		const _Item *Item = Player->Character->Vendor->Items[i];
		if(Item && !Cursor.IsEqual(i, WINDOW_VENDOR)) {

			// Get bag button
			std::stringstream Buffer;
			Buffer << "button_vendor_bag_" << i;
			ae::_Element *Button = ae::Assets.Elements[Buffer.str()];

			// Get position of slot
			glm::vec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2.0f;

			// Draw item
			ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
			if(Item->Texture)
				ae::Graphics.DrawCenteredImage(DrawPosition, Item->Texture);

			// Draw price
			DrawItemPrice(Item, 1, DrawPosition, true);
		}
	}
}

// Draw the trade screen
void _HUD::DrawTrade() {
	if(!TradeElement->Active)
		return;

	TradeElement->Render();

	// Draw items
	DrawTradeItems(Player, "button_trade_yourbag_", WINDOW_TRADEYOURS);
	DrawTradeItems(Player->Character->TradePlayer, "button_trade_theirbag_", WINDOW_TRADETHEIRS);
}

// Draws trading items
void _HUD::DrawTradeItems(_Object *Player, const std::string &ElementPrefix, int Window) {
	if(!Player)
		return;

	// Draw offered items
	int BagIndex = 0;
	_Bag &Bag = Player->Inventory->Bags[_Bag::TRADE];
	for(size_t i = 0; i < Bag.Slots.size(); i++) {

		// Get inventory slot
		_InventorySlot *Item = &Bag.Slots[i];
		if(Item->Item && !Cursor.IsEqual(i, Window)) {

			// Get bag button
			std::stringstream Buffer;
			Buffer << ElementPrefix << BagIndex;
			ae::_Element *Button = ae::Assets.Elements[Buffer.str()];

			// Get position of slot
			glm::vec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2.0f;

			// Draw item
			ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
			ae::Graphics.DrawCenteredImage(DrawPosition, Item->Item->Texture);

			// Draw count
			if(Item->Count > 1)
				ae::Assets.Fonts["hud_tiny"]->DrawText(std::to_string(Item->Count), DrawPosition + glm::vec2(20, 20), ae::RIGHT_BASELINE);
		}

		BagIndex++;
	}
}

// Draw the trader screen
void _HUD::DrawTrader() {
	if(!Player->Character->Trader) {
		TraderElement->Active = false;
		return;
	}

	TraderElement->Render();

	// Draw trader items
	for(size_t i = 0; i < Player->Character->Trader->Items.size(); i++) {

		// Get button position
		std::stringstream Buffer;
		Buffer << "button_trader_bag_" << i;
		ae::_Element *Button = ae::Assets.Elements[Buffer.str()];
		glm::vec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2.0f;

		// Draw item
		const _Item *Item = Player->Character->Trader->Items[i].Item;
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawCenteredImage(DrawPosition, Item->Texture);

		glm::vec4 Color;
		if(!Player->Inventory->IsValidSlot(RequiredItemSlots[i]))
			Color = ae::Assets.Colors["red"];
		else
			Color = glm::vec4(1.0f);

		ae::Assets.Fonts["hud_small"]->DrawText(std::to_string(Player->Character->Trader->Items[i].Count), DrawPosition + glm::vec2(0, -32), ae::CENTER_BASELINE, Color);
	}

	// Get reward button
	ae::_Element *RewardButton = ae::Assets.Elements["button_trader_bag_reward"];
	glm::vec2 DrawPosition = (RewardButton->Bounds.Start + RewardButton->Bounds.End) / 2.0f;

	// Draw item
	if(Player->Character->Trader->RewardItem) {
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawCenteredImage(DrawPosition, Player->Character->Trader->RewardItem->Texture);
		ae::Assets.Fonts["hud_small"]->DrawText(std::to_string(Player->Character->Trader->Count), DrawPosition + glm::vec2(0, -32), ae::CENTER_BASELINE);
	}
}

// Draw the blacksmith
void _HUD::DrawBlacksmith() {
	if(!Player->Character->Blacksmith) {
		BlacksmithElement->Active = false;
		return;
	}

	// Get UI elements
	ae::_Element *BlacksmithTitle = ae::Assets.Elements["label_blacksmith_title"];
	ae::_Element *BlacksmithLevel = ae::Assets.Elements["label_blacksmith_level"];
	ae::_Element *UpgradeButton = ae::Assets.Elements["button_blacksmith_upgrade"];

	// Set title
	BlacksmithTitle->Text = Player->Character->Blacksmith->Name;
	BlacksmithLevel->Text = "Level " + std::to_string(Player->Character->Blacksmith->Level);

	// Draw element
	BlacksmithElement->Render();

	// Draw item
	if(Player->Inventory->IsValidSlot(UpgradeSlot)) {

		// Get upgrade bag button
		ae::_Element *BagButton = ae::Assets.Elements["button_blacksmith_bag"];
		glm::vec2 DrawPosition = (BagButton->Bounds.Start + BagButton->Bounds.End) / 2.0f;

		const _InventorySlot &InventorySlot = Player->Inventory->GetSlot(UpgradeSlot);
		const _Item *Item = InventorySlot.Item;
		if(Item) {
			ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
			ae::Graphics.DrawCenteredImage(DrawPosition, Item->Texture);

			BlacksmithCost->SetActive(true);
			UpgradeButton->SetEnabled(true);

			// Get cost
			int Cost = Item->GetUpgradePrice(InventorySlot.Upgrades+1);

			// Update cost label
			std::stringstream Buffer;
			Buffer << Cost << " gold";
			BlacksmithCost->Color = ae::Assets.Colors["gold"];
			BlacksmithCost->Text = Buffer.str();

			// Check upgrade conditions
			bool Disabled = false;
			if(Player->Character->Gold < Cost)
				Disabled = true;

			// Check blacksmith level
			if(InventorySlot.Upgrades >= Player->Character->Blacksmith->Level) {
				Disabled = true;
				BlacksmithCost->Text = "I can't upgrade this";
			}

			// Check item level
			if(InventorySlot.Upgrades >= Item->MaxLevel) {
				Disabled = true;
				BlacksmithCost->Text = "Max Level";
			}

			// Disable button
			if(Disabled) {
				BlacksmithCost->Color = ae::Assets.Colors["red"];
				UpgradeButton->SetEnabled(false);
			}
		}
		else
			UpgradeButton->SetEnabled(false);
	}
	else {
		BlacksmithCost->SetActive(false);
		UpgradeButton->SetEnabled(false);
	}
}

// Draw minigame
void _HUD::DrawMinigame(double BlendFactor) {
	if(!Player->Character->Minigame || !Minigame) {
		MinigameElement->Active = false;
		return;
	}

	// Get token count
	int Tokens = Player->Inventory->CountItem(Player->Character->Minigame->RequiredItem);
	ae::Assets.Elements["label_minigame_tokens"]->Text = std::to_string(Tokens);

	// Update text color
	ae::_Element *CostElement = ae::Assets.Elements["label_minigame_cost"];
	if(Tokens < Player->Character->Minigame->Cost)
		CostElement->Color = ae::Assets.Colors["red"];
	else
		CostElement->Color = ae::Assets.Colors["gold"];

	// Draw element
	Minigame->GetUIBoundary(MinigameElement->Bounds);
	MinigameElement->Size = MinigameElement->Bounds.End - MinigameElement->Bounds.Start;
	MinigameElement->CalculateChildrenBounds();
	MinigameElement->Render();

	// Draw game
	Minigame->Render(BlendFactor);
}

// Draw the action bar
void _HUD::DrawActionBar() {
	if(!Player || !ActionBarElement->Active)
		return;

	ActionBarElement->Render();

	// Draw action bar
	for(size_t i = 0; i < Player->Character->ActionBar.size(); i++) {

		// Get button position
		std::stringstream Buffer;
		Buffer << "button_actionbar_" << i;
		ae::_Element *Button = ae::Assets.Elements[Buffer.str()];
		glm::vec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2.0f;

		// Draw item icon
		const _Item *Item = Player->Character->ActionBar[i].Item;
		if(Item) {
			ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
			ae::Graphics.DrawCenteredImage(DrawPosition, Item->Texture);

			if(!Item->IsSkill())
				ae::Assets.Fonts["hud_tiny"]->DrawText(std::to_string(Player->Character->ActionBar[i].Count), DrawPosition + glm::vec2(20, 19), ae::RIGHT_BASELINE);
		}

		// Draw hotkey
		ae::Assets.Fonts["hud_small"]->DrawText(ae::Actions.GetInputNameForAction((int)(Action::GAME_SKILL1 + i)), DrawPosition + glm::vec2(-16, 19), ae::CENTER_BASELINE);
	}
}

// Draw the character stats page
void _HUD::DrawCharacterStats() {
	if(!CharacterElement->Active)
		return;

	CharacterElement->Render();

	// Set up UI
	int SpacingY = 20;
	glm::vec2 Spacing(10, 0);
	glm::vec2 DrawPosition = CharacterElement->Bounds.Start;
	DrawPosition.x += CharacterElement->Size.x/2 + 15;
	DrawPosition.y += 20 + SpacingY;
	std::stringstream Buffer;

	// Damage
	Buffer << Player->Character->MinDamage << " - " << Player->Character->MaxDamage;
	ae::Assets.Fonts["hud_small"]->DrawText("Damage", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Armor
	Buffer << Player->Character->Armor;
	ae::Assets.Fonts["hud_small"]->DrawText("Armor", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Damage Block
	Buffer << Player->Character->DamageBlock;
	ae::Assets.Fonts["hud_small"]->DrawText("Damage Block", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Pierce
	if(Player->Character->Pierce != 0) {
		Buffer << Player->Character->Pierce;
		ae::Assets.Fonts["hud_small"]->DrawText("Pierce", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Health Regen
	if(Player->Character->HealthRegen != 0) {
		Buffer << Player->Character->HealthRegen;
		ae::Assets.Fonts["hud_small"]->DrawText("Health Regen", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Mana Regen
	if(Player->Character->ManaRegen != 0) {
		Buffer << Player->Character->ManaRegen;
		ae::Assets.Fonts["hud_small"]->DrawText("Mana Regen", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Move speed
	Buffer << Player->Character->MoveSpeed << "%";
	ae::Assets.Fonts["hud_small"]->DrawText("Move Speed", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Battle speed
	Buffer << Player->Character->BattleSpeed << "%";
	ae::Assets.Fonts["hud_small"]->DrawText("Battle Speed", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Hit chance
	Buffer << Player->Character->HitChance << "%";
	ae::Assets.Fonts["hud_small"]->DrawText("Hit Chance", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Evasion
	Buffer << Player->Character->Evasion << "%";
	ae::Assets.Fonts["hud_small"]->DrawText("Evasion", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Drop rate
	if(Player->Character->DropRate != 0) {
		Buffer << Player->Character->DropRate;
		ae::Assets.Fonts["hud_small"]->DrawText("Drop Rate", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Separator
	DrawPosition.y += SpacingY;

	// Resistances
	bool HasResist = false;
	for(auto &Resistance : Player->Character->Resistances) {
		if(Resistance.first == 0)
			continue;

		Buffer << Resistance.second << "%";
		ae::Assets.Fonts["hud_small"]->DrawText(Player->Stats->DamageTypes.at(Resistance.first) + " Resist", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;

		HasResist = true;
	}

	// Separator
	if(HasResist)
		DrawPosition.y += SpacingY;

	// Play time
	int64_t PlayTime = (int64_t)Player->Record->PlayTime;
	if(PlayTime < 60)
		Buffer << PlayTime << "s";
	else if(PlayTime < 3600)
		Buffer << PlayTime / 60 << "m";
	else
		Buffer << PlayTime / 3600 << "h" << (PlayTime / 60 % 60) << "m";

	ae::Assets.Fonts["hud_small"]->DrawText("Play Time", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Battle time
	int64_t BattleTime = (int64_t)Player->Record->BattleTime;
	if(BattleTime < 60)
		Buffer << BattleTime << "s";
	else if(BattleTime < 3600)
		Buffer << BattleTime / 60 << "m";
	else
		Buffer << BattleTime / 3600 << "h" << (BattleTime / 60 % 60) << "m";

	ae::Assets.Fonts["hud_small"]->DrawText("Battle Time", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Monster kills
	if(Player->Record->MonsterKills > 0) {
		Buffer << Player->Record->MonsterKills;
		ae::Assets.Fonts["hud_small"]->DrawText("Monster Kills", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Player kills
	if(Player->Record->PlayerKills > 0) {
		Buffer << Player->Record->PlayerKills;
		ae::Assets.Fonts["hud_small"]->DrawText("Player Kills", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Deaths
	if(Player->Record->Deaths > 0) {
		Buffer << Player->Record->Deaths;
		ae::Assets.Fonts["hud_small"]->DrawText("Deaths", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Bounty
	if(Player->Record->Bounty > 0) {
		Buffer << Player->Record->Bounty;
		ae::Assets.Fonts["hud_small"]->DrawText("Bounty", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Gold lost
	if(Player->Record->GoldLost > 0) {
		Buffer << Player->Record->GoldLost;
		ae::Assets.Fonts["hud_small"]->DrawText("Gold Lost", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Games played
	if(Player->Record->GamesPlayed > 0) {
		Buffer << Player->Record->GamesPlayed;
		ae::Assets.Fonts["hud_small"]->DrawText("Games Played", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

}

// Draws the skill page
void _HUD::DrawSkills() {
	if(!SkillsElement->Active)
		return;

	SkillsElement->Render();

	// Show remaining skill points
	std::string Text = std::to_string(Player->Character->GetSkillPointsAvailable()) + " skill point";
	if(Player->Character->GetSkillPointsAvailable() != 1)
		Text += "s";

	glm::vec2 DrawPosition = glm::vec2((SkillsElement->Bounds.End.x + SkillsElement->Bounds.Start.x) / 2, SkillsElement->Bounds.End.y - 30);
	ae::Assets.Fonts["hud_medium"]->DrawText(Text, DrawPosition, ae::CENTER_BASELINE);

	// Show skill points unused
	int SkillPointsUnused = Player->Character->SkillPointsUsed - Player->Character->SkillPointsOnActionBar;
	if(SkillPointsUnused > 0) {
		DrawPosition.y += 21;

		Text = std::to_string(SkillPointsUnused) + " skill point";
		if(SkillPointsUnused != 1)
			Text += "s";

		Text += " unused";

		ae::Assets.Fonts["hud_small"]->DrawText(Text, DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["red"]);
	}
}

// Draw the party screen
void _HUD::DrawParty() {
	if(!PartyElement->Active)
		return;

	PartyElement->Render();
}

// Draw hud message
void _HUD::DrawMessage() {
	if(!MessageElement->Active)
		return;

	// Get time left
	double TimeLeft = HUD_MESSAGE_TIMEOUT - Message.Time;
	if(TimeLeft > 0) {

		// Get alpha
		float Fade = 1.0f;
		if(TimeLeft < HUD_MESSAGE_FADETIME)
			Fade = (float)(TimeLeft / HUD_MESSAGE_FADETIME);

		MessageElement->SetFade(Fade);
		MessageElement->Render();
	}
	else {
		MessageElement->SetActive(false);
	}
}

// Draw recently acquired items
void _HUD::DrawRecentItems() {
	if(!RecentItems.size() || !Player->Character->IsAlive())
		return;

	// Draw label
	double TimeLeft = HUD_RECENTITEM_TIMEOUT - LowestRecentItemTime;
	RecentItemsElement->SetFade(1.0f);
	if(TimeLeft < HUD_RECENTITEM_FADETIME)
		RecentItemsElement->SetFade((float)(TimeLeft / HUD_RECENTITEM_FADETIME));

	RecentItemsElement->SetActive(true);
	RecentItemsElement->Render();

	// Draw items
	glm::vec2 DrawPosition = ae::Assets.Elements["element_hud_recentitems"]->Bounds.Start;
	DrawPosition.y += 25;
	for(auto &RecentItem : RecentItems) {

		// Get alpha
		double TimeLeft = HUD_RECENTITEM_TIMEOUT - RecentItem.Time;
		glm::vec4 Color(glm::vec4(1.0f));
		Color.a = 1.0f;
		if(TimeLeft < HUD_RECENTITEM_FADETIME)
			Color.a = (float)(TimeLeft / HUD_RECENTITEM_FADETIME);

		// Draw item
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawCenteredImage(DrawPosition, RecentItem.Item->Texture, Color);

		// Draw count
		if(RecentItem.Count > 1)
			ae::Assets.Fonts["hud_tiny"]->DrawText(std::to_string(RecentItem.Count), DrawPosition + glm::vec2(20, 20), ae::RIGHT_BASELINE, Color);

		DrawPosition.y += RecentItem.Item->Texture->Size.y + 5;
	}

	RecentItemsElement->SetActive(false);
}

// Set hud message
void _HUD::SetMessage(const std::string &Text) {
	if(!Text.length()) {
		MessageElement->SetActive(false);
		return;
	}

	Message.Time = 0;
	MessageLabel->Text = Text;

	ae::_TextBounds Bounds;
	MessageLabel->Font->GetStringDimensions(Text, Bounds, true);
	MessageElement->Size = glm::vec2(Bounds.Width + 100, Bounds.AboveBase + Bounds.BelowBase + 26);
	MessageElement->SetActive(true);
	MessageElement->CalculateBounds();
}

// Show the teleport window
void _HUD::StartTeleport() {
	TeleportElement->SetActive(true);
}

// Stop teleporting
void _HUD::StopTeleport() {
	TeleportElement->SetActive(false);
}

// Draws the item under the cursor
void _HUD::DrawCursorItem() {
	if(Cursor.InventorySlot.Item) {
		glm::vec2 DrawPosition = ae::Input.GetMouse();
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawCenteredImage(DrawPosition, Cursor.InventorySlot.Item->Texture, ae::Assets.Colors["itemfade"]);
	}
}

// Draws an item's price
void _HUD::DrawItemPrice(const _Item *Item, int Count, const glm::vec2 &DrawPosition, bool Buy) {
	if(!Player->Character->Vendor)
		return;

	// Real price
	int Price = Item->GetPrice(Player->Character->Vendor, Count, Buy);

	// Color
	glm::vec4 Color;
	if(Buy && Player->Character->Gold < Price)
		Color = ae::Assets.Colors["red"];
	else
		Color = ae::Assets.Colors["light_gold"];

	ae::Assets.Fonts["hud_tiny"]->DrawText(std::to_string(Price), DrawPosition + glm::vec2(20, -11), ae::RIGHT_BASELINE, Color);
}

// Buys an item from the vendor
void _HUD::BuyItem(_Cursor *Item, _Slot TargetSlot) {
	_Slot VendorSlot;
	VendorSlot.Index = Item->Slot.Index;

	// Notify server
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::VENDOR_EXCHANGE);
	Packet.WriteBit(1);
	Packet.Write<uint8_t>((uint8_t)Item->InventorySlot.Count);
	VendorSlot.Serialize(Packet);
	TargetSlot.Serialize(Packet);
	PlayState.Network->SendPacket(Packet);
}

// Sells an item
void _HUD::SellItem(_Cursor *CursorItem, int Amount) {
	if(!CursorItem->InventorySlot.Item || !Player->Character->Vendor)
		return;

	// Notify server
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::VENDOR_EXCHANGE);
	Packet.WriteBit(0);
	Packet.Write<uint8_t>((uint8_t)Amount);
	CursorItem->Slot.Serialize(Packet);
	PlayState.Network->SendPacket(Packet);
}

// Adjust skill level
void _HUD::AdjustSkillLevel(uint32_t SkillID, int Amount) {
	if(SkillID == 0)
		return;

	if(Amount < 0 && !Player->CanRespec()) {
		SetMessage("You can only respec on spawn points");
		return;
	}

	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::SKILLS_SKILLADJUST);

	// Sell skill
	Packet.Write<uint32_t>(SkillID);
	Packet.Write<int>(Amount);

	int OldSkillLevel = Player->Character->Skills[SkillID];
	Player->Character->AdjustSkillLevel(SkillID, Amount);

	// Equip new skills
	if(Amount > 0 && OldSkillLevel == 0) {
		EquipSkill(SkillID);
	}

	PlayState.Network->SendPacket(Packet);

	// Update player
	Player->Character->CalculateStats();
	RefreshSkillButtons();
}

// Sets the player's action bar
void _HUD::SetActionBar(size_t Slot, size_t OldSlot, const _Action &Action) {
	if(Player->Character->ActionBar[Slot] == Action)
		return;

	// Check for bringing new skill/item onto bar
	if(OldSlot >= Player->Character->ActionBar.size()) {

		// Remove duplicate skills
		for(size_t i = 0; i < Player->Character->ActionBar.size(); i++) {
			if(Player->Character->ActionBar[i] == Action)
				Player->Character->ActionBar[i].Unset();
		}
	}
	// Rearrange action bar
	else {
		Player->Character->ActionBar[OldSlot] = Player->Character->ActionBar[Slot];
	}

	// Update player
	Player->Character->ActionBar[Slot] = Action;
	Player->Character->CalculateStats();

	// Notify server
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::ACTIONBAR_CHANGED);
	for(size_t i = 0; i < Player->Character->ActionBar.size(); i++) {
		Player->Character->ActionBar[i].Serialize(Packet);
	}

	PlayState.Network->SendPacket(Packet);
}

// Equip a skill
void _HUD::EquipSkill(uint32_t SkillID) {
	const _Item *Skill = PlayState.Stats->Items.at(SkillID);
	if(Skill) {

		// Check skill
		if(!Player->Character->HasLearned(Skill))
			return;

		if(!Player->Character->Skills[SkillID])
			return;

		// Find existing action
		for(size_t i = 0; i < Player->Character->ActionBar.size(); i++) {
			if(Player->Character->ActionBar[i].Item == Skill)
				return;
		}

		// Find an empty slot
		for(size_t i = 0; i < Player->Character->ActionBar.size(); i++) {
			if(!Player->Character->ActionBar[i].IsSet()) {
				SetActionBar(i, Player->Character->ActionBar.size(), Skill);
				return;
			}
		}
	}
}

// Delete memory used by skill page
void _HUD::ClearSkills() {

	// Delete children
	for(auto &Child : SkillsElement->Children)
		delete Child;

	SkillsElement->Children.clear();
}

// Shows or hides the plus/minus buttons
void _HUD::RefreshSkillButtons() {

	// Get remaining points
	int SkillPointsRemaining = Player->Character->GetSkillPointsAvailable();

	// Loop through buttons
	for(auto &Element : SkillsElement->Children) {
		if(Element->Name == "label_skills_level") {
			uint32_t SkillID = (uint32_t)Element->Index;
			Element->Text = std::to_string(Player->Character->Skills[SkillID]);
		}
		else if(Element->Name == "button_skills_plus") {

			// Get skill
			uint32_t SkillID = (uint32_t)Element->Parent->Index;
			if(SkillPointsRemaining <= 0 || Player->Character->Skills[SkillID] >= Player->Stats->Items.at(SkillID)->MaxLevel)
				Element->SetActive(false);
			else
				Element->SetActive(true);
		}
		else if(Element->Name == "button_skills_minus") {

			// Get skill
			uint32_t SkillID = (uint32_t)Element->Parent->Index;
			if(Player->Character->Skills[SkillID] == 0)
				Element->SetActive(false);
			else
				Element->SetActive(true);
		}
	}
}

// Trade with another player
void _HUD::SendTradeRequest() {
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::TRADE_REQUEST);
	PlayState.Network->SendPacket(Packet);
}

// Cancel a trade
void _HUD::SendTradeCancel() {
	if(!Player)
		return;

	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::TRADE_CANCEL);
	PlayState.Network->SendPacket(Packet);

	Player->Character->TradePlayer = nullptr;
}

// Make sure the trade gold box is valid and send gold to player
void _HUD::ValidateTradeGold() {
	if(!Player || !TradeElement->Active)
		return;

	ae::_Element *GoldTextBox = ae::Assets.Elements["textbox_trade_gold_yours"];

	// Get gold amount
	int Gold = ae::ToNumber<int>(GoldTextBox->Text);
	if(Gold < 0)
		Gold = 0;
	else if(Gold > Player->Character->Gold)
		Gold = std::max(0, Player->Character->Gold);

	// Set text
	GoldTextBox->SetText(std::to_string(Gold));

	// Send amount
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::TRADE_GOLD);
	Packet.Write<int>(Gold);
	PlayState.Network->SendPacket(Packet);

	// Reset agreement
	ResetAcceptButton();
}

// Send party password to server and close screen
void _HUD::SendPartyInfo() {
	if(!Player || !PartyElement->Active)
		return;

	// Send info
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::PARTY_INFO);
	Packet.WriteString(PartyTextBox->Text.c_str());
	PlayState.Network->SendPacket(Packet);

	// Close screen
	CloseParty();
}

// Update accept button label text
void _HUD::UpdateAcceptButton() {
	ae::_Element *AcceptButton = ae::Assets.Elements["button_trade_accept_yours"];
	ae::_Element *LabelTradeStatusYours = ae::Assets.Elements["label_trade_status_yours"];
	if(AcceptButton->Checked) {
		LabelTradeStatusYours->Text = "Accepted";
		LabelTradeStatusYours->Color = ae::Assets.Colors["green"];
	}
	else {
		LabelTradeStatusYours->Text = "Accept";
		LabelTradeStatusYours->Color = glm::vec4(1.0f);
	}
}

// Initialize an inventory tab
void _HUD::InitInventoryTab(int Index) {
	for(auto &Child : InventoryTabsElement->Children)
		Child->Checked = Child->Index == Index ? true : false;

	InventoryTabsElement->SetActive(true);
	EquipmentElement->SetActive(false);
	InventoryElement->SetActive(false);
	KeysElement->SetActive(false);
	if(Index == 0) {
		EquipmentElement->SetActive(true);
		InventoryElement->SetActive(true);
	}
	else if(Index == 1) {
		KeysElement->SetActive(true);
	}
}

// Resets the trade agreement
void _HUD::ResetAcceptButton() {
	ae::_Element *AcceptButton = ae::Assets.Elements["button_trade_accept_yours"];
	AcceptButton->Checked = false;
	UpdateAcceptButton();

	UpdateTradeStatus(false);
}

// Resets upper trade window status
void _HUD::ResetTradeTheirsWindow() {
	TradeTheirsElement->SetActive(false);
	ae::Assets.Elements["label_trade_status"]->SetActive(true);
	ae::Assets.Elements["textbox_trade_gold_theirs"]->Enabled = false;
	ae::Assets.Elements["textbox_trade_gold_theirs"]->SetText("0");
	ae::Assets.Elements["textbox_trade_gold_yours"]->SetText("0");
	ae::Assets.Elements["label_trade_name_yours"]->Text = Player->Name;
	ae::Assets.Elements["image_trade_portrait_yours"]->Texture = Player->Character->Portrait;
}

// Update their status label
void _HUD::UpdateTradeStatus(bool Accepted) {
	ae::_Element *LabelTradeStatusTheirs = ae::Assets.Elements["label_trade_status_theirs"];
	if(Accepted) {
		LabelTradeStatusTheirs->Text = "Accepted";
		LabelTradeStatusTheirs->Color = ae::Assets.Colors["green"];
	}
	else {
		LabelTradeStatusTheirs->Text = "Unaccepted";
		LabelTradeStatusTheirs->Color = ae::Assets.Colors["red"];
	}
}

// Split a stack of items
void _HUD::SplitStack(const _Slot &Slot, uint8_t Count) {

	// Don't split trade items
	if(Slot.BagType == _Bag::BagType::TRADE)
		return;

	// Build packet
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::INVENTORY_SPLIT);
	Slot.Serialize(Packet);
	Packet.Write<uint8_t>(Count);

	PlayState.Network->SendPacket(Packet);
}

// Convert window into a player bag
_Bag::BagType _HUD::GetBagFromWindow(int Window) {

	switch(Window) {
		case WINDOW_EQUIPMENT:
			return _Bag::BagType::EQUIPMENT;
		break;
		case WINDOW_INVENTORY:
			return _Bag::BagType::INVENTORY;
		break;
		case WINDOW_TRADEYOURS:
		case WINDOW_TRADETHEIRS:
			return _Bag::BagType::TRADE;
		break;
	}

	return _Bag::BagType::NONE;
}

// Return true if player is typing gold
bool _HUD::IsTypingGold() {
	return ae::FocusedElement == ae::Assets.Elements["textbox_trade_gold_yours"];
}

// Return true if player is typing in the party screen
bool _HUD::IsTypingParty() {
	return PartyElement->Active;
}

// Return true if the chatbox is open
bool _HUD::IsChatting() {
	return ChatTextBox->Active;
}

// Set chat textbox string based on history
void _HUD::UpdateSentHistory(int Direction) {
	if(SentHistory.size() == 0)
		return;

	if(Direction < 0 && SentHistoryIterator != SentHistory.begin()) {
		SentHistoryIterator--;

		ChatTextBox->Text = *SentHistoryIterator;
		ChatTextBox->CursorPosition = ChatTextBox->Text.length();
	}
	else if(Direction > 0 && SentHistoryIterator != SentHistory.end()) {

		SentHistoryIterator++;
		if(SentHistoryIterator == SentHistory.end()) {
			ChatTextBox->Text = "";
			ChatTextBox->CursorPosition = 0;
			return;
		}

		ChatTextBox->Text = *SentHistoryIterator;
		ChatTextBox->CursorPosition = ChatTextBox->Text.length();
	}
}

// Set player for HUD
void _HUD::SetPlayer(_Object *Player) {
	this->Player = Player;

	Tooltip.Reset();
	Cursor.Reset();
}

// Resize action bar
void _HUD::SetActionBarSize(size_t Size) {

	// Set all off
	for(size_t i = 0; i < ACTIONBAR_MAX_SIZE; i++)
		ae::Assets.Elements["button_actionbar_" + std::to_string(i)]->SetActive(false);

	// Turn on
	for(size_t i = 0; i < Size; i++)
		ae::Assets.Elements["button_actionbar_" + std::to_string(i)]->SetActive(true);

	// Center actionbar
	ae::_Element *Button = ae::Assets.Elements["button_actionbar_0"];
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

// Add multiple statchange ui elements
void _HUD::AddStatChange(_StatChange &StatChange) {
	if(StatChange.Values.size() == 0 || !StatChange.Object)
		return;

	if(StatChange.HasStat(StatType::HEALTH)) {
		_StatChangeUI StatChangeUI;
		StatChangeUI.Object = StatChange.Object;
		if(StatChangeUI.Object->Character->Battle) {
			StatChangeUI.StartPosition = StatChangeUI.Object->Fighter->StatPosition;
			StatChangeUI.Battle = true;
		}
		else
			StatChangeUI.StartPosition = HealthElement->Bounds.Start + glm::vec2(HealthElement->Size.x / 2.0f, 0);
		StatChangeUI.Change = StatChange.Values[StatType::HEALTH].Integer;
		StatChangeUI.Font = ae::Assets.Fonts["hud_medium"];
		StatChangeUI.SetText(ae::Assets.Colors["red"], ae::Assets.Colors["green"]);
		StatChanges.push_back(StatChangeUI);
	}

	if(StatChange.HasStat(StatType::MANA)) {
		_StatChangeUI StatChangeUI;
		StatChangeUI.Object = StatChange.Object;
		if(StatChangeUI.Object->Character->Battle) {
			StatChangeUI.StartPosition = StatChangeUI.Object->Fighter->StatPosition + glm::vec2(0, 32);
			StatChangeUI.Battle = true;
		}
		else
			StatChangeUI.StartPosition = ManaElement->Bounds.Start + glm::vec2(ManaElement->Size.x / 2.0f, 0);
		StatChangeUI.Change = StatChange.Values[StatType::MANA].Integer;
		StatChangeUI.Font = ae::Assets.Fonts["hud_medium"];
		StatChangeUI.SetText(ae::Assets.Colors["blue"], ae::Assets.Colors["light_blue"]);
		StatChanges.push_back(StatChangeUI);
	}

	if(StatChange.HasStat(StatType::EXPERIENCE)) {
		_StatChangeUI StatChangeUI;
		StatChangeUI.Object = StatChange.Object;
		StatChangeUI.StartPosition = ExperienceElement->Bounds.Start + ExperienceElement->Size / 2.0f;
		StatChangeUI.Change = StatChange.Values[StatType::EXPERIENCE].Integer;
		StatChangeUI.Direction = -2.0f;
		StatChangeUI.Timeout = HUD_STATCHANGE_TIMEOUT_LONG;
		StatChangeUI.Font = ae::Assets.Fonts["battle_large"];
		StatChangeUI.SetText(glm::vec4(1.0f), glm::vec4(1.0f));
		StatChanges.push_back(StatChangeUI);
	}

	if(StatChange.HasStat(StatType::GOLD) || StatChange.HasStat(StatType::GOLDSTOLEN)) {
		_StatChangeUI StatChangeUI;
		StatChangeUI.Object = StatChange.Object;

		// Check for battle
		if(StatChangeUI.Object->Character->Battle) {
			StatChangeUI.StartPosition = StatChangeUI.Object->Fighter->ResultPosition + glm::vec2(0, 32);
			StatChangeUI.Battle = true;
		}
		else  {
			StatChangeUI.StartPosition = GoldElement->Bounds.Start;
			StatChangeUI.StartPosition.x += -45;
		}

		// Get amount
		if(StatChange.HasStat(StatType::GOLD))
			StatChangeUI.Change = StatChange.Values[StatType::GOLD].Integer;
		else
			StatChangeUI.Change = StatChange.Values[StatType::GOLDSTOLEN].Integer;

		StatChangeUI.Direction = 1.5f;
		StatChangeUI.Timeout = HUD_STATCHANGE_TIMEOUT_LONG;
		StatChangeUI.Font = ae::Assets.Fonts["menu_buttons"];
		StatChangeUI.SetText(ae::Assets.Colors["gold"], ae::Assets.Colors["gold"]);
		StatChanges.push_back(StatChangeUI);

		// Play sound
		if(StatChangeUI.Change != 0)
			PlayState.PlayCoinSound();
	}
}

// Remove all battle stat changes
void _HUD::ClearBattleStatChanges() {

	// Draw stat changes
	for(auto &StatChange : StatChanges) {
		if(StatChange.Battle)
			StatChange.Time = StatChange.Timeout;
	}
}

// Update hud labels
void _HUD::UpdateLabels() {
	std::stringstream Buffer;

	// Update name
	ae::Assets.Elements["label_hud_name"]->Text = Player->Name;

	// Update level
	Buffer << "Level " << Player->Character->Level;
	ae::Assets.Elements["label_hud_level"]->Text = Buffer.str();
	Buffer.str("");

	// Update party
	if(Player->Character->PartyName.size())
		ae::Assets.Elements["label_hud_party"]->Text = "Party: " + Player->Character->PartyName;
	else
		ae::Assets.Elements["label_hud_party"]->Text = "No Party";

	// Update hardcore status
	ae::Assets.Elements["label_hud_hardcore"]->SetActive(Player->Character->Hardcore);

	// Update gold
	Buffer << Player->Character->Gold << " Gold";
	GoldElement->Text = Buffer.str();
	Buffer.str("");
	if(Player->Character->Gold < 0)
		GoldElement->Color = ae::Assets.Colors["red"];
	else
		GoldElement->Color = ae::Assets.Colors["gold"];
}
