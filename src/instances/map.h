/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#pragma once

// Libraries
#include <irrlicht.h>
#include <vector>
#include <list>
#include <cstdint>

const int MAP_TILE_WIDTH = 32;
const int MAP_TILE_HEIGHT = 32;

// Structures
struct TileStruct {
	TileStruct() : Texture(NULL), Zone(0), EventType(0), EventData(0), Wall(false), PVP(true) { }
	irr::video::ITexture *Texture;
	int Zone;
	int EventType, EventData;
	bool Wall;
	bool PVP;
};

struct IndexedEventStruct {
	IndexedEventStruct(const TileStruct *TTile, const irr::core::position2di &TPosition) : Tile(TTile), Position(TPosition) { }
	const TileStruct *Tile;
	irr::core::position2di Position;
};

// Forward Declarations
class ObjectClass;
class PlayerClass;
class _Packet;

// Classes
class MapClass {

	public:

		enum EventType {
			EVENT_NONE,
			EVENT_SPAWN,
			EVENT_MAPCHANGE,
			EVENT_VENDOR,
			EVENT_TRADER,
			EVENT_COUNT
		};

		MapClass(const irr::core::stringc &TFilename, int TWidth, int THeight);
		MapClass(const irr::core::stringc &TFilename);
		MapClass(int TMapID);
		~MapClass();

		void Update(uint32_t TDeltaTime);

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
		void AddObject(ObjectClass *TObject);
		void RemoveObject(ObjectClass *TObject);
		const std::list<ObjectClass *> &GetObjects() const;
		void GetClosePlayers(const PlayerClass *TPlayer, float TDistanceSquared, std::list<PlayerClass *> &TPlayers);
		PlayerClass *GetClosestPlayer(const PlayerClass *TPlayer, float TMaxDistanceSquared, int TState);

		void SendPacketToPlayers(_Packet *TPacket, PlayerClass *ExceptionPlayer=NULL);

		// Events
		IndexedEventStruct *GetIndexedEvent(int TEventType, int TEventData);

		// Map editing
		int GetWidth() const { return Width; }
		int GetHeight() const { return Height; }
		bool IsValidPosition(int TX, int TY) const { return TX >= 0 && TY >= 0 && TX < Width && TY < Height; }
		void SetNoZoneTexture(irr::video::ITexture *TTexture) { NoZoneTexture = TTexture; }

		void GetTile(int TX, int TY, TileStruct &TTile) const { TTile = Tiles[TX][TY]; }
		const TileStruct *GetTile(int TX, int TY) const { return &Tiles[TX][TY]; }
		void SetTile(int TX, int TY, const TileStruct *TTile) { Tiles[TX][TY] = *TTile; }

		// File IO
		int SaveMap();
		int LoadMap();

	private:

		void Init();

		void AllocateMap();
		void FreeMap();

		void SendObjectUpdates();

		void GetTextureListFromMap(std::vector<irr::video::ITexture *> &TTextures);
		int GetTextureIndex(std::vector<irr::video::ITexture *> &TTextures, irr::video::ITexture *TTexture);

		// Map file
		int ID;
		irr::core::stringc Filename;

		// Viewing
		irr::core::dimension2di ViewSize;
		irr::core::position2di CameraScroll;

		// Map data
		TileStruct **Tiles;
		int Width, Height;

		// Textures
		irr::video::ITexture *NoZoneTexture;
		std::vector<irr::video::ITexture *> Textures;

		// Events
		std::vector<IndexedEventStruct> IndexedEvents;

		// Objects
		uint32_t ObjectUpdateTime;
		std::list<ObjectClass *> Objects;
};
