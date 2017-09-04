/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#include <objects/components/inventory.h>
#include <objects/item.h>
#include <ae/buffer.h>
#include <constants.h>
#include <stats.h>

// Constructor
_Inventory::_Inventory() {

	Bags.resize(_Bag::COUNT);
	Bags[_Bag::EQUIPMENT].Slots.resize(EquipmentType::COUNT);
	Bags[_Bag::INVENTORY].Slots.resize(PLAYER_INVENTORYSIZE);
	Bags[_Bag::TRADE].Slots.resize(PLAYER_TRADEITEMS);
	for(auto &Slot : Bags[_Bag::EQUIPMENT].Slots)
		Slot.MaxCount = 1;

	Bags[_Bag::NONE].ID = _Bag::NONE;
	Bags[_Bag::EQUIPMENT].ID = _Bag::EQUIPMENT;
	Bags[_Bag::INVENTORY].ID = _Bag::INVENTORY;
	Bags[_Bag::TRADE].ID = _Bag::TRADE;

	Bags[_Bag::EQUIPMENT].Name = "equipment";
	Bags[_Bag::INVENTORY].Name = "inventory";
	Bags[_Bag::TRADE].Name = "trade";
}

// Serialize
void _Inventory::Serialize(_Buffer &Data) {

	// Serialize bags
	for(auto &Bag : Bags)
		Bag.Serialize(Data);
}

// Serialize a inventory slot
void _Inventory::SerializeSlot(_Buffer &Data, const _Slot &Slot) {
	if(!Slot.BagType)
		throw std::runtime_error("_Slot::Serialize - Bag is NULL");

	// Slot index
	Slot.Serialize(Data);

	// Item
	GetSlot(Slot).Serialize(Data);
}

// Unserialize
void _Inventory::Unserialize(_Buffer &Data, const _Stats *Stats) {

	// Unserialize bags
	for(auto &Bag : Bags)
		Bag.Unserialize(Data, Stats);
}

// Unserialize one slot
void _Inventory::UnserializeSlot(_Buffer &Data, const _Stats *Stats) {

	// Get slot
	_Slot Slot;
	Slot.Unserialize(Data);

	// Get item
	GetSlot(Slot).Unserialize(Data, Stats);
}

// Search for an item in the inventory
bool _Inventory::FindItem(const _Item *Item, size_t &Slot, size_t StartSlot) {

	_Bag &Bag = Bags[_Bag::INVENTORY];
	for(size_t i = 0; i < Bag.Slots.size(); i++) {
		if(StartSlot >= Bag.Slots.size())
			StartSlot = 0;

		if(Bag.Slots[StartSlot].Item == Item) {
			Slot = StartSlot;
			return true;
		}

		StartSlot++;
	}

	return false;
}

// Return true if a certain item id is in the inventory
bool _Inventory::HasItemID(uint32_t ItemID) {
	for(auto &Bag : Bags) {
		for(size_t i = 0; i < Bag.Slots.size(); i++) {
			if(Bag.Slots[i].Item && Bag.Slots[i].Item->ID == ItemID)
				return true;
		}
	}

	return false;
}

// Count the number of a certain item in inventory
int _Inventory::CountItem(const _Item *Item) {
	int Count = 0;
	_Bag &Bag = Bags[_Bag::INVENTORY];
	for(size_t i = 0; i < Bag.Slots.size(); i++) {
		if(Bag.Slots[i].Item == Item)
			Count += Bag.Slots[i].Count;
	}

	return Count;
}

