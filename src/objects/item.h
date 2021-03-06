/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2021 Alan Witkowski
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
	OFFHAND,
	MAP,
	RELIC,
};

// Classes
class _Item {

	public:

		static int64_t GetEnchantCost(_Object *Source, int Level);

		int GetAttributeCount(int Upgrades) const;
		void DrawTooltip(const glm::vec2 &Offset, _Object *Player, const _Cursor &Tooltip, const _Slot &CompareSlot) const;
		int DrawDescription(bool Render, _Object *Object, const std::string &Function, glm::vec2 &DrawPosition, bool Blacksmith, int DrawLevel, int PlayerMaxLevelSkill, int EnchanterMaxLevel, int Upgrades, bool ShowLevel, float Width, float SpacingY) const;
		int DrawSetDescription(bool Render, _Object *Object, glm::vec2 &DrawPosition, bool Blacksmith, float Width, float SpacingY) const;

		bool IsSkill() const { return Type == ItemType::SKILL; }
		bool IsConsumable() const { return Type == ItemType::CONSUMABLE; }
		bool IsKey() const { return Type == ItemType::KEY; }
		bool IsUnlockable() const { return Type == ItemType::UNLOCKABLE; }
		bool IsEquippable() const { return Type == ItemType::OFFHAND || Type == ItemType::RELIC || (Type >= ItemType::HELMET && Type <= ItemType::AMULET); }
		bool IsStackable() const { return !IsEquippable(); }
		bool IsCursed() const { return Attributes.at("Cursed").Int; }
		bool UseMouseTargetting() const { return TargetID == TargetType::SELF || TargetID == TargetType::ENEMY || TargetID == TargetType::ALLY || TargetID == TargetType::ANY || TargetID == TargetType::ENEMY_CORPSE_AOE; }
		bool CanTargetEnemy() const {  return TargetID == TargetType::ENEMY || TargetID == TargetType::ENEMY_ALL || TargetID == TargetType::ANY || TargetID == TargetType::ENEMY_CORPSE_AOE; }
		bool CanTargetAlly() const {  return TargetID == TargetType::SELF || TargetID == TargetType::ALLY || TargetID == TargetType::ALLY_ALL || TargetID == TargetType::ANY; }
		int GetTargetCount(_Scripting *Scripting, _Object *Object, bool InitialTarget=true) const;

		void GetEquipmentSlot(_Slot &Slot) const;

		int64_t GetPrice(_Scripting *Scripting, _Object *Source, const _Vendor *Vendor, int QueryCount, bool Buy, int Level=0) const;
		int64_t GetUpgradeCost(_Object *Source, int Level) const;

		bool CanUse(_Scripting *Scripting, _ActionResult &ActionResult) const;
		bool CanTarget(_Scripting *Scripting, _Object *Source, _Object *Target, bool ForceTargetAlive=false) const;
		bool CheckScope(ScopeType CheckScope) const;
		void ApplyCost(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void Use(_Scripting *Scripting, _ActionResult &ActionResult, int Priority=0) const;
		void GetStats(_Scripting *Scripting, _ActionResult &ActionResult, int SetLevel, int MaxSetLevel) const;
		void PlaySound(_Scripting *Scripting) const;

		float GetAttribute(const std::string &Name, int Upgrades) const;
		float GetAverageDamage(int Upgrades) const;
		float GetCooldownReduction(int Upgrades) const;
		template<typename T> T GetUpgradedValue(const std::string &AttributeName, int Upgrades, T Value) const;

		const _Stats *Stats;

		uint32_t ID;
		std::string Name;
		std::string Script;
		std::string Proc;
		const ae::_Texture *Texture;
		const ae::_Texture *AltTexture;
		ItemType Type;
		bool BulkBuy;
		int Category;
		int Level;
		int MaxLevel;
		int Increase;
		double Duration;
		double Cooldown;
		int64_t Cost;
		uint32_t DamageTypeID;
		uint32_t SetID;
		std::unordered_map<std::string, _Value> Attributes;
		int Chance;
		uint32_t ResistanceTypeID;
		int Tradable;
		bool TargetAlive;
		TargetType TargetID;
		ScopeType Scope;
		uint32_t UnlockID;

	private:

		glm::vec4 GetCompareColor(float ItemValue, float EquippedValue) const;

};

inline bool CompareItems(const _Item *First, const _Item *Second) {
	return First->Category < Second->Category || (First->Category == Second->Category && First->Name < Second->Name);
}
