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
class _Fighter;
class _Controller;
class _Monster;
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

// Classes
class _Object : public _ManagerBase {

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
		bool IsMonster() const;
		_StatusEffect *UpdateStats(_StatChange &StatChange);
		void ApplyDeathPenalty(float Penalty, int BountyLoss);

		// Battles
		void UpdateMonsterAI(double FrameTime);
		void CreateBattleElement(_Element *Parent);
		void RemoveBattleElement();
		bool CanBattle() const;
		void StopBattle();

		// Status effects
		void ResolveBuff(_StatusEffect *StatusEffect, const std::string &Function);

		// Actions
		void SetActionUsing(_Buffer &Data, _Manager<_Object> *ObjectManager);

		// Movement
		bool AcceptingMoveInput();
		void GetDirectionFromInput(int InputState, glm::ivec2 &Direction);
		int Move();

		// Trader
		void AcceptTrader(std::vector<_Slot> &Slots);

		// Minigames
		void SendSeed(bool Generate);

		// Map
		bool CanRespec() const;
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
		_Fighter *Fighter;
		_Controller *Controller;
		_Monster *Monster;

		// Pointers
		const _Stats *Stats;
		_Map *Map;
		_Battle *Battle;
		_HUD *HUD;
		_Scripting *Scripting;
		_Server *Server;
		_Peer *Peer;

		// Movement
		glm::ivec2 Position;
		glm::ivec2 ServerPosition;

		// Battle
		std::unordered_map<uint32_t, double> BattleCooldown;

		// Actions
		std::list<_Object *> Targets;
		_Action Action;

		// Render
		const _Texture *ModelTexture;
		const _Texture *StatusTexture;
		const _Texture *Portrait;
		uint32_t PortraitID;
		uint32_t ModelID;
		uint8_t Status;

		// Map
		NetworkIDType LoadMapID;
		NetworkIDType SpawnMapID;
		uint32_t SpawnPoint;
		double TeleportTime;

		// HUD
		bool MenuOpen;
		bool InventoryOpen;
		bool SkillsOpen;

		// Events
		const _Vendor *Vendor;
		const _Trader *Trader;
		const _Blacksmith *Blacksmith;
		const _MinigameType *Minigame;
		uint32_t Seed;

		// Bots
		bool Bot;
		std::list<void *> Path;

	private:

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
