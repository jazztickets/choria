/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef PLAYER_H
#define PLAYER_H

// Libraries
#include <irrlicht/irrlicht.h>
#include <enet/enet.h>
#include "../engine/constants.h"
#include "object.h"
#include "fighter.h"

// Constants
const int PLAYER_ATTACKTIME = 1000;
const int PLAYER_TRADEITEMS = 8;

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Forward Declarations
class BattleClass;
class ItemClass;
class SkillClass;
class DatabaseClass;
struct TileStruct;
struct VendorStruct;
struct TraderStruct;

struct InventoryStruct {
	const ItemClass *Item;
	int Count;
};

// Classes
class PlayerClass : public ObjectClass, public FighterClass {

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

		PlayerClass();
		~PlayerClass();

		int GetState() const { return State; }
		void SetState(int State) { this->State = State; }

		void Update(u32 FrameTime);
		void RenderWorld();

		// Connection
		void SetPeer(ENetPeer *Peer) { Peer = Peer; }
		ENetPeer *GetPeer() const { return Peer; }

		// Account
		void Save();
		void SetDatabase(DatabaseClass *Database) { Database = Database; }
		void SetAccountID(int ID) { AccountID = ID; }
		int GetAccountID() const { return AccountID; }
		void SetCharacterID(int ID) { CharacterID = ID; }
		int GetCharacterID() const { return CharacterID; }
		void SetPortraitID(int ID);
		int GetPortraitID() const { return PortraitID; }

		// Stats
		void SetPlayTime(u32 Value) { PlayTime = Value; }
		void SetDeaths(int Value) { Deaths = Value; }
		void SetMonsterKills(int Value) { MonsterKills = Value; }
		void SetPlayerKills(int Value) { PlayerKills = Value; }
		void SetBounty(int Value) { Bounty = Value; }
		u32 GetPlayTime() const { return PlayTime; }
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
		int GetPotionBattle(int Type);
		bool UsePotionBattle(int Slot, int SkillType, int &HealthChange, int &ManaChange);
		bool UsePotionWorld(int Slot);
		bool UseInventory(int Slot);
		void SetInventory(int Slot, int ItemID, int Count);
		void SetInventory(int Slot, InventoryStruct *Item);
		InventoryStruct *GetInventory(int Slot);
		const ItemClass *GetInventoryItem(int Slot);
		bool MoveInventory(int OldSlot, int NewSlot);
		bool UpdateInventory(int Slot, int Amount);
		bool AddItem(const ItemClass *Item, int Count, int Slot);
		bool IsBackpackFull();
		bool IsEmptySlot(int Slot) { return Inventory[Slot].Item == NULL; }
		void MoveTradeToInventory();
		void SplitStack(int Slot, int Count);

		// Movement
		bool CanMove() { return MoveTime > PLAYER_MOVETIME; }
		bool MovePlayer(int Direction);
		int GetCurrentZone();

		// Actions
		void SetActionBarLength(int TLength) { ActionBarLength = TLength; }
		int GetActionBarLength() const { return ActionBarLength; }
		void UseActionWorld(int Slot);

		// Skills
		void ClearSkillsUnlocked();
		void AddSkillUnlocked(const SkillClass *Skill);
		const array<const SkillClass *> &GetSkillsUnlocked() const;
		
		// Battles
		void GenerateNextBattle();
		void SetNextBattle(int Value) { NextBattle = Value; }
		int GetNextBattle() const { return NextBattle; }
		void SetBattle(BattleClass *Battle) { Battle = Battle; }
		void StartBattle(BattleClass *Battle);
		void ExitBattle();

		// Vendor
		void SetVendor(const VendorStruct *Vendor);
		const VendorStruct *GetVendor();

		// Trader
		void SetTrader(const TraderStruct *TTrader);
		const TraderStruct *GetTrader();
		int GetRequiredItemSlots(const TraderStruct *TTrader, int *Slots);
		void AcceptTrader(const TraderStruct *TTrader, int *Slots, int TRewardSlot);
		
		// Map
		void SetSpawnMapID(int Value) { SpawnMapID = Value; }
		int GetSpawnMapID() const { return SpawnMapID; }
		void SetSpawnPoint(int Value) { SpawnPoint = Value; }
		int GetSpawnPoint() const { return SpawnPoint; }
		const TileStruct *GetTile();

		// World
		void ToggleBusy(bool Value);
		void StartTownPortal();
		u32 GetTownPortalTime() const { return TownPortalTime; }
		void SetStateImage(ITexture *TImage) { StateImage = TImage; }

		// PVP
		bool CanAttackPlayer();
		void ResetAttackPlayerTime() { AttackPlayerTime = 0; }

		// Trading
		void SetTradePlayer(PlayerClass *Player) { TradePlayer = Player; }
		PlayerClass *GetTradePlayer() const { return TradePlayer; }
		void SetTradeGold(int Value) { TradeGold = Value; }
		int GetTradeGold() const { return TradeGold; }
		void SetTradeAccepted(bool Value) { TradeAccepted = Value; }
		bool GetTradeAccepted() { return TradeAccepted; }

		static bool IsSlotInventory(int Slot) { return Slot >= INVENTORY_BACKPACK && Slot < INVENTORY_TRADE; } 
		static bool IsSlotTrade(int Slot) { return Slot >= INVENTORY_TRADE && Slot < INVENTORY_COUNT; } 

	private:

		void CalculateLevelStats();
		void CalculateGearStats();
		void CalculateFinalStats();

		bool CanEquipItem(int Slot, const ItemClass *Item);
		void SwapItem(int Slot, int OldSlot);

		// Connection information
		int AccountID, CharacterID;
		ENetPeer *Peer;
		DatabaseClass *Database;

		// States
		int State;
		u32 MoveTime, AutoSaveTime;

		// Texture
		int PortraitID;
		ITexture *WorldImage, *StateImage;

		// Map
		int SpawnMapID, SpawnPoint;
		u32 TownPortalTime;

		// Stats
		u32 PlayTime, PlayTimeAccumulator;
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
		u32 AttackPlayerTime;

		// Items
		InventoryStruct Inventory[INVENTORY_COUNT];
		const VendorStruct *Vendor;
		const TraderStruct *Trader;

		// Skills
		int ActionBarLength;
		array<const SkillClass *> SkillsUnlocked;

		// Trading
		u32 TradeRequestTime;
		int TradeGold;
		bool TradeAccepted;
		PlayerClass *TradePlayer;
};

#endif
