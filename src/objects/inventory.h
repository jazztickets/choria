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
#include <string>
#include <cstdint>

// Forward Declarations
class _Object;
class _Item;
class _Buffer;
class _Stats;
class _Trader;

// Structures
struct _InventorySlot {
	_InventorySlot() : Item(nullptr), Count(0) { }
	_InventorySlot(const _Item *Item, int Count) : Item(Item), Count(Count) { }

	const _Item *Item;
	int Count;
};

const int PLAYER_TRADEITEMS = 8;

enum InventoryType : int {
	HEAD,
	BODY,
	LEGS,
	HAND1,
	HAND2,
	RING1,
	RING2,
	BAG,
	TRADE = BAG + 24,
	COUNT = TRADE + PLAYER_TRADEITEMS,
};

// Classes
class _Inventory {

	public:

		_Inventory();

		void Serialize(_Buffer &Data);
		void Unserialize(_Buffer &Data, _Stats *Stats);

		bool FindItem(const _Item *Item, int &Slot);
		int CountItem(const _Item *Item);

		void SetInventory(int Slot, const _InventorySlot &Item);
		const _Item *GetBagItem(int Slot);
		bool MoveInventory(int OldSlot, int NewSlot);
		bool UpdateInventory(int Slot, int Amount);
		bool AddItem(const _Item *Item, int Count, int Slot);
		bool IsBagFull();
		bool IsEmptySlot(int Slot) { return Inventory[Slot].Item == nullptr; }
		void MoveTradeToInventory();
		void SplitStack(int Slot, int Count);

		// Traders
		int GetRequiredItemSlots(const _Trader *Trader, int *Slots);

		_InventorySlot Inventory[InventoryType::COUNT];

		static bool IsSlotInventory(int Slot) { return Slot >= InventoryType::BAG && Slot < InventoryType::TRADE; }
		static bool IsSlotTrade(int Slot) { return Slot >= InventoryType::TRADE && Slot < InventoryType::COUNT; }

	private:

		bool CanEquipItem(int Slot, const _Item *Item);
		void SwapItem(int Slot, int OldSlot);

};
