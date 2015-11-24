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
#include <objects/object.h>
#include <texture.h>
#include <vector>

// Constants
const int FIGHTER_MAXSKILLS = 8;

// Forward Declarations
class _Battle;
class _Skill;
class _Texture;
struct _FighterResult;

// Classes
class _Fighter : public _Object {

	public:

		_Fighter(int Type);
		virtual ~_Fighter();

		// Render
		void RenderBattle(bool ShowResults, float TimerPercent, _FighterResult *Result, bool IsTarget);

		// Stats
		void UpdateHealth(int Value);
		void UpdateMana(int Value);
		void RestoreHealthMana();

		void UpdateRegen(int &HealthUpdate, int &ManaUpdate);

		// Battles
		int GetCommand();
		int GetSide() const { return BattleSlot & 1; }
		int GenerateDamage();
		int GenerateDefense();
		void UpdateTarget(const std::vector<_Fighter *> &Fighters);

		virtual int GetExperienceGiven() const { return ExperienceGiven; }
		virtual int GetGoldGiven() const { return GoldGiven; }

		virtual void UpdateExperience(int Value) { }
		virtual int GetSkillLevel(int Slot) const { return 1; }

		// Skills
		void SetSkillBar(int Slot, const _Skill *Skill) { SkillBar[Slot] = Skill; }
		const _Skill *GetSkillBar(int Slot);
		int GetSkillBarID(int Slot);

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
		int Command, Target, BattleSlot;
		const _Skill *SkillUsing, *SkillUsed;

		// Render
		const _Texture *Portrait;
		glm::ivec2 Offset;

		// Monster
		std::vector<_Fighter *> Opponents;
		int ID;
		int ExperienceGiven, GoldGiven;
		int AI;

	protected:

};
