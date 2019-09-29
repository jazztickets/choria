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
#include <objects/usable.h>
#include <ae/texture.h>
#include <glm/vec4.hpp>

// Forward Declarations
class _Object;
class _Stats;
struct _Vendor;
struct _Cursor;
struct _Slot;
struct _WeaponType;

// Classes
class _BaseItem : public _Usable {

	public:

		_BaseItem();

		void DrawTooltip(const glm::vec2 &Position, const _Object *Player, const _Cursor &Tooltip, const _Slot &CompareSlot) const override;

		bool IsConsumable() const override { return Type == ItemType::CONSUMABLE; }
		bool IsKey() const override { return Type == ItemType::KEY; }
		bool IsUnlockable() const override { return Type == ItemType::UNLOCKABLE; }
		bool IsEquippable() const override { return Type >= ItemType::HELMET && Type <= ItemType::AMULET; }
		bool IsStackable() const { return !IsEquippable(); }

		void GetEquipmentSlot(_Slot &Slot) const;

		int GetPrice(const _Vendor *Vendor, int QueryCount, bool Buy, int Upgrades=0) const override;
		int GetUpgradePrice(int Upgrades) const;

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

		ItemType Type;
		const _WeaponType *WeaponType;
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
		uint32_t ResistanceTypeID;
		int Resistance;
		bool Tradable;

	private:

		bool CheckRequirements(_Scripting *Scripting, _ActionResult &ActionResult) const override;
		glm::vec4 GetCompareColor(float ItemValue, float EquippedValue) const;

};

inline bool CompareItems(const _BaseItem *First, const _BaseItem *Second) {
	return First->Name < Second->Name;
}
