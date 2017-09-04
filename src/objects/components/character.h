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
struct _Vendor;
struct _Trader;
struct _MinigameType;
struct _Blacksmith;

// Structures
struct _Unlock {
	_Unlock() : Level(0) { }

	int Level;
};

// Classes
class _Character {

	public:

		_Character(_Object *Object);
		~_Character();

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

		// Battle
		void GenerateNextBattle();
		int GenerateDamage();

		// Actions
		void RefreshActionBarCount();
		bool GetActionFromActionBar(_Action &ReturnAction, size_t Slot);

		// Skills
		bool HasLearned(const _Item *Skill) const;
		int GetSkillPointsAvailable() const { return SkillPoints - SkillPointsUsed; }
		void AdjustSkillLevel(uint32_t SkillID, int Amount);

		// Status effects
		bool AddStatusEffect(_StatusEffect *StatusEffect);
		void DeleteStatusEffects();

		// Unlocks
		bool HasUnlocked(const _Item *Item) const;

		// Base
		_Object *Object;
		uint32_t CharacterID;
		double UpdateTimer;

		// Render
		const _Texture *StatusTexture;
		const _Texture *Portrait;
		uint32_t PortraitID;

		// State
		std::unordered_map<uint32_t, double> BattleCooldown;
		std::string PartyName;
		int Gold;
		int NextBattle;
		int Invisible;
		int Stunned;
		bool Hardcore;
		uint8_t Status;

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

		// Events
		const _Vendor *Vendor;
		const _Trader *Trader;
		const _Blacksmith *Blacksmith;
		const _MinigameType *Minigame;
		uint32_t Seed;

		// Trading
		_Object *TradePlayer;
		int TradeGold;
		bool WaitingForTrade;
		bool TradeAccepted;

		// Actions
		std::list<_Object *> Targets;
		_Action Action;

		// Map
		NetworkIDType LoadMapID;
		NetworkIDType SpawnMapID;
		uint32_t SpawnPoint;
		double TeleportTime;

		// HUD
		bool MenuOpen;
		bool InventoryOpen;
		bool SkillsOpen;

		// Bots
		bool Bot;
		std::list<void *> Path;

	private:

		void CalculateStatBonuses(_StatChange &StatChange);

};