// Checks if an item can be equipped
bool _Inventory::CanEquipItem(size_t Slot, const _Item *Item) {
	if(!Item)
		return true;

	// Check type
	switch(Slot) {
		case EquipmentType::HEAD:
			if(Item->Type == ItemType::HELMET)
				return true;
		break;
		case EquipmentType::BODY:
			if(Item->Type == ItemType::ARMOR)
				return true;
		break;
		case EquipmentType::LEGS:
			if(Item->Type == ItemType::BOOTS)
				return true;
		break;
		case EquipmentType::HAND1:
			if(Item->Type == ItemType::ONEHANDED_WEAPON)
				return true;

			if(Item->Type == ItemType::TWOHANDED_WEAPON && Bags[_Bag::EQUIPMENT].Slots[EquipmentType::HAND2].Item == nullptr)
				return true;
		break;
		case EquipmentType::HAND2:
			if(Item->Type == ItemType::SHIELD && (Bags[_Bag::EQUIPMENT].Slots[EquipmentType::HAND1].Item == nullptr || Bags[_Bag::EQUIPMENT].Slots[EquipmentType::HAND1].Item->Type != ItemType::TWOHANDED_WEAPON))
				return true;
		break;
		case EquipmentType::RING1:
		case EquipmentType::RING2:
			if(Item->Type == ItemType::RING)
				return true;
		break;
		case EquipmentType::AMULET:
			if(Item->Type == ItemType::AMULET)
				return true;
		break;
		default:
		break;
	}

	return false;
}

// Moves an item from one slot to another
bool _Inventory::MoveInventory(_Buffer &Data, const _Slot &OldSlot, const _Slot &NewSlot) {
	if(!IsValidSlot(OldSlot) || !IsValidSlot(NewSlot) || !CanSwap(OldSlot, NewSlot))
		return false;

	// Add to stack
	_InventorySlot &OldInventorySlot = GetSlot(OldSlot);
	_InventorySlot &NewInventorySlot = GetSlot(NewSlot);
	if(NewInventorySlot.Item && NewInventorySlot.Item->IsStackable() && NewInventorySlot.Item == OldInventorySlot.Item && NewInventorySlot.Upgrades == OldInventorySlot.Upgrades) {
		NewInventorySlot.Count += OldInventorySlot.Count;

		// Group stacks
		if(NewInventorySlot.Count > NewInventorySlot.MaxCount) {
			OldInventorySlot.Count = NewInventorySlot.Count - NewInventorySlot.MaxCount;
			NewInventorySlot.Count = NewInventorySlot.MaxCount;
		}
		else
			OldInventorySlot.Reset();
	}
	else {

		// Check for reverse swap
		if(!CanSwap(NewSlot, OldSlot))
			return false;

		SwapItem(NewSlot, OldSlot);
	}

	// Build packet
	SerializeSlot(Data, NewSlot);
	SerializeSlot(Data, OldSlot);

	return true;
}

// Swaps two items
void _Inventory::SwapItem(const _Slot &Slot, const _Slot &OldSlot) {
	_InventorySlot TempItem;

	// Swap items
	TempItem = GetSlot(Slot);
	GetSlot(Slot) = GetSlot(OldSlot);
	GetSlot(OldSlot) = TempItem;
}

// Determine if an item can be swapped
bool _Inventory::CanSwap(const _Slot &OldSlot, const _Slot &NewSlot) {
	if(OldSlot == NewSlot)
		return false;

	// Check if the item is even equippable
	if(NewSlot.BagType == _Bag::BagType::EQUIPMENT && !CanEquipItem(NewSlot.Index, GetSlot(OldSlot).Item))
		return false;

	if(NewSlot.BagType == _Bag::BagType::TRADE && GetSlot(OldSlot).Item && !GetSlot(OldSlot).Item->Tradable)
		return false;

	return true;
}

// Updates an item's count, deleting if necessary
bool _Inventory::DecrementItemCount(const _Slot &Slot, int Amount) {

	_InventorySlot &InventorySlot = GetSlot(Slot);
	InventorySlot.Count += Amount;
	if(InventorySlot.Count <= 0) {
		InventorySlot.Reset();
		return true;
	}

	return false;
}

