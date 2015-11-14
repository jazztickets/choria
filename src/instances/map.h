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

struct _IndexedEvent {
	_IndexedEvent(const _Tile *Tile, const glm::ivec2 &Position) : Tile(Tile), Position(Position) { }
	const _Tile *Tile;
	glm::ivec2 Position;
};

// Forward Declarations
class _Object;
class _Player;
class _Buffer;

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

		_Map(const std::string &TFilename, int TWidth, int THeight);
		_Map(const std::string &TFilename);
		_Map(int TMapID);
		~_Map();

		void Update(double FrameTime);

		// States
		int GetID() const { return ID; }
		const std::string &GetFilename() const { return Filename; }

		// Graphics
		void Render();
		void RenderForMapEditor(bool TDrawWall, bool TDrawZone, bool TDrawPVP);

		void SetCameraScroll(const glm::ivec2 &TPosition);
		const glm::ivec2 &GetCameraScroll() const { return CameraScroll; }
		const glm::ivec2 &GetViewSize() const { return ViewSize; }

		bool GridToScreen(const glm::ivec2 &TGridPosition, glm::ivec2 &TScreenPosition) const;
		void ScreenToGrid(const glm::ivec2 &TScreenPosition, glm::ivec2 &TGridPosition) const;

		// Collision
		bool CanMoveTo(const glm::ivec2 &TPosition);

		// Object management
		void AddObject(_Object *TObject);
		void RemoveObject(_Object *TObject);
		const std::list<_Object *> &GetObjects() const;
		void GetClosePlayers(const _Player *TPlayer, float TDistanceSquared, std::list<_Player *> &TPlayers);
		_Player *GetClosestPlayer(const _Player *TPlayer, float TMaxDistanceSquared, int TState);

		void SendPacketToPlayers(_Buffer *TPacket, _Player *ExceptionPlayer=nullptr, _Network::SendType Type=_Network::RELIABLE);

		// Events
		_IndexedEvent *GetIndexedEvent(int TEventType, int TEventData);

		// Map editing
		int GetWidth() const { return Width; }
		int GetHeight() const { return Height; }
		bool IsValidPosition(int TX, int TY) const { return TX >= 0 && TY >= 0 && TX < Width && TY < Height; }
		void SetNoZoneTexture(const _Texture *TTexture) { NoZoneTexture = TTexture; }

		void GetTile(int TX, int TY, _Tile &TTile) const { TTile = Tiles[TX][TY]; }
		const _Tile *GetTile(int TX, int TY) const { return &Tiles[TX][TY]; }
		void SetTile(int TX, int TY, const _Tile *TTile) { Tiles[TX][TY] = *TTile; }

		// File IO
		int SaveMap();
		int LoadMap();

	private:

		void Init();

		void AllocateMap();
		void FreeMap();

		void SendObjectUpdates();

		void GetTextureListFromMap(std::vector<const _Texture *> &TTextures);
		int GetTextureIndex(std::vector<const _Texture *> &TTextures, const _Texture *TTexture);

		// Map file
		int ID;
		std::string Filename;

		// Viewing
		glm::ivec2 ViewSize;
		glm::ivec2 CameraScroll;

		// Map data
		_Tile **Tiles;
		int32_t Width, Height;

		// Textures
		const _Texture *NoZoneTexture;
		const _Texture *DefaultNoZoneTexture;
		std::vector<const _Texture *> Textures;

		// Events
		std::vector<_IndexedEvent> IndexedEvents;

		// Objects
		double ObjectUpdateTime;
		std::list<_Object *> Objects;
};
