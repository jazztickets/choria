/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#pragma once

// Libraries
#include <objects/statchange.h>
#include <objects/inventory.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <vector>
#include <list>
#include <string>
#include <cstdint>

// Forward Declarations
class _Object;
class _Item;
class _StatusEffect;
class _Element;
class _Action;
class _Scripting;
class _Map;
struct _InventorySlot;
struct _Vendor;
struct _Trader;
struct _KeyEvent;
struct _MouseEvent;

// Structures
struct _Cursor {

	_Cursor() { Reset(); }

	void Reset() {
		InventorySlot.Reset();
		Cost = 0;
		StatusEffect = nullptr;
		Slot.BagType = _Bag::BagType::NONE;
		Slot.Index = NOSLOT;
		Window = -1;
	}

	bool IsEqual(size_t Slot, int Window) { return this->Slot.Index == Slot && this->Window == Window; }

	_InventorySlot InventorySlot;
	const _StatusEffect *StatusEffect;
	_Slot Slot;
	int Cost;
	int Window;
};

struct _Message {
	_Message() : Message(""), Color(1.0f), Time(0) { }

	std::string Message;
	glm::vec4 Color;
	double Time;
};

struct _RecentItem {
	_RecentItem() : Item(nullptr), Count(0), Time(0.0) { }

	const _Item *Item;
	int Count;
	double Time;
};

// Classes
class _HUD {

	public:

		enum WindowType {
			WINDOW_BATTLE,
			WINDOW_BUTTONBAR,
			WINDOW_HUD_EFFECTS,
			WINDOW_ACTIONBAR,
			WINDOW_EQUIPMENT,
			WINDOW_INVENTORY,
			WINDOW_VENDOR,
			WINDOW_TRADER,
			WINDOW_BLACKSMITH,
			WINDOW_TRADETHEIRS,
			WINDOW_TRADEYOURS,
			WINDOW_SKILLS,
			WINDOW_MINIGAME,
		};

		_HUD();
		~_HUD();

		void Reset();

		// Updates
		void HandleEnter();
		void HandleMouseButton(const _MouseEvent &MouseEvent);
		void Update(double FrameTime);

		// Render
		void Render(_Map *Map, double BlendFactor, double Time);
		void DrawRecentItems();

		// Events
		void SetMessage(const std::string &Text);
		void StartTeleport();
		void StopTeleport();

		// Objects
		void SetPlayer(_Object *Player);
		void SetActionBarSize(size_t Size);
		void RemoveStatChanges(_Object *Owner);
		void AddStatChange(_StatChange &StatChange);
		void ClearBattleStatChanges();
		void UpdateLabels();

		// Button bar
		void ToggleTeleport();
		void ToggleInventory();
		void ToggleTrade();
		void ToggleSkills();
		void ToggleParty();
		void ToggleInGameMenu(bool Force);
		void ToggleCharacterStats();

		// Windows
		void InitVendor();
		void InitTrader();
		void InitBlacksmith();
		void InitMinigame();
		void InitSkills();
		void InitParty();
		void InitTrade();
		bool CloseWindows(bool SendStatus, bool SendNotify=true);

		// Chat
		void ToggleChat();
		bool IsChatting();
		void AddChatMessage(_Message &Chat) { ChatHistory.push_back(Chat); }
		void UpdateSentHistory(int Direction);
		void CloseChat();

		// Trade
		bool CloseTrade(bool SendNotify=true);
		bool IsTypingGold();
		void ResetAcceptButton();
		void ResetTradeTheirsWindow();
		void UpdateTradeStatus(bool Accepted);
		void ValidateTradeGold();

		// Party
		void SendPartyInfo();
		bool IsTypingParty();

		// Input
		bool EnableMouseCombat;

		// Stats
		bool ShowDebug;

		// Scripting
		_Scripting *Scripting;

		// UI
		std::list<_StatChangeUI> StatChanges;
		std::list<_RecentItem> RecentItems;

	private:

		bool CloseInventory();
		bool CloseVendor();
		bool CloseTrader();
		bool CloseBlacksmith();
		bool CloseMinigame();
		bool CloseSkills();
		bool CloseParty();
		bool CloseTeleport();

		void DrawChat(double Time, bool IgnoreTimeout);
		void DrawHudEffects();
		void DrawInventory();
		void DrawBag(_Bag::BagType Type);
		void DrawTeleport();
		void DrawCharacterStats();
		void DrawVendor();
		void DrawTrade();
		void DrawTrader();
		void DrawBlacksmith();
		void DrawMinigame();
		void DrawActionBar();
		void DrawSkills();
		void DrawParty();
		void DrawMessage();
		void DrawItemPrice(const _Item *Item, int Count, const glm::vec2 &DrawPosition, bool Buy);
		void DrawCursorItem();
		void DrawTradeItems(_Object *Player, const std::string &ElementPrefix, int Window);

		void BuyItem(_Cursor *Item, _Slot TargetSlot=_Slot());
		void SellItem(_Cursor *CursorItem, int Amount);

		void AdjustSkillLevel(uint32_t SkillID, int Amount);
		void SetActionBar(size_t Slot, size_t OldSlot, const _Action &Action);
		void EquipSkill(uint32_t SkillID);
		void ClearSkills();

		void RefreshSkillButtons();
		void SendTradeRequest();
		void SendTradeCancel();
		void UpdateAcceptButton();

		void SplitStack(const _Slot &Slot, uint8_t Count);
		_Bag::BagType GetBagFromWindow(int Window);

		// UI
		_Element *DiedElement;
		_Element *StatusEffectsElement;
		_Element *ActionBarElement;
		_Element *ButtonBarElement;
		_Element *EquipmentElement;
		_Element *InventoryElement;
		_Element *CharacterElement;
		_Element *VendorElement;
		_Element *TradeElement;
		_Element *TradeTheirsElement;
		_Element *TraderElement;
		_Element *BlacksmithElement;
		_Element *MinigameElement;
		_Element *SkillsElement;
		_Element *PartyElement;
		_Element *TeleportElement;
		_Element *ChatElement;
		_Element *HealthElement;
		_Element *ManaElement;
		_Element *ExperienceElement;
		_Element *RecentItemsElement;
		_Element *MessageElement;
		_Element *PartyTextBox;
		_Element *GoldElement;
		_Element *MessageLabel;
		_Element *BlacksmithCost;
		_Element *RespawnInstructions;
		_Cursor Cursor;
		_Cursor Tooltip;

		// HUD
		_Message Message;

		// Objects
		_Object *Player;
		double LowestRecentItemTime;

		// Chat
		std::list<_Message> ChatHistory;
		std::list<std::string> SentHistory;
		std::list<std::string>::iterator SentHistoryIterator;
		_Element *ChatTextBox;

		// Traders
		std::vector<_Slot> RequiredItemSlots;
		_Slot RewardItemSlot;

		// Blacksmith
		_Slot UpgradeSlot;
};
