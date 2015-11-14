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
#include <IEventReceiver.h>
#include <IGUIButton.h>
#include <IGUITabControl.h>
#include <IGUIEditBox.h>
#include <IGUIImage.h>
#include <IGUIButton.h>
#include <list>
#include <string>
#include <cstdint>

// Forward Declarations
class _Player;
class _Item;
class _Skill;
struct _InventorySlot;
struct _Vendor;
struct _Trader;

// Structures
struct _CursorItem {
	void Set(const _Item *TItem, int TCost, int TCount, int TSlot) {
		Item = TItem;
		Cost = TCost;
		Count = TCount;
		Slot = TSlot;
	}
	void Reset() {
		Item = nullptr;
		Count = 0;
		Slot = -1;
		Window = -1;
	}

	bool IsEqual(int TSlot, int TWindow) { return Slot == TSlot && Window == TWindow; }

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

		enum ElementType {
			ELEMENT_TOWNPORTAL=50,
			ELEMENT_INVENTORY,
			ELEMENT_TRADE,
			ELEMENT_CHARACTER,
			ELEMENT_SKILLS,
			ELEMENT_MAINMENU,
			ELEMENT_MAINMENURESUME,
			ELEMENT_MAINMENUEXIT,
			ELEMENT_TRADERACCEPT,
			ELEMENT_TRADERCANCEL,
			ELEMENT_CHATBOX,
			ELEMENT_GOLDTRADEBOX,
			ELEMENT_TRADEACCEPT,
			ELEMENT_SKILLPLUS0=100,
			ELEMENT_SKILLMINUS0=200,
		};

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
		void HandleMouseMotion(int TMouseX, int TMouseY);
		bool HandleMousePress(int TButton, int TMouseX, int TMouseY);
		void HandleMouseRelease(int TButton, int TMouseX, int TMouseY);
		void HandleGUI(irr::gui::EGUI_EVENT_TYPE TEventType, irr::gui::IGUIElement *TElement);

		void Update(double FrameTime);
		void PreGUIDraw();
		void Draw();

		// Objects
		void SetPlayer(_Player *TPlayer) { Player = TPlayer; }

		// Windows
		void InitButtonBar();
		void InitMenu();
		void InitInventory(int TX=400, int TY=300, bool TSendBusy=true);
		void InitVendor(int TVendorID);
		void InitTrader(int TTraderID);
		void InitCharacter();
		void InitSkills();
		void InitTrade();
		void CloseWindows();

		// Chat
		void ToggleChat();
		bool IsChatting() { return Chatting; }
		void HandleKeyPress(irr::EKEY_CODE TKey);
		void AddChatMessage(_ChatMessage &TChat) { ChatHistory.push_back(TChat); }

		// Trade
		void CloseTrade(bool TSendNotify=true);
		bool IsTypingGold() { return TypingGold; }
		void ResetAcceptButton();

		// Town portal
		void ToggleTownPortal();

	private:

		void CloseMenu();
		void CloseInventory();
		void CloseVendor();
		void CloseTrader();
		void CloseCharacter();
		void CloseSkills();
		void CloseChat();

		void DrawChat();
		void DrawTopHUD();
		void DrawInventory();
		void DrawTownPortal();
		void DrawCharacter();
		void DrawVendor();
		void DrawTrader();
		void DrawTrade();
		void DrawSkills();
		void DrawItemPrice(const _Item *TItem, int TCount, int TDrawX, int TDrawY, bool TBuy);
		void DrawCursorItem();
		void DrawItemTooltip();
		void DrawCursorSkill();
		void DrawSkillTooltip();
		void DrawSkillDescription(const _Skill *Skill, int TLevel, int TDrawX, int &TDrawY);
		void DrawTradeItems(_Player *TPlayer, int TDrawX, int TDrawY, bool TDrawAll);

		void GetItem(irr::core::position2di &TPoint, _CursorItem &TCursorItem);
		void GetInventoryItem(irr::core::position2di &TPoint, _CursorItem &TCursorItem);
		void GetVendorItem(irr::core::position2di &TPoint, _CursorItem &TCursorItem);
		void GetTraderItem(irr::core::position2di &TPoint, _CursorItem &TCursorItem);
		void GetTradeItem(irr::core::position2di &TPoint, _CursorItem &TCursorItem);
		void BuyItem(_CursorItem *TCursorItem, int TTargetSlot);
		void SellItem(_CursorItem *TCursorItem, int TAmount);

		void GetSkill(irr::core::position2di &TPoint, _CursorSkill &TCursorSkill);
		void GetSkillPageSkill(irr::core::position2di &TPoint, _CursorSkill &TCursorSkill);
		void SetSkillBar(int TSlot, int TOldSlot, const _Skill *TSkill);

		void RefreshSkillButtons();
		void SendBusy(bool TValue);
		void SendTradeRequest();
		void SendTradeCancel();
		int ValidateTradeGold();

		void SplitStack(int TSlot, int TCount);

		int *State;

		// Objects
		_Player *Player;
		const _Vendor *Vendor;
		const _Trader *Trader;

		// GUI
		irr::gui::IGUITab *TabMenu, *TabInventory, *TabVendor, *TabTrader, *TabSkill, *TabTrade;
		irr::gui::IGUIImage *TraderWindow;
		_CursorItem CursorItem, TooltipItem;
		bool CharacterOpen;

		// Chat
		irr::gui::IGUIEditBox *ChatBox;
		std::list<_ChatMessage> ChatHistory;
		bool Chatting;

		// Skills
		_CursorSkill CursorSkill, TooltipSkill;
		bool SkillBarChanged;

		// Traders
		int RequiredItemSlots[8], RewardItemSlot;

		// Trading
		irr::gui::IGUIEditBox *TradeGoldBox;
		irr::gui::IGUIButton *TradeAcceptButton;
		bool TypingGold;
};

extern _HUD HUD;
