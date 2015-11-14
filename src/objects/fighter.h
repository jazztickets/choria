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

// Constants
const int FIGHTER_MAXSKILLS = 8;

// Forward Declarations
class _Battle;
class _Skill;
class _Texture;
struct _FighterResult;

// Classes
class _Fighter {

	public:

		enum FighterType {
			TYPE_PLAYER,
			TYPE_MONSTER,
		};

		_Fighter(int TType);
		virtual ~_Fighter();

		// Object
		int GetType() const { return Type; }

		// Render
		void RenderBattle(bool TShowResults, float TTimerPercent, _FighterResult *TResult, bool TTarget);
		void SetOffset(const glm::ivec2 &TPosition) { Offset = TPosition; }

		// Stats
		void SetName(const std::string &TName) { Name = TName; }
		void SetHealth(int TValue) { Health = TValue; }
		void SetMaxHealth(int TValue) { MaxHealth = TValue; }
		void SetMana(int TValue) { Mana = TValue; }
		void SetMaxMana(int TValue) { MaxMana = TValue; }

		const std::string &GetName() const { return Name; }
		int GetHealth() const { return Health; }
		int GetMaxHealth() const { return MaxHealth; }
		int GetMana() const { return Mana; }
		int GetMaxMana() const { return MaxMana; }

		float GetHealthRegen() const { return HealthRegen; }
		float GetManaRegen() const { return ManaRegen; }
		float GetHealthAccumulator() const { return HealthAccumulator; }
		float GetManaAccumulator() const { return ManaAccumulator; }

		void UpdateHealth(int TValue);
		void UpdateMana(int TValue);
		void RestoreHealthMana();

		void UpdateRegen(int &THealthUpdate, int &TManaUpdate);
		void SetRegenAccumulators(float THealthAccumulator, float TManaAccumulator);

		int GetLevel() const { return Level; }

		// Battles
		_Battle *GetBattle();
		void SetCommand(int TCommand) { Command = TCommand; }
		virtual int GetCommand() { return Command; }
		void SetSkillUsed(const _Skill *TSkill) { SkillUsed = TSkill; }
		const _Skill *GetSkillUsed() const { return SkillUsed; }
		void SetSkillUsing(const _Skill *TSkill) { SkillUsing = TSkill; }
		const _Skill *GetSkillUsing() const { return SkillUsing; }
		void SetTarget(int TTarget) { Target = TTarget; }
		int GetTarget() const { return Target; }
		void SetSlot(int TSlot) { Slot = TSlot; }
		int GetSlot() const { return Slot; }
		int GetSide() const { return Slot & 1; }
		int GenerateDamage();
		int GenerateDefense();
		int GetMinDamage() const { return MinDamage; }
		int GetMaxDamage() const { return MaxDamage; }
		int GetMinDefense() const { return MinDefense; }
		int GetMaxDefense() const { return MaxDefense; }

		virtual int GetExperienceGiven() const { return 0; }
		virtual int GetGoldGiven() const { return 0; }

		virtual void UpdateExperience(int TValue) { }

		virtual int GetSkillLevel(int TSlot) const { return 1; }

		// Skills
		void SetSkillBar(int TSlot, const _Skill *TSkill) { SkillBar[TSlot] = TSkill; }
		const _Skill *GetSkillBar(int TSlot);
		int GetSkillBarID(int TSlot);

	protected:

		// Objects
		int Type;

		// Action bar
		const _Skill *SkillBar[FIGHTER_MAXSKILLS];

		// Stats
		std::string Name;
		int Level;
		int Health, MaxHealth;
		int Mana, MaxMana;
		int MinDamage, MaxDamage;
		int MinDefense, MaxDefense;
		float HealthRegen, ManaRegen, HealthAccumulator, ManaAccumulator;

		// Battle
		_Battle *Battle;
		int Command, Target, Slot;
		const _Skill *SkillUsing, *SkillUsed;

		// Render
		const _Texture *Portrait;
		glm::ivec2 Offset;
};
