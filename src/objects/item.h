/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
#include <objects/action.h>
#include <texture.h>

// Forward Declarations
class _Object;
class _Texture;
class _Scripting;
class _Stats;
struct _Vendor;
struct _Cursor;

enum class ItemType : uint32_t {
	NONE,
	SKILL,
	HELMET,
	ARMOR,
	BOOTS,
	ONEHANDED_WEAPON,
	TWOHANDED_WEAPON,
	SHIELD,
	RING,
	CONSUMABLE,
	TRADABLE,
	UNLOCKABLE,
};

// Classes
class _Item {

	public:

		void DrawTooltip(const glm::vec2 &Offset, _Scripting *Scripting, const _Object *Player, const _Cursor &Tooltip) const;
		void DrawDescription(_Scripting *Scripting, glm::vec2 &DrawPosition, int DrawLevel, bool ShowLevel, float Width, float SpacingY) const;

		bool IsSkill() const { return Type == ItemType::SKILL; }
		bool IsUnlockable() const { return Type == ItemType::UNLOCKABLE; }
		bool IsEquippable() const { return Type >= ItemType::HELMET && Type <= ItemType::RING; }
		bool IsStackable() const { return !IsEquippable(); }
		bool UseMouseTargetting() const { return TargetID == TargetType::SELF || TargetID == TargetType::ENEMY || TargetID == TargetType::ALLY || TargetID == TargetType::ENEMY_MULTI || TargetID == TargetType::ALLY_MULTI || TargetID == TargetType::ANY; }
		bool CanTargetEnemy() const {  return TargetID == TargetType::ENEMY || TargetID == TargetType::ENEMY_MULTI || TargetID == TargetType::ENEMY_ALL || TargetID == TargetType::ANY; }
		bool CanTargetAlly() const {  return TargetID == TargetType::SELF || TargetID == TargetType::ALLY || TargetID == TargetType::ALLY_MULTI || TargetID == TargetType::ALLY_ALL || TargetID == TargetType::ANY; }
		int GetTargetCount() const;

		size_t GetEquipmentSlot() const;

		int GetPrice(const _Vendor *Vendor, int QueryCount, bool Buy) const;
		int GetUpgradePrice(int Level) const;

		bool CanUse(_Scripting *Scripting, _ActionResult &ActionResult) const;
		bool CanTarget(_Object *Source, _Object *Target) const;
		bool CheckScope(ScopeType CheckScope) const;
		void ApplyCost(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void Use(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void GetStats(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void PlaySound(_Scripting *Scripting) const;

		int GetMinDamage(int Upgrades) const;
		int GetMaxDamage(int Upgrades) const;
		int GetArmor(int Upgrades) const;
		int GetDamageBlock(int Upgrades) const;
		int GetMaxHealth(int Upgrades) const;
		int GetMaxMana(int Upgrades) const;
		int GetHealthRegen(int Upgrades) const;
		int GetManaRegen(int Upgrades) const;
		int GetBattleSpeed(int Upgrades) const;
		int GetMoveSpeed(int Upgrades) const;
		int GetResistance(int Upgrades) const;
		template<typename T> T GetUpgradedValue(StatType Type, int Upgrades, T Value) const;

		_Stats *Stats;

		uint32_t ID;
		std::string Name;
		std::string Script;
		const _Texture *Texture;
		ItemType Type;
		int Level;
		int MaxLevel;
		int LevelRequired;
		int Cost;
		uint32_t DamageTypeID;
		int MinDamage;
		int MaxDamage;
		int Armor;
		int DamageBlock;
		int MaxHealth;
		int MaxMana;
		int HealthRegen;
		int ManaRegen;
		int BattleSpeed;
		int MoveSpeed;
		uint32_t ResistanceTypeID;
		int Resistance;
		bool Tradable;
		bool TargetAlive;
		TargetType TargetID;
		ScopeType Scope;
		uint32_t UnlockID;

	private:

};

inline bool CompareItems(const _Item *First, const _Item *Second) {
	return First->Name < Second->Name;
}