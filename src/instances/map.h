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
#include <network/network.h>
#include <packet.h>
#include <texture.h>
#include <vector>
#include <list>
#include <cstdint>
#include <string>
#include <states/editor.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <unordered_map>

// Forward Declarations
class _Object;
class _Buffer;
class _Camera;
class _Server;
class _Stats;
class _Peer;

// Structures
struct _Tile {
	_Tile() : Texture(nullptr), Zone(0), EventType(0), EventData(0), Wall(false), PVP(true) { }
	const _Texture *Texture;
	int Zone;
	int EventType;
	int EventData;
	bool Wall;
	bool PVP;
};

// Classes
class _Map {

	public:

		enum EventType {
			EVENT_NONE,
			EVENT_SPAWN,
			EVENT_MAPCHANGE,
			EVENT_VENDOR,
			EVENT_TRADER,
			EVENT_COUNT
		};

		_Map();
		_Map(const glm::ivec2 &Size);
		_Map(int ID, _Stats *Stats);
		~_Map();

		void Update(double FrameTime);
		void CheckEvents(_Object *Object);

		// Graphics
		void Render(_Camera *Camera, _Stats *Stats, _Object *ClientPlayer, int RenderFlags=0);

		// Collision
		bool CanMoveTo(const glm::ivec2 &Position);

		// Peer management
		void BroadcastPacket(_Buffer &Buffer, _Network::SendType Type=_Network::RELIABLE);
		const std::list<const _Peer *> &GetPeers() const { return Peers; }
		void AddPeer(const _Peer *Peer) { Peers.push_back(Peer); }
		void RemovePeer(const _Peer *Peer);

		// Object management
		NetworkIDType GenerateObjectID();
		void SendObjectUpdates();
		void DeleteObjects();
		_Object *GetObjectByID(NetworkIDType ObjectID);
		void AddObject(_Object *Object);
		void AddObject(_Object *Object, NetworkIDType NetworkID);
		void RemoveObject(const _Object *RemoveObject);
		void SendObjectList(_Peer *Peer);
		void GetClosePlayers(const _Object *Player, float DistanceSquared, std::list<_Object *> &Players);
		_Object *FindTradePlayer(const _Object *Player, float MaxDistanceSquared);
		bool FindEvent(int EventType, int EventData, glm::ivec2 &Position);

		// Map editing
		bool IsValidPosition(const glm::ivec2 &Position) const { return Position.x >= 0 && Position.y >= 0 && Position.x < Size.x && Position.y < Size.y; }
		glm::vec2 GetValidPosition(const glm::vec2 &Position);
		glm::ivec2 GetValidCoord(const glm::ivec2 &Position);

		void GetTile(const glm::ivec2 &Position, _Tile &Tile) const { Tile = Tiles[Position.x][Position.y]; }
		const _Tile *GetTile(const glm::ivec2 &Position) const { return &Tiles[Position.x][Position.y]; }
		void SetTile(const glm::ivec2 &Position, const _Tile *Tile) { Tiles[Position.x][Position.y] = *Tile; }

		// File IO
		void Load(const std::string &Path);
		bool Save(const std::string &Path);

		// Map file
		uint32_t ID;

		// Map data
		_Tile **Tiles;
		glm::ivec2 Size;

		// Graphics
		glm::vec4 AmbientLight;
		const _Texture *NoZoneTexture;

		// Objects
		std::list<_Object *> Objects;
		double ObjectUpdateTime;

		// Network
		_Server *Server;

	private:

		void AllocateMap();
		void FreeMap();

		// Network
		std::list<const _Peer *> Peers;
		NetworkIDType ObjectUpdateCount;

		// Object creation
		std::unordered_map<NetworkIDType, bool> ObjectIDs;
		NetworkIDType NextObjectID;

};
