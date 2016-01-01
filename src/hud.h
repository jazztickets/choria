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
#pragma once

// Libraries
#include <objects/statchange.h>
#include <vector>
#include <list>
#include <string>
#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

// Forward Declarations
class _Object;
class _Item;
class _StatusEffect;
class _Element;
class _Label;
class _TextBox;
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
	void Reset() {
		Item = nullptr;
		StatusEffect = nullptr;
		Cost = 0;
		Count = 0;
		Slot = (size_t)-1;
		Window = -1;
	}

	bool IsEqual(size_t Slot, int Window) { return this->Slot == Slot && this->Window == Window; }

	const _Item *Item;
	const _StatusEffect *StatusEffect;
	int Cost;
	int Count;
	size_t Slot;
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
			WINDOW_BUTTONBAR,
			WINDOW_INVENTORY,
			WINDOW_VENDOR,
			WINDOW_TRADER,
			WINDOW_TRADETHEIRS,
			WINDOW_TRADEYOURS,
			WINDOW_SKILLS,
			WINDOW_ACTIONBAR,
			WINDOW_BATTLE,
			WINDOW_HUD_EFFECTS,
		};

		_HUD();
		~_HUD();

		// Updates
		void HandleEnter();
		void MouseEvent(const _MouseEvent &MouseEvent);
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

		// Button bar
		void ToggleTeleport();
		void ToggleInventory();
		void ToggleTrade();
		void ToggleSkills();
		void ToggleInGameMenu();

		// Windows
		void InitInventory(bool SendStatus);
		void InitVendor();
		void InitTrader();
		void InitSkills();
		void InitTrade();
		bool CloseWindows();

		// Chat
		void ToggleChat();
		bool IsChatting();
		void AddChatMessage(_Message &Chat) { ChatHistory.push_back(Chat); }
		void CloseChat();

		// Trade
		bool CloseTrade(bool SendNotify=true);
		bool IsTypingGold();
		void ResetAcceptButton();
		void ResetTradeTheirsWindow();
		void UpdateTradeStatus(bool Accepted);
		void ValidateTradeGold();

		// Stats
		bool ShowStats;

		// Scripting
		_Scripting *Scripting;

		// UI
		std::list<_StatChangeUI> StatChanges;
		std::list<_RecentItem> RecentItems;

	private:

		bool CloseInventory();
		bool CloseVendor();
		bool CloseTrader();
		bool CloseSkills();
		bool CloseTeleport();

		void DrawChat(double Time, bool IgnoreTimeout);
		void DrawHudEffects();
		void DrawInventory();
		void DrawTeleport();
		void DrawCharacter();
		void DrawVendor();
		void DrawTrade();
		void DrawTrader();
		void DrawActionBar();
		void DrawSkills();
		void DrawMessage();
		void DrawItemPrice(const _Item *Item, int Count, const glm::vec2 &DrawPosition, bool Buy);
		void DrawCursorItem();
		void DrawTradeItems(_Object *Player, const std::string &ElementPrefix, int Window);

		void BuyItem(_Cursor *Item, size_t TargetSlot);
		void SellItem(_Cursor *Item, int Amount);

		void AdjustSkillLevel(uint32_t SkillID, int Amount);
		void SetActionBar(size_t Slot, size_t OldSlot, const _Action &Action);
		void ClearSkills();

		void RefreshSkillButtons();
		void SendTradeRequest();
		void SendTradeCancel();
		void UpdateAcceptButton();

		void SplitStack(uint8_t Slot, uint8_t Count);

		// UI
		_Element *DiedElement;
		_Element *StatusEffectsElement;
		_Element *ActionBarElement;
		_Element *ButtonBarElement;
		_Element *InventoryElement;
		_Element *CharacterElement;
		_Element *VendorElement;
		_Element *TradeElement;
		_Element *TradeTheirsElement;
		_Element *TraderElement;
		_Element *SkillsElement;
		_Element *TeleportElement;
		_Element *ChatElement;
		_Element *HealthElement;
		_Element *ManaElement;
		_Element *ExperienceElement;
		_Element *RecentItemsElement;
		_Element *MessageElement;
		_Label *GoldElement;
		_Label *MessageLabel;
		_Cursor Cursor;
		_Cursor Tooltip;

		// HUD
		_Message Message;

		// Objects
		_Object *Player;
		double LowestRecentItemTime;

		// Chat
		std::list<_Message> ChatHistory;
		_TextBox *ChatTextBox;

		// Skills
		bool ActionBarChanged;

		// Traders
		std::vector<size_t> RequiredItemSlots;
		size_t RewardItemSlot;
};

extern _HUD HUD;
