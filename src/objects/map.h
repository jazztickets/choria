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
#include <ae/baseobject.h>
#include <ae/network.h>
#include <ae/texture.h>
#include <path/micropather.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/fwd.hpp>
#include <vector>
#include <list>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <map>
#include <sstream>

// Forward Declarations
class _Object;
class _Server;
class _Stats;
class _Battle;
class _Scripting;
struct _MapStat;

namespace ae {
	class _Buffer;
	class _Camera;
	class _Atlas;
	class _Program;
	class _Peer;
	class _Framebuffer;
}

// Structures
struct _Event {
	_Event() : Type(0), Data(0) { }
	_Event(uint32_t Type, uint32_t Data) : Type(Type), Data(Data) { }

	bool operator==(const _Event &Event) const { return Event.Type == Type && Event.Data == Data; }
	bool operator<(const _Event &Event) const { return std::tie(Event.Type, Event.Data) < std::tie(Type, Data); }

	uint32_t Type;
	uint32_t Data;
};

struct _Tile {
	_Tile() : TextureIndex{0, 0}, Zone(0), Wall(false), PVP(false) { }
	uint32_t TextureIndex[2];
	uint32_t Zone;
	_Event Event;
	bool Wall;
	bool PVP;
};

// Classes
class _Map : public ae::_BaseObject, public micropather::Graph {

	public:

		enum EventType {
			EVENT_NONE,
			EVENT_SPAWN,
			EVENT_MAPENTRANCE,
			EVENT_MAPCHANGE,
			EVENT_VENDOR,
			EVENT_TRADER,
			EVENT_KEY,
			EVENT_SCRIPT,
			EVENT_PORTAL,
			EVENT_JUMP,
			EVENT_BLACKSMITH,
			EVENT_MINIGAME,
			EVENT_COUNT
		};

		_Map();
		~_Map();

		void AllocateMap();
		void ResizeMap(glm::ivec2 Offset, glm::ivec2 NewSize);
		void InitAtlas(const std::string AtlasPath, bool Static=false);
		void CloseAtlas();

		void Update(double FrameTime) override;

		// Events
		void CheckEvents(_Object *Object, _Scripting *Scripting) const;
		void CheckBattle(_Object *Object, const _Tile *Tile) const;
		void IndexEvents();
		void GetClockAsString(std::stringstream &Buffer) const;
		void SetAmbientLightByClock();
		void StartEvent(_Object *Object, _Event Event) const;
		bool IsPVPZone(const glm::ivec2 &Position) const;

		// Graphics
		void Render(ae::_Camera *Camera, ae::_Framebuffer *Framebuffer, _Object *ClientPlayer, double BlendFactor, int RenderFlags=0);
		void RenderLayer(const std::string &Program, glm::vec4 &Bounds, const glm::vec3 &Offset, int Layer, bool Static=false);
		int AddLights(const std::list<_Object *> *ObjectList, const ae::_Program *Program, glm::vec4 AABB);

		// Collision
		bool CanMoveTo(const glm::ivec2 &Position, _Object *Object);

		// Peer management
		void BroadcastPacket(ae::_Buffer &Buffer, ae::_Network::SendType Type=ae::_Network::RELIABLE);

		// Object management
		void SendObjectUpdates();
		void AddObject(_Object *Object);
		void RemoveObject(const _Object *RemoveObject);
		void SendObjectList(ae::_Peer *Peer);
		void GetPotentialBattlePlayers(const _Object *Player, float DistanceSquared, size_t Max, std::list<_Object *> &Players);
		_Battle *GetCloseBattle(const _Object *Player, bool &HitPrivateParty);
		void GetPVPPlayers(const _Object *Player, std::list<_Object *> &Players, bool UsePVPZone);
		_Object *FindTradePlayer(const _Object *Player, float MaxDistanceSquared);
		_Object *FindDeadPlayer(const _Object *Player, float MaxDistanceSquared);
		bool FindEvent(const _Event &Event, glm::ivec2 &Position) const;
		void DeleteStaticObject(const glm::ivec2 &Position);

		// Map editing
		bool IsValidPosition(const glm::ivec2 &Position) const { return Position.x >= 0 && Position.y >= 0 && Position.x < Size.x && Position.y < Size.y; }
		glm::vec2 GetValidPosition(const glm::vec2 &Position) const;
		glm::ivec2 GetValidCoord(const glm::ivec2 &Position) const;

		void GetTile(const glm::ivec2 &Position, _Tile &Tile) const { Tile = Tiles[Position.x][Position.y]; }
		const _Tile *GetTile(const glm::ivec2 &Position) const { return &Tiles[Position.x][Position.y]; }
		void SetTile(const glm::ivec2 &Position, const _Tile *Tile) { Tiles[Position.x][Position.y] = *Tile; }

		// File IO
		void Load(const _MapStat *MapStat, bool Static=false);
		bool Save(const std::string &Path);

		void NodeToPosition(void *Node, glm::ivec2 &Position) {
			int Index = (int)(intptr_t)Node;
			Position.y = Index / Size.x;
			Position.x = Index - Position.y * Size.x;
		}

		void *PositionToNode(const glm::ivec2 &Position) { return (void *)(intptr_t)(Position.y * Size.x + Position.x); }

		// Map data
		_Tile **Tiles;
		glm::ivec2 Size;
		std::map<_Event, std::vector<glm::ivec2>> IndexedEvents;

		// Graphics
		bool UseAtlas;
		const ae::_Atlas *TileAtlas;
		glm::vec4 AmbientLight;
		int IsOutside;
		double Clock;

		// Background
		std::string BackgroundMapFile;
		glm::vec3 BackgroundOffset;
		_Map *BackgroundMap;

		// Objects
		std::list<_Object *> Objects;
		std::list<_Object *> StaticObjects;
		double ObjectUpdateTime;

		// Stats
		const _Stats *Stats;

		// Audio
		std::string Music;

		// Network
		_Server *Server;

		// Editor
		uint32_t MaxZoneColors;
		uint32_t CurrentZoneColors;

		// Path finding
		micropather::MicroPather *Pather;

	private:

		void FreeMap();

		// Path finding
		float LeastCostEstimate(void *StateStart, void *StateEnd) override;
		void AdjacentCost(void *State, std::vector<micropather::StateCost> *Neighbors) override;
		void PrintStateInfo(void *State) override { }

		// Rendering
		uint32_t TileVertexBufferID[2];
		uint32_t TileElementBufferID;
		glm::vec4 *TileVertices[2];
		glm::u32vec3 *TileFaces;

		// Network
		std::list<const ae::_Peer *> Peers;

};