// Find a suitable slot for an item
_Slot _Inventory::FindSlotForItem(const _Item *Item, int Upgrades, int Count) {
	_Slot Slot = FindSlotForItemInBag(_Bag::EQUIPMENT, Item, Upgrades, Count);
	if(!IsValidSlot(Slot))
		Slot = FindSlotForItemInBag(_Bag::INVENTORY, Item, Upgrades, Count);

	return Slot;
}

// Find a slot for an item in a certain bag
_Slot _Inventory::FindSlotForItemInBag(_Bag::BagType BagType, const _Item *Item, int Upgrades, int Count) {
	_Slot EmptySlot;
	_Bag &Bag = Bags[BagType];
	for(size_t i = 0; i < Bag.Slots.size(); i++) {

		// Try to find an existing stack first
		if(Item->IsStackable() && Bag.Slots[i].Item == Item && Bag.Slots[i].Upgrades == Upgrades && Bag.Slots[i].Count + Count <= Bag.Slots[i].MaxCount)
			return _Slot(BagType, i);

		// Keep track of the first empty slot in case stack is not found
		if(EmptySlot.BagType == _Bag::BagType::NONE && Bag.Slots[i].Item == nullptr && (CanEquipItem(i, Item) || BagType != _Bag::EQUIPMENT)) {
			EmptySlot.BagType = BagType;
			EmptySlot.Index = i;
		}
	}

	return EmptySlot;
}

// Attempts to add an item to the inventory
bool _Inventory::AddItem(const _Item *Item, int Upgrades, int Count, _Slot TargetSlot) {
	if(!Count)
		return false;

	bool Added = false;
	for(int i = 0; i < Count; i++) {
		_Slot Slot = TargetSlot;

		// Place somewhere in bag
		if(!IsValidSlot(Slot)) {

			// Search for a suitable slot
			Slot = FindSlotForItem(Item, Upgrades, 1);
			if(!IsValidSlot(Slot))
				return false;
		}
		// Trying to equip an item
		else if(Slot.BagType == _Bag::EQUIPMENT) {

			// Make sure it can be equipped
			if(!CanEquipItem(Slot.Index, Item))
				return false;
		}

		// Add item
		_InventorySlot &InventorySlot = GetSlot(Slot);
		if(Item->IsStackable() && InventorySlot.Item == Item && InventorySlot.Upgrades == Upgrades && InventorySlot.Count + 1 <= InventorySlot.MaxCount) {
			InventorySlot.Count += 1;
			Added = true;
		}
		else if(InventorySlot.Item == nullptr) {
			InventorySlot.Item = Item;
			InventorySlot.Upgrades = Upgrades;
			InventorySlot.Count = 1;
			Added = true;
		}
	}

	return Added;
}

// Moves the player's trade items to their bag
void _Inventory::MoveTradeToInventory() {

	_Bag &Bag = Bags[_Bag::TRADE];
	for(size_t i = 0; i < Bag.Slots.size(); i++) {
		if(Bag.Slots[i].Item && AddItem(Bag.Slots[i].Item, Bag.Slots[i].Upgrades, Bag.Slots[i].Count))
			Bag.Slots[i].Reset();
	}
}

