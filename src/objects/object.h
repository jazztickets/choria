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
#include <constants.h>
#include <glm/vec2.hpp>
#include <packet.h>
#include <vector>
#include <unordered_map>
#include <cstdint>

// Forward Declarations
class _Map;
class _Peer;
class _Texture;
class _Battle;
class _Skill;
class _Item;
class _Stats;
struct _Tile;
struct _Vendor;
struct _Trader;
struct _ActionResult;

// Structures
struct _InventorySlot {
	const _Item *Item;
	int Count;
};

// Classes
class _Object {

	public:

		enum Type {
			PLAYER,
			MONSTER,
		};

		enum PlayerStateType {
			STATE_NONE,
			STATE_BATTLE,
			STATE_VENDOR,
			STATE_TRADER,
			STATE_TRADE,
			STATE_TELEPORT,
			STATE_BUSY,
		};

		enum MoveDirectionType {
			MOVE_UP    = (1 << 0),
			MOVE_DOWN  = (1 << 1),
			MOVE_LEFT  = (1 << 2),
			MOVE_RIGHT = (1 << 3),
		};

		enum InventoryType {
			INVENTORY_HEAD,
			INVENTORY_BODY,
			INVENTORY_LEGS,
			INVENTORY_HAND1,
			INVENTORY_HAND2,
			INVENTORY_RING1,
			INVENTORY_RING2,
			INVENTORY_BACKPACK,
			INVENTORY_TRADE = INVENTORY_BACKPACK + 24,
			INVENTORY_COUNT = INVENTORY_TRADE + PLAYER_TRADEITEMS,
		};

		_Object();
		virtual ~_Object();

		void Render(const _Object *ClientPlayer=nullptr);
		void Update(double FrameTime);

		// -- FROM FIGHTER --

		// Render
		void RenderBattle(bool ShowResults, float TimerPercent, _ActionResult *Result, bool IsTarget);

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
		void UpdateTarget(const std::vector<_Object *> &Fighters);

		int GetExperienceGiven() const { return ExperienceGiven; }
		int GetGoldGiven() const { return GoldGiven; }

		void UpdateExperience(int Value) { }

		// Skills
		const _Skill *GetActionBar(int Slot);
		uint32_t GetActionBarID(int Slot) const;

		// -- END FROM FIGHTER --

		// -- FROM PLAYER --
		//void UpdateExperience(int Value) { Experience += Value; }
		//int GetGoldGiven() const { return (int)(Gold * 0.1f); }

		// Account
		void Save();

		// Stats
		void CalculatePlayerStats();

		// Experience
		float GetNextLevelPercent() const;

		// Gold
		void UpdateGold(int Value);

		// Inventory
		void UpdatePotionsLeft(int PotionType) { PotionsLeft[PotionType]--; }
		int GetPotionBattle(int PotionType);
		bool UsePotionBattle(int Slot, int SkillType, int &HealthChange, int &ManaChange);
		bool UsePotionWorld(int Slot);
		bool UseInventory(int Slot);
		void SetInventory(int Slot, int ItemID, int Count);
		void SetInventory(int Slot, _InventorySlot *Item);
		const _Item *GetInventoryItem(int Slot);
		bool MoveInventory(int OldSlot, int NewSlot);
		bool UpdateInventory(int Slot, int Amount);
		bool AddItem(const _Item *Item, int Count, int Slot);
		bool IsBackpackFull();
		bool IsEmptySlot(int Slot) { return Inventory[Slot].Item == nullptr; }
		void MoveTradeToInventory();
		void SplitStack(int Slot, int Count);

		// Movement
		bool AcceptingMoveInput();
		bool CanMove() { return MoveTime > PLAYER_MOVETIME; }
		int Move();
		int GetCurrentZone();
		bool IsInvisible() const { return InvisPower > 0; }

		// Skills
		void SetSkillLevel(int SkillID, int Points) { SkillLevels[SkillID] = Points; }
		int GetSkillPointsRemaining() const { return SkillPoints - SkillPointsUsed; }
		void AdjustSkillLevel(uint32_t SkillID, int Adjust);
		void CalculateSkillPoints();

		// Battles
		void GenerateNextBattle();
		void StartBattle(_Battle *Battle);
		void StopBattle();

		// Trader
		int GetRequiredItemSlots(int *Slots);
		void AcceptTrader(int *Slots, int RewardSlot);

		// Map
		const _Tile *GetTile();

		// World
		void SetBusy(bool Value);
		void StartTeleport();

		// PVP
		bool CanAttackPlayer();
		void ResetAttackPlayerTime() { AttackPlayerTime = 0; }

		static bool IsSlotInventory(int Slot) { return Slot >= INVENTORY_BACKPACK && Slot < INVENTORY_TRADE; }
		static bool IsSlotTrade(int Slot) { return Slot >= INVENTORY_TRADE && Slot < INVENTORY_COUNT; }

		// -- END FROM PLAYER --

		// -- OBJECT  --
		_Map *Map;
		_Peer *Peer;
		int Type;
		int InputState;
		int Moved;
		bool Deleted;
		bool WaitForServer;
		glm::ivec2 Position;
		glm::ivec2 ServerPosition;
		NetworkIDType NetworkID;

		// -- FIGHTER  --

		// Action bar
		const _Skill *ActionBar[ACTIONBAR_SIZE];

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
		glm::ivec2 BattleOffset;

		// Monster
		std::vector<_Object *> Opponents;
		int DatabaseID;
		int ExperienceGiven, GoldGiven;
		int AI;

		// -- PLAYER  --
		int CharacterID;

		// States
		int State;
		double MoveTime;

		// Texture
		int PortraitID;
		const _Texture *WorldImage;
		const _Texture *StateImage;

		// Map
		int SpawnMapID, SpawnPoint;
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
		int InvisPower;

		// Items
		_InventorySlot Inventory[INVENTORY_COUNT];
		const _Vendor *Vendor;
		const _Trader *Trader;

		// Skills
		std::unordered_map<uint32_t, int32_t> SkillLevels;
		int SkillPoints, SkillPointsUsed;
		int PotionsLeft[2], MaxPotions[2];

		// Trading
		int TradeGold;
		bool TradeAccepted;
		_Object *TradePlayer;

		_Stats *Stats;

	protected:

		void CalculateLevelStats();
		void CalculateGearStats();
		void CalculateSkillStats();
		void CalculateFinalStats();

		bool CanEquipItem(int Slot, const _Item *Item);
		void SwapItem(int Slot, int OldSlot);

		int Dummy;

};
