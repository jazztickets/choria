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

// Constructor
_Inventory::_Inventory() {

	for(int i = 0; i < InventoryType::COUNT; i++) {
		Inventory[i].Item = nullptr;
		Inventory[i].Count = 0;
	}
}

// Search for an item in the inventory
bool _Inventory::FindItem(const _Item *Item, int &Slot) {
	for(int i = InventoryType::BACKPACK; i < InventoryType::TRADE; i++) {
		if(Inventory[i].Item == Item) {
			Slot = i;
			return true;
		}
	}

	return false;
}

// Count the number of a certain item in inventory
int _Inventory::CountItem(const _Item *Item) {
	int Count = 0;
	for(int i = InventoryType::BACKPACK; i < InventoryType::TRADE; i++) {
		if(Inventory[i].Item == Item)
			Count += Inventory[i].Count;
	}

	return Count;
}

// Sets an item in the inventory
void _Inventory::SetInventory(int Slot, const _InventorySlot &Item) {

	if(Item.Item == nullptr) {
		Inventory[Slot].Item = nullptr;
		Inventory[Slot].Count = 0;
	}
	else {
		Inventory[Slot].Item = Item.Item;
		Inventory[Slot].Count = Item.Count;
	}
}

// Gets an inventory item
const _Item *_Inventory::GetInventoryItem(int Slot) {

	// Check for bad slots
	if(Slot < InventoryType::BACKPACK || Slot >= InventoryType::TRADE || !Inventory[Slot].Item)
		return nullptr;

	return Inventory[Slot].Item;
}

// Checks if an item can be equipped
bool _Inventory::CanEquipItem(int Slot, const _Item *Item) {
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
bool _Inventory::MoveInventory(int OldSlot, int NewSlot) {
	if(OldSlot == NewSlot)
		return false;

	// Check if the item is even equippable
	if(NewSlot < InventoryType::BACKPACK && !CanEquipItem(NewSlot, Inventory[OldSlot].Item))
		return false;

	// Add to stack
	if(Inventory[NewSlot].Item == Inventory[OldSlot].Item) {
		Inventory[NewSlot].Count += Inventory[OldSlot].Count;

		// Group stacks
		if(Inventory[NewSlot].Count > INVENTORY_MAX_STACK) {
			Inventory[OldSlot].Count = Inventory[NewSlot].Count - INVENTORY_MAX_STACK;
			Inventory[NewSlot].Count = INVENTORY_MAX_STACK;
		}
		else
			Inventory[OldSlot].Item = nullptr;
	}
	else {

		// Disable reverse equip for now
		if(OldSlot < InventoryType::BACKPACK && !CanEquipItem(OldSlot, Inventory[NewSlot].Item))
			return false;

		SwapItem(NewSlot, OldSlot);
	}

	return true;
}

// Swaps two items
void _Inventory::SwapItem(int Slot, int OldSlot) {
	_InventorySlot TempItem;

	// Swap items
	TempItem = Inventory[Slot];
	Inventory[Slot] = Inventory[OldSlot];
	Inventory[OldSlot] = TempItem;
}

// Updates an item's count, deleting if necessary
bool _Inventory::UpdateInventory(int Slot, int Amount) {

	Inventory[Slot].Count += Amount;
	if(Inventory[Slot].Count <= 0) {
		Inventory[Slot].Item = nullptr;
		Inventory[Slot].Count = 0;
		return true;
	}

	return false;
}

// Attempts to add an item to the inventory
bool _Inventory::AddItem(const _Item *Item, int Count, int Slot) {

	// Place somewhere in backpack
	if(Slot == -1) {

		// Find existing item
		int EmptySlot = -1;
		for(int i = InventoryType::BACKPACK; i < InventoryType::TRADE; i++) {
			if(Inventory[i].Item == Item && Inventory[i].Count + Count <= INVENTORY_MAX_STACK) {
				Inventory[i].Count += Count;
				return true;
			}

			// Keep track of the first empty slot
			if(Inventory[i].Item == nullptr && EmptySlot == -1)
				EmptySlot = i;
		}

		// Found an empty slot
		if(EmptySlot != -1) {
			Inventory[EmptySlot].Item = Item;
			Inventory[EmptySlot].Count = Count;
			return true;
		}

		return false;
	}
	// Trying to equip an item
	else if(Slot < InventoryType::BACKPACK) {

		// Make sure it can be equipped
		if(!CanEquipItem(Slot, Item))
			return false;

		// Set item
		Inventory[Slot].Item = Item;
		Inventory[Slot].Count = Count;

		return true;
	}

	// Add item
	if(Inventory[Slot].Item == Item && Inventory[Slot].Count + Count <= INVENTORY_MAX_STACK) {
		Inventory[Slot].Count += Count;
		return true;
	}
	else if(Inventory[Slot].Item == nullptr) {
		Inventory[Slot].Item = Item;
		Inventory[Slot].Count = Count;
		return true;
	}

	return false;
}

// Determines if the player's backpack is full
bool _Inventory::IsBackpackFull() {

	// Search backpack
	for(int i = InventoryType::BACKPACK; i < InventoryType::TRADE; i++) {
		if(Inventory[i].Item == nullptr)
			return false;
	}

	return true;
}

// Moves the player's trade items to their backpack
void _Inventory::MoveTradeToInventory() {

	for(int i = InventoryType::TRADE; i < InventoryType::COUNT; i++) {
		if(Inventory[i].Item && AddItem(Inventory[i].Item, Inventory[i].Count, -1))
			Inventory[i].Item = nullptr;
	}
}

// Splits a stack
void _Inventory::SplitStack(int Slot, int Count) {
	if(Slot < 0 || Slot >= InventoryType::COUNT)
		return;

	// Make sure stack is large enough
	_InventorySlot *SplitItem = &Inventory[Slot];
	if(SplitItem->Item && SplitItem->Count > Count) {

		// Find an empty slot or existing item
		int EmptySlot = Slot;
		_InventorySlot *Item;
		do {
			EmptySlot++;
			if(EmptySlot >= InventoryType::TRADE)
				EmptySlot = InventoryType::BACKPACK;

			Item = &Inventory[EmptySlot];
		} while(!(EmptySlot == Slot || Item->Item == nullptr || (Item->Item == SplitItem->Item && Item->Count <= INVENTORY_MAX_STACK - Count)));

		// Split item
		if(EmptySlot != Slot) {
			SplitItem->Count -= Count;
			AddItem(SplitItem->Item, Count, EmptySlot);
		}
	}
}
