/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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
#include <ae/bounds.h>
#include <ae/baseobject.h>
#include <ae/physics.h>
#include <glm/vec2.hpp>
#include <unordered_map>
#include <list>
#include <vector>
#include <cstdint>

// Forward Declarations
class _Character;
class _Inventory;
class _Fighter;
class _Controller;
class _Monster;
class _Light;
class _Map;
class _Battle;
class _Buff;
class _Stats;
class _Server;
class _Scripting;
class _StatChange;
class _StatusEffect;
class _HUD;
struct _Model;
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
		void Render(const _Object *ClientPlayer=nullptr);
		void RenderBattle(_Object *ClientPlayer, double Time);

		// Save
		void SerializeSaveData(Json::Value &Data) const;
		void UnserializeSaveData(const std::string &JsonString);

		// Network
		void SerializeUpdate(ae::_Buffer &Data);
		void SerializeCreate(ae::_Buffer &Data);
		void UnserializeCreate(ae::_Buffer &Data);
		void SerializeStats(ae::_Buffer &Data);
		void UnserializeStats(ae::_Buffer &Data);
		void SerializeBattle(ae::_Buffer &Data);
		void UnserializeBattle(ae::_Buffer &Data, bool IsClient);
		void SendPacket(ae::_Buffer &Packet);

		// Stats
		bool IsMonster() const;
		_StatusEffect *UpdateStats(_StatChange &StatChange);
		void ApplyDeathPenalty(float Penalty, int BountyLoss);

		// Battles
		void UpdateMonsterAI(double FrameTime);
		void CreateBattleElement(ae::_Element *Parent);
		void RemoveBattleElement();
		void StopBattle();

		// Collision
		bool CheckAABB(const ae::_Bounds &Bounds) const;

		// Status effects
		void ResolveBuff(_StatusEffect *StatusEffect, const std::string &Function);

		// Actions
		void SetActionUsing(ae::_Buffer &Data, ae::_Manager<_Object> *ObjectManager);

		// Movement
		void SetPositionFromCoords(const glm::ivec2 &Coords) { Position = glm::vec2(Coords) + glm::vec2(0.5f); }
		void GetDirectionFromInput(int InputState, glm::vec2 &Direction);
		glm::ivec2 GetTilePosition() const { return Position; }
		int Move();

		// Trader
		void AcceptTrader(std::vector<_Slot> &Slots);

		// Minigames
		void SendSeed(bool Generate);

		// Map
		bool CanRespec() const;
		const _Tile *GetTile() const;

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
		_Light *Light;
		ae::_Shape Shape;

		// Pointers
		const _Stats *Stats;
		_Map *Map;
		_Scripting *Scripting;
		_Server *Server;
		ae::_Peer *Peer;

		// Movement
		glm::vec2 Position;
		glm::vec2 ServerPosition;

		// Render
		const _Model *Model;
		const ae::_Texture *BuildTexture;

	private:

};

inline bool CompareObjects(const _Object *First, const _Object *Second) {
	return First->NetworkID < Second->NetworkID;
}
