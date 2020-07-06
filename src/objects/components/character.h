/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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
#include <ae/type.h>
#include <objects/action.h>
#include <cstdint>
#include <vector>
#include <list>
#include <unordered_map>

// Forward Declarations
class _Object;
class _Stats;
class _Battle;
class _HUD;
class _StatusEffect;
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

		enum StatusImageType {
			STATUS_NONE,
			STATUS_MENU,
			STATUS_INVENTORY,
			STATUS_VENDOR,
			STATUS_SKILLS,
			STATUS_TRADE,
			STATUS_TRADER,
			STATUS_BLACKSMITH,
			STATUS_MINIGAME,
			STATUS_BATTLE,
			STATUS_TELEPORT,
			STATUS_DEAD,
		};

		_Character(_Object *Object);
		~_Character();

		// Updates
		void Update(double FrameTime);
		void UpdateHealth(int &Value);
		void UpdateMana(int Value);
		void UpdateGold(int Value);
		void UpdateExperience(int Value);
		void UpdateStatus();

		// Stats
		void CalculateStats();
		void CalculateLevelStats();
		float GetNextLevelPercent() const;
		bool IsAlive() const { return Health > 0; }
		float GetHealthPercent() const { return MaxHealth > 0 ? Health / (float)MaxHealth : 0; }
		float GetManaPercent() const { return MaxMana > 0 ? Mana / (float)MaxMana : 0; }

		// Input
		bool AcceptingMoveInput();
		bool CanOpenTrade() const { return IsAlive() && !Battle; }
		bool CanOpenSkills() const { return IsAlive() && !Battle; }
		bool CanOpenInventory() const { return IsAlive() && !Battle; }
		bool CanOpenParty() const { return IsAlive() && !Battle; }
		bool CanTeleport() const { return IsAlive() && !Battle; }
		bool CanBattle() const { return !Battle && IsAlive() && Status == STATUS_NONE && !Invisible; }
		bool CanPVP() const { return !Battle && IsAlive(); }

		// Battle
		void GenerateNextBattle();
		int GenerateDamage();
		float GetDamagePower(int DamageTypeID);
		void ClearSummonBuffs();

		// Actions
		void RefreshActionBarCount();
		bool GetActionFromActionBar(_Action &ReturnAction, size_t Slot);
		void GetSummonsFromBuffs(std::vector<std::pair<_Summon, _StatusEffect *> > &Summons);

		// Skills
		bool HasLearned(const _Item *Skill) const;
		int GetSkillPointsAvailable() const { return SkillPoints - SkillPointsUsed; }
		void AdjustSkillLevel(uint32_t SkillID, int Amount);

		// UI
		bool CanTrade() const;
		void ResetUIState();
		bool AddStatusEffect(_StatusEffect *StatusEffect);
		void DeleteStatusEffects();

		// Unlocks
		bool HasUnlocked(const _Item *Item) const;

		// Base
		_Object *Object;
		uint32_t CharacterID;
		uint32_t BuildID;
		double UpdateTimer;

		// Pointers
		_Battle *Battle;
		_HUD *HUD;

		// Render
		const ae::_Texture *StatusTexture;
		const ae::_Texture *Portrait;
		uint32_t PortraitID;

		// State
		std::unordered_map<uint32_t, double> BattleCooldown;
		std::string PartyName;
		int Gold;
		int NextBattle;
		int Invisible;
		int Stunned;
		bool DiagonalMovement;
		bool Hardcore;
		uint8_t Status;

		// Records
		double PlayTime;
		double BattleTime;
		int Deaths;
		int MonsterKills;
		int PlayerKills;
		int GamesPlayed;
		int Bounty;
		int GoldLost;
		int Rebirths;

		// Rebirths
		int EternalStrength;
		int EternalGuard;
		int EternalFortitude;
		int EternalSpirit;
		int EternalWisdom;
		int EternalWealth;
		int EternalAlacrity;
		int EternalKnowledge;

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
		float BaseManaReductionRatio;
		float BaseHealthUpdateMultiplier;
		float BaseAttackPower;
		int BaseMinDamage;
		int BaseMaxDamage;
		int BaseArmor;
		int BaseDamageBlock;
		int BasePierce;
		int BaseMoveSpeed;
		int BaseBattleSpeed;
		double BaseAttackPeriod;
		int BaseEvasion;
		int BaseHitChance;
		int BaseAllSkills;

		// Final attributes
		int Health;
		int MaxHealth;
		int Mana;
		int MaxMana;
		int HealthRegen;
		int ManaRegen;
		float ExperienceMultiplier;
		float GoldMultiplier;
		float MaxHealthMultiplier;
		float MaxManaMultiplier;
		float ManaReductionRatio;
		float HealthUpdateMultiplier;
		float AttackPower;
		float PhysicalPower;
		float FirePower;
		float ColdPower;
		float LightningPower;
		float BleedPower;
		float PoisonPower;
		float PetPower;
		float HealPower;
		float ManaPower;
		int MinDamage;
		int MaxDamage;
		int Armor;
		int DamageBlock;
		int Pierce;
		int MoveSpeed;
		int BattleSpeed;
		int EquipmentBattleSpeed;
		int Evasion;
		int HitChance;
		int AllSkills;
		int SummonLimit;
		std::unordered_map<uint32_t, int> Resistances;

		// Status effects
		std::list<_StatusEffect *> StatusEffects;

		// Unlocks
		std::unordered_map<uint32_t, _Unlock> Unlocks;

		// Skills
		std::unordered_map<uint32_t, int> Skills;
		int SkillPoints;
		int SkillPointsUnlocked;
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
		ae::NetworkIDType LoadMapID;
		ae::NetworkIDType SpawnMapID;
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
