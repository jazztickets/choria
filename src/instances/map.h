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
#include <texture.h>
#include <vector>
#include <list>
#include <cstdint>
#include <string>
#include <states/editor.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

// Forward Declarations
class _Object;
class _Player;
class _Buffer;
class _Camera;

// Structures
struct _Tile {
	_Tile() : Texture(nullptr), Zone(0), EventType(0), EventData(0), Wall(false), PVP(true) { }
	const _Texture *Texture;
	int32_t Zone;
	int32_t EventType;
	int32_t EventData;
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

		_Map(const std::string &Filename, const glm::ivec2 &Size);
		_Map(const std::string &Filename);
		_Map(int ID);
		~_Map();

		void Update(double FrameTime);

		// Graphics
		void Render(_Camera *Camera, int RenderFlags=0);

		// Collision
		bool CanMoveTo(const glm::ivec2 &Position);

		// Object management
		void AddObject(_Object *Object);
		void RemoveObject(_Object *RemoveObject);
		void GetClosePlayers(const _Player *Player, float DistanceSquared, std::list<_Player *> &Players);
		_Player *GetClosestPlayer(const _Player *Player, float MaxDistanceSquared, int State);
		bool FindEvent(int EventType, int EventData, glm::ivec2 &Position);

		void SendPacketToPlayers(_Buffer *Packet, _Player *ExceptionPlayer=nullptr, _Network::SendType Type=_Network::RELIABLE);

		// Map editing
		bool IsValidPosition(const glm::ivec2 &Position) const { return Position.x >= 0 && Position.y >= 0 && Position.x < Size.x && Position.y < Size.y; }
		glm::vec2 GetValidPosition(const glm::vec2 &Position);

		void GetTile(const glm::ivec2 &Position, _Tile &Tile) const { Tile = Tiles[Position.x][Position.y]; }
		const _Tile *GetTile(const glm::ivec2 &Position) const { return &Tiles[Position.x][Position.y]; }
		void SetTile(const glm::ivec2 &Position, const _Tile *Tile) { Tiles[Position.x][Position.y] = *Tile; }

		// File IO
		void Load();
		void Save(const std::string &String);

		// Map file
		int ID;
		std::string Filename;

		// Map data
		_Tile **Tiles;
		glm::ivec2 Size;

		// Graphics
		glm::vec4 AmbientLight;
		const _Texture *NoZoneTexture;

		// Objects
		double ObjectUpdateTime;
		std::list<_Object *> Objects;

	private:

		void Init();

		void AllocateMap();
		void FreeMap();

		void SendObjectUpdates();

		void GetTextureListFromMap(std::vector<const _Texture *> &SearchTextures);
		int GetTextureIndex(std::vector<const _Texture *> &SearchTextures, const _Texture *Texture);

};
