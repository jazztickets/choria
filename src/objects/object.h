/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
#include <objects/managerbase.h>
#include <objects/action.h>
#include <glm/vec2.hpp>
#include <packet.h>
#include <unordered_map>
#include <list>
#include <vector>
#include <cstdint>

// Forward Declarations
class _Map;
class _Peer;
class _Texture;
class _Battle;
class _Buff;
class _Inventory;
class _Stats;
class _Server;
class _Buffer;
class _Scripting;
class _StatChange;
class _StatusEffect;
class _Element;
class _HUD;
struct _Tile;
struct _Vendor;
struct _Trader;
struct _ActionResult;

struct _Unlock {
	_Unlock() : Level(0) { }

	int Level;
};

// Classes
class _Object : public _ManagerBase {

	public:

		enum StatusImageType {
			STATUS_NONE,
			STATUS_PAUSE,
			STATUS_INVENTORY,
			STATUS_VENDOR,
			STATUS_SKILLS,
			STATUS_TRADE,
			STATUS_TRADER,
			STATUS_BATTLE,
			STATUS_TELEPORT,
			STATUS_DEAD,
		};

		enum MoveDirectionType {
			MOVE_UP    = (1 << 0),
			MOVE_DOWN  = (1 << 1),
			MOVE_LEFT  = (1 << 2),
			MOVE_RIGHT = (1 << 3),
		};

		_Object();
		~_Object();

		bool operator<(const _Object &Object) { return this->NetworkID < Object.NetworkID; }

		void Update(double FrameTime) override;
		void Render(const _Object *ClientPlayer=nullptr);
		void RenderBattle(_Object *ClientPlayer, double Time);

		// Network
		void SerializeCreate(_Buffer &Data);
		void SerializeUpdate(_Buffer &Data);
		void SerializeStats(_Buffer &Data);
		void SerializeBattle(_Buffer &Data);
		void UnserializeCreate(_Buffer &Data);
		void UnserializeStats(_Buffer &Data);
		void UnserializeBattle(_Buffer &Data);

		// Stats
		bool IsAlive() const { return Health > 0.001f; }
		float GetHealthPercent() const { return MaxHealth > 0 ? Health / (float)MaxHealth : 0; }
		float GetManaPercent() const { return MaxMana > 0 ? Mana / (float)MaxMana : 0; }
		_StatusEffect *UpdateStats(_StatChange &StatChange);
		void UpdateHealth(float &Value);
		void UpdateMana(float Value);
		void CalculateStats();
		float GetNextLevelPercent() const;
		void UpdateGold(int Value);
		void UpdateExperience(int Value);
		void ApplyDeathPenalty();

		// Battles
		void UpdateAI(const std::list<_Object *> &Fighters, double FrameTime);
		int GenerateDamage();
		void CreateBattleElement(_Element *Parent);
		void RemoveBattleElement();
		bool CanBattle() const;
		void GenerateNextBattle();
		void StopBattle();

		// Status effects
		bool AddStatusEffect(_StatusEffect *StatusEffect);
		void ResolveBuff(_StatusEffect *StatusEffect, const std::string &Function);

		// Actions
		void RefreshActionBarCount();
		bool GetActionFromSkillbar(_Action &ReturnAction, size_t Slot);
		void SetActionUsing(_Buffer &Data, _Manager<_Object> *ObjectManager);

		// Movement
		bool AcceptingMoveInput();
		void GetDirectionFromInput(int InputState, glm::ivec2 &Direction);
		int Move();

		// Skills
		bool HasLearned(const _Item *Skill) const;
		bool HasUnlocked(const _Item *Item) const;
		int GetSkillPointsRemaining() const { return SkillPoints - SkillPointsUsed; }
		void AdjustSkillLevel(uint32_t SkillID, int Amount);

		// Trader
		void AcceptTrader(std::vector<size_t> &Slots);

		// Map
		const _Tile *GetTile();
		NetworkIDType GetMapID() const;

