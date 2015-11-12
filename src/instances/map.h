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
#include <irrlicht.h>

// Structures
struct _Tile {
	_Tile() : Texture(nullptr), Zone(0), EventType(0), EventData(0), Wall(false), PVP(true) { }
	_Texture *Texture;
	int32_t Zone;
	int32_t EventType;
	int32_t EventData;
	bool Wall;
	bool PVP;
};

struct _IndexedEvent {
	_IndexedEvent(const _Tile *Tile, const irr::core::position2di &Position) : Tile(Tile), Position(Position) { }
	const _Tile *Tile;
	irr::core::position2di Position;
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

		_Map(const irr::core::stringc &TFilename, int TWidth, int THeight);
		_Map(const irr::core::stringc &TFilename);
		_Map(int TMapID);
		~_Map();

		void Update(double FrameTime);

		// States
		int GetID() const { return ID; }
		const irr::core::stringc &GetFilename() const { return Filename; }

		// Graphics
		void Render();
		void RenderForMapEditor(bool TDrawWall, bool TDrawZone, bool TDrawPVP);

		void SetCameraScroll(const irr::core::position2di &TPosition);
		const irr::core::position2di &GetCameraScroll() const { return CameraScroll; }
		const irr::core::dimension2di &GetViewSize() const { return ViewSize; }

		bool GridToScreen(const irr::core::position2di &TGridPosition, irr::core::position2di &TScreenPosition) const;
		void ScreenToGrid(const irr::core::position2di &TScreenPosition, irr::core::position2di &TGridPosition) const;

		// Collision
		bool CanMoveTo(const irr::core::position2di &TPosition);

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
		void SetNoZoneTexture(_Texture *TTexture) { NoZoneTexture = TTexture; }

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

		void GetTextureListFromMap(std::vector<_Texture *> &TTextures);
		int GetTextureIndex(std::vector<_Texture *> &TTextures, _Texture *TTexture);

		// Map file
		int ID;
		irr::core::stringc Filename;

		// Viewing
		irr::core::dimension2di ViewSize;
		irr::core::position2di CameraScroll;

		// Map data
		_Tile **Tiles;
		int32_t Width, Height;

		// Textures
		const _Texture *NoZoneTexture;
		const _Texture *DefaultNoZoneTexture;
		std::vector<_Texture *> Textures;

		// Events
		std::vector<_IndexedEvent> IndexedEvents;

		// Objects
		double ObjectUpdateTime;
		std::list<_Object *> Objects;
};
