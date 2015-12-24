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
		};

		enum MoveDirectionType {
			MOVE_UP    = (1 << 0),
			MOVE_DOWN  = (1 << 1),
			MOVE_LEFT  = (1 << 2),
			MOVE_RIGHT = (1 << 3),
		};

		_Object();
		~_Object();

		void Update(double FrameTime) override;
		void OnDelete() override;
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
		void UpdateStats(_StatChange &StatChange);
		void UpdateHealth(int Value);
		void UpdateMana(int Value);
		void RestoreHealthMana();
		void CalculateStats();
		float GetNextLevelPercent() const;
		void UpdateGold(int Value);

		// Battles
		void UpdateAI(const std::list<_Object *> &Fighters, double FrameTime);
		int GenerateDamage();
		int GenerateDefense();
		void CreateBattleElement(_Element *Parent);
		void RemoveBattleElement();
		bool CanBattle();
		void GenerateNextBattle();
		void StopBattle();

		// Status effects
		bool AddStatusEffect(_StatusEffect *StatusEffect);

		// Actions
		void RefreshActionBarCount();
		bool GetActionFromSkillbar(_Action &ReturnAction, size_t Slot);
		void SetActionUsing(_Buffer &Data, _Manager<_Object> *ObjectManager);

		// Movement
		bool AcceptingMoveInput();
		bool CanMove();
		int Move();

		// Skills
		bool HasLearned(const _Item *Skill);
		int GetSkillPointsRemaining() const { return SkillPoints - SkillPointsUsed; }
		void AdjustSkillLevel(uint32_t SkillID, int Adjust);
		void CalculateSkillPoints();

		// Trader
		void AcceptTrader(std::vector<size_t> &Slots);

		// Map
		const _Tile *GetTile();

		// PVP
		bool CanAttackPlayer();
		void ResetAttackPlayerTime() { AttackPlayerTime = 0; }

		_Map *Map;
		_HUD *HUD;
		_Scripting *Scripting;
		_Server *Server;
		_Peer *Peer;
		int InputState;
		int Moved;
		bool WaitForServer;
		glm::ivec2 Position;
		glm::ivec2 ServerPosition;

		// Action bar
		std::vector<_Action>ActionBar;

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
		_Element *BattleElement;
		float BattleSpeed;
		double TurnTimer;
		double AITimer;
		uint8_t BattleSide;
		_Action PotentialAction;
		std::list<uint32_t> ItemDropsReceived;
		std::list<_StatusEffect *> StatusEffects;

		// Actions
		std::list<_Object *> Targets;
		_Action Action;

		// Render
		const _Texture *Portrait;
		glm::vec2 BattleOffset;
		glm::vec2 ResultPosition;
		glm::vec2 StatPosition;

		// Monster
		uint32_t DatabaseID;
		int ExperienceGiven, GoldGiven;
		std::string AI;

		// -- PLAYER  --
		uint32_t CharacterID;

		// States
		bool CheckEvent;
		bool Paused;
		double MoveTime;

		// Texture
		uint8_t Status;
		uint32_t PortraitID;
		const _Texture *WorldTexture;
		const _Texture *StatusTexture;

		// Map
		NetworkIDType SpawnMapID;
		uint32_t SpawnPoint;
		double TeleportTime;

		// Stats
		int PlayTime;
		double PlayTimeAccumulator;
		int Deaths, MonsterKills, PlayerKills, Bounty;
		int Gold;
		int Experience, ExperienceNeeded, ExperienceNextLevel;
		int MinDamageBonus, MaxDamageBonus, MinDefenseBonus, MaxDefenseBonus;

		// Item stats
		float WeaponDamageModifier;
		int WeaponMinDamage, WeaponMaxDamage;
		int ArmorMinDefense, ArmorMaxDefense;

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
		int SkillPoints, SkillPointsUsed;

		// Trading
		int TradeGold;
		bool WaitingForTrade;
		bool TradeAccepted;
		_Object *TradePlayer;

		_Stats *Stats;

	protected:

		void DeleteStatusEffects();
		void CalculateLevelStats();
		void CalculateGearStats();
		void CalculateSkillStats();
		void CalculateFinalStats();

};