		// Input
		bool CanOpenTrade() { return IsAlive() && !Battle; }
		bool CanOpenSkills() { return IsAlive() && !Battle; }
		bool CanOpenInventory() { return IsAlive() && !Battle; }
		bool CanTeleport() { return IsAlive() && !Battle; }

		// UI
		void ResetUIState();

		// Client
		_Stats *Stats;
		_Map *Map;
		_HUD *HUD;
		_Scripting *Scripting;
		_Server *Server;
		_Peer *Peer;
		std::list<int> InputStates;
		int Moved;
		bool WaitForServer;
		bool CheckEvent;
		bool Paused;
		double MoveTime;
		std::string ClientMessage;
		glm::ivec2 Position;
		glm::ivec2 ServerPosition;

		// Action bar
		std::vector<_Action>ActionBar;

		// Base stats
		float BaseMaxHealth;
		float BaseMaxMana;
		float BaseHealthRegen;
		float BaseManaRegen;
		float BaseHealPower;
		int BaseMinDamage;
		int BaseMaxDamage;
		int BaseArmor;
		int BaseDamageBlock;
		float BaseMoveSpeed;
		double BaseBattleSpeed;
		float BaseEvasion;
		float BaseHitChance;

		// Stats
		double UpdateTimer;
		bool CalcLevelStats;
		std::unordered_map<uint32_t, _Unlock> Unlocks;

		// Final stats
		std::string Name;
		int Level;
		float Health;
		float MaxHealth;
		float Mana;
		float MaxMana;
		float HealthRegen;
		float ManaRegen;
		float HealPower;
		int MinDamage;
		int MaxDamage;
		int Armor;
		int DamageBlock;
		float MoveSpeed;
		double BattleSpeed;
		float Evasion;
		float HitChance;
		std::unordered_map<uint32_t, float> Resistances;

		// Player stats
		double PlayTime;
		int Deaths;
		int MonsterKills;
		int PlayerKills;
		int Bounty;
		int Gold;
		int Experience;
		int ExperienceNeeded;
		int ExperienceNextLevel;

		// Battle
		_Battle *Battle;
		_Element *BattleElement;
		double TurnTimer;
		double AITimer;
		uint8_t BattleSide;
		_Action PotentialAction;
		std::list<uint32_t> ItemDropsReceived;
		std::list<_StatusEffect *> StatusEffects;

		// Actions
		std::list<_Object *> Targets;
		_Object *LastTarget;
		_Action Action;

		// Render
		const _Texture *Portrait;
		glm::vec2 BattleOffset;
		glm::vec2 ResultPosition;
		glm::vec2 StatPosition;

		// Monster
		uint32_t DatabaseID;
		int ExperienceGiven;
		int GoldGiven;
		std::string AI;

		// Account
		uint32_t CharacterID;

		// Texture
		uint8_t Status;
		uint32_t PortraitID;
		const _Texture *WorldTexture;
		const _Texture *StatusTexture;

		// Map
		NetworkIDType LoadMapID;
		NetworkIDType SpawnMapID;
		uint32_t SpawnPoint;
		double TeleportTime;

		// Battle
		int NextBattle;
		double AttackPlayerTime;
		int Invisible;

		// Items
		bool InventoryOpen;
		_Inventory *Inventory;
		const _Vendor *Vendor;
		const _Trader *Trader;

		// Skills
		bool SkillsOpen;
		std::unordered_map<uint32_t, int32_t> Skills;
		int SkillPoints;
		int SkillPointsUsed;
		int SkillPointsOnActionBar;

		// Trading
		int TradeGold;
		bool WaitingForTrade;
		bool TradeAccepted;
		_Object *TradePlayer;

	private:

		void DeleteStatusEffects();

		void CalculateStatBonuses(_StatChange &StatChange);
		void CalculateLevelStats();

		void SendPacket(_Buffer &Packet);

};
