/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020 Alan Witkowski
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
#include <hud/hud.h>
#include <hud/character_screen.h>
#include <hud/inventory_screen.h>
#include <hud/vendor_screen.h>
#include <hud/trade_screen.h>
#include <hud/trader_screen.h>
#include <hud/blacksmith_screen.h>
#include <hud/enchanter_screen.h>
#include <hud/skill_screen.h>
#include <objects/object.h>
#include <objects/item.h>
#include <objects/components/character.h>
#include <objects/components/inventory.h>
#include <objects/components/fighter.h>
#include <objects/components/controller.h>
#include <objects/statuseffect.h>
#include <objects/buff.h>
#include <objects/battle.h>
#include <objects/map.h>
#include <objects/minigame.h>
#include <states/play.h>
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

	UpdateButtonBarLabels();
	ae::Assets.Elements["label_hud_pvp"]->Text = "";

	DarkOverlayElement = ae::Assets.Elements["element_dark_overlay"];
	ConfirmElement = ae::Assets.Elements["element_confirm"];
	DiedElement = ae::Assets.Elements["element_died"];
	StatusEffectsElement = ae::Assets.Elements["element_hud_statuseffects"];
	ActionBarElement = ae::Assets.Elements["element_actionbar"];
	ButtonBarElement = ae::Assets.Elements["element_buttonbar"];
	MinigameElement = ae::Assets.Elements["element_minigame"];
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
	RespawnInstructions = ae::Assets.Elements["label_died_respawn"];

	DarkOverlayElement->SetActive(false);
	ConfirmElement->SetActive(false);
	DiedElement->SetActive(false);
	StatusEffectsElement->SetActive(true);
	ActionBarElement->SetActive(true);
	ButtonBarElement->SetActive(true);
	MinigameElement->SetActive(false);
	PartyElement->SetActive(false);
	TeleportElement->SetActive(false);
	ChatElement->SetActive(false);
	HealthElement->SetActive(true);
	ManaElement->SetActive(true);
	ExperienceElement->SetActive(true);
	GoldElement->SetActive(true);
	MessageElement->SetActive(false);
	RecentItemsElement->SetActive(false);

	ae::Assets.Elements["label_hud_pvp"]->SetActive(true);
	ae::Assets.Elements["element_hud"]->SetActive(true);

	CharacterScreen = new _CharacterScreen(this, ae::Assets.Elements["element_character"]);
	InventoryScreen = new _InventoryScreen(this, ae::Assets.Elements["element_inventory_tabs"]);
	VendorScreen = new _VendorScreen(this, ae::Assets.Elements["element_vendor"]);
	TradeScreen = new _TradeScreen(this, ae::Assets.Elements["element_trade"]);
	TraderScreen = new _TraderScreen(this, ae::Assets.Elements["element_trader"]);
	BlacksmithScreen = new _BlacksmithScreen(this, ae::Assets.Elements["element_blacksmith"]);
	EnchanterScreen = new _EnchanterScreen(this, ae::Assets.Elements["element_enchanter"]);
	SkillScreen = new _SkillScreen(this, ae::Assets.Elements["element_skills"]);
}

// Shutdown
_HUD::~_HUD() {
	Reset();

	delete CharacterScreen;
	delete InventoryScreen;
	delete VendorScreen;
	delete TradeScreen;
	delete TraderScreen;
	delete BlacksmithScreen;
	delete EnchanterScreen;
	delete SkillScreen;
}

// Format time
void _HUD::FormatTime(std::stringstream &Buffer, int64_t Time) {
	if(Time < 60)
		Buffer << Time << "s";
	else if(Time < 3600)
		Buffer << Time / 60 << "m";
	else
		Buffer << Time / 3600 << "h" << (Time / 60 % 60) << "m";
}

// Format large number into K, M, etc.
void _HUD::FormatLargeNumber(std::stringstream &Buffer, int Number) {
	if(std::abs(Number) >= 1000000)
		Buffer << ae::Round(Number / 1000000.0f) << "M";
	else if(std::abs(Number) >= 10000)
		Buffer << ae::Round(Number / 1000.0f) << "K";
	else
		Buffer << Number;
}

