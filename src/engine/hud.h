/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef HUD_H
#define HUD_H

// Libraries
#include <irrlicht/irrlicht.h>
#include "constants.h"

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
	void Set(int Window, const ItemClass *Item, int Cost, int Count, int Slot) {
		this->Window = Window;
		this->Item = Item;
		this->Cost = Cost;
		this->Count = Count;
		this->Slot = Slot;
	}
	void Reset() {
		Item = NULL;
		Count = 0;
		Slot = -1;
		Window = -1;
	}

	bool IsEqual(int Slot, int Window) { return this->Slot == Slot && this->Window == Window; }

	const ItemClass *Item;
	int Cost;
	int Count;
	int Slot;
	int Window;
};

struct CursorActionStruct {
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
	ChatStruct(const stringc &Message) : Message(Message), TimeOut(0) { }
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
			ELEMENT_ACTIONS,
			ELEMENT_MAINMENU,
			ELEMENT_MAINMENURESUME,
			ELEMENT_MAINMENUEXIT,
			ELEMENT_TRADERACCEPT,
			ELEMENT_TRADERCANCEL,
			ELEMENT_CHATBOX,
			ELEMENT_GOLDTRADEBOX,
			ELEMENT_TRADEACCEPT,
			ELEMENT_ACTION1,
			ELEMENT_ACTION2,
			ELEMENT_ACTION3,
			ELEMENT_ACTION4,
			ELEMENT_ACTION5,
			ELEMENT_ACTION6,
			ELEMENT_ACTION7,
			ELEMENT_ACTION8,
		};

		enum WindowType {
			WINDOW_INVENTORY,
			WINDOW_VENDOR,
			WINDOW_TRADER,
			WINDOW_TRADETHEM,
			WINDOW_TRADEYOU,
			WINDOW_SKILLS,
			WINDOW_ACTIONBAR
		};

		int Init();
		int Close();

		// Updates
		void HandleMouseMotion(int MouseX, int MouseY);
		bool HandleMousePress(int Button, int MouseX, int MouseY);
		void HandleMouseRelease(int Button, int MouseX, int MouseY);
		void HandleGUI(EGUI_EVENT_TYPE EventType, IGUIElement *Element);

		void Update(u32 FrameTime);
		void PreGUIDraw();
		void Draw();
		void RefreshActionBar();

		// Objects
		void SetPlayer(PlayerClass *Player) { this->Player = Player; }

		// Windows
		void InitButtonBar();
		void InitActionBar();
		void InitMenu();
		void InitInventory(int X=400, int Y=300, bool SendBusy=true);
		void InitVendor(int VendorID);
		void InitTrader(int TraderID);
		void InitCharacter();
		void InitSkills();
		void InitTrade();
		void CloseWindows();

		// Chat
		void ToggleChat();
		bool IsChatting() { return Chatting; }
		void HandleKeyPress(EKEY_CODE Key);
		void AddChatMessage(ChatStruct &Chat) { ChatHistory.push_back(Chat); }

		// Trade
		void CloseTrade(bool SendNotify=true);
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
		void DrawItemPrice(const ItemClass *Item, int Count, int DrawX, int DrawY, bool Buy);
		void DrawCursorItem();
		void DrawItemTooltip();
		void DrawCursorAction();
		void DrawSkillTooltip();
		void DrawTradeItems(PlayerClass *TradePlayer, int DrawX, int DrawY, bool DrawAll);

		void BuyItem(CursorItemStruct *CursorItem, int TargetSlot);
		void SellItem(CursorItemStruct *CursorItem, int Amount);
		void SetTooltipItem(const InventoryStruct *InventoryItem, int Slot);

		void SetActionBar(int Index, int OldIndex, const SkillClass *Skill);
		void GetActionBarTooltip();

		void SendBusy(bool Value);
		void SendTradeRequest();
		void SendTradeCancel();
		int ValidateTradeGold();

		void SplitStack(int Slot, int Count);

		// Objects
		int *State;
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
		IGUIButton *ActionButtons[GAME_MAXACTIONS];
		array<const SkillClass *> *SkillCategories;
		CursorActionStruct CursorAction, TooltipAction;
		bool ActionBarChanged;

		// Traders
		int RequiredItemSlots[TRADE_SLOTS], RewardItemSlot;

		// Trading
		IGUIEditBox *TradeGoldBox;
		IGUIButton *TradeAcceptButton;
		bool TypingGold;
};

// Singletons
extern HUDClass HUD;

#endif
