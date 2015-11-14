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
#pragma once

// Libraries
#include <texture.h>

// Forward Declarations
struct _Vendor;

// Classes
class _Item {
	friend class _Stats;

	public:

		enum Type {
			TYPE_HEAD,
			TYPE_BODY,
			TYPE_LEGS,
			TYPE_WEAPON1HAND,
			TYPE_WEAPON2HAND,
			TYPE_SHIELD,
			TYPE_RING,
			TYPE_POTION,
			TYPE_TRADE,
		};

		int GetID() const { return ID; }
		int GetType() const { return Type; }
		const _Texture *GetImage() const { return Image; }
		const std::string &GetName() const { return Name; }
		int GetLevel() const { return Level; }
		int GetHealthRestore() const { return HealthRestore; }
		int GetManaRestore() const { return ManaRestore; }
		int GetMaxHealth() const { return MaxHealth; }
		int GetMaxMana() const { return MaxMana; }
		float GetHealthRegen() const { return HealthRegen; }
		float GetManaRegen() const { return ManaRegen; }
		int GetInvisPower() const { return InvisPower; }

		bool IsHealthPotion() const { return Type == TYPE_POTION && HealthRestore > 0; }
		bool IsManaPotion() const { return Type == TYPE_POTION && ManaRestore > 0; }
		bool IsInvisPotion() const { return Type == TYPE_POTION && InvisPower > 0; }
		bool IsPotionType(int TType) const { return (TType == 0 && IsHealthPotion()) || (TType == 1 && IsManaPotion()); }

		void GetDamageRange(int &TMin, int &TMax) const;
		void GetDefenseRange(int &TMin, int &TMax) const;
		void GetType(std::string &TString) const;
		int GetPrice(const _Vendor *TVendor, int TCount, bool TBuy) const;

	private:

		int ID;
		std::string Name;
		int Level;
		int Type;
		const _Texture *Image;
		int LevelRequired;
		int Cost;
		float Damage;
		float DamageRange;
		float Defense;
		float DefenseRange;
		int DamageType;
		int HealthRestore;
		int ManaRestore;
		int MaxHealth;
		int MaxMana;
		float HealthRegen;
		float ManaRegen;
		int InvisPower;
};
