/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#include <cstdint>
#include <unordered_map>

// Forward Declarations
class _Object;
class _Stats;

// Classes
class _Fighter {

	public:

		_Fighter(_Object *Object);

		void Update(double FrameTime);
		void CalculateLevelStats(const _Stats *Stats);

		// Base
		_Object *Object;
		double UpdateTimer;

		// Levels
		bool CalcLevelStats;
		int Level;
		int Experience;
		int ExperienceNeeded;
		int ExperienceNextLevel;

		// Base attributes
		int BaseMaxHealth;
		int BaseMaxMana;
		int BaseHealthRegen;
		int BaseManaRegen;
		float BaseHealPower;
		float BaseAttackPower;
		int BaseMinDamage;
		int BaseMaxDamage;
		int BaseArmor;
		int BaseDamageBlock;
		int BaseMoveSpeed;
		int BaseBattleSpeed;
		int BaseEvasion;
		int BaseHitChance;
		int BaseDropRate;

		// Final attributes
		int Health;
		int MaxHealth;
		int Mana;
		int MaxMana;
		int HealthRegen;
		int ManaRegen;
		float HealPower;
		float AttackPower;
		int MinDamage;
		int MaxDamage;
		int Armor;
		int DamageBlock;
		int MoveSpeed;
		int BattleSpeed;
		int Evasion;
		int HitChance;
		int DropRate;
		std::unordered_map<uint32_t, int> Resistances;

		// Skills
		int SkillPoints;
		int SkillPointsUsed;
		int SkillPointsOnActionBar;

	private:

};