// Splits an item stack
bool _Inventory::SplitStack(_Buffer &Data, const _Slot &Slot, int Count) {
	if(Slot.Index == NOSLOT || Slot.BagType != _Bag::BagType::INVENTORY)
		return false;

	_Bag &Bag = Bags[_Bag::BagType::INVENTORY];

	// Make sure stack is large enough
	_InventorySlot &SplitItem = GetSlot(Slot);
	if(SplitItem.Item && SplitItem.Count > Count) {

		// Get starting search position
		_Slot EmptySlot = Slot;

		// Find an empty slot or existing item starting from bag
		bool Found = false;
		for(size_t i = 0; i < Bag.Slots.size(); i++) {
			EmptySlot.Index++;
			if(EmptySlot.Index >= Bag.Slots.size())
				EmptySlot.Index = 0;

			_InventorySlot &Item = GetSlot(EmptySlot);
			if(Item.Item == nullptr || (Item.Item == SplitItem.Item && Item.Upgrades == SplitItem.Upgrades && Item.Count <= GetSlot(EmptySlot).MaxCount - Count)) {
				Found = true;
				break;
			}
		}

		// Split item
		if(Found && EmptySlot != Slot) {
			SplitItem.Count -= Count;
			AddItem(SplitItem.Item, SplitItem.Upgrades, Count, EmptySlot);

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
_Slot _Inventory::GetRequiredItemSlots(const _Trader *Trader, std::vector<_Slot> &RequiredItemSlots) {

	// Find a slot for the reward
	_Slot RewardItemSlot = FindSlotForItem(Trader->RewardItem, Trader->Upgrades, Trader->Count);

	// Go through required items
	for(size_t i = 0; i < Trader->Items.size(); i++) {
		const _Item *RequiredItem = Trader->Items[i].Item;
		int RequiredCount = Trader->Items[i].Count;
		RequiredItemSlots[i].BagType = _Bag::BagType::NONE;

		// Search for the required item
		for(auto &Bag : Bags) {
			if(Bag.ID == _Bag::BagType::TRADE)
				continue;

			for(size_t j = 0; j < Bag.Slots.size(); j++) {
				_InventorySlot &InventoryItem = Bag.Slots[j];
				if(InventoryItem.Item == RequiredItem && InventoryItem.Count >= RequiredCount) {
					RequiredItemSlots[i].BagType = Bag.ID;
					RequiredItemSlots[i].Index = j;
					break;
				}
			}
		}

		// Didn't find an item
		if(!IsValidSlot(RequiredItemSlots[i]))
			RewardItemSlot.BagType = _Bag::BagType::NONE;
	}

	return RewardItemSlot;
}

// Serialize a slot
void _InventorySlot::Serialize(_Buffer &Data) {
	if(Item) {
		Data.Write<uint32_t>(Item->ID);
		Data.Write<uint8_t>((uint8_t)Upgrades);
		Data.Write<uint8_t>((uint8_t)Count);
	}
	else
		Data.Write<uint32_t>(0);
}

// Unserialize a slot
void _InventorySlot::Unserialize(_Buffer &Data, const _Stats *Stats) {

	uint32_t ItemID = Data.Read<uint32_t>();
	if(ItemID) {
		Item = Stats->Items.at(ItemID);
		Upgrades = Data.Read<uint8_t>();
		Count = Data.Read<uint8_t>();
	}
	else
		Reset();
}

// Serialize a slot
void _Slot::Serialize(_Buffer &Data) const {
	Data.Write<uint8_t>(BagType);
	Data.Write<uint8_t>((uint8_t)Index);
}

// Unserialize a slot
void _Slot::Unserialize(_Buffer &Data) {
	BagType = (_Bag::BagType)Data.Read<uint8_t>();
	Index = Data.Read<uint8_t>();
	if(Index == (uint8_t)-1)
		Index = NOSLOT;
}

// Serialize bag
void _Bag::Serialize(_Buffer &Data) {

	// Get item count
	uint8_t ItemCount = 0;
	for(size_t i = 0; i < Slots.size(); i++) {
		if(Slots[i].Item)
			ItemCount++;
	}

	// Write items
	Data.Write<uint8_t>(ItemCount);
	for(size_t i = 0; i < Slots.size(); i++) {
		if(Slots[i].Item) {
			Data.Write<uint8_t>((uint8_t)i);
			Slots[i].Serialize(Data);
		}
	}
}

// Unserialize bag
void _Bag::Unserialize(_Buffer &Data, const _Stats *Stats) {

	// Reset inventory
	std::fill(Slots.begin(), Slots.end(), _InventorySlot());

	// Read items
	uint8_t ItemCount = Data.Read<uint8_t>();
	for(uint8_t i = 0; i < ItemCount; i++) {
		uint8_t Index = Data.Read<uint8_t>();
		Slots[Index].Unserialize(Data, Stats);
	}
}