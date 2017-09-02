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
#include <objects/action.h>
#include <objects/object.h>
#include <objects/components/inventory.h>
#include <objects/statuseffect.h>
#include <objects/buff.h>
#include <objects/item.h>
#include <objects/battle.h>
#include <ae/buffer.h>
#include <packet.h>
#include <constants.h>
#include <server.h>
#include <stats.h>
#include <iostream>

// Serialize action
void _Action::Serialize(_Buffer &Data) {

	uint32_t ItemID = 0;
	if(Item)
		ItemID = Item->ID;

	Data.Write<uint32_t>(ItemID);
}

// Unserialize action
void _Action::Unserialize(_Buffer &Data, const _Stats *Stats) {

	uint32_t ItemID = Data.Read<uint32_t>();

	Item = Stats->Items.at(ItemID);
}

// Resolve action
bool _Action::Resolve(_Buffer &Data, _Object *Source, ScopeType Scope) {

	// Check for deleted targets
	for(auto Iterator = Source->Targets.begin(); Iterator != Source->Targets.end(); ) {
		if((*Iterator)->Deleted)
			Iterator = Source->Targets.erase(Iterator);
		else
			++Iterator;
	}

	// Build action result struct
	_ActionResult ActionResult;
	ActionResult.Source.Object = Source;
	ActionResult.Scope = Scope;
	ActionResult.ActionUsed = Source->Action;
	const _Item *ItemUsed = Source->Action.Item;
	bool SkillUnlocked = false;
	bool ItemUnlocked = false;
	bool DecrementItem = false;

	// Use item
	if(ItemUsed) {
		if(!ItemUsed->CanUse(Source->Scripting, ActionResult))
			return false;

		// Apply skill cost
		if(ItemUsed->IsSkill() && Source->HasLearned(ItemUsed) && InventorySlot == -1) {

			ItemUsed->ApplyCost(Source->Scripting, ActionResult);
		}
		// Apply item cost
		else {
			size_t Index;
			if(!Source->Inventory->FindItem(ItemUsed, Index, (size_t)InventorySlot))
				return false;

			Source->Inventory->DecrementItemCount(_Slot(_Bag::BagType::INVENTORY, Index), -1);
			DecrementItem = true;
			if(ItemUsed->IsSkill()) {
				Source->Skills[ItemUsed->ID] = 0;
				SkillUnlocked = true;
			}
			else if(ItemUsed->IsUnlockable()) {
				Source->Unlocks[ItemUsed->UnlockID].Level = 1;
				ItemUnlocked = true;
			}
		}
	}

	// Update stats
	Source->UpdateStats(ActionResult.Source);
	Source->TurnTimer = 0.0;

	// Build packet for results
	Data.Write<PacketType>(PacketType::ACTION_RESULTS);
	Data.WriteBit(DecrementItem);
	Data.WriteBit(SkillUnlocked);
	Data.WriteBit(ItemUnlocked);

	// Write action used
	uint32_t ItemID = ItemUsed ? ItemUsed->ID : 0;
	Data.Write<uint32_t>(ItemID);
	Data.Write<char>((char)ActionResult.ActionUsed.InventorySlot);

	// Write source updates
	ActionResult.Source.Serialize(Data);

	// Update each target
	Data.Write<uint8_t>((uint8_t)Source->Targets.size());
	for(auto &Target : Source->Targets) {

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
	Source->Action.Unset();
	Source->Targets.clear();

	return true;
}

// Handle summon actions
void _Action::HandleSummons(_ActionResult &ActionResult) {
	if(!ActionResult.Summon.ID)
		return;

	_Object *Object = ActionResult.Source.Object;
	_Battle *Battle = Object->Battle;
	if(Battle) {

		// Get database id
		uint32_t SummonDatabaseID = ActionResult.Summon.ID;

		// Check for existing summon
		_Object *ExistingSummon = nullptr;
		int SideCount = 0;
		for(auto &Fighter : Battle->Fighters) {
			if(Fighter->BattleSide == Object->BattleSide) {
				if(Fighter->Owner == Object && Fighter->DatabaseID == SummonDatabaseID) {
					ExistingSummon = Fighter;
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

			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::STAT_CHANGE);
			Heal.Serialize(Packet);
			Battle->BroadcastPacket(Packet);
		}
		else if(SideCount < BATTLE_MAXFIGHTERS_SIDE) {

			// Create monster
			_Object *Monster = Object->Server->ObjectManager->Create();
			Monster->Server = Object->Server;
			Monster->Scripting = Object->Scripting;
			Monster->DatabaseID = SummonDatabaseID;
			Monster->Stats = Object->Stats;
			Monster->Owner = Object;
			Object->Stats->GetMonsterStats(Monster->DatabaseID, Monster);

			// Add stats from script
			Monster->Character->Health = Monster->Character->BaseMaxHealth = ActionResult.Summon.Health;
			Monster->Character->Mana = Monster->Character->BaseMaxMana = ActionResult.Summon.Mana;
			Monster->Character->BaseMinDamage = ActionResult.Summon.MinDamage;
			Monster->Character->BaseMaxDamage = ActionResult.Summon.MaxDamage;
			Monster->Character->BaseArmor = ActionResult.Summon.Armor;
			Monster->CalculateStats();

			// Create packet for new object
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::WORLD_CREATEOBJECT);
			Monster->SerializeCreate(Packet);
			Battle->BroadcastPacket(Packet);

			// Add monster to battle
			Battle->AddFighter(Monster, 0, true);
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
	LastPosition(0, 0),
	Position(0, 0),
	Texture(nullptr),
	Time(0.0),
	Timeout(HUD_ACTIONRESULT_TIMEOUT),
	Speed(HUD_ACTIONRESULT_SPEED),
	Miss(false),
	Scope(ScopeType::ALL) {
}
