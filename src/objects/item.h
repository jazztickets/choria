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
#include <objects/action.h>

// Forward Declarations
class _Object;
class _Scripting;
class _Stats;
struct _Vendor;
struct _Cursor;
struct _Slot;

namespace ae {
	class _Texture;
}

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
	AMULET,
	CONSUMABLE,
	TRADABLE,
	UNLOCKABLE,
	KEY,
};

// Classes
class _Item {

	public:

		void DrawTooltip(const glm::vec2 &Offset, _Scripting *Scripting, const _Object *Player, const _Cursor &Tooltip, const _Slot &CompareSlot) const;
		void DrawDescription(_Scripting *Scripting, glm::vec2 &DrawPosition, int DrawLevel, bool ShowLevel, float Width, float SpacingY) const;

		bool IsSkill() const { return Type == ItemType::SKILL; }
		bool IsConsumable() const { return Type == ItemType::CONSUMABLE; }
		bool IsKey() const { return Type == ItemType::KEY; }
		bool IsUnlockable() const { return Type == ItemType::UNLOCKABLE; }
		bool IsEquippable() const { return Type >= ItemType::HELMET && Type <= ItemType::AMULET; }
		bool IsStackable() const { return !IsEquippable(); }
		bool UseMouseTargetting() const { return TargetID == TargetType::SELF || TargetID == TargetType::ENEMY || TargetID == TargetType::ALLY || TargetID == TargetType::ENEMY_MULTI || TargetID == TargetType::ALLY_MULTI || TargetID == TargetType::ANY; }
		bool CanTargetEnemy() const {  return TargetID == TargetType::ENEMY || TargetID == TargetType::ENEMY_MULTI || TargetID == TargetType::ENEMY_ALL || TargetID == TargetType::ANY; }
		bool CanTargetAlly() const {  return TargetID == TargetType::SELF || TargetID == TargetType::ALLY || TargetID == TargetType::ALLY_MULTI || TargetID == TargetType::ALLY_ALL || TargetID == TargetType::ANY; }
		int GetTargetCount(_Scripting *Scripting, _Object *Object) const;

		void GetEquipmentSlot(_Slot &Slot) const;

		int GetPrice(const _Vendor *Vendor, int QueryCount, bool Buy, int Level=0) const;
		int GetUpgradePrice(int Level) const;

		bool CanUse(_Scripting *Scripting, _ActionResult &ActionResult) const;
		bool CanTarget(_Object *Source, _Object *Target) const;
		bool CheckScope(ScopeType CheckScope) const;
		void ApplyCost(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void Use(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void GetStats(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void PlaySound(_Scripting *Scripting) const;

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
		float GetBattleSpeed(int Upgrades) const;
		float GetMoveSpeed(int Upgrades) const;
		float GetResistance(int Upgrades) const;
		float GetDropRate(int Upgrades) const;
		float GetAllSkills(int Upgrades) const;
		template<typename T> T GetUpgradedValue(StatType Type, int Upgrades, T Value) const;

		const _Stats *Stats;

		uint32_t ID;
		std::string Name;
		std::string Script;
		const ae::_Texture *Texture;
		ItemType Type;
		int Category;
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
		int AllSkills;
		uint32_t ResistanceTypeID;
		int Resistance;
		bool Tradable;
		bool TargetAlive;
		TargetType TargetID;
		ScopeType Scope;
		uint32_t UnlockID;

	private:

		glm::vec4 GetCompareColor(float ItemValue, float EquippedValue) const;

};

inline bool CompareItems(const _Item *First, const _Item *Second) {
	return First->Name < Second->Name;
}
