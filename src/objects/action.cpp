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
#include <objects/action.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <objects/components/inventory.h>
#include <objects/components/fighter.h>
#include <objects/components/monster.h>
#include <objects/statuseffect.h>
#include <objects/buff.h>
#include <objects/item.h>
#include <objects/battle.h>
#include <ae/manager.h>
#include <ae/buffer.h>
#include <packet.h>
#include <constants.h>
#include <server.h>
#include <stats.h>
#include <iostream>

// Serialize action
void _Action::Serialize(ae::_Buffer &Data) {
	uint32_t ItemID = 0;
	if(Item)
		ItemID = Item->ID;

	Data.Write<uint32_t>(ItemID);
}

// Unserialize action
void _Action::Unserialize(ae::_Buffer &Data, const _Stats *Stats) {
	uint32_t ItemID = Data.Read<uint32_t>();

	Item = Stats->Items.at(ItemID);
}

// Resolve action
bool _Action::Start(_Object *Source, ScopeType Scope) {
	ApplyTime = 0.0;
	Time = 0.0;

	// Check for deleted targets
	for(auto Iterator = Source->Character->Targets.begin(); Iterator != Source->Character->Targets.end(); ) {
		if((*Iterator)->Deleted)
			Iterator = Source->Character->Targets.erase(Iterator);
		else
			++Iterator;
	}

	// Build action result struct
	_ActionResult ActionResult;
	ActionResult.Source.Object = Source;
	ActionResult.Scope = Scope;
	ActionResult.ActionUsed = Source->Character->Action;
	const _Item *ItemUsed = Source->Character->Action.Item;
	bool SkillUnlocked = false;
	bool ItemUnlocked = false;
	bool KeyUnlocked = false;
	bool DecrementItem = false;

	// Use item
	if(ItemUsed) {
		if(!ItemUsed->CanUse(Source->Scripting, ActionResult))
			return false;

		// Apply skill cost
		if(ItemUsed->IsSkill() && Source->Character->HasLearned(ItemUsed) && InventorySlot == -1) {

			ItemUsed->ApplyCost(Source->Scripting, ActionResult);
		}
		// Apply item cost
		else {
			size_t Index;
			if(!Source->Inventory->FindItem(ItemUsed, Index, (size_t)InventorySlot))
				return false;

			Source->Inventory->UpdateItemCount(_Slot(BagType::INVENTORY, Index), -1);
			DecrementItem = true;
			if(ItemUsed->IsSkill()) {
				Source->Character->Skills[ItemUsed->ID] = 0;
				SkillUnlocked = true;
			}
			else if(ItemUsed->IsUnlockable()) {
				Source->Character->Unlocks[ItemUsed->UnlockID].Level = 1;
				ItemUnlocked = true;
			}
			else if(ItemUsed->IsKey()) {
				Source->Inventory->GetBag(BagType::KEYS).Slots.push_back(_InventorySlot(ItemUsed, 1));
				KeyUnlocked = true;
			}
		}
	}

	// Update stats
	Source->UpdateStats(ActionResult.Source);

	// Build packet for results
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::ACTION_START);
	Packet.WriteBit(DecrementItem);
	Packet.WriteBit(SkillUnlocked);
	Packet.WriteBit(ItemUnlocked);
	Packet.WriteBit(KeyUnlocked);

	//TODO fix
	double ReactTime = 0.5;
	double FlyTime = 0.3;
	if(Source->Character->Battle)
		ApplyTime = ReactTime + FlyTime;

	// Write action used
	uint32_t ItemID = ItemUsed ? ItemUsed->ID : 0;
	Packet.Write<uint32_t>(ItemID);
	Packet.Write<char>((char)Source->Character->Action.InventorySlot);
	Packet.Write<float>(ReactTime);
	Packet.Write<float>(FlyTime);

	// Write source updates
	ActionResult.Source.Serialize(Packet);

	// Send list of targets
	Packet.Write<uint8_t>((uint8_t)Source->Character->Targets.size());
	for(auto &Target : Source->Character->Targets) {
		Packet.Write<ae::NetworkIDType>(Target->NetworkID);
	}

	// Send packet
	Source->SendPacket(Packet);

	return true;
}

