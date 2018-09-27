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
#pragma once

// Libraries
#include <objects/statchange.h>
#include <objects/components/inventory.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <vector>
#include <list>
#include <string>
#include <cstdint>

// Forward Declarations
class _CharacterScreen;
class _InventoryScreen;
class _VendorScreen;
class _TradeScreen;
class _TraderScreen;
class _BlacksmithScreen;
class _SkillScreen;
class _Object;
class _Item;
class _StatusEffect;
class _Action;
class _Scripting;
class _Map;
class _Minigame;

namespace ae {
	class _Element;
	struct _KeyEvent;
	struct _MouseEvent;
}

// Structures
struct _Cursor {

	_Cursor() { Reset(); }

	void Reset() {
		InventorySlot.Reset();
		Cost = 0;
		StatusEffect = nullptr;
		Slot.Type = BagType::NONE;
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

	friend class _CharacterScreen;
	friend class _InventoryScreen;
	friend class _VendorScreen;
	friend class _TradeScreen;
	friend class _TraderScreen;
	friend class _BlacksmithScreen;
	friend class _SkillScreen;

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
			WINDOW_INVENTORY_TABS,
			WINDOW_KEYS,
		};

		_HUD();
		~_HUD();

		void Reset();

		// Updates
		void HandleEnter();
		void HandleMouseButton(const ae::_MouseEvent &MouseEvent);
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
		void ToggleParty();
		void ToggleInGameMenu(bool Force);
		void UpdateButtonBarLabels();

		// Windows
		void InitConfirm(const std::string &WarningMessage);
		void InitMinigame();
		void InitParty();
		bool CloseWindows(bool SendStatus, bool SendNotify=true);

		// Chat
		void ToggleChat();
		bool IsChatting();
		void AddChatMessage(_Message &Chat) { ChatHistory.push_back(Chat); }
		void UpdateSentHistory(int Direction);
		void CloseChat();

		// Trade
		bool IsTypingGold();

		// Party
		void SendPartyInfo();
		bool IsTypingParty();

		// Input
		bool EnableMouseCombat;

		// Stats
		bool ShowDebug;

		// Scripting
		_Scripting *Scripting;

		// Screens
		_CharacterScreen *CharacterScreen;
		_InventoryScreen *InventoryScreen;
		_VendorScreen *VendorScreen;
		_TradeScreen *TradeScreen;
		_TraderScreen *TraderScreen;
		_BlacksmithScreen *BlacksmithScreen;
		_SkillScreen *SkillScreen;

		// Minigames
		_Minigame *Minigame;

		// UI
		std::list<_StatChangeUI> StatChanges;
		std::list<_RecentItem> RecentItems;

	private:

		bool CloseConfirm();
		bool CloseMinigame();
		bool CloseParty();
		bool CloseTeleport();

		void DrawConfirm();
		void DrawChat(double Time, bool IgnoreTimeout);
		void DrawHudEffects();
		void DrawTeleport();
		void DrawMinigame(double BlendFactor);
		void DrawActionBar();
		void DrawParty();
		void DrawMessage();
		void DrawItemPrice(const _Item *Item, int Count, const glm::vec2 &DrawPosition, bool Buy);
		void DrawCursorItem();

		void SetActionBar(size_t Slot, size_t OldSlot, const _Action &Action);

		void SplitStack(const _Slot &Slot, uint8_t Count);
		BagType GetBagFromWindow(int Window);

		// UI
		ae::_Element *DarkOverlayElement;
		ae::_Element *ConfirmElement;
		ae::_Element *DiedElement;
		ae::_Element *StatusEffectsElement;
		ae::_Element *ActionBarElement;
		ae::_Element *ButtonBarElement;
		ae::_Element *MinigameElement;
		ae::_Element *PartyElement;
		ae::_Element *TeleportElement;
		ae::_Element *ChatElement;
		ae::_Element *HealthElement;
		ae::_Element *ManaElement;
		ae::_Element *ExperienceElement;
		ae::_Element *RecentItemsElement;
		ae::_Element *MessageElement;
		ae::_Element *PartyTextBox;
		ae::_Element *GoldElement;
		ae::_Element *MessageLabel;
		ae::_Element *RespawnInstructions;
		_Cursor Cursor;
		_Cursor Tooltip;
		_Message Message;

		// Objects
		_Object *Player;
		double LowestRecentItemTime;

		// Chat
		std::list<_Message> ChatHistory;
		std::list<std::string> SentHistory;
		std::list<std::string>::iterator SentHistoryIterator;
		ae::_Element *ChatTextBox;

		// Inventory
		_Slot DeleteSlot;

};
