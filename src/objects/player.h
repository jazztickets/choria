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
			STATE_TOWNPORTAL,
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
		void SetPlayTime(int TValue) { PlayTime = TValue; }
		void SetDeaths(int TValue) { Deaths = TValue; }
		void SetMonsterKills(int TValue) { MonsterKills = TValue; }
		void SetPlayerKills(int TValue) { PlayerKills = TValue; }
		void SetBounty(int TValue) { Bounty = TValue; }
		int GetPlayTime() const { return PlayTime; }
		int GetDeaths() const { return Deaths; }
		int GetMonsterKills() const { return MonsterKills; }
		int GetPlayerKills() const { return PlayerKills; }
		int GetBounty() const { return Bounty; }
		float GetHealthRegen() const { return HealthRegen; }
		float GetManaRegen() const { return ManaRegen; }

		void UpdateDeaths(int TValue) { Deaths += TValue; }
		void UpdateMonsterKills(int TValue) { MonsterKills += TValue; }
		void UpdatePlayerKills(int TValue) { PlayerKills += TValue; }

		void CalculatePlayerStats();

		// Experience
		void SetExperience(int TValue) { Experience = TValue; }
		int GetExperience() const { return Experience; }
		int GetExperienceNeeded() const { return ExperienceNeeded; }
		int GetExperienceNextLevel() const { return ExperienceNextLevel; }
		float GetNextLevelPercent() const;
		void UpdateExperience(int TValue) { Experience += TValue; }

		// Gold
		void SetGold(int TValue) { Gold = TValue; }
		int GetGold() const { return Gold; }
		int GetGoldGiven() const { return (int)(Gold * 0.1f); }
		void UpdateGold(int TValue);

		// Inventory
		void SetPotionsLeft(int THealthCount, int TManaCount);
		void UpdatePotionsLeft(int TType) { PotionsLeft[TType]--; }
		int GetPotionBattle(int TType);
		bool UsePotionBattle(int TSlot, int TSkillType, int &THealthChange, int &TManaChange);
		bool UsePotionWorld(int TSlot);
		bool UseInventory(int TSlot);
		void SetInventory(int TSlot, int TItemID, int TCount);
		void SetInventory(int TSlot, _InventorySlot *TItem);
		_InventorySlot *GetInventory(int TSlot);
		const _Item *GetInventoryItem(int TSlot);
		bool MoveInventory(int TOldSlot, int TNewSlot);
		bool UpdateInventory(int TSlot, int TAmount);
		bool AddItem(const _Item *TItem, int TCount, int TSlot);
		bool IsBackpackFull();
		bool IsEmptySlot(int TSlot) { return Inventory[TSlot].Item == nullptr; }
		void MoveTradeToInventory();
		void SplitStack(int TSlot, int TCount);

		// Movement
		bool CanMove() { return MoveTime > PLAYER_MOVETIME; }
		bool MovePlayer(int TDirection);
		int GetCurrentZone();
		void SetInvisPower(int TValue) { InvisPower = TValue; }
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
		void SetNextBattle(int TValue) { NextBattle = TValue; }
		void UpdateNextBattle(int TChange) { NextBattle += TChange; }
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
		int GetRequiredItemSlots(const _Trader *TTrader, int *TSlots);
		void AcceptTrader(const _Trader *TTrader, int *TSlots, int TRewardSlot);

		// Map
		void SetSpawnMapID(int TValue) { SpawnMapID = TValue; }
		int GetSpawnMapID() const { return SpawnMapID; }
		void SetSpawnPoint(int TValue) { SpawnPoint = TValue; }
		int GetSpawnPoint() const { return SpawnPoint; }
		const _Tile *GetTile();

		// World
		void ToggleBusy(bool Value);
		void StartTownPortal();
		double GetTownPortalTime() const { return TownPortalTime; }
		void SetStateImage(irr::video::ITexture *TImage) { StateImage = TImage; }

		// PVP
		bool CanAttackPlayer();
		void ResetAttackPlayerTime() { AttackPlayerTime = 0; }

		// Trading
		void SetTradePlayer(_Player *TPlayer) { TradePlayer = TPlayer; }
		_Player *GetTradePlayer() const { return TradePlayer; }
		void SetTradeGold(int TValue) { TradeGold = TValue; }
		int GetTradeGold() const { return TradeGold; }
		void SetTradeAccepted(bool TValue) { TradeAccepted = TValue; }
		bool GetTradeAccepted() { return TradeAccepted; }

		static bool IsSlotInventory(int TSlot) { return TSlot >= INVENTORY_BACKPACK && TSlot < INVENTORY_TRADE; }
		static bool IsSlotTrade(int TSlot) { return TSlot >= INVENTORY_TRADE && TSlot < INVENTORY_COUNT; }

	private:

		void CalculateLevelStats();
		void CalculateGearStats();
		void CalculateSkillStats();
		void CalculateFinalStats();

		bool CanEquipItem(int TSlot, const _Item *TItem);
		void SwapItem(int TSlot, int TOldSlot);

		// Connection information
		int AccountID, CharacterID;
		ENetPeer *Peer;
		_Database *Database;

		// States
		int State;
		double MoveTime, AutoSaveTime;

		// Texture
		int PortraitID;
		irr::video::ITexture *WorldImage, *StateImage;

		// Map
		int SpawnMapID, SpawnPoint;
		double TownPortalTime;

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
};
