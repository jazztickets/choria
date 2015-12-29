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
#include <objects/action.h>
#include <objects/object.h>
#include <objects/inventory.h>
#include <objects/statuseffect.h>
#include <objects/buff.h>
#include <objects/item.h>
#include <constants.h>
#include <stats.h>
#include <buffer.h>
#include <iostream>

// Serialize action
void _Action::Serialize(_Buffer &Data) {

	uint32_t ItemID = 0;
	if(Item)
		ItemID = Item->ID;

	Data.Write<uint32_t>(ItemID);
}

// Unserialize action
void _Action::Unserialize(_Buffer &Data, _Stats *Stats) {

	uint32_t ItemID = Data.Read<uint32_t>();

	Item = Stats->Items[ItemID];
}

// Resolve action
bool _Action::Resolve(_Buffer &Data, _Object *Source, ScopeType Scope) {

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

		if(ItemUsed->IsSkill() && Source->HasLearned(ItemUsed) && InventorySlot == -1) {

			ItemUsed->ApplyCost(Source->Scripting, ActionResult);
		}
		else {
			size_t Index;
			if(!ActionResult.Source.Object->Inventory->FindItem(ItemUsed, Index, (size_t)InventorySlot))
				return false;

			ActionResult.Source.Object->Inventory->DecrementItemCount(Index, -1);
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
	ActionResult.Source.Object->UpdateStats(ActionResult.Source);

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
		ActionResult.Target.Object = Target;

		// Update objects
		if(!SkillUnlocked)
			ItemUsed->Use(Source->Scripting, ActionResult);

		// Update target
		ActionResult.Target.Object->UpdateStats(ActionResult.Target);

		// Write stat changes
		ActionResult.Target.Serialize(Data);

		// Add buffs
		if(ActionResult.Buff) {
			_StatusEffect *StatusEffect = new _StatusEffect();
			StatusEffect->Buff = ActionResult.Buff;
			StatusEffect->Level = ActionResult.BuffLevel;
			StatusEffect->Count = ActionResult.BuffDuration;
			bool Added = ActionResult.Target.Object->AddStatusEffect(StatusEffect);

			// Write status effect
			StatusEffect->Serialize(Data);

			// Call buff's begin function
			_StatChange StatChange;
			StatChange.Object = ActionResult.Target.Object;
			StatusEffect->Buff->ExecuteScript(Source->Scripting, "Begin", ActionResult.BuffLevel, StatChange);

			// Update target
			ActionResult.Target.Object->UpdateStats(StatChange);

			// Write stat change from begin call
			StatChange.Serialize(Data);

			if(!Added)
				delete StatusEffect;
		}
		else
			Data.Write<uint32_t>(0);
	}

	// Reset object
	Source->TurnTimer = 0.0;
	Source->Action.Unset();
	Source->Targets.clear();

	return true;
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
	Buff(nullptr),
	BuffLevel(0),
	BuffDuration(0),
	Time(0.0),
	Timeout(HUD_ACTIONRESULT_TIMEOUT),
	Speed(HUD_ACTIONRESULT_SPEED),
	Scope(ScopeType::ALL) {
}
