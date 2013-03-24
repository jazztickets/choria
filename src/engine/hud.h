/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2010  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef HUD_H
#define HUD_H

// Libraries
#include <irrlicht.h>
#include "singleton.h"

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Forward Declarations
class PlayerClass;
class ItemClass;
class SkillClass;
struct InventoryStruct;
struct VendorStruct;
struct TraderStruct;

// Structures
struct CursorItemStruct {
	void Set(const ItemClass *TItem, int TCost, int TCount, int TSlot) {
		Item = TItem;
		Cost = TCost;
		Count = TCount;
		Slot = TSlot;
	}
	void Reset() {
		Item = NULL;
		Count = 0;
		Slot = -1;
		Window = -1;
	}

	bool IsEqual(int TSlot, int TWindow) { return Slot == TSlot && Window == TWindow; }

	const ItemClass *Item;
	int Cost;
	int Count;
	int Slot;
	int Window;
};

struct CursorSkillStruct {
	void Reset() {
		Skill = NULL;
		Slot = -1;
		Window = -1;
	}

	const SkillClass *Skill;
	int Slot;
	int Window;
};

struct ChatStruct {
	ChatStruct() : Message(""), TimeOut(0) { }
	ChatStruct(const stringc &TMessage) : Message(TMessage), TimeOut(0) { }
	stringc Message;
	u32 TimeOut;
};

// Classes
class HUDClass {

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

		int Init();
		int Close();

		// Updates
		void HandleMouseMotion(int TMouseX, int TMouseY);
		bool HandleMousePress(int TButton, int TMouseX, int TMouseY);
		void HandleMouseRelease(int TButton, int TMouseX, int TMouseY);
		void HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement);

		void Update(u32 TDeltaTime);
		void PreGUIDraw();
		void Draw();

		// Objects
		void SetPlayer(PlayerClass *TPlayer) { Player = TPlayer; }

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
		void HandleKeyPress(EKEY_CODE TKey);
		void AddChatMessage(ChatStruct &TChat) { ChatHistory.push_back(TChat); }

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
		void DrawItemPrice(const ItemClass *TItem, int TCount, int TDrawX, int TDrawY, bool TBuy);
		void DrawCursorItem();
		void DrawItemTooltip();
		void DrawCursorSkill();
		void DrawSkillTooltip();
		void DrawSkillDescription(const SkillClass *Skill, int TLevel, int TDrawX, int &TDrawY);
		void DrawTradeItems(PlayerClass *TPlayer, int TDrawX, int TDrawY, bool TDrawAll);

		void GetItem(position2di &TPoint, CursorItemStruct &TCursorItem);
		void GetInventoryItem(position2di &TPoint, CursorItemStruct &TCursorItem);
		void GetVendorItem(position2di &TPoint, CursorItemStruct &TCursorItem);
		void GetTraderItem(position2di &TPoint, CursorItemStruct &TCursorItem);
		void GetTradeItem(position2di &TPoint, CursorItemStruct &TCursorItem);
		void BuyItem(CursorItemStruct *TCursorItem, int TTargetSlot);
		void SellItem(CursorItemStruct *TCursorItem, int TAmount);

		void GetSkill(position2di &TPoint, CursorSkillStruct &TCursorSkill);
		void GetSkillPageSkill(position2di &TPoint, CursorSkillStruct &TCursorSkill);
		void SetSkillBar(int TSlot, int TOldSlot, const SkillClass *TSkill);

		void RefreshSkillButtons();
		void SendBusy(bool TValue);
		void SendTradeRequest();
		void SendTradeCancel();
		int ValidateTradeGold();

		void SplitStack(int TSlot, int TCount);

		int *State;

		// Objects
		PlayerClass *Player;
		const VendorStruct *Vendor;
		const TraderStruct *Trader;

		// GUI
		IGUITab *TabMenu, *TabInventory, *TabVendor, *TabTrader, *TabSkill, *TabTrade;
		IGUIImage *TraderWindow;
		CursorItemStruct CursorItem, TooltipItem;
		bool CharacterOpen;

		// Chat
		IGUIEditBox *ChatBox;
		list<ChatStruct> ChatHistory;
		bool Chatting;

		// Skills
		CursorSkillStruct CursorSkill, TooltipSkill;
		bool SkillBarChanged;

		// Traders
		int RequiredItemSlots[8], RewardItemSlot;

		// Trading
		IGUIEditBox *TradeGoldBox;
		IGUIButton *TradeAcceptButton;
		bool TypingGold;
};

// Singletons
typedef SingletonClass<HUDClass> HUD;

#endif
