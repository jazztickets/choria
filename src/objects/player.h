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

		int GetState() const { return State; }
		void SetState(int TState) { State = TState; }

		void Update(double FrameTime);
		void RenderWorld(const _Map *TMap, const _Object *TClientPlayer=nullptr);

		// Connection
		void SetPeer(ENetPeer *TPeer) { Peer = TPeer; }
		ENetPeer *GetPeer() const { return Peer; }

		// Account
		void Save();
		void SetDatabase(_Database *TDatabase) { Database = TDatabase; }
		void SetAccountID(int TID) { AccountID = TID; }
		int GetAccountID() const { return AccountID; }
		void SetCharacterID(int TID) { CharacterID = TID; }
		int GetCharacterID() const { return CharacterID; }
		void SetPortraitID(int TID);
		int GetPortraitID() const { return PortraitID; }

		// Stats
		void SetPlayTime(int Value) { PlayTime = Value; }
		void SetDeaths(int Value) { Deaths = Value; }
		void SetMonsterKills(int Value) { MonsterKills = Value; }
		void SetPlayerKills(int Value) { PlayerKills = Value; }
		void SetBounty(int Value) { Bounty = Value; }
		int GetPlayTime() const { return PlayTime; }
		int GetDeaths() const { return Deaths; }
		int GetMonsterKills() const { return MonsterKills; }
		int GetPlayerKills() const { return PlayerKills; }
		int GetBounty() const { return Bounty; }
		float GetHealthRegen() const { return HealthRegen; }
		float GetManaRegen() const { return ManaRegen; }

		void UpdateDeaths(int Value) { Deaths += Value; }
		void UpdateMonsterKills(int Value) { MonsterKills += Value; }
		void UpdatePlayerKills(int Value) { PlayerKills += Value; }

		void CalculatePlayerStats();

		// Experience
		void SetExperience(int Value) { Experience = Value; }
		int GetExperience() const { return Experience; }
		int GetExperienceNeeded() const { return ExperienceNeeded; }
		int GetExperienceNextLevel() const { return ExperienceNextLevel; }
		float GetNextLevelPercent() const;
		void UpdateExperience(int Value) { Experience += Value; }

		// Gold
		void SetGold(int Value) { Gold = Value; }
		int GetGold() const { return Gold; }
		int GetGoldGiven() const { return (int)(Gold * 0.1f); }
		void UpdateGold(int Value);

		// Inventory
		void SetPotionsLeft(int THealthCount, int TManaCount);
		void UpdatePotionsLeft(int TType) { PotionsLeft[TType]--; }
		int GetPotionBattle(int TType);
		bool UsePotionBattle(int Slot, int TSkillType, int &THealthChange, int &TManaChange);
		bool UsePotionWorld(int Slot);
		bool UseInventory(int Slot);
		void SetInventory(int Slot, int TItemID, int TCount);
		void SetInventory(int Slot, _InventorySlot *TItem);
		_InventorySlot *GetInventory(int Slot);
		const _Item *GetInventoryItem(int Slot);
		bool MoveInventory(int TOldSlot, int TNewSlot);
		bool UpdateInventory(int Slot, int TAmount);
		bool AddItem(const _Item *TItem, int TCount, int Slot);
		bool IsBackpackFull();
		bool IsEmptySlot(int Slot) { return Inventory[Slot].Item == nullptr; }
		void MoveTradeToInventory();
		void SplitStack(int Slot, int TCount);

		// Movement
		bool CanMove() { return MoveTime > PLAYER_MOVETIME; }
		bool MovePlayer(int Direction);
		int GetCurrentZone();
		void SetInvisPower(int Value) { InvisPower = Value; }
		int GetInvisPower() const { return InvisPower; }
		bool IsInvisible() const { return InvisPower > 0; }

		// Skills
		void SetSkillLevel(int TSkillID, int TPoints) { SkillLevels[TSkillID] = TPoints; }
		int GetSkillLevel(int TSkillID) const { return SkillLevels[TSkillID]; }
		int GetSkillPointsRemaining() const { return SkillPoints - SkillPointsUsed; }
		void AdjustSkillLevel(int TSkillID, int TAdjust);
		void CalculateSkillPoints();

		// Battles
		void GenerateNextBattle();
		void SetNextBattle(int Value) { NextBattle = Value; }
		int GetNextBattle() const { return NextBattle; }
		void SetBattle(_Battle *TBattle) { Battle = TBattle; }
		void StartBattle(_Battle *TBattle);
		void StopBattle();

		// Vendor
		void SetVendor(const _Vendor *TVendor);
		const _Vendor *GetVendor();

		// Trader
		void SetTrader(const _Trader *TTrader);
		const _Trader *GetTrader();
		int GetRequiredItemSlots(const _Trader *TTrader, int *Slots);
		void AcceptTrader(const _Trader *TTrader, int *Slots, int TRewardSlot);

		// Map
		void SetSpawnMapID(int Value) { SpawnMapID = Value; }
		int GetSpawnMapID() const { return SpawnMapID; }
		void SetSpawnPoint(int Value) { SpawnPoint = Value; }
		int GetSpawnPoint() const { return SpawnPoint; }
		const _Tile *GetTile();

		// World
		void ToggleBusy(bool Value);
		void StartTeleport();
		double GetTeleportTime() const { return TeleportTime; }
		void SetStateImage(const _Texture *TImage) { StateImage = TImage; }

		// PVP
		bool CanAttackPlayer();
		void ResetAttackPlayerTime() { AttackPlayerTime = 0; }

		// Trading
		void SetTradePlayer(_Player *TPlayer) { TradePlayer = TPlayer; }
		_Player *GetTradePlayer() const { return TradePlayer; }
		void SetTradeGold(int Value) { TradeGold = Value; }
		int GetTradeGold() const { return TradeGold; }
		void SetTradeAccepted(bool Value) { TradeAccepted = Value; }
		bool GetTradeAccepted() { return TradeAccepted; }

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

		bool CanEquipItem(int Slot, const _Item *TItem);
		void SwapItem(int Slot, int TOldSlot);

};
