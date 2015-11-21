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
#include <enet/enet.h>
#include <constants.h>
#include <objects/object.h>
#include <objects/fighter.h>

// Forward Declarations
class _Battle;
class _Item;
class _Skill;
class _Database;
struct _Tile;
struct _Vendor;
struct _Trader;

struct _InventorySlot {
	const _Item *Item;
	int Count;
};

// Classes
class _Player : public _Object, public _Fighter {

	public:

		enum PlayerStateType {
			STATE_WALK,
			STATE_BATTLE,
			STATE_VENDOR,
			STATE_TRADER,
			STATE_WAITTRADE,
			STATE_TRADE,
			STATE_TELEPORT,
			STATE_BUSY,
		};

		enum MoveDirectionType {
			MOVE_LEFT,
			MOVE_UP,
			MOVE_RIGHT,
			MOVE_DOWN,
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

		enum SkillType {
			SKILL_COUNT = 25
		};

		_Player();
		~_Player();

		void Update(double FrameTime) override;
		void RenderWorld(const _Object *ClientPlayer=nullptr) override;

		// Account
		void Save();
		void SetPortraitID(int TID);

		// Stats
		void CalculatePlayerStats();

		// Experience
		float GetNextLevelPercent() const;
		void UpdateExperience(int Value) override { Experience += Value; }

		// Gold
		int GetGoldGiven() const override { return (int)(Gold * 0.1f); }
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
		bool CanMove() { return MoveTime > PLAYER_MOVETIME; }
		bool MovePlayer(int Direction);
		int GetCurrentZone();
		bool IsInvisible() const { return InvisPower > 0; }

		// Skills
		void SetSkillLevel(int TSkillID, int TPoints) { SkillLevels[TSkillID] = TPoints; }
		int GetSkillLevel(int TSkillID) const override { return SkillLevels[TSkillID]; }
		int GetSkillPointsRemaining() const { return SkillPoints - SkillPointsUsed; }
		void AdjustSkillLevel(int SkillID, int Adjust);
		void CalculateSkillPoints();

		// Battles
		void GenerateNextBattle();
		void StartBattle(_Battle *TBattle);
		void StopBattle();

		// Trader
		int GetRequiredItemSlots(int *Slots);
		void AcceptTrader(const _Trader *TTrader, int *Slots, int TRewardSlot);

		// Map
		const _Tile *GetTile();

		// World
		void ToggleBusy(bool Value);
		void StartTeleport();

		// PVP
		bool CanAttackPlayer();
		void ResetAttackPlayerTime() { AttackPlayerTime = 0; }

		static bool IsSlotInventory(int Slot) { return Slot >= INVENTORY_BACKPACK && Slot < INVENTORY_TRADE; }
		static bool IsSlotTrade(int Slot) { return Slot >= INVENTORY_TRADE && Slot < INVENTORY_COUNT; }

		// Connection information
		int AccountID, CharacterID;
		ENetPeer *Peer;
		_Database *Database;

		// States
		int State;
		double MoveTime, AutoSaveTime;

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
		int SkillLevels[SKILL_COUNT];
		int SkillPoints, SkillPointsUsed;
		int PotionsLeft[2], MaxPotions[2];

		// Trading
		double TradeRequestTime;
		int TradeGold;
		bool TradeAccepted;
		_Player *TradePlayer;

	private:

		void CalculateLevelStats();
		void CalculateGearStats();
		void CalculateSkillStats();
		void CalculateFinalStats();

		bool CanEquipItem(int Slot, const _Item *Item);
		void SwapItem(int Slot, int OldSlot);

};
