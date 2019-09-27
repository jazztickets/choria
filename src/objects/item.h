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
#include <ae/texture.h>
#include <enums.h>
#include <glm/vec4.hpp>

// Forward Declarations
class _Object;
class _Scripting;
class _Stats;
struct _Vendor;
struct _Cursor;
struct _Slot;
struct _ActionResult;
struct _WeaponType;

namespace ae {
	class _Texture;
}

// Classes
class _BaseItem {

	public:

		_BaseItem();

		void DrawTooltip(const glm::vec2 &Offset, _Scripting *Scripting, const _Object *Player, const _Cursor &Tooltip, const _Slot &CompareSlot) const;
		void DrawDescription(_Scripting *Scripting, glm::vec2 &DrawPosition, int DrawLevel, bool ShowLevel, float Width, float SpacingY) const;

		bool IsSkill() const { return Type == ItemType::SKILL; }
		bool IsConsumable() const { return Type == ItemType::CONSUMABLE; }
		bool IsKey() const { return Type == ItemType::KEY; }
		bool IsUnlockable() const { return Type == ItemType::UNLOCKABLE; }
		bool IsEquippable() const { return Type >= ItemType::HELMET && Type <= ItemType::AMULET; }
		bool IsStackable() const { return !IsEquippable(); }
		bool UseMouseTargetting() const { return Target == TargetType::SELF || Target == TargetType::ENEMY || Target == TargetType::ALLY || Target == TargetType::MULTIPLE_ENEMIES || Target == TargetType::MULTIPLE_ALLIES || Target == TargetType::ANY; }
		bool CanTargetEnemy() const {  return Target == TargetType::ENEMY || Target == TargetType::MULTIPLE_ENEMIES || Target == TargetType::ALL_ENEMIES || Target == TargetType::ANY; }
		bool CanTargetAlly() const {  return Target == TargetType::SELF || Target == TargetType::ALLY || Target == TargetType::MULTIPLE_ALLIES || Target == TargetType::ALL_ALLIES || Target == TargetType::ANY; }
		int GetTargetCount() const;

		void GetEquipmentSlot(_Slot &Slot) const;

		int GetPrice(const _Vendor *Vendor, int QueryCount, bool Buy, int Level=0) const;
		int GetUpgradePrice(int Level) const;

		// Scripts
		bool CanUse(_Scripting *Scripting, _ActionResult &ActionResult) const;
		bool CanTarget(_Object *Source, _Object *Target) const;
		bool CheckScope(ScopeType CheckScope) const;
		void ApplyCost(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void Use(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void GetStats(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void PlaySound(_Scripting *Scripting) const;
		bool GetAttackTimes(_Scripting *Scripting, _Object *Object, double &AttackDelay, double &AttackTime, double &Cooldown) const;

		float GetAverageDamage(int Upgrades) const;
		float GetMinDamage(int Upgrades) const;
		float GetMaxDamage(int Upgrades) const;
		float GetArmor(int Upgrades) const;
		float GetDamageBlock(int Upgrades) const;
		float GetPierce(int Upgrades) const;
		float GetMaxHealth(int Upgrades) const;
		float GetMaxMana(int Upgrades) const;
		float GetHealthRegen(int Upgrades) const;
		float GetManaRegen(int Upgrades) const;
		float GetMoveSpeed(int Upgrades) const;
		float GetResistance(int Upgrades) const;
		float GetDropRate(int Upgrades) const;
		template<typename T> T GetUpgradedValue(StatType Type, int Upgrades, T Value) const;

		const _Stats *Stats;

		std::string ID;
		uint16_t NetworkID;
		std::string Name;
		std::string Script;
		const ae::_Texture *Texture;
		ItemType Type;
		const _WeaponType *WeaponType;
		int Level;
		int MaxLevel;
		double Duration;
		int Cost;
		uint32_t DamageTypeID;
		int MinDamage;
		int MaxDamage;
		int Armor;
		int DamageBlock;
		int Pierce;
		int MaxHealth;
		int MaxMana;
		int HealthRegen;
		int ManaRegen;
		int BattleSpeed;
		int MoveSpeed;
		int DropRate;
		double AttackDelay;
		double AttackTime;
		double Cooldown;
		uint32_t ResistanceTypeID;
		int Resistance;
		bool Tradable;
		bool TargetAlive;
		TargetType Target;
		ScopeType Scope;
		uint32_t UnlockID;

	private:

		glm::vec4 GetCompareColor(float ItemValue, float EquippedValue) const;

};

inline bool CompareItems(const _BaseItem *First, const _BaseItem *Second) {
	return First->Name < Second->Name;
}
