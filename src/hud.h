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
#include <list>
#include <string>
#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

// Forward Declarations
class _Player;
class _Item;
class _Skill;
class _Element;
class _TextBox;
struct _InventorySlot;
struct _Vendor;
struct _Trader;
struct _KeyEvent;
struct _MouseEvent;

// Structures
struct _Cursor {
	void Reset() {
		Item = nullptr;
		Skill = nullptr;
		Cost = 0;
		Count = 0;
		Slot = -1;
		Window = -1;
	}

	bool IsEqual(int Slot, int Window) { return this->Slot == Slot && this->Window == Window; }

	const _Item *Item;
	const _Skill *Skill;
	int Cost;
	int Count;
	int Slot;
	int Window;
};

struct _ChatMessage {
	_ChatMessage() : Message(""), Color(1.0f), Time(0) { }

	std::string Message;
	glm::vec4 Color;
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
			WINDOW_ACTIONBAR
		};

		void Init();
		void Close();

		// Updates
		void HandleEnter();
		void MouseEvent(const _MouseEvent &MouseEvent);
		void Update(double FrameTime);
		void Render();

		// Objects
		void SetPlayer(_Player *Player);

		// Button bar
		void ToggleTeleport();
		void ToggleInventory();
		void ToggleTrade();
		void ToggleSkills();
		void ToggleMenu();

		// Windows
		void InitInventory(bool SendBusy);
		void InitVendor(int VendorID);
		void InitTrader(int TraderID);
		void InitSkills();
		void InitTrade();
		void CloseWindows();

		// Chat
		void ToggleChat();
		bool IsChatting();
		void AddChatMessage(_ChatMessage &Chat) { ChatHistory.push_back(Chat); }
		void CloseChat();

		// Trade
		void CloseTrade(bool SendNotify=true);
		bool IsTypingGold();
		void ResetAcceptButton();
		void UpdateTradeStatus(bool Accepted);
		void ValidateTradeGold();

	private:

		void CloseInventory();
		void CloseVendor();
		void CloseTrader();
		void CloseSkills();

		void DrawChat(bool IgnoreTimeout);
		void DrawInventory();
		void DrawTeleport();
		void DrawCharacter();
		void DrawVendor();
		void DrawTrade();
		void DrawTrader();
		void DrawActionBar();
		void DrawSkills();
		void DrawItemPrice(const _Item *Item, int Count, const glm::ivec2 &DrawPosition, bool Buy);
		void DrawCursorItem();
		void DrawCursorSkill();
		void DrawTradeItems(_Player *Player, const std::string &ElementPrefix, int Window);

		void BuyItem(_Cursor *Item, int TargetSlot);
		void SellItem(_Cursor *Item, int Amount);

		void AdjustSkillLevel(int SkillID, int Direction);
		void SetSkillBar(int Slot, int OldSlot, const _Skill *Skill);
		void ClearSkills();

		void RefreshSkillButtons();
		void SendTradeRequest();
		void SendTradeCancel();
		void UpdateAcceptButton();

		void SplitStack(int Slot, int Count);

		// Objects
		_Player *Player;

		// UI
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
		_Cursor Cursor;
		_Cursor Tooltip;

		// Chat
		std::list<_ChatMessage> ChatHistory;
		_TextBox *ChatTextBox;

		// Skills
		bool SkillBarChanged;

		// Traders
		int RequiredItemSlots[8], RewardItemSlot;
};

extern _HUD HUD;
