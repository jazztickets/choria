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
#include <unordered_map>
#include <list>
#include <vector>
#include <cstdint>

// Forward Declarations
class _Map;
class _Peer;
class _Texture;
class _Battle;
class _Skill;
class _Item;
class _Stats;
class _Buffer;
class _Scripting;
struct _Tile;
struct _Vendor;
struct _Trader;
struct _ActionResult;

// Structures
struct _InventorySlot {
	const _Item *Item;
	int Count;
};

// Action
struct _Action {
	_Action() : Skill(nullptr), Item(nullptr), Count(0) { }
	_Action(const _Skill *Skill) : _Action() { this->Skill = Skill; }
	_Action(const _Item *Item) : _Action() { this->Item = Item; }

	bool operator==(const _Action &Action) const { return Action.Skill == Skill && Action.Item == Item; }
	bool operator!=(const _Action &Action) const { return !(Action.Skill == Skill && Action.Item == Item); }

	void Serialize(_Buffer &Data);
	void Unserialize(_Buffer &Data, _Stats *Stats);

	bool IsSet() const { return !(Skill == nullptr && Item == nullptr); }
	void Unset() { Skill = nullptr; Item = nullptr; Count = 0; }

	const _Skill *Skill;
	const _Item *Item;
	int Count;
};

// Classes
class _Object {

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
		~_Object();

		void Render(const _Object *ClientPlayer=nullptr);
		void Update(double FrameTime);
		void Serialize(_Buffer &Packet);
		void SerializeUpdate(_Buffer &Packet);
		void Unserialize(_Buffer &Packet);

		// -- FROM FIGHTER --

		// Render
		void RenderBattle(_Object *ClientPlayer, double Time);

		// Stats
		void UpdateHealth(int Value);
		void UpdateMana(int Value);
		void RestoreHealthMana();

		void UpdateRegen(int &HealthUpdate, int &ManaUpdate);

		// Battles
		void UpdateAI(_Scripting *Scripting, const std::list<_Object *> &Fighters, double FrameTime);
		int GenerateDamage();
		int GenerateDefense();

		// -- END FROM FIGHTER --

		// Stats
		void CalculatePlayerStats();

		// Experience
		float GetNextLevelPercent() const;

		// Gold
		void UpdateGold(int Value);

		// Inventory
		bool FindItem(const _Item *Item, int &Slot);
		int CountItem(const _Item *Item);
		void RefreshActionBarCount();
		bool UseAction(uint8_t Slot);
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
		bool IsInvisible() const { return InvisPower > 0; }

		// Skills
		void SetSkillLevel(int SkillID, int Points) { SkillLevels[SkillID] = Points; }
		int GetSkillPointsRemaining() const { return SkillPoints - SkillPointsUsed; }
		void AdjustSkillLevel(uint32_t SkillID, int Adjust);
		void CalculateSkillPoints();

		// Battles
		bool CanBattle();
		void GenerateNextBattle();
		void StopBattle();

		// Trader
		int GetRequiredItemSlots(int *Slots);
		void AcceptTrader(int *Slots, int RewardSlot);

		// Map
		const _Tile *GetTile();

		// PVP
		bool CanAttackPlayer();
		void ResetAttackPlayerTime() { AttackPlayerTime = 0; }

		static bool IsSlotInventory(int Slot) { return Slot >= INVENTORY_BACKPACK && Slot < INVENTORY_TRADE; }
		static bool IsSlotTrade(int Slot) { return Slot >= INVENTORY_TRADE && Slot < INVENTORY_COUNT; }

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
		float BattleSpeed;
		double TurnTimer;
		double AITimer;
		int BattleID;
		int BattleSide;
		std::list<_Object *> BattleTargets;
		_Action BattleAction;
		_Action PotentialAction;
		std::list<uint32_t> ItemDropsReceived;

		// Render
		const _Texture *Portrait;
		glm::ivec2 BattleOffset;
		glm::ivec2 ResultPosition;

		// Monster
		uint32_t DatabaseID;
		int ExperienceGiven, GoldGiven;
		std::string AI;

		// -- PLAYER  --
		int CharacterID;

		// States
		bool CheckEvent;
		bool Paused;
		double MoveTime;

		// Texture
		int Status;
		uint32_t PortraitID;
		const _Texture *WorldTexture;
		const _Texture *StatusTexture;

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
		bool InventoryOpen;
		_InventorySlot Inventory[INVENTORY_COUNT];
		const _Vendor *Vendor;
		const _Trader *Trader;

		// Skills
		bool SkillsOpen;
		std::unordered_map<uint32_t, int32_t> SkillLevels;
		int SkillPoints, SkillPointsUsed;

		// Trading
		int TradeGold;
		bool WaitingForTrade;
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

};
