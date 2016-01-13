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
#include <vector>
#include <string>
#include <cstdint>

// Forward Declarations
struct _Trader;
class _Object;
class _Item;
class _Buffer;
class _Stats;

// Structures
struct _InventorySlot {
	_InventorySlot() : Item(nullptr), Count(0) { }
	_InventorySlot(const _Item *Item, int Count) : Item(Item), Count(Count) { }

	void Serialize(_Buffer &Data);
	void Unserialize(_Buffer &Data, _Stats *Stats);

	const _Item *Item;
	int Count;
};

const int PLAYER_TRADEITEMS = 8;

enum InventoryType : size_t {
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

		// Network
		void Serialize(_Buffer &Data);
		void SerializeSlot(_Buffer &Data, size_t Slot);
		void Unserialize(_Buffer &Data, _Stats *Stats);
		void UnserializeSlot(_Buffer &Data, _Stats *Stats);

		bool FindItem(const _Item *Item, size_t &Slot, size_t StartSlot);
		bool HasItemID(uint32_t ItemID);
		int CountItem(const _Item *Item);

		const _Item *GetBagItem(size_t Slot);
		bool MoveInventory(_Buffer &Data, size_t OldSlot, size_t NewSlot);
		bool DecrementItemCount(size_t Slot, int Amount);
		size_t FindSlotForItem(const _Item *Item, int Count);
		bool AddItem(const _Item *Item, int Count, size_t Slot=(size_t)-1);
		void MoveTradeToInventory();
		bool SplitStack(_Buffer &Data, size_t Slot, int Count);

		// Traders
		size_t GetRequiredItemSlots(const _Trader *Trader, std::vector<size_t> &BagIndex);

		// Static functions
		static bool IsSlotBag(size_t Slot) { return Slot >= InventoryType::BAG && Slot < InventoryType::TRADE; }
		static bool IsSlotTrade(size_t Slot) { return Slot >= InventoryType::TRADE && Slot < InventoryType::COUNT; }

		std::vector<_InventorySlot> Slots;

	private:

		bool CanEquipItem(size_t Slot, const _Item *Item);
		void SwapItem(size_t Slot, size_t OldSlot);
		bool CanSwap(size_t OldSlot, size_t NewSlot);

};
