/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020 Alan Witkowski
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
#include <ae/baseobject.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <unordered_map>
#include <list>
#include <vector>
#include <string>
#include <cstdint>

// Forward Declarations
class _Character;
class _Inventory;
class _Fighter;
class _Controller;
class _Monster;
class _Map;
class _Battle;
class _Buff;
class _Stats;
class _Server;
class _Scripting;
class _StatChange;
class _StatusEffect;
class _HUD;
struct _Tile;
struct _ActionResult;
struct _Slot;

namespace ae {
	template<class T> class _Manager;
	class _Buffer;
	class _Texture;
	class _Element;
	class _Peer;
}

namespace Json {
	class Value;
}

// Classes
class _Object : public ae::_BaseObject {

	public:

		enum MoveDirectionType {
			MOVE_UP    = (1 << 0),
			MOVE_DOWN  = (1 << 1),
			MOVE_LEFT  = (1 << 2),
			MOVE_RIGHT = (1 << 3),
		};

		_Object();
		~_Object() override;

		// Updates
		void Update(double FrameTime) override;
		void UpdateBot(double FrameTime);
		void Render(glm::vec4 &ViewBounds, const _Object *ClientPlayer=nullptr);
		void RenderBattle(_Object *ClientPlayer, double Time, bool ShowLevel);

		// Save
		void SerializeSaveData(Json::Value &Data) const;
		void UnserializeSaveData(const std::string &JsonString);

		// Network
		void SerializeCreate(ae::_Buffer &Data);
		void SerializeUpdate(ae::_Buffer &Data);
		void SerializeStats(ae::_Buffer &Data);
		void SerializeBattle(ae::_Buffer &Data);
		void SerializeStatusEffects(ae::_Buffer &Data);
		void UnserializeCreate(ae::_Buffer &Data);
		void UnserializeStats(ae::_Buffer &Data);
		void UnserializeBattle(ae::_Buffer &Data, bool IsClient);
		void UnserializeStatusEffects(ae::_Buffer &Data);
		void SendPacket(ae::_Buffer &Packet);
		void SendActionClear();

		// Stats
		bool IsMonster() const;
		_StatusEffect *UpdateStats(_StatChange &StatChange, _Object *Source=nullptr);
		void ApplyDeathPenalty(bool InBattle, float Penalty, int BountyLoss);

		// Battles
		void UpdateMonsterAI(double FrameTime);
		void CreateBattleElement(ae::_Element *Parent);
		void RemoveBattleElement();
		void StopBattle();

		// Status effects
		void ResolveBuff(_StatusEffect *StatusEffect, const std::string &Function);

		// Actions
		void SetActionUsing(ae::_Buffer &Data, ae::_Manager<_Object> *ObjectManager);

		// Movement
		void GetDirectionFromInput(int InputState, glm::ivec2 &Direction);
		int Move();

		// Trader
		void AcceptTrader(std::vector<_Slot> &Slots);

		// Minigames
		void SendSeed(bool Generate);

		// Map
		bool CanRespec() const;
		const _Tile *GetTile() const;
		ae::NetworkIDType GetMapID() const;

		// Path finding
		bool Pathfind(const glm::ivec2 &StartPosition, const glm::ivec2 &EndPosition);
		int GetInputStateFromPath();

		// Base
		std::string Name;

		// Components
		_Character *Character;
		_Inventory *Inventory;
		_Fighter *Fighter;
		_Controller *Controller;
		_Monster *Monster;

		// Pointers
		const _Stats *Stats;
		_Map *Map;
		_Scripting *Scripting;
		_Server *Server;
		ae::_Peer *Peer;
		uint32_t QueuedMapChange;

		// Movement
		glm::ivec2 Position;
		glm::ivec2 ServerPosition;

		// Render
		const ae::_Texture *ModelTexture;
		uint32_t ModelID;
		int Light;

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
