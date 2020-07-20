/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020  Alan Witkowski
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
#include <glm/vec2.hpp>

// Constants
const int TOOLTIP_SIDE_PADDING = 36;
const int TOOLTIP_HELP_SPACING = 28;
const int TOOLTIP_SPACING = 36;

// Forward Declarations
class _Scripting;
class _Object;
class _BaseItem;
class _BaseSkill;
class _Action;
struct _Slot;
struct _Cursor;
struct _Vendor;
struct _ActionResult;
namespace ae {
	class _Texture;
}

// Enumerations
enum class ActionResultFlag : uint8_t {
	NONE      = 0,
	SKILL     = 1 << 0,
	DECREMENT = 1 << 1,
	KEY       = 1 << 2,
	UNLOCK    = 1 << 3,
};

inline bool operator&(const ActionResultFlag &Left, const ActionResultFlag &Right) { return (uint8_t)Left & (uint8_t)Right; }
inline ActionResultFlag operator|(const ActionResultFlag &Left, const ActionResultFlag &Right) { return (ActionResultFlag)((uint8_t)Left | (uint8_t)Right); }
inline ActionResultFlag &operator|=(ActionResultFlag &Left, ActionResultFlag Right) { return Left = Left | Right; }

// An object that goes into an action bar slot. Either skill or item.
class _Usable {

	public:

		_Usable();
		virtual ~_Usable();

		// Objects
		const _BaseItem *AsItem() const { return (const _BaseItem *)this; }
		const _BaseSkill *AsSkill() const { return (const _BaseSkill *)this; }

		// Types
		virtual bool RequiresTarget() const { return true; }
		virtual bool IsSkill() const { return false; }
		virtual bool IsConsumable() const { return false; }
		virtual bool IsKey() const { return false; }
		virtual bool IsUnlockable() const { return false; }
		virtual bool IsEquippable() const { return false; }
		virtual bool IsDestroyable() const { return false; }

		// HUD
		void DrawTooltipBase(const glm::vec2 &Position, const _Object *Player, const std::string &TypeText, glm::vec2 &DrawPosition, glm::vec2 &Size) const;
		virtual void DrawTooltip(const glm::vec2 &Position, const _Object *Player, const _Cursor &Tooltip, const _Slot &CompareSlot) const { }
		virtual int GetPrice(const _Vendor *Vendor, int QueryCount, bool Buy, int Upgrades=0) const { return 0; }
		void DrawDescription(_Scripting *Scripting, glm::vec2 &DrawPosition, int DrawLevel, bool ShowLevel, float Width, float SpacingY) const;

		// Scripts
		bool CallCanUse(_Scripting *Scripting, _ActionResult &ActionResult) const;
		bool CallGetAttackTimesAdjust(_Scripting *Scripting, _Object *Object, double &AttackDelayAdjust, double &AttackTimeAdjust, double &CooldownAdjust) const;
		void CallStats(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void CallApplyCost(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void CallUse(_Scripting *Scripting, _ActionResult &ActionResult) const;
		void CallPlaySound(_Scripting *Scripting) const;

		// Requirements
		virtual bool ApplyCost(_ActionResult &ActionResult, ActionResultFlag &ResultFlags) const { return true; }
		virtual bool CanEquip(_Scripting *Scripting, _Object *Object) const;
		bool CheckScope(ScopeType CheckScope) const;

		// Targetting
		bool UseMouseTargetting() const { return Target != TargetType::NONE; }
		bool CanTarget(_Object *SourceObject, _Object *TargetObject) const;
		bool CanTargetEnemy() const {  return Target == TargetType::ENEMY || Target == TargetType::ANY; }
		bool CanTargetAlly() const {  return Target == TargetType::SELF || Target == TargetType::ALLY || Target == TargetType::ANY; }
		int GetTargetCount(_Scripting *Scripting, _Object *Object) const;

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
		float Stamina;
		TargetType Target;
		ScopeType Scope;
		bool TargetAlive;

	protected:

		virtual bool CheckRequirements(_Scripting *Scripting, _ActionResult &ActionResult) const { return true; }
};
