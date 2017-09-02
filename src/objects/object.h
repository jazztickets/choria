/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#include <ae/managerbase.h>
#include <objects/components/character.h>
#include <glm/vec2.hpp>
#include <unordered_map>
#include <list>
#include <vector>
#include <cstdint>

// Forward Declarations
class _Character;
class _Inventory;
class _Record;
class _Map;
class _Peer;
class _Texture;
class _Battle;
class _Buff;
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
struct _MinigameType;
struct _Blacksmith;
struct _ActionResult;
struct _Slot;

namespace Json {
	class Value;
}

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
			STATUS_BLACKSMITH,
			STATUS_MINIGAME,
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

		// Updates
		void Update(double FrameTime) override;
		void UpdateBot(double FrameTime);
		void Render(const _Object *ClientPlayer=nullptr);
		void RenderBattle(_Object *ClientPlayer, double Time);

		// Save
		void SerializeSaveData(Json::Value &Data) const;
		void UnserializeSaveData(const std::string &JsonString);

		// Network
		void SerializeCreate(_Buffer &Data);
		void SerializeUpdate(_Buffer &Data);
		void SerializeStats(_Buffer &Data);
		void SerializeBattle(_Buffer &Data);
		void UnserializeCreate(_Buffer &Data);
		void UnserializeStats(_Buffer &Data);
		void UnserializeBattle(_Buffer &Data);
		void SendPacket(_Buffer &Packet);

		// Stats
		bool IsMonster() const { return DatabaseID != 0; }
		_StatusEffect *UpdateStats(_StatChange &StatChange);
		void UpdateHealth(int &Value);
		void UpdateMana(int Value);
		float GetNextLevelPercent() const;
		void UpdateGold(int Value);
		void UpdateExperience(int Value);
		void ApplyDeathPenalty(float Penalty, int BountyLoss);

		// Battles
		void UpdateMonsterAI(const std::list<_Object *> &Fighters, double FrameTime);
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
		bool GetActionFromSkillbar(_Action &ReturnAction, size_t Slot);
		void SetActionUsing(_Buffer &Data, _Manager<_Object> *ObjectManager);

		// Movement
		bool AcceptingMoveInput();
		void GetDirectionFromInput(int InputState, glm::ivec2 &Direction);
		int Move();

		// Skills
		bool CanRespec() const;
		bool HasLearned(const _Item *Skill) const;
		bool HasUnlocked(const _Item *Item) const;
		int GetSkillPointsAvailable() const { return Character->SkillPoints - Character->SkillPointsUsed; }
		void AdjustSkillLevel(uint32_t SkillID, int Amount);

		// Trader
		void AcceptTrader(std::vector<_Slot> &Slots);

		// Minigames
		void SendSeed(bool Generate);

		// Map
		const _Tile *GetTile() const;
		NetworkIDType GetMapID() const;

		// Input
		bool CanOpenTrade() { return Character->IsAlive() && !Battle; }
		bool CanOpenSkills() { return Character->IsAlive() && !Battle; }
		bool CanOpenInventory() { return Character->IsAlive() && !Battle; }
		bool CanOpenParty() { return Character->IsAlive() && !Battle; }
		bool CanTeleport() { return Character->IsAlive() && !Battle; }

		// Path finding
		bool Pathfind(const glm::ivec2 &StartPosition, const glm::ivec2 &EndPosition);
		int GetInputStateFromPath();

		// UI
		void ResetUIState();

		// Base
		std::string Name;

		// Components
		_Character *Character;
		_Inventory *Inventory;
		_Record *Record;

		// Client
		const _Stats *Stats;
		_Map *Map;
		_HUD *HUD;
		_Scripting *Scripting;
		_Server *Server;
		_Peer *Peer;
		std::unordered_map<uint32_t, double> BattleCooldown;
		std::list<int> InputStates;
		int Moved;
		bool UseCommand;
		bool WaitForServer;
		bool CheckEvent;
		bool Paused;
		double MoveTime;
		std::string ClientMessage;
		glm::ivec2 Position;
		glm::ivec2 ServerPosition;

		// Stats
		std::unordered_map<uint32_t, _Unlock> Unlocks;

		// Battle
		_Battle *Battle;
		_Element *BattleElement;
		_Action PotentialAction;
		std::list<uint32_t> ItemDropsReceived;
		std::list<_StatusEffect *> StatusEffects;
		double TurnTimer;
		double AttackPlayerTime;
		int NextBattle;
		int GoldStolen;
		bool JoinedBattle;
		uint8_t BattleSide;

		// Actions
		std::list<_Object *> Targets;
		_Object *LastTarget[2];
		_Action Action;

		// Render
		const _Texture *ModelTexture;
		const _Texture *StatusTexture;
		const _Texture *Portrait;
		glm::vec2 BattleOffset;
		glm::vec2 ResultPosition;
		glm::vec2 StatPosition;
		uint32_t PortraitID;
		uint32_t ModelID;
		uint8_t Status;

		// Monster
		_Object *Owner;
		uint32_t DatabaseID;
		int ExperienceGiven;
		int GoldGiven;
		std::string AI;

		// Map
		NetworkIDType LoadMapID;
		NetworkIDType SpawnMapID;
		uint32_t SpawnPoint;
		double TeleportTime;

		// HUD
		bool InventoryOpen;

		// Events
		const _Vendor *Vendor;
		const _Trader *Trader;
		const _Blacksmith *Blacksmith;
		const _MinigameType *Minigame;
		uint32_t Seed;

		// Skills
		std::unordered_map<uint32_t, int> Skills;
		bool SkillsOpen;

		// Trading
		_Object *TradePlayer;
		int TradeGold;
		bool WaitingForTrade;
		bool TradeAccepted;

		// Party
		std::string PartyName;

		// Bots
		bool Bot;
		std::list<void *> Path;

	private:

		void DeleteStatusEffects();

		template <typename Type>
		void GetValue(const std::unordered_map<std::string, std::string> &Map, const std::string &Field, Type &Value) {
			const auto &MapIterator = Map.find(Field);
			if(MapIterator != Map.end()) {
				std::stringstream Stream(MapIterator->second);
				Stream >> Value;
			}
		}

};

inline bool CompareObjects(const _Object *First, const _Object *Second) {
	return First->NetworkID < Second->NetworkID;
}
