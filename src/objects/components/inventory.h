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
#include <constants.h>
#include <vector>
#include <string>
#include <cstdint>

const int PLAYER_TRADEITEMS = 8;
const int PLAYER_INVENTORYSIZE = 24;
const size_t NOSLOT = (size_t)-1;

// Forward Declarations
struct _Trader;
class _Object;
class _Item;
class _Stats;
class _Inventory;

namespace ae {
	class _Buffer;
}

enum EquipmentType : size_t {
	HEAD,
	BODY,
	LEGS,
	HAND1,
	HAND2,
	RING1,
	RING2,
	AMULET,
	COUNT,
};

// Single inventory slot
struct _InventorySlot {
	_InventorySlot() { Reset(); }
	_InventorySlot(const _Item *Item, int Count) : Item(Item), Upgrades(0), Count(Count), MaxCount(INVENTORY_MAX_STACK) { }

	void Serialize(ae::_Buffer &Data);
	void Unserialize(ae::_Buffer &Data, const _Stats *Stats);
	void Reset() { Item = nullptr; Upgrades = 0; Count = 0; MaxCount = INVENTORY_MAX_STACK; }

	const _Item *Item;
	int Upgrades;
	int Count;
	int MaxCount;
};

// Bags contain multiple slots
struct _Bag {

	enum BagType : uint8_t {
		NONE,
		EQUIPMENT,
		INVENTORY,
		TRADE,
		COUNT,
	};

	_Bag() : ID(_Bag::NONE) { }

	void Serialize(ae::_Buffer &Data);
	void Unserialize(ae::_Buffer &Data, const _Stats *Stats);

	std::vector<_InventorySlot> Slots;
	std::string Name;
	BagType ID;
};

// Slots point to a bag and index in the bag
struct _Slot {
	_Slot() : BagType(_Bag::BagType::NONE), Index(NOSLOT) { }
	_Slot(_Bag::BagType BagType, size_t Index) : BagType(BagType), Index(Index) { }

	bool operator==(const _Slot &Slot) const { return this->Index == Slot.Index && this->BagType == Slot.BagType; }
	bool operator!=(const _Slot &Slot) const { return !(*this == Slot); }

	void Serialize(ae::_Buffer &Data) const;
	void Unserialize(ae::_Buffer &Data);
	void Reset() { BagType = _Bag::BagType::NONE; Index = NOSLOT; }

	_Bag::BagType BagType;
	size_t Index;
};

// Classes
class _Inventory {

	public:

		_Inventory();

		// Network
		void Serialize(ae::_Buffer &Data);
		void SerializeSlot(ae::_Buffer &Data, const _Slot &Slot);
		void Unserialize(ae::_Buffer &Data, const _Stats *Stats);
		void UnserializeSlot(ae::_Buffer &Data, const _Stats *Stats);

		bool FindItem(const _Item *Item, size_t &Slot, size_t StartSlot);
		bool HasItemID(uint32_t ItemID);
		int CountItem(const _Item *Item);
		bool IsValidSlot(const _Slot &Slot) { return (int)Slot.BagType > 0 && (int)Slot.BagType < _Bag::BagType::COUNT && Slot.Index < Bags[Slot.BagType].Slots.size(); }
		_InventorySlot &GetSlot(const _Slot &Slot) { return Bags[Slot.BagType].Slots[Slot.Index]; }

		bool MoveInventory(ae::_Buffer &Data, const _Slot &OldSlot, const _Slot &NewSlot);
		int UpdateItemCount(const _Slot &Slot, int Amount);
		void SpendItems(const _Item *Item, int Count);
		_Slot FindSlotForItem(const _Item *Item, int Upgrades, int Count);
		_Slot FindSlotForItemInBag(_Bag::BagType BagType, const _Item *Item, int Upgrades, int Count);
		bool AddItem(const _Item *Item, int Upgrades, int Count, _Slot TargetSlot=_Slot());
		void MoveTradeToInventory();
		bool SplitStack(ae::_Buffer &Data, const _Slot &Slot, int Count);

		// Traders
		_Slot GetRequiredItemSlots(const _Trader *Trader, std::vector<_Slot> &RequiredItemSlots);

		// Items
		std::vector<_Bag> Bags;

	private:

		bool CanEquipItem(size_t Slot, const _Item *Item);
		void SwapItem(const _Slot &Slot, const _Slot &OldSlot);
		bool CanSwap(const _Slot &OldSlot, const _Slot &NewSlot);

};
