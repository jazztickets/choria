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
#include <enums.h>
#include <string>
#include <cstdint>

// Forward Declarations
class _Scripting;
class _Object;
class _BaseItem;
class _Skill;
struct _ActionResult;
namespace ae {
	class _Texture;
}

// An object that goes into an action bar slot
class _Usable {

	public:

		_Usable();
		virtual ~_Usable();

		// Objects
		const _BaseItem *AsItem() const { return (const _BaseItem *)this; }
		const _Skill *AsSkill() const { return (const _Skill *)this; }
		virtual bool IsSkill() const { return false; }
		virtual bool IsConsumable() const { return false; }
		virtual bool IsKey() const { return false; }
		virtual bool IsUnlockable() const { return false; }

		// Scripts
//		virtual bool CanUse(_Scripting *Scripting, _ActionResult &ActionResult) const;
		bool CanUse(_Scripting *Scripting, _ActionResult &ActionResult) const;
		bool GetAttackTimes(_Scripting *Scripting, _Object *Object, double &AttackDelay, double &AttackTime, double &Cooldown) const;
		void GetStats(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void ApplyCost(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void Use(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void PlaySound(_Scripting *Scripting) const;
		bool CheckScope(ScopeType CheckScope) const;

		// Targetting
		bool UseMouseTargetting() const { return Target == TargetType::SELF || Target == TargetType::ENEMY || Target == TargetType::ALLY || Target == TargetType::ANY; }
		bool CanTarget(_Object *SourceObject, _Object *TargetObject) const;
		bool CanTargetEnemy() const {  return Target == TargetType::ENEMY || Target == TargetType::ANY; }
		bool CanTargetAlly() const {  return Target == TargetType::SELF || Target == TargetType::ALLY || Target == TargetType::ANY; }
		int GetTargetCount() const;

		std::string ID;
		std::string Name;
		std::string Script;
		const ae::_Texture *Texture;
		uint16_t NetworkID;
		int Level;
		int MaxLevel;
		double Duration;
		double AttackDelay;
		double AttackTime;
		double Cooldown;
		TargetType Target;
		ScopeType Scope;
		bool TargetAlive;

};