// Apply action after animation
bool _Action::Apply(ae::_Buffer &Data, _Object *Source, ScopeType Scope) {

	// Create action result
	_ActionResult ActionResult;
	ActionResult.Source.Object = Source;
	ActionResult.Scope = Scope;
	ActionResult.ActionUsed = Source->Character->Action;

	// Get item used
	const _Item *ItemUsed = Source->Character->Action.Item;

	// Unlock skill
	bool SkillUnlocked = false;
	if(ItemUsed->IsSkill()) {
		Source->Character->Skills[ItemUsed->ID] = 0;
		SkillUnlocked = true;
	}

	// Update each target
	Data.Write<uint8_t>((uint8_t)Source->Character->Targets.size());
	for(auto &Target : Source->Character->Targets) {

		// Set objects
		ActionResult.Source.Reset();
		ActionResult.Source.Object = Source;
		ActionResult.Target.Object = Target;

		// Call Use script
		if(!SkillUnlocked)
			ItemUsed->Use(Source->Scripting, ActionResult);

		// Update objects
		ActionResult.Source.Object->UpdateStats(ActionResult.Source);
		ActionResult.Target.Object->UpdateStats(ActionResult.Target);
		HandleSummons(ActionResult);

		// Write stat changes
		ActionResult.Source.Serialize(Data);
		ActionResult.Target.Serialize(Data);
	}

	// Reset object
	Source->Character->Action.Unset();
	Source->Character->Targets.clear();

	return true;
}

// Handle summon actions
void _Action::HandleSummons(_ActionResult &ActionResult) {
	if(!ActionResult.Summon.ID)
		return;

	_Object *SourceObject = ActionResult.Source.Object;
	_Battle *Battle = SourceObject->Character->Battle;
	if(Battle) {

		// Get database id
		uint32_t SummonDatabaseID = ActionResult.Summon.ID;

		// Check for existing summon
		_Object *ExistingSummon = nullptr;
		int SideCount = 0;
		for(auto &Object : Battle->Objects) {
			if(Object->Fighter->BattleSide == SourceObject->Fighter->BattleSide) {
				if(Object->Monster->Owner == SourceObject && Object->Monster->DatabaseID == SummonDatabaseID) {
					ExistingSummon = Object;
				}

				SideCount++;
			}
		}

		// Heal summon
		if(ExistingSummon) {
			_StatChange Heal;
			Heal.Object = ExistingSummon;
			Heal.Values[StatType::HEALTH].Integer = ExistingSummon->Character->MaxHealth;
			ExistingSummon->UpdateStats(Heal);

			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::STAT_CHANGE);
			Heal.Serialize(Packet);
			Battle->BroadcastPacket(Packet);
		}
		else if(SideCount < BATTLE_MAX_OBJECTS_PER_SIDE) {

			// Create monster
			_Object *Object = SourceObject->Server->ObjectManager->Create();
			Object->Server = SourceObject->Server;
			Object->Scripting = SourceObject->Scripting;
			Object->Monster->DatabaseID = SummonDatabaseID;
			Object->Stats = SourceObject->Stats;
			Object->Monster->Owner = SourceObject;
			SourceObject->Stats->GetMonsterStats(Object->Monster->DatabaseID, Object);

			// Add stats from script
			Object->Character->Health = Object->Character->BaseMaxHealth = ActionResult.Summon.Health;
			Object->Character->Mana = Object->Character->BaseMaxMana = ActionResult.Summon.Mana;
			Object->Character->BaseMinDamage = ActionResult.Summon.MinDamage;
			Object->Character->BaseMaxDamage = ActionResult.Summon.MaxDamage;
			Object->Character->BaseArmor = ActionResult.Summon.Armor;
			Object->Character->CalculateStats();

			// Create packet for new object
			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::WORLD_CREATEOBJECT);
			Object->SerializeCreate(Packet);
			Battle->BroadcastPacket(Packet);

			// Add monster to battle
			Battle->AddObject(Object, 0, true);
		}
	}
}

// Return target type of action used
TargetType _Action::GetTargetType() {
	if(Item)
		return Item->TargetID;

	return TargetType::NONE;
}

// Constructor
_ActionResult::_ActionResult() :
	LastPosition(0.0f, 0.0f),
	Position(0.0f, 0.0f),
	Texture(nullptr),
	Time(0.0),
	Timeout(HUD_ACTIONRESULT_TIMEOUT),
	Speed(HUD_ACTIONRESULT_SPEED),
	Scope(ScopeType::ALL) {
}