// Reset state
void _HUD::Reset() {
	delete Minigame;
	Minigame = nullptr;

	CloseWindows(false);
	SkillScreen->ClearSkills();
	EnchanterScreen->ClearSkills();
	RecentItems.clear();
}

// Reset chat history and messages
void _HUD::ResetChat() {
	PlayState.HUD->ChatHistory.clear();
	PlayState.HUD->SentHistory.clear();
	PlayState.HUD->SetMessage("");
}

// Handle the enter key
void _HUD::HandleEnter() {

	if(IsTypingGold()) {
		ae::FocusedElement = nullptr;
		TradeScreen->ValidateTradeGold();
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
				Minigame->HandleMouseButton(MouseEvent);

				// Pay
				if(Minigame->State == _Minigame::StateType::DROPPED) {
					ae::_Buffer Packet;
					Packet.Write<PacketType>(PacketType::MINIGAME_PAY);
					PlayState.Network->SendPacket(Packet);

					Player->Character->GamesPlayed++;
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
						if(ae::Input.ModKeyDown(KMOD_SHIFT) || (Config.RightClickSell && Player->Character->Vendor)) {
							VendorScreen->SellItem(&Tooltip, 1 + (INVENTORY_SPLIT_MODIFIER - 1) * ae::Input.ModKeyDown(KMOD_CTRL));
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
						if(Tooltip.Window == WINDOW_VENDOR)
							VendorScreen->BuyItem(&Tooltip);
					}
				break;
				case WINDOW_ACTIONBAR:
					if(SkillScreen->Element->Active || InventoryScreen->Element->Active) {
						if(MouseEvent.Button == SDL_BUTTON_LEFT) {
							Cursor = Tooltip;
						}
					}
				break;
				case WINDOW_SKILLS:
					if(ae::Graphics.Element->HitElement && ae::Graphics.Element->HitElement->Name == "button_skills_skill") {
						if(MouseEvent.Button == SDL_BUTTON_LEFT) {
							if(Player->Character->Skills[Tooltip.InventorySlot.Item->ID] > 0)
								Cursor = Tooltip;
						}
						else if(MouseEvent.Button == SDL_BUTTON_RIGHT) {
							SkillScreen->EquipSkill(Tooltip.InventorySlot.Item->ID);
						}
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

					if(DeleteSlot == BlacksmithScreen->UpgradeSlot)
						BlacksmithScreen->UpgradeSlot.Reset();

					if(DeleteSlot.Type == BagType::TRADE)
						TradeScreen->ResetAcceptButton();
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
				InventoryScreen->Toggle();
			}
			else if(ButtonBarElement->GetClickedElement()->Name == "button_buttonbar_trade") {
				TradeScreen->Toggle();
			}
			else if(ButtonBarElement->GetClickedElement()->Name == "button_buttonbar_party") {
				ToggleParty(false);
			}
			else if(ButtonBarElement->GetClickedElement()->Name == "button_buttonbar_skills") {
				SkillScreen->Toggle();
			}
			else if(ButtonBarElement->GetClickedElement()->Name == "button_buttonbar_menu") {
				ToggleInGameMenu(true);
			}
			else if(ButtonBarElement->GetClickedElement()->Name == "button_buttonbar_fullscreen") {
				Menu.SetFullscreen(!Config.Fullscreen);
			}
		}
		// Check inventory tabs
		else if(InventoryScreen->Element->GetClickedElement()) {
			InventoryScreen->InitInventoryTab(InventoryScreen->Element->GetClickedElement()->Index);
		}
		// Check skill level up/down
		else if(SkillScreen->Element->GetClickedElement()) {
			if(SkillScreen->Element->GetClickedElement()->Name == "button_skills_plus") {
				SkillScreen->AdjustSkillLevel((uint32_t)SkillScreen->Element->GetClickedElement()->Index, 1 + 4 * ae::Input.ModKeyDown(KMOD_SHIFT));
			}
			else if(SkillScreen->Element->GetClickedElement()->Name == "button_skills_minus") {
				SkillScreen->AdjustSkillLevel((uint32_t)SkillScreen->Element->GetClickedElement()->Index, -(1 + 4 * ae::Input.ModKeyDown(KMOD_SHIFT)));
			}
		}
		// Check enchanter buy
		else if(EnchanterScreen->Element->GetClickedElement()) {
			if(EnchanterScreen->Element->GetClickedElement()->Name == "button_max_skill_levels_buy") {
				uint32_t SkillID = (uint32_t)EnchanterScreen->Element->GetClickedElement()->Index;
				ae::_Buffer Packet;
				Packet.Write<PacketType>(PacketType::ENCHANTER_BUY);
				Packet.Write<uint32_t>(SkillID);
				PlayState.Network->SendPacket(Packet);
				Player->Character->AdjustMaxSkillLevel(SkillID, 1);
			}
		}
		// Accept trader button
		else if(TraderScreen->Element->GetClickedElement() == ae::Assets.Elements["button_trader_accept"]) {
			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::TRADER_ACCEPT);
			PlayState.Network->SendPacket(Packet);
		}
		// Cancel trader button
		else if(TraderScreen->Element->GetClickedElement() == ae::Assets.Elements["button_trader_cancel"]) {
			CloseWindows(true);
		}
		// Upgrade item
		else if(BlacksmithScreen->Element->GetClickedElement() == ae::Assets.Elements["button_blacksmith_upgrade"]) {
			if(Player->Inventory->IsValidSlot(BlacksmithScreen->UpgradeSlot)) {
				ae::_Buffer Packet;
				Packet.Write<PacketType>(PacketType::BLACKSMITH_UPGRADE);
				BlacksmithScreen->UpgradeSlot.Serialize(Packet);
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
								if(Cursor.Slot == BlacksmithScreen->UpgradeSlot || Tooltip.Slot == BlacksmithScreen->UpgradeSlot)
									BlacksmithScreen->UpgradeSlot.Type = BagType::NONE;
							}
						break;
						// Sell an item
						case WINDOW_VENDOR:
							VendorScreen->SellItem(&Cursor, Cursor.InventorySlot.Count);
						break;
						// Upgrade an item
						case WINDOW_BLACKSMITH:

							// Replace item if dragging onto upgrade slot only
							if(Cursor.InventorySlot.Item->IsEquippable() && Tooltip.Slot.Index != 1)
								BlacksmithScreen->UpgradeSlot = Cursor.Slot;
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
							if(!ae::Graphics.Element->HitElement) {
								InitConfirm("Delete this item?");
								DeleteSlot = Cursor.Slot;
							}
						} break;
					}
				break;
				// Buy an item
				case WINDOW_VENDOR:
					if(Tooltip.Window == WINDOW_EQUIPMENT || Tooltip.Window == WINDOW_INVENTORY) {
						BagType BagType = GetBagFromWindow(Tooltip.Window);
						VendorScreen->BuyItem(&Cursor, _Slot(BagType, Tooltip.Slot.Index));
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
					if(TradeScreen->Element->GetClickedElement() == AcceptButton) {
						AcceptButton->Checked = !AcceptButton->Checked;
						TradeScreen->UpdateAcceptButton();

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
			Tooltip.Slot.Type = GetBagFromWindow(Tooltip.Window);
		}

		switch(Tooltip.Window) {
			case WINDOW_EQUIPMENT:
			case WINDOW_INVENTORY:
			case WINDOW_TRADEYOURS: {
				if(Player->Inventory->IsValidSlot(Tooltip.Slot)) {
					Tooltip.InventorySlot = Player->Inventory->GetSlot(Tooltip.Slot);
					if(Tooltip.InventorySlot.Item && Player->Character->Vendor)
						Tooltip.Cost = Tooltip.InventorySlot.Item->GetPrice(Scripting, Player, Player->Character->Vendor, Tooltip.InventorySlot.Count, false, Tooltip.InventorySlot.Upgrades);
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
						Tooltip.Cost = Tooltip.InventorySlot.Item->GetPrice(Scripting, Player, Player->Character->Vendor, Tooltip.InventorySlot.Count, true);
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
				if(Player->Character->Blacksmith && Player->Inventory->IsValidSlot(BlacksmithScreen->UpgradeSlot)) {
					Tooltip.InventorySlot = Player->Inventory->GetSlot(BlacksmithScreen->UpgradeSlot);
					if(Tooltip.InventorySlot.Upgrades < Tooltip.InventorySlot.Item->MaxLevel)
						Tooltip.InventorySlot.Upgrades++;
				}
			} break;
			case WINDOW_SKILLS:
			case WINDOW_ENCHANTER: {
				Tooltip.InventorySlot.Item = PlayState.Stats->Items.at((uint32_t)Tooltip.Slot.Index);
				Tooltip.Cost = _Item::GetEnchantCost(Player->Character->MaxSkillLevels[Tooltip.Slot.Index]);
			} break;
			case WINDOW_ACTIONBAR: {
				if(Tooltip.Slot.Index < Player->Character->ActionBar.size())
					Tooltip.InventorySlot.Item = Player->Character->ActionBar[Tooltip.Slot.Index].Item;
			} break;
			case WINDOW_BATTLE: {
				_Object *MouseObject = (_Object *)HitElement->UserData;
				if(EnableMouseCombat && MouseObject && Player->Character->Battle && Player->Fighter->PotentialAction.IsSet() && Player->Fighter->PotentialAction.Item->UseMouseTargetting() && Player->Fighter->PotentialAction.Item->CanTarget(Scripting, Player, MouseObject)) {
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
		ae::Assets.Elements["element_trade_theirs"]->SetActive(false);
		if(Player->Character->TradePlayer) {
			ae::Assets.Elements["element_trade_theirs"]->SetActive(true);
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

// Refresh various screens
void _HUD::Refresh() {
	SkillScreen->RefreshSkillButtons(!ae::Input.ModKeyDown(KMOD_ALT));
	EnchanterScreen->RefreshBuyButtons();
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
		glm::vec2 Offset = glm::vec2(ae::Graphics.ViewportSize.x - 150 * ae::_Element::GetUIScale(), 31 * ae::_Element::GetUIScale());
		float SpacingY = 22 * ae::_Element::GetUIScale();

		Buffer << ae::Graphics.FramesPerSecond << " FPS";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::ivec2(Offset + glm::vec2(0, SpacingY * 0)));
		Buffer.str("");

		Buffer << std::fixed << std::setprecision(4) << PlayState.Network->GetSentSpeed() / 1024.0f << " KB/s out";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::ivec2(Offset + glm::vec2(0, SpacingY * 1)));
		Buffer.str("");

		Buffer << std::fixed << std::setprecision(4) << PlayState.Network->GetReceiveSpeed() / 1024.0f << " KB/s in";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::ivec2(Offset + glm::vec2(0, SpacingY * 2)));
		Buffer.str("");

		Buffer << PlayState.Network->GetRTT() << "ms";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::ivec2(Offset + glm::vec2(0, SpacingY * 3)));
		Buffer.str("");
	}

	// Draw button bar
	ButtonBarElement->Render();

	// Draw hud elements while alive or in battle
	if(Player->Character->IsAlive() || Player->Character->Battle) {
		DiedElement->SetActive(false);
		ae::Assets.Elements["element_hud"]->Render();

		// Draw action bar
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
		DrawMinigame(BlendFactor);
		InventoryScreen->Render(BlendFactor);
		VendorScreen->Render(BlendFactor);
		TradeScreen->Render(BlendFactor);
		TraderScreen->Render(BlendFactor);
		BlacksmithScreen->Render(BlendFactor);
		EnchanterScreen->Render(BlendFactor);
		SkillScreen->Render(BlendFactor);
		CharacterScreen->Render(BlendFactor);
		ae::Assets.Elements["label_hud_pvp"]->Render();
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
						CompareSlot = BlacksmithScreen->UpgradeSlot;
					break;
					case WINDOW_EQUIPMENT:
						CompareSlot.Type = BagType::NONE;
					break;
					default:
					break;
				}

				// Check for valid slot
				if(Player->Inventory->IsValidSlot(CompareSlot)) {

					// Draw equipped item tooltip
					_Cursor EquippedTooltip;
					EquippedTooltip.InventorySlot = Player->Inventory->GetSlot(CompareSlot);
					if(EquippedTooltip.InventorySlot.Item)
						EquippedTooltip.InventorySlot.Item->DrawTooltip(glm::vec2(0, -1), PlayState.Scripting, Player, EquippedTooltip, _Slot());
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

			// Send message to server
			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::CHAT_MESSAGE);
			Packet.WriteString(ChatTextBox->Text.c_str());
			PlayState.Network->SendPacket(Packet);

			// Add message to history if not a repeat
			if(SentHistory.size() == 0 || (SentHistory.size() > 0 && SentHistory.back() != ChatTextBox->Text))
				SentHistory.push_back(ChatTextBox->Text);
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

// Open/close party screen
void _HUD::ToggleParty(bool IgnoreNextChar) {
	if(Player->Controller->WaitForServer || !Player->Character->CanOpenParty())
		return;

	if(!PartyElement->Active) {
		CloseWindows(true);
		InitParty();
		Framework.IgnoreNextInputEvent = IgnoreNextChar;
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
		Menu.ShowRespawn = !Player->Character->Battle && !Player->Character->IsAlive() && !Player->Character->Hardcore;
		Menu.InitInGame();
	}
}

// Update key/button names for button bar
void _HUD::UpdateButtonBarLabels() {
	ae::Assets.Elements["label_buttonbar_join"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_JOIN).substr(0, HUD_KEYNAME_LENGTH);
	ae::Assets.Elements["label_buttonbar_inventory"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_INVENTORY).substr(0, HUD_KEYNAME_LENGTH);
	ae::Assets.Elements["label_buttonbar_trade"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_TRADE).substr(0, HUD_KEYNAME_LENGTH);
	ae::Assets.Elements["label_buttonbar_skills"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_SKILLS).substr(0, HUD_KEYNAME_LENGTH);
	ae::Assets.Elements["label_buttonbar_party"]->Text = ae::Actions.GetInputNameForAction(Action::GAME_PARTY).substr(0, HUD_KEYNAME_LENGTH);
	ae::Assets.Elements["label_buttonbar_menu"]->Text = ae::Actions.GetInputNameForAction(Action::MENU_BACK).substr(0, HUD_KEYNAME_LENGTH);
}

// Set clickable state of action/button bar
void _HUD::SetBarState(bool State) {
	ButtonBarElement->SetClickable(State, 2);
	ActionBarElement->SetClickable(State, 2);
}

// Initialize the confirm screen
void _HUD::InitConfirm(const std::string &WarningMessage) {
	ae::Assets.Elements["label_confirm_warning"]->Text = WarningMessage;

	ConfirmElement->SetActive(true);
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

	// Disable certain ui elements
	SetBarState(false);

	// Create minigame
	Minigame = new _Minigame(Player->Character->Minigame);
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
	SentHistoryIterator = SentHistory.end();
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

	DarkOverlayElement->SetActive(true);
	DarkOverlayElement->Render();

	ConfirmElement->Render();
}

// Close minigame
bool _HUD::CloseMinigame() {
	bool WasOpen = MinigameElement->Active;

	// Re-enable ui elements
	SetBarState(true);

	// Cleanup
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
	WasOpen |= InventoryScreen->Close();
	WasOpen |= BlacksmithScreen->Close();
	WasOpen |= EnchanterScreen->Close();
	WasOpen |= SkillScreen->Close();
	WasOpen |= VendorScreen->Close();
	WasOpen |= TraderScreen->Close();
	WasOpen |= TradeScreen->Close(SendNotify);
	WasOpen |= CloseConfirm();
	WasOpen |= CloseParty();
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
	ae::_Font *Font = ae::Assets.Fonts["hud_small"];
	int SpacingY = -(Font->MaxAbove + Font->MaxBelow);
	glm::vec2 DrawPosition = glm::vec2(ChatElement->Bounds.Start.x, ChatElement->Bounds.End.y);
	DrawPosition.y += 2 * SpacingY;

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
		Font->DrawText(ChatMessage.Message, DrawPosition, ae::LEFT_BASELINE, Color);
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
			StatusEffect->HUDElement->BaseOffset = Offset;
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

	// Update board size
	ae::_Element *BoardElement = ae::Assets.Elements["element_minigame_board"];
	Minigame->GetUIBoundary(BoardElement->Bounds);
	BoardElement->Size = BoardElement->Bounds.End - BoardElement->Bounds.Start;
	BoardElement->CalculateChildrenBounds(false);

	// Draw element
	MinigameElement->Render();

	// Draw game
	Minigame->Render(BlendFactor);
}

// Draw the skill and belt bar
void _HUD::DrawActionBar() {
	if(!Player || !ActionBarElement->Active)
		return;

	ActionBarElement->Render();

	// Draw skill bar
	for(int i = 0; i < ACTIONBAR_MAX_SKILLS; i++) {

		// Get button position
		std::stringstream Buffer;
		Buffer << "button_actionbar_" << i;
		ae::_Element *Button = ae::Assets.Elements[Buffer.str()];
		glm::vec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2.0f;

		// Draw item icon
		const _Item *Item = Player->Character->ActionBar[i].Item;
		if(Item) {
			ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
			ae::Graphics.DrawScaledImage(DrawPosition, Item->Texture);

			// Draw cooldown
			auto CooldownIterator = Player->Character->Cooldowns.find(Item->ID);
			if(CooldownIterator != Player->Character->Cooldowns.end() && CooldownIterator->second.MaxDuration > 0.0) {
				double CooldownPercent = CooldownIterator->second.Duration / CooldownIterator->second.MaxDuration;

				// Set up graphics
				ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
				ae::Graphics.SetColor(glm::vec4(0, 0, 0, 0.7f));

				// Draw dark percentage bg
				float OverlayHeight = (1.0 - CooldownPercent) * (Button->Bounds.End.y - Button->Bounds.Start.y);
				ae::Graphics.DrawRectangle(Button->Bounds.Start + glm::vec2(0, OverlayHeight), Button->Bounds.End, true);

				// Draw timer
				std::stringstream Buffer;
				Buffer << std::fixed << std::setprecision(1) << ae::Round((float)CooldownIterator->second.Duration);
				ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + glm::vec2(0, 7) * ae::_Element::GetUIScale(), ae::CENTER_BASELINE);
			}

			if(!Item->IsSkill())
				ae::Assets.Fonts["hud_tiny"]->DrawText(std::to_string(Player->Character->ActionBar[i].Count), DrawPosition + glm::vec2(28, 26) * ae::_Element::GetUIScale(), ae::RIGHT_BASELINE);
		}

		// Draw hotkey
		ae::Assets.Fonts["hud_tiny"]->DrawText(ae::Actions.GetInputNameForAction((int)(Action::GAME_SKILL1 + i)), DrawPosition + glm::vec2(-28, -15) * ae::_Element::GetUIScale(), ae::LEFT_BASELINE);
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
	for(auto &RecentItem : RecentItems) {

		// Get alpha
		double TimeLeft = HUD_RECENTITEM_TIMEOUT - RecentItem.Time;
		glm::vec4 Color(glm::vec4(1.0f));
		Color.a = 1.0f;
		if(TimeLeft < HUD_RECENTITEM_FADETIME)
			Color.a = (float)(TimeLeft / HUD_RECENTITEM_FADETIME);

		// Draw item
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawScaledImage(DrawPosition, RecentItem.Item->Texture, Color);

		// Draw count
		ae::Assets.Fonts["hud_small"]->DrawText("+" + std::to_string(RecentItem.Count), DrawPosition + glm::vec2(-35, 7) * ae::_Element::GetUIScale(), ae::RIGHT_BASELINE, Color);

		// Update position
		DrawPosition.y -= (RecentItem.Item->Texture->Size.y + 5) * ae::_Element::GetUIScale();

		// Don't draw off screen
		if(DrawPosition.y < (RecentItem.Item->Texture->Size.y * ae::_Element::GetUIScale()))
			break;
	}

	RecentItemsElement->SetActive(false);
}

// Set hud message
void _HUD::SetMessage(const std::string &Text) {
	if(!Text.length()) {
		MessageElement->SetActive(false);
		return;
	}

	// Set message data
	Message.Time = 0;
	MessageLabel->Text = Text;

	// Set background size
	ae::_TextBounds Bounds;
	MessageLabel->Font->GetStringDimensions(Text, Bounds, true);
	MessageElement->BaseSize = glm::vec2(Bounds.Width, Bounds.AboveBase + Bounds.BelowBase) / ae::_Element::GetUIScale() + glm::vec2(140, 36);
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
		ae::Graphics.DrawScaledImage(DrawPosition, Cursor.InventorySlot.Item->Texture, ae::Assets.Colors["itemfade"]);
	}
}

// Draws an item's price
void _HUD::DrawItemPrice(const _Item *Item, int Count, const glm::vec2 &DrawPosition, bool Buy, int Level) {
	if(!Player->Character->Vendor)
		return;

	// Real price
	int Price = Item->GetPrice(Scripting, Player, Player->Character->Vendor, Count, Buy, Level);

	// Color
	glm::vec4 Color;
	if(Buy && Player->Character->Gold < Price)
		Color = ae::Assets.Colors["red"];
	else
		Color = ae::Assets.Colors["light_gold"];

	std::stringstream Buffer;
	_HUD::FormatLargeNumber(Buffer, Price);
	ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + glm::vec2(28, -15) * ae::_Element::GetUIScale()), ae::RIGHT_BASELINE, Color);
}

