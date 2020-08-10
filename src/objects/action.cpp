/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020 Alan Witkowski
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
#include <ae/random.h>
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
bool _Action::Resolve(ae::_Buffer &Data, _Object *Source, ScopeType Scope) {

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

			// Handle different item types
			int ConsumeRoll = 0;
			if(ItemUsed->IsSkill()) {

				// Learn skill
				Source->Character->Skills[ItemUsed->ID] = 0;
				Source->Character->MaxSkillLevels[ItemUsed->ID] = GAME_DEFAULT_MAX_SKILL_LEVEL;
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
			else {
				if(Source->Character->Battle)
					ConsumeRoll = ae::GetRandomInt(1, 100);
			}

			// Roll to consume item
			if(ConsumeRoll <= Source->Character->ConsumeChance) {
				Source->Inventory->UpdateItemCount(_Slot(BagType::INVENTORY, Index), -1);
				DecrementItem = true;
			}
		}

		// Set cooldown
		if(!SkillUnlocked && ItemUsed->Cooldown > 0.0) {
			Source->Character->Cooldowns[ItemUsed->ID].Duration = ItemUsed->Cooldown * Source->Character->CooldownMultiplier;
			Source->Character->Cooldowns[ItemUsed->ID].MaxDuration = ItemUsed->Cooldown * Source->Character->CooldownMultiplier;
		}
	}

	// Update stats
	Source->UpdateStats(ActionResult.Source);
	Source->Fighter->TurnTimer = 0.0;

	// Build packet for results
	Data.Write<PacketType>(PacketType::ACTION_RESULTS);
	Data.WriteBit(DecrementItem);
	Data.WriteBit(SkillUnlocked);
	Data.WriteBit(ItemUnlocked);
	Data.WriteBit(KeyUnlocked);

	// Write action used
	uint32_t ItemID = ItemUsed ? ItemUsed->ID : 0;
	Data.Write<uint32_t>(ItemID);
	Data.Write<char>((char)ActionResult.ActionUsed.InventorySlot);

	// Write source updates
	ActionResult.Source.Serialize(Data);

	// Update each target
	Data.Write<uint8_t>((uint8_t)Source->Character->Targets.size());
	for(auto &Target : Source->Character->Targets) {

		// Set objects
		ActionResult.Source.Reset();
		ActionResult.Target.Reset();
		ActionResult.Source.Object = Source;
		ActionResult.Target.Object = Target;

		// Call Use script
		if(!SkillUnlocked)
			ItemUsed->Use(Source->Scripting, ActionResult);

		// Update objects
		ActionResult.Source.Object->UpdateStats(ActionResult.Source);
		ActionResult.Target.Object->UpdateStats(ActionResult.Target, ActionResult.Source.Object);
		HandleSummons(ActionResult);

		// Write stat changes
		ActionResult.Source.Serialize(Data);
		ActionResult.Target.Serialize(Data);
	}

	// Reset object
	Source->Character->Action.Unset();
	Source->Character->Targets.clear();

	// Remove object from battle
	if(Source->Fighter->FleeBattle) {
		Source->Character->Battle->RemoveObject(Source);
		Source->Fighter->FleeBattle = false;
	}

	Source->Character->IdleTime = 0;

	return true;
}

// Handle summon actions
void _Action::HandleSummons(_ActionResult &ActionResult) {
	if(!ActionResult.Summon.ID)
		return;

	_Object *SourceObject = ActionResult.Source.Object;
	_Battle *Battle = SourceObject->Character->Battle;
	if(Battle) {

		// Check for existing summons and get count of fighters on player's side
		std::vector<_Object *>ExistingSummons;
		ExistingSummons.reserve(BATTLE_MAX_OBJECTS_PER_SIDE);
		int SideCount = 0;
		for(auto &Object : Battle->Objects) {
			if(Object->Fighter->BattleSide == SourceObject->Fighter->BattleSide) {
				if(Object->Monster->Owner == SourceObject && Object->Monster->SpellID == ActionResult.Summon.SpellID)
					ExistingSummons.push_back(Object);

				SideCount++;
			}
		}

		// Create new summon if below limit
		if(SideCount < BATTLE_MAX_OBJECTS_PER_SIDE && (int)ExistingSummons.size() < ActionResult.Summon.Limit) {

			// Create object
			_Object *Object = SourceObject->Server->CreateSummon(SourceObject, ActionResult.Summon);

			// Create packet for new object
			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::WORLD_CREATEOBJECT);
			Object->SerializeCreate(Packet);
			Battle->BroadcastPacket(Packet);

			// Add monster to battle
			Battle->AddObject(Object, SourceObject->Fighter->BattleSide, true);
		}
		// Heal existing summon if already at limit
		else if(ExistingSummons.size()) {

			// Get lowest health summon
			int LowestHealth = ExistingSummons[0]->Character->Health;
			_Object *LowestHealthSummon = ExistingSummons[0];
			for(auto &ExistingSummon : ExistingSummons) {
				if(ExistingSummon->Character->Health < LowestHealth) {
					LowestHealth = ExistingSummon->Character->Health;
					LowestHealthSummon = ExistingSummon;
				}
			}

			_StatChange Heal;
			Heal.Object = LowestHealthSummon;
			Heal.Values["Health"].Integer = LowestHealthSummon->Character->MaxHealth;
			LowestHealthSummon->UpdateStats(Heal);

			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::STAT_CHANGE);
			Heal.Serialize(Packet);
			Battle->BroadcastPacket(Packet);
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
	Miss(false),
	Scope(ScopeType::ALL) {
}
