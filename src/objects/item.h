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
	EQUIPPABLE = RING,
	CONSUMABLE,
	TRADABLE,
};

// Classes
class _Item {

	public:

		void DrawTooltip(_Scripting *Scripting, const _Object *Player, const _Cursor &Tooltip) const;
		void DrawDescription(_Scripting *Scripting, glm::vec2 &DrawPosition, int DrawLevel, bool ShowLevel, float Width, float SpacingY) const;

		bool IsEquippable() const { return Type <= ItemType::EQUIPPABLE; }
		bool IsSkill() const { return Type == ItemType::SKILL; }

		void GetDamageRange(int &Min, int &Max) const;
		void GetDefenseRange(int &Min, int &Max) const;
		int GetPrice(const _Vendor *Vendor, int QueryCount, bool Buy) const;

		bool CanUse(_Scripting *Scripting, _ActionResult &ActionResult) const;
		bool CheckScope(ScopeType CheckScope) const;
		void ApplyCost(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void Use(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void Stats(_Scripting *Scripting, _ActionResult &ActionResult) const;

		uint32_t ID;
		std::string Name;
		std::string Script;
		int Level;
		ItemType Type;
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
