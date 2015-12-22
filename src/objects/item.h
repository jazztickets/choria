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
#include <objects/action.h>
#include <texture.h>

// Forward Declarations
class _Object;
class _Texture;
class _Scripting;
struct _Vendor;
struct _Cursor;

// Classes
class _Item {

	public:

		enum Type {
			TYPE_NONE,
			TYPE_SKILL,
			TYPE_HEAD,
			TYPE_BODY,
			TYPE_LEGS,
			TYPE_WEAPON1HAND,
			TYPE_WEAPON2HAND,
			TYPE_SHIELD,
			TYPE_RING,
			TYPE_EQUIPPABLE = TYPE_RING,
			TYPE_POTION,
			TYPE_TRADE,
		};

		void DrawTooltip(_Scripting *Scripting, const _Object *Player, const _Cursor &Tooltip) const;

		bool IsEquippable() const { return Type <= TYPE_EQUIPPABLE; }

		void GetDamageRange(int &Min, int &Max) const;
		void GetDefenseRange(int &Min, int &Max) const;
		void GetType(std::string &String) const;
		int GetPrice(const _Vendor *Vendor, int QueryCount, bool Buy) const;

		bool CanUse(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void ApplyCost(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void Use(_Scripting *Scripting, _ActionResult &ActionResult) const;

		uint32_t ID;
		std::string Name;
		std::string Script;
		int Level;
		int Type;
		const _Texture *Texture;
		int LevelRequired;
		int Cost;
		float Damage;
		float DamageRange;
		float Defense;
		float DefenseRange;
		int DamageType;
		int MaxHealth;
		int MaxMana;
		float HealthRegen;
		float ManaRegen;
		TargetType TargetID;
		bool TargetAlive;
		ScopeType Scope;

	private:

};