// Sets the player's action bar
void _HUD::SetActionBar(size_t Slot, size_t OldSlot, const _Action &Action) {
	if(Player->Character->ActionBar[Slot] == Action)
		return;

	// Check for bringing new skill/item onto bar
	if(OldSlot >= Player->Character->ActionBar.size()) {

		// Check for valid item types
		if(Action.Item && !(Action.Item->IsSkill() || Action.Item->IsConsumable()))
			return;

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

// Split a stack of items
void _HUD::SplitStack(const _Slot &Slot, uint8_t Count) {

	// Don't split trade items
	if(Slot.Type == BagType::TRADE)
		return;

	// Build packet
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::INVENTORY_SPLIT);
	Slot.Serialize(Packet);
	Packet.Write<uint8_t>(Count);

	PlayState.Network->SendPacket(Packet);
}

// Convert window into a player bag
BagType _HUD::GetBagFromWindow(int Window) {

	switch(Window) {
		case WINDOW_EQUIPMENT:
			return BagType::EQUIPMENT;
		break;
		case WINDOW_INVENTORY:
			return BagType::INVENTORY;
		break;
		case WINDOW_TRADEYOURS:
		case WINDOW_TRADETHEIRS:
			return BagType::TRADE;
		break;
	}

	return BagType::NONE;
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
void _HUD::UpdateActionBarSize() {

	// Enable unlocked slots
	for(int i = 0; i < ACTIONBAR_MAX_SKILLS; i++)
		ae::Assets.Elements["button_actionbar_" + std::to_string(i)]->SetEnabled(i < Player->Character->SkillBarSize);
	for(int i = ACTIONBAR_BELT_STARTS; i < ACTIONBAR_MAX_SIZE; i++)
		ae::Assets.Elements["button_actionbar_" + std::to_string(i)]->SetEnabled(i < ACTIONBAR_BELT_STARTS + Player->Character->BeltSize);
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
		StatChangeUI.Change = StatChange.Values[StatType::HEALTH].Integer;
		if(StatChangeUI.Object->Character->Battle) {
			float OffsetX = 55;
			if(StatChangeUI.Change < 0)
				OffsetX = 0;

			StatChangeUI.StartPosition = StatChangeUI.Object->Fighter->StatPosition + glm::vec2(OffsetX, -50) * ae::_Element::GetUIScale();
			StatChangeUI.Battle = true;
		}
		else
			StatChangeUI.StartPosition = HealthElement->Bounds.Start + glm::vec2(HealthElement->Size.x / 2.0f, 0);
		StatChangeUI.Font = ae::Assets.Fonts["hud_medium"];
		StatChangeUI.SetText(ae::Assets.Colors["red"], ae::Assets.Colors["green"]);
		StatChanges.push_back(StatChangeUI);
	}

	if(StatChange.HasStat(StatType::MANA)) {
		_StatChangeUI StatChangeUI;
		StatChangeUI.Object = StatChange.Object;
		StatChangeUI.Change = StatChange.Values[StatType::MANA].Integer;
		if(StatChangeUI.Object->Character->Battle) {
			float OffsetX = 55;
			if(StatChangeUI.Change < 0)
				OffsetX = 0;

			StatChangeUI.StartPosition = StatChangeUI.Object->Fighter->StatPosition + glm::vec2(OffsetX, -14) * ae::_Element::GetUIScale();
			StatChangeUI.Battle = true;
		}
		else
			StatChangeUI.StartPosition = ManaElement->Bounds.Start + glm::vec2(ManaElement->Size.x / 2.0f, 0);
		StatChangeUI.Font = ae::Assets.Fonts["hud_medium"];
		StatChangeUI.SetText(ae::Assets.Colors["blue"], ae::Assets.Colors["light_blue"]);
		StatChanges.push_back(StatChangeUI);
	}

	if(StatChange.HasStat(StatType::EXPERIENCE)) {
		_StatChangeUI StatChangeUI;
		StatChangeUI.Object = StatChange.Object;
		StatChangeUI.StartPosition = ExperienceElement->Bounds.Start + glm::vec2(ExperienceElement->Size.x / 2.0f, -150 * ae::_Element::GetUIScale());
		StatChangeUI.Change = StatChange.Values[StatType::EXPERIENCE].Integer;
		StatChangeUI.Direction = -1.0f;
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
			StatChangeUI.StartPosition = StatChangeUI.Object->Fighter->ResultPosition;
			StatChangeUI.Battle = true;
		}
		else  {
			ae::_TextBounds TextBounds;
			GoldElement->Font->GetStringDimensions(GoldElement->Text, TextBounds);
			StatChangeUI.StartPosition = glm::vec2(GoldElement->Bounds.Start.x + TextBounds.Width / 2, GoldElement->Bounds.Start.y - 35 * ae::_Element::GetUIScale());
		}

		// Get amount
		if(StatChange.HasStat(StatType::GOLD))
			StatChangeUI.Change = StatChange.Values[StatType::GOLD].Integer;
		else
			StatChangeUI.Change = StatChange.Values[StatType::GOLDSTOLEN].Integer;

		StatChangeUI.Direction = -1.5f;
		StatChangeUI.Timeout = HUD_STATCHANGE_TIMEOUT_LONG;
		StatChangeUI.Font = ae::Assets.Fonts["menu_buttons"];
		StatChangeUI.SetText(ae::Assets.Colors["gold"], ae::Assets.Colors["gold"]);
		StatChanges.push_back(StatChangeUI);

		// Play sound
		if(StatChangeUI.Change != 0)
			PlayState.PlayCoinSound();
	}

	// Update screens
	Refresh();
}

// Remove all battle stat changes
void _HUD::ClearBattleStatChanges() {
	for(auto &StatChange : StatChanges) {
		if(StatChange.Battle)
			StatChange.Time = StatChange.Timeout;
	}
}

// Update hud labels
void _HUD::UpdateLabels() {
	std::stringstream Buffer;

	// Update portrait
	ae::Assets.Elements["image_hud_portrait"]->Texture = Player->Character->Portrait;

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
	Buffer << Player->Character->Gold;
	GoldElement->Text = Buffer.str();
	Buffer.str("");
	if(Player->Character->Gold < 0)
		GoldElement->Color = ae::Assets.Colors["red"];
	else
		GoldElement->Color = ae::Assets.Colors["gold"];
}
