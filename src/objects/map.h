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
#include <enums.h>
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

// Constants
const int MAP_VERSION = 1;
const int MAP_TILE_WIDTH = 64;
const int MAP_TILE_HEIGHT = 64;
const int MAP_LAYERS = 4;
const double MAP_DAY_LENGTH = 24.0*60.0;
const double MAP_CLOCK_SPEED = 1.0;
const glm::vec4 MAP_AMBIENT_LIGHT = glm::vec4(0.3, 0.3, 0.3, 1);
const std::string MAPS_PATH = "maps/";

// Forward Declarations
class _Object;
class _Server;
class _Stats;
class _Battle;

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
	_Event() : Type(EventType::NONE) { }
	_Event(EventType Type, const std::string &Data) : Type(Type), Data(Data) { }

	bool operator==(const _Event &Event) const { return Event.Type == Type && Event.Data == Data; }
	bool operator<(const _Event &Event) const { return std::tie(Event.Type, Event.Data) < std::tie(Type, Data); }

	EventType Type;
	std::string Data;
};

struct _Tile {
	_Tile() : TextureIndex{0, 0, 0, 0}, BaseTextureIndex(0), Hierarchy(0), Wall(false), PVP(false) { }
	uint32_t TextureIndex[MAP_LAYERS];
	uint32_t BaseTextureIndex;
	int Hierarchy;
	_Event Event;
	std::string ZoneID;
	bool Wall;
	bool PVP;
};

struct _TileVertexBuffer {
	float Data[5];
};

// Classes
class _Map : public ae::_BaseObject, public micropather::Graph {

	public:

		_Map();
		~_Map() override;

		void AllocateMap();
		void ResizeMap(glm::ivec2 Offset, glm::ivec2 NewSize);
		void InitAtlas(const std::string AtlasPath);
		void InitVertices(bool Static=false);

		void Update(double FrameTime) override;

		// Events
		void CheckEvents(_Object *Object);
		void IndexEvents();
		void GetClockAsString(std::stringstream &Buffer) const;
		void SetAmbientLightByClock();
		void StartEvent(_Object *Object, _Event Event) const;
		bool IsPVPZone(const glm::ivec2 &Position) const;

		// Graphics
		void BuildLayers(bool NoTrans=false);
		void Render(ae::_Camera *Camera, ae::_Framebuffer *Framebuffer, _Object *ClientPlayer, double BlendFactor, int RenderFlags=0);
		void RenderTiles(const std::string &Program, glm::vec4 &Bounds, const glm::vec3 &Offset, bool Static=false);
		int AddLights(const std::list<_Object *> *ObjectList, const ae::_Program *Program, glm::vec4 AABB);

		// Collision
		bool CanMoveTo(const glm::ivec2 &Position, _Object *Object);

		// Network
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
		void Load(const std::string &Path, bool Static=false);
		bool Save(const std::string &Path);

		void NodeToPosition(void *Node, glm::ivec2 &Position) {
			int Index = (int)(intptr_t)Node;
			Position.y = Index / Size.x;
			Position.x = Index - Position.y * Size.x;
		}

		void *PositionToNode(const glm::ivec2 &Position) { return (void *)(intptr_t)(Position.y * Size.x + Position.x); }

		// Map data
		bool Loaded;
		std::string Name;
		_Tile **Tiles;
		glm::ivec2 Size;
		std::map<_Event, std::vector<glm::ivec2>> IndexedEvents;

		// Graphics
		bool UseAtlas;
		const ae::_Atlas *TileAtlas;
		const ae::_Atlas *TransAtlas;
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

		void CloseAtlas();
		void FreeMap();

		// Path finding
		float LeastCostEstimate(void *StateStart, void *StateEnd) override;
		void AdjacentCost(void *State, std::vector<micropather::StateCost> *Neighbors) override;
		void PrintStateInfo(void *State) override { }

		// Tiles
		uint32_t GetTransition(_Tile &Tile, const glm::ivec2 &CheckPosition, uint32_t Bit);

		// Rendering
		uint32_t TileVertexBufferID;
		uint32_t TileElementBufferID;
		_TileVertexBuffer *TileVertices;
		glm::u32vec3 *TileFaces;

		// Network
		std::list<const ae::_Peer *> Peers;

};
