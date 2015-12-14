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
#include <objects/inventory.h>
#include <objects/item.h>
#include <constants.h>
#include <buffer.h>
#include <stats.h>

// Constructor
_Inventory::_Inventory() {

	Slots.resize(InventoryType::COUNT);
}

// Serialize
void _Inventory::Serialize(_Buffer &Data) {

	// Get item count
	uint8_t ItemCount = 0;
	for(uint8_t i = 0; i < InventoryType::COUNT; i++) {
		if(Slots[i].Item)
			ItemCount++;
	}

	// Write items
	Data.Write<uint8_t>(ItemCount);
	for(uint8_t i = 0; i < InventoryType::COUNT; i++) {
		if(Slots[i].Item) {
			Data.Write<uint8_t>(i);
			Slots[i].Serialize(Data);
		}
	}

}

// Serialize a inventory slot
void _Inventory::SerializeSlot(_Buffer &Data, size_t Slot) {
	Data.Write<uint8_t>((uint8_t)Slot);
	Slots[Slot].Serialize(Data);
}

// Unserialize
void _Inventory::Unserialize(_Buffer &Data, _Stats *Stats) {

	// Reset inventory
	std::fill(Slots.begin(), Slots.end(), _InventorySlot(nullptr, 0));

	// Read items
	uint8_t ItemCount = Data.Read<uint8_t>();
	for(uint8_t i = 0; i < ItemCount; i++) {
		UnserializeSlot(Data, Stats);
	}
}

// Unserialize one slot
void _Inventory::UnserializeSlot(_Buffer &Data, _Stats *Stats) {
	size_t Slot = Data.Read<uint8_t>();
	Slots[Slot].Unserialize(Data, Stats);
}

// Search for an item in the inventory
bool _Inventory::FindItem(const _Item *Item, size_t &Slot) {
	for(size_t i = InventoryType::BAG; i < InventoryType::TRADE; i++) {
		if(Slots[i].Item == Item) {
			Slot = i;
			return true;
		}
	}

	return false;
}

// Count the number of a certain item in inventory
int _Inventory::CountItem(const _Item *Item) {
	int Count = 0;
	for(size_t i = InventoryType::BAG; i < InventoryType::TRADE; i++) {
		if(Slots[i].Item == Item)
			Count += Slots[i].Count;
	}

	return Count;
}

// Gets an item from the inventory bag
const _Item *_Inventory::GetBagItem(size_t Slot) {

	// Check for bad slots
	if(Slot < InventoryType::BAG || Slot >= InventoryType::TRADE || !Slots[Slot].Item)
		return nullptr;

	return Slots[Slot].Item;
}

// Checks if an item can be equipped
bool _Inventory::CanEquipItem(size_t Slot, const _Item *Item) {
	if(!Item)
		return true;

	// Check type
	switch(Slot) {
		case InventoryType::HEAD:
			if(Item->Type == _Item::TYPE_HEAD)
				return true;
		break;
		case InventoryType::BODY:
			if(Item->Type == _Item::TYPE_BODY)
				return true;
		break;
		case InventoryType::LEGS:
			if(Item->Type == _Item::TYPE_LEGS)
				return true;
		break;
		case InventoryType::HAND1:
			if(Item->Type == _Item::TYPE_WEAPON1HAND)
				return true;
		break;
		case InventoryType::HAND2:
			if(Item->Type == _Item::TYPE_SHIELD)
				return true;
		break;
		case InventoryType::RING1:
		case InventoryType::RING2:
			if(Item->Type == _Item::TYPE_RING)
				return true;
		break;
		default:
		break;
	}

	return false;
}

// Moves an item from one slot to another
bool _Inventory::MoveInventory(_Buffer &Data, size_t OldSlot, size_t NewSlot) {
	if(OldSlot == NewSlot)
		return false;

	// Check if the item is even equippable
	if(NewSlot < InventoryType::BAG && !CanEquipItem(NewSlot, Slots[OldSlot].Item))
		return false;

	// Add to stack
	if(Slots[NewSlot].Item == Slots[OldSlot].Item) {
		Slots[NewSlot].Count += Slots[OldSlot].Count;

		// Group stacks
		if(Slots[NewSlot].Count > INVENTORY_MAX_STACK) {
			Slots[OldSlot].Count = Slots[NewSlot].Count - INVENTORY_MAX_STACK;
			Slots[NewSlot].Count = INVENTORY_MAX_STACK;
		}
		else
			Slots[OldSlot].Item = nullptr;

	}
	else {

		// Check for swapping a non-equippable item
		if(OldSlot < InventoryType::BAG && !CanEquipItem(OldSlot, Slots[NewSlot].Item))
			return false;

		SwapItem(NewSlot, OldSlot);
	}

	// Build packet
	SerializeSlot(Data, NewSlot);
	SerializeSlot(Data, OldSlot);

	return true;
}

// Swaps two items
void _Inventory::SwapItem(size_t Slot, size_t OldSlot) {
	_InventorySlot TempItem;

	// Swap items
	TempItem = Slots[Slot];
	Slots[Slot] = Slots[OldSlot];
	Slots[OldSlot] = TempItem;
}

