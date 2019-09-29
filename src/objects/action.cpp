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
	uint16_t NetworkID = 0;
	if(Usable) {
		NetworkID = Usable->NetworkID;
		Data.Write<uint16_t>(NetworkID);
		Data.WriteBit(Usable->IsSkill());
	}
	else {
		Data.Write<uint16_t>(NetworkID);
	}
}

// Unserialize action
void _Action::Unserialize(ae::_Buffer &Data, const _Stats *Stats) {
	uint16_t ItemID = Data.Read<uint16_t>();
	bool IsSkill = false;
	if(ItemID) {
		IsSkill = Data.ReadBit();
		if(IsSkill)
			Usable = Stats->SkillsIndex.at(ItemID);
		else
			Usable = Stats->ItemsIndex.at(ItemID);
	}
	else
		Usable = nullptr;
}

// Resolve action
bool _Action::Start(_Object *Source, ScopeType Scope) {
	const _Usable *Usable = Source->Character->Action.Usable;
	if(!Usable)
		return false;

	// Check for deleted targets
	for(auto Iterator = Source->Character->Targets.begin(); Iterator != Source->Character->Targets.end(); ) {
		if((*Iterator)->Deleted)
			Iterator = Source->Character->Targets.erase(Iterator);
		else
			++Iterator;
	}

	ApplyTime = 0.0;
	Time = 0.0;

	// Build action result struct
	_ActionResult ActionResult;
	ActionResult.Source.Object = Source;
	ActionResult.Scope = Scope;
	ActionResult.ActionUsed = Source->Character->Action;

	double AttackDelay = 0.0;
	double AttackTime = 0.0;
	double Cooldown = 0.0;

	// Check use
	if(!Usable->CallCanUse(Source->Scripting, ActionResult))
		return false;

	// Get attack times
	AttackDelay = Usable->AttackDelay;
	AttackTime = Usable->AttackTime;

	// Get attack times from skill
	Usable->CallGetAttackTimes(Source->Scripting, Source, AttackDelay, AttackTime, Cooldown);
	if(Source->Character->Battle)
		ApplyTime = AttackDelay + AttackTime;

	// Apply cost of action
	ActionResultFlag ActionFlags = ActionResultFlag::NONE;
	if(!Usable->ApplyCost(ActionResult, ActionFlags))
		return false;

	// Set skill flag
	if(Usable->IsSkill())
		ActionFlags |= ActionResultFlag::SKILL;

	// Update stats
	Source->UpdateStats(ActionResult.Source);

	// Build packet for results
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::ACTION_START);
	Packet.Write<ActionResultFlag>(ActionFlags);

	// Write action used
	Packet.Write<uint16_t>(Usable->NetworkID);
	Packet.Write<uint8_t>(Source->Character->Action.InventorySlot);
	Packet.Write<float>(AttackDelay);
	Packet.Write<float>(AttackTime);

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

	// Update each target
	Data.Write<uint8_t>((uint8_t)Source->Character->Targets.size());
	for(auto &Target : Source->Character->Targets) {

		// Set objects
		ActionResult.Source.Reset();
		ActionResult.Source.Object = Source;
		ActionResult.Target.Object = Target;

		// Call Use script
		Source->Character->Action.Usable->CallUse(Source->Scripting, ActionResult);

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
		uint16_t SummonDatabaseID = ActionResult.Summon.ID;

		// Check for existing summon
		_Object *ExistingSummon = nullptr;
		int SideCount = 0;
		for(auto &Object : Battle->Objects) {
			if(Object->Fighter->BattleSide == SourceObject->Fighter->BattleSide) {
				if(Object->Monster->Owner == SourceObject && Object->Monster->MonsterStat->NetworkID == SummonDatabaseID) {
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
			Object->Stats = SourceObject->Stats;
			Object->Monster->MonsterStat = Object->Stats->MonstersIndex.at(SummonDatabaseID);
			Object->Monster->Owner = SourceObject;
			SourceObject->Stats->GetMonsterStats(Object->Monster->MonsterStat, Object);

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
	if(Usable)
		return Usable->Target;

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
