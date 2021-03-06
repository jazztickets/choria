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
struct _Enchanter;

// Structures
struct _Unlock {
	_Unlock() : Level(0) { }

	int Level;
};

struct _SetData {
	_SetData() : EquippedCount(0), MaxLevel(0), Level(0) { }

	int EquippedCount;
	int MaxLevel;
	int Level;
};

struct _Cooldown {
	_Cooldown() : Duration(0), MaxDuration(0) { }

	double Duration;
	double MaxDuration;
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

		// Initialize
		void Init();

		// Updates
		void Update(double FrameTime);
		void UpdateHealth(int &Value);
		void UpdateMana(int Value);
		void UpdateGold(int64_t Value);
		void UpdateExperience(int64_t Value);
		void UpdateAllResist(int Value);
		void UpdateElementalResist(int Value);

		// Stats
		void CalculateStats();
		void CalculateLevelStats();
		float GetNextLevelPercent() const;
		bool IsAlive() const { return Attributes.at("Health").Int > 0; }
		float GetHealthPercent() const { return Attributes.at("MaxHealth").Int > 0 ? Attributes.at("Health").Int / (float)Attributes.at("MaxHealth").Int : 0; }
		float GetManaPercent() const { return Attributes.at("MaxMana").Int > 0 ? Attributes.at("Mana").Int / (float)Attributes.at("MaxMana").Int : 0; }
		int64_t GetItemCost(int64_t ItemCost);

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
		bool IsZoneOnCooldown(uint32_t Zone) { return BossCooldowns.find(Zone) != BossCooldowns.end(); }
		void GenerateNextBattle();
		int GenerateDamage();
		float GetAverageDamage() const { return (Attributes.at("MinDamage").Int + Attributes.at("MaxDamage").Int) / 2.0f; }
		float GetDamagePowerMultiplier(int DamageTypeID);

		// Actions
		ScopeType GetScope() { return Battle ? ScopeType::BATTLE : ScopeType::WORLD; }
		void RefreshActionBarCount();
		bool GetActionFromActionBar(_Action &ReturnAction, std::size_t Slot);
		void GetSummonsFromBuffs(std::vector<std::pair<_Summon, _StatusEffect *> > &Summons);
		bool CanEquipSkill(const _Item *Skill);

		// Skills
		bool HasLearned(const _Item *Skill) const;
		int GetSkillPointsAvailable() const { return SkillPoints - SkillPointsUsed; }
		void AdjustSkillLevel(uint32_t SkillID, int Amount, bool SoftMax);
		void AdjustMaxSkillLevel(uint32_t SkillID, int Amount);
		void InitializeSkillLevels();

		// UI
		bool IsTrading() { return WaitingForTrade || TradePlayer; }
		bool CanTrade() const;
		void ResetUIState(bool ResetMenuState=true);
		bool AddStatusEffect(_StatusEffect *StatusEffect);
		void DeleteStatusEffects();
		uint8_t GetStatus();
		void UpdateStatusTexture();

		// Unlocks
		void ClearUnlocks();
		int UnlockBySearch(const std::string &Search, int Count);
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
		std::unordered_map<uint32_t, double> BossCooldowns;
		std::unordered_map<uint32_t, int> BossKills;
		std::string PartyName;
		double IdleTime;
		int NextBattle;
		int Invisible;
		bool Hardcore;
		bool Offline;
		uint8_t Status;

		// Levels
		bool CalcLevelStats;
		int Level;
		int64_t ExperienceNeeded;
		int64_t ExperienceNextLevel;

		// Base attributes
		int BaseMaxHealth;
		int BaseMaxMana;
		int BaseMinDamage;
		int BaseMaxDamage;
		int BaseArmor;
		int BaseDamageBlock;
		int BaseBattleSpeed;
		int BaseSpellDamage;
		double BaseAttackPeriod;

		// Final attributes
		std::unordered_map<std::string, _Value> Attributes;
		std::unordered_map<std::string, int> BaseResistances;
		std::unordered_map<uint32_t, _SetData> Sets;
		int BattleSpeedBeforeBuffs;

		// Status effects
		std::list<_StatusEffect *> StatusEffects;

		// Unlocks
		std::unordered_map<uint32_t, _Unlock> Unlocks;

		// Skills
		std::unordered_map<uint32_t, int> Skills;
		std::unordered_map<uint32_t, int> MaxSkillLevels;
		std::unordered_map<uint32_t, _Cooldown> Cooldowns;
		int SkillPoints;
		int SkillPointsUnlocked;
		int SkillPointsUsed;
		int SkillPointsOnActionBar;

		// Action bar
		std::vector<_Action> ActionBar;
		int BeltSize;
		int SkillBarSize;

		// Events
		const _Vendor *Vendor;
		const _Trader *Trader;
		const _Blacksmith *Blacksmith;
		const _Enchanter *Enchanter;
		const _MinigameType *Minigame;
		uint32_t Seed;

		// Trading
		_Object *TradePlayer;
		int64_t TradeGold;
		bool WaitingForTrade;
		bool TradeAccepted;

		// Actions
		std::vector<_Object *> Targets;
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
