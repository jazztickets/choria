/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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

// Constants
const size_t NOSLOT = (size_t)-1;
const int INVENTORY_MAX_STACK = 255;

// Forward Declarations
struct _Trader;
class _Object;
class _BaseItem;
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

enum class BagType : size_t {
	NONE,
	EQUIPMENT,
	INVENTORY,
	TRADE,
	KEYS,
	STASH,
	COUNT,
};

// Single inventory slot
struct _InventorySlot {
	_InventorySlot() { Reset(); }
	_InventorySlot(const _BaseItem *Item, int Count) : Item(Item), Upgrades(0), Count(Count), MaxCount(INVENTORY_MAX_STACK) { }

	void Serialize(ae::_Buffer &Data);
	void Unserialize(ae::_Buffer &Data, const _Stats *Stats);
	void Reset() { Item = nullptr; Upgrades = 0; Count = 0; MaxCount = INVENTORY_MAX_STACK; }

	const _BaseItem *Item;
	int Upgrades;
	int Count;
	int MaxCount;
};

// Bags contain multiple slots
struct _Bag {
	_Bag() : Type(BagType::NONE), StaticSize(true) { }

	void Serialize(ae::_Buffer &Data);
	void Unserialize(ae::_Buffer &Data, const _Stats *Stats);
	bool HasItem(const std::string &ID);

	std::vector<_InventorySlot> Slots;
	std::string ID;
	BagType Type;
	bool StaticSize;
};

// Slots point to a bag and index in the bag
struct _Slot {
	_Slot() : Type(BagType::NONE), Index(NOSLOT) { }
	_Slot(BagType BagType, size_t Index) : Type(BagType), Index(Index) { }

	bool operator==(const _Slot &Slot) const { return this->Index == Slot.Index && this->Type == Slot.Type; }
	bool operator!=(const _Slot &Slot) const { return !(*this == Slot); }

	void Serialize(ae::_Buffer &Data) const;
	void Unserialize(ae::_Buffer &Data);
	void Reset() { Type = BagType::NONE; Index = NOSLOT; }

	BagType Type;
	size_t Index;
};

// Classes
class _Inventory {

	friend class _Save;
	friend class _Stats;

	public:

		_Inventory();

		// Network
		void Serialize(ae::_Buffer &Data);
		void SerializeSlot(ae::_Buffer &Data, const _Slot &Slot);
		void Unserialize(ae::_Buffer &Data, const _Stats *Stats);
		void UnserializeSlot(ae::_Buffer &Data, const _Stats *Stats);

		bool FindItem(const _BaseItem *Item, size_t &Slot, size_t StartSlot);
		bool HasItem(const std::string &ID);
		int CountItem(const _BaseItem *Item);
		bool IsValidSlot(const _Slot &Slot) { return Slot.Type > BagType::NONE && Slot.Type < BagType::COUNT && Slot.Index < GetBag(Slot.Type).Slots.size(); }
		_InventorySlot &GetSlot(const _Slot &Slot) { return GetBag(Slot.Type).Slots[Slot.Index]; }

		bool MoveInventory(ae::_Buffer &Data, const _Slot &OldSlot, const _Slot &NewSlot);
		int UpdateItemCount(const _Slot &Slot, int Amount);
		void SpendItems(const _BaseItem *Item, int Count);
		_Slot FindSlotForItem(const _BaseItem *Item, int Upgrades, int Count);
		_Slot FindSlotForItemInBag(BagType BagType, const _BaseItem *Item, int Upgrades, int Count);
		bool AddItem(const _BaseItem *Item, int Upgrades, int Count, _Slot TargetSlot=_Slot());
		void MoveTradeToInventory();
		bool SplitStack(ae::_Buffer &Data, const _Slot &Slot, int Count);

		// Traders
		_Slot GetRequiredItemSlots(const _Trader *Trader, std::vector<_Slot> &RequiredItemSlots);

		// Bags
		_Bag &GetBag(BagType Bag) { return Bags[(size_t)Bag]; }
		_Bag *GetBagByID(const std::string &ID);
		std::vector<_Bag> &GetBags() { return Bags; }

	private:

		bool CanEquipItem(size_t Slot, const _BaseItem *Item);
		void SwapItem(const _Slot &Slot, const _Slot &OldSlot);
		bool CanSwap(const _Slot &OldSlot, const _Slot &NewSlot);

		// Items
		std::vector<_Bag> Bags;

};
