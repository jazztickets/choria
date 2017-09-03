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
#include <objects/action.h>
#include <cstdint>
#include <vector>
#include <unordered_map>

// Forward Declarations
class _Object;
class _Stats;

// Structures
struct _Unlock {
	_Unlock() : Level(0) { }

	int Level;
};

// Classes
class _Character {

	public:

		_Character(_Object *Object);

		// Updates
		void Update(double FrameTime);
		void UpdateHealth(int &Value);
		void UpdateMana(int Value);
		void UpdateGold(int Value);
		void UpdateExperience(int Value);

		// Stats
		void CalculateStats();
		void CalculateLevelStats();
		float GetNextLevelPercent() const;
		bool IsAlive() const { return Health > 0; }
		float GetHealthPercent() const { return MaxHealth > 0 ? Health / (float)MaxHealth : 0; }
		float GetManaPercent() const { return MaxMana > 0 ? Mana / (float)MaxMana : 0; }

		// Actions
		void RefreshActionBarCount();
		bool GetActionFromActionBar(_Action &ReturnAction, size_t Slot);

		// Skills
		bool HasLearned(const _Item *Skill) const;
		int GetSkillPointsAvailable() const { return SkillPoints - SkillPointsUsed; }
		void AdjustSkillLevel(uint32_t SkillID, int Amount);

		// Unlocks
		bool HasUnlocked(const _Item *Item) const;

		// Base
		_Object *Object;
		uint32_t CharacterID;
		double UpdateTimer;

		// Gold
		int Gold;

		// State
		int Invisible;
		int Stunned;
		bool Hardcore;

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

		// Status effects
		std::list<_StatusEffect *> StatusEffects;

		// Unlocks
		std::unordered_map<uint32_t, _Unlock> Unlocks;

		// Skills
		std::unordered_map<uint32_t, int> Skills;
		int SkillPoints;
		int SkillPointsUsed;
		int SkillPointsOnActionBar;

		// Action bar
		std::vector<_Action> ActionBar;

	private:

		void CalculateStatBonuses(_StatChange &StatChange);

};
