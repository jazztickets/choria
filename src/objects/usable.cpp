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
#include <objects/usable.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <objects/components/fighter.h>
#include <scripting.h>

//TODO fix
#include <objects/components/inventory.h>

// Constructor
_Usable::_Usable() :
	Texture(nullptr),
	NetworkID(0),
	Level(0),
	MaxLevel(0),
	Duration(0.0),
	AttackDelay(0.0),
	AttackTime(0.0),
	Cooldown(0.0),
	Target(TargetType::NONE),
	Scope(ScopeType::NONE),
	TargetAlive(true) {
}

// Destructor
_Usable::~_Usable() {

}

// Apply the cost
void _Usable::ApplyCost(_Scripting *Scripting, _ActionResult &ActionResult) const {
	if(Scripting->StartMethodCall(Script, "ApplyCost")) {
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushActionResult(&ActionResult);
		Scripting->MethodCall(2, 1);
		Scripting->GetActionResult(1, ActionResult);
		Scripting->FinishMethodCall();
	}
}

// Get passive stats
void _Usable::GetStats(_Scripting *Scripting, _ActionResult &ActionResult) const {
	if(Scripting->StartMethodCall(Script, "Stats")) {
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->PushStatChange(&ActionResult.Source);
		Scripting->MethodCall(3, 1);
		Scripting->GetStatChange(1, ActionResult.Source);
		Scripting->FinishMethodCall();
	}
}

// Return attack times from skill script. Return false if function doesn't exist.
bool _Usable::GetAttackTimes(_Scripting *Scripting, _Object *Object, double &AttackDelay, double &AttackTime, double &Cooldown) const {

	// Check script's function
	if(Scripting->StartMethodCall(Script, "GetAttackTimes")) {
		Scripting->PushObject(Object);
		Scripting->MethodCall(1, 3);
		AttackDelay = Scripting->GetReal(1);
		AttackTime = Scripting->GetReal(2);
		Cooldown = Scripting->GetReal(3);
		Scripting->FinishMethodCall();

		return true;
	}

	return false;
}

// Use an item
void _Usable::Use(_Scripting *Scripting, _ActionResult &ActionResult) const {
	if(Scripting->StartMethodCall(Script, "Use")) {
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushInt(ActionResult.ActionUsed.Duration);
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->PushObject(ActionResult.Target.Object);
		Scripting->PushActionResult(&ActionResult);
		Scripting->MethodCall(5, 1);
		Scripting->GetActionResult(1, ActionResult);
		Scripting->FinishMethodCall();
	}
}

// Play audio through scripting
void _Usable::PlaySound(_Scripting *Scripting) const {
	if(Scripting->StartMethodCall(Script, "PlaySound")) {
		Scripting->MethodCall(0, 0);
		Scripting->FinishMethodCall();
	}
}

// Check if the item can be used in the given scope
bool _Usable::CheckScope(ScopeType CheckScope) const {
	if(Scope == ScopeType::NONE || (Scope != ScopeType::ALL && Scope != CheckScope))
		return false;

	return true;
}

// Return true if the item can be used
bool _Usable::CanUse(_Scripting *Scripting, _ActionResult &ActionResult) const {
	_Object *Object = ActionResult.Source.Object;
	if(!Object)
		return false;

	// Unlocking skill for the first time
	if(IsSkill() && ActionResult.ActionUsed.InventorySlot != -1)
		return !Object->Character->HasLearned(AsSkill());

	// Check for item in key bag
	if(IsKey())
		return !Object->Inventory->GetBag(BagType::KEYS).HasItem(ID);

	// Unlocking item
	if(IsUnlockable())
		return !Object->Character->HasUnlocked(AsItem());

	// Check for item count
	if(!ActionResult.ActionUsed.Usable->IsSkill()) {
		size_t Index;
		if(!Object->Inventory->FindItem(ActionResult.ActionUsed.Usable->AsItem(), Index, (size_t)ActionResult.ActionUsed.InventorySlot))
			return false;
	}

	// Check scope
	if(!CheckScope(ActionResult.Scope))
		return false;

	// Check if target is alive
	if(Object->Character->Targets.size() == 1) {
		_Object *Target = *Object->Character->Targets.begin();
		if(!CanTarget(Object, Target))
			return false;
	}

	// Check script's function
	if(Scripting->StartMethodCall(Script, "CanUse")) {
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->MethodCall(2, 1);
		int Value = Scripting->GetBoolean(1);
		Scripting->FinishMethodCall();

		return Value;
	}

	return true;
}

// Check if an item can target an object
bool _Usable::CanTarget(_Object *SourceObject, _Object *TargetObject) const {
	if(TargetAlive && !TargetObject->Character->IsAlive())
		return false;

	if(!TargetAlive && TargetObject->Character->IsAlive())
		return false;

	if(SourceObject->Character->Battle) {

		if(SourceObject->Fighter->BattleSide == TargetObject->Fighter->BattleSide && !CanTargetAlly())
			return false;

		if(SourceObject->Fighter->BattleSide != TargetObject->Fighter->BattleSide && !CanTargetEnemy())
			return false;
	}

	return true;
}

// Get target count based on target type
int _Usable::GetTargetCount() const {

	int TargetCount = 0;
	switch(Target) {
		case TargetType::NONE:
		break;
		default:
			TargetCount = 1;
		break;
	}

	return TargetCount;
}
