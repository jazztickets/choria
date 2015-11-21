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

// Forward Declarations
class _Player;
class _Item;
class _Skill;
struct _InventorySlot;
struct _Vendor;
struct _Trader;
struct _MouseEvent;

// Structures
struct _CursorItem {
	void Set(const _Item *Item, int Cost, int Count, int Slot) {
		this->Item = Item;
		this->Cost = Cost;
		this->Count = Count;
		this->Slot = Slot;
	}
	void Reset() {
		this->Item = nullptr;
		this->Count = 0;
		this->Slot = -1;
		this->Window = -1;
	}

	bool IsEqual(int Slot, int Window) { return this->Slot == Slot && this->Window == Window; }

	const _Item *Item;
	int Cost;
	int Count;
	int Slot;
	int Window;
};

struct _CursorSkill {
	void Reset() {
		Skill = nullptr;
		Slot = -1;
		Window = -1;
	}

	const _Skill *Skill;
	int Slot;
	int Window;
};

struct _ChatMessage {
	_ChatMessage() : Message(""), TimeOut(0) { }
	_ChatMessage(const std::string &TMessage) : Message(TMessage), TimeOut(0) { }
	std::string Message;
	double TimeOut;
};

// Classes
class _HUD {

	public:

		enum WindowType {
			WINDOW_INVENTORY,
			WINDOW_VENDOR,
			WINDOW_TRADER,
			WINDOW_TRADETHEM,
			WINDOW_TRADEYOU,
			WINDOW_SKILLS,
			WINDOW_SKILLBAR
		};

		void Init();
		void Close();

		// Updates
		void MouseEvent(const _MouseEvent &MouseEvent);
		void Update(double FrameTime);
		void Render();

		// Objects
		void SetPlayer(_Player *Player);

		// Windows
		void InitMenu();
		void InitInventory(bool SendBusy);
		void InitVendor(int VendorID);
		void InitTrader(int TraderID);
		void InitCharacter();
		void InitSkills();
		void InitTrade();
		void CloseWindows();

		// Chat
		void ToggleChat();
		bool IsChatting() { return Chatting; }
		void AddChatMessage(_ChatMessage &Chat) { ChatHistory.push_back(Chat); }

		// Trade
		void CloseTrade(bool SendNotify=true);
		bool IsTypingGold() { return TypingGold; }
		void ResetAcceptButton();

		// Teleport to town
		void ToggleTeleport();

	private:

		void CloseMenu();
		void CloseInventory();
		void CloseVendor();
		void CloseTrader();
		void CloseCharacter();
		void CloseSkills();
		void CloseChat();

		void DrawChat();
		void DrawInventory();
		void DrawTeleport();
		void DrawCharacter();
		void DrawVendor();
		void DrawTrader();
		void DrawTrade();
		void DrawActionBar();
		void DrawSkills();
		void DrawItemPrice(const _Item *Item, int Count, const glm::ivec2 &DrawPosition, bool Buy);
		void DrawCursorItem();
		void DrawItemTooltip();
		void DrawCursorSkill();
		void DrawSkillTooltip();
		void DrawSkillDescription(const _Skill *Skill, int SkillLevel, glm::ivec2 &DrawPosition, int Width);
		void DrawTradeItems(_Player *Player, bool DrawAll);

		void BuyItem(_CursorItem *Item, int TargetSlot);
		void SellItem(_CursorItem *Item, int Amount);

		void AdjustSkillLevel(int SkillID, int Direction);
		void SetSkillBar(int Slot, int OldSlot, const _Skill *Skill);
		void ClearSkills();

		void RefreshSkillButtons();
		void SendBusySignal(bool Value);
		void SendTradeRequest();
		void SendTradeCancel();
		int ValidateTradeGold();

		void SplitStack(int Slot, int Count);

		int *State;

		// Objects
		_Player *Player;
		const _Vendor *Vendor;

		// GUI
		_CursorItem CursorItem;
		_CursorItem TooltipItem;
		bool CharacterOpen;

		// Chat
		std::list<_ChatMessage> ChatHistory;
		bool Chatting;

		// Skills
		_CursorSkill CursorSkill;
		_CursorSkill TooltipSkill;
		bool SkillBarChanged;

		// Traders
		int RequiredItemSlots[8], RewardItemSlot;

		// Trading
		bool TypingGold;
};

extern _HUD HUD;