// Updates an item's count, deleting if necessary
bool _Inventory::DecrementItemCount(size_t Slot, int Amount) {

	Slots[Slot].Count += Amount;
	if(Slots[Slot].Count <= 0) {
		Slots[Slot].Item = nullptr;
		Slots[Slot].Count = 0;
		return true;
	}

	return false;
}

// Find a suitable slot for an item
size_t _Inventory::FindSlotForItem(const _Item *Item, int Count) {

	size_t EmptySlot = Slots.size();
	for(size_t i = InventoryType::HEAD; i < InventoryType::TRADE; i++) {

		// Try to find an existing stack first
		if(Slots[i].Item == Item && Slots[i].Count + Count <= INVENTORY_MAX_STACK)
			return i;

		// Keep track of the first empty slot in case stack is not found
		if(EmptySlot == Slots.size() &&
		   Slots[i].Item == nullptr &&
		   (CanEquipItem(i, Item) || i >= InventoryType::BAG)) {
				EmptySlot = i;
		}
	}

	return EmptySlot;
}

// Attempts to add an item to the inventory
bool _Inventory::AddItem(const _Item *Item, int Count, size_t Slot) {

	// Place somewhere in bag
	if(Slot >= Slots.size()) {

		// Search for a suitable slot
		Slot = FindSlotForItem(Item, Count);
		if(Slot >= Slots.size())
			return false;

	}
	// Trying to equip an item
	else if(Slot < InventoryType::BAG) {

		// Make sure it can be equipped
		if(!CanEquipItem(Slot, Item))
			return false;

	}

	// Add item
	if(Slots[Slot].Item == Item && Slots[Slot].Count + Count <= INVENTORY_MAX_STACK) {
		Slots[Slot].Count += Count;
		return true;
	}
	else if(Slots[Slot].Item == nullptr) {
		Slots[Slot].Item = Item;
		Slots[Slot].Count = Count;
		return true;
	}

	return false;
}

// Moves the player's trade items to their bag
void _Inventory::MoveTradeToInventory() {

	for(size_t i = InventoryType::TRADE; i < InventoryType::COUNT; i++) {
		if(Slots[i].Item && AddItem(Slots[i].Item, Slots[i].Count))
			Slots[i].Item = nullptr;
	}
}

// Splits a stack
bool _Inventory::SplitStack(_Buffer &Data, size_t Slot, int Count) {
	if(Slot >= Slots.size())
		return false;

	// Make sure stack is large enough
	_InventorySlot *SplitItem = &Slots[Slot];
	if(SplitItem->Item && SplitItem->Count > Count) {

		// Get starting search position
		size_t EmptySlot = Slot;
		if(EmptySlot < InventoryType::BAG)
			EmptySlot = InventoryType::BAG;

		// Find an empty slot or existing item starting from bag
		bool Found = false;
		for(size_t i = InventoryType::BAG; i < InventoryType::TRADE; i++) {
			EmptySlot++;
			if(EmptySlot >= InventoryType::TRADE)
				EmptySlot = InventoryType::BAG;

			_InventorySlot *Item = &Slots[EmptySlot];
			if(Item->Item == nullptr || (Item->Item == SplitItem->Item && Item->Count <= INVENTORY_MAX_STACK - Count)) {
				Found = true;
				break;
			}
		}

		// Split item
		if(Found && EmptySlot != Slot) {
			SplitItem->Count -= Count;
			AddItem(SplitItem->Item, Count, EmptySlot);

			// Write old and new slot
			Data.Write<uint8_t>(2);
			SerializeSlot(Data, Slot);
			SerializeSlot(Data, EmptySlot);

			return true;
		}
	}

	return false;
}

// Fills an array with inventory indices correlating to a trader's required items
size_t _Inventory::GetRequiredItemSlots(const _Trader *Trader, std::vector<size_t> &BagIndex) {

	// Find a slot for the reward
	size_t RewardItemSlot = FindSlotForItem(Trader->RewardItem, Trader->Count);

	// Go through required items
	for(size_t i = 0; i < Trader->TraderItems.size(); i++) {
		const _Item *RequiredItem = Trader->TraderItems[i].Item;
		int RequiredCount = Trader->TraderItems[i].Count;
		BagIndex[i] = Slots.size();

		// Search for the required item
		for(size_t j = InventoryType::HEAD; j < InventoryType::TRADE; j++) {
			_InventorySlot *InventoryItem = &Slots[j];
			if(InventoryItem->Item == RequiredItem && InventoryItem->Count >= RequiredCount) {
				BagIndex[i] = j;
				break;
			}
		}

		// Didn't find an item
		if(BagIndex[i] >= Slots.size())
			RewardItemSlot = Slots.size();
	}

	return RewardItemSlot;
}

// Serialize a slot
void _InventorySlot::Serialize(_Buffer &Data) {
	if(Item) {
		Data.Write<uint32_t>(Item->ID);
		Data.Write<uint8_t>((uint8_t)Count);
	}
	else
		Data.Write<uint32_t>(0);
}

// Unserialize a slot
void _InventorySlot::Unserialize(_Buffer &Data, _Stats *Stats) {

	uint32_t ItemID = Data.Read<uint32_t>();
	if(ItemID) {
		Item = Stats->Items[ItemID];
		Count = Data.Read<uint8_t>();
	}
	else {
		Item = nullptr;
		Count = 0;
	}
}
