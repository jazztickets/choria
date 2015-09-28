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

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

const int MAP_TILE_WIDTH	= 32;
const int MAP_TILE_HEIGHT	= 32;

// Structures
struct TileStruct {
	TileStruct() : Texture(NULL), Zone(0), EventType(0), EventData(0), Wall(false), PVP(true) { }
	ITexture *Texture;
	int Zone;
	int EventType, EventData;
	bool Wall;
	bool PVP;
};

struct IndexedEventStruct {
	IndexedEventStruct(const TileStruct *TTile, const position2di &TPosition) : Tile(TTile), Position(TPosition) { }
	const TileStruct *Tile;
	position2di Position;
};

// Forward Declarations
class ObjectClass;
class PlayerClass;
class PacketClass;

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

		MapClass(const stringc &TFilename, int TWidth, int THeight);
		MapClass(const stringc &TFilename);
		MapClass(int TMapID);
		~MapClass();

		void Update(u32 TDeltaTime);

		// States
		int GetID() const { return ID; }
		const stringc &GetFilename() const { return Filename; }

		// Graphics
		void Render();
		void RenderForMapEditor(bool TDrawWall, bool TDrawZone, bool TDrawPVP);

		void SetCameraScroll(const position2di &TPosition);
		const position2di &GetCameraScroll() const { return CameraScroll; }
		const dimension2di &GetViewSize() const { return ViewSize; }

		bool GridToScreen(const position2di &TGridPosition, position2di &TScreenPosition) const;
		void ScreenToGrid(const position2di &TScreenPosition, position2di &TGridPosition) const;

		// Collision
		bool CanMoveTo(const position2di &TPosition);

		// Object management
		void AddObject(ObjectClass *TObject);
		void RemoveObject(ObjectClass *TObject);
		const list<ObjectClass *> &GetObjects() const;
		void GetClosePlayers(const PlayerClass *TPlayer, float TDistanceSquared, list<PlayerClass *> &TPlayers);
		PlayerClass *GetClosestPlayer(const PlayerClass *TPlayer, float TMaxDistanceSquared, int TState);

		void SendPacketToPlayers(PacketClass *TPacket, PlayerClass *ExceptionPlayer=NULL);

		// Events
		IndexedEventStruct *GetIndexedEvent(int TEventType, int TEventData);

		// Map editing
		int GetWidth() const { return Width; }
		int GetHeight() const { return Height; }
		bool IsValidPosition(int TX, int TY) const { return TX >= 0 && TY >= 0 && TX < Width && TY < Height; }
		void SetNoZoneTexture(ITexture *TTexture) { NoZoneTexture = TTexture; }

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

		void GetTextureListFromMap(array<ITexture *> &TTextures);
		int GetTextureIndex(array<ITexture *> &TTextures, ITexture *TTexture);

		// Map file
		int ID;
		stringc Filename;

		// Viewing
		dimension2di ViewSize;
		position2di CameraScroll;

		// Map data
		TileStruct **Tiles;
		int Width, Height;

		// Textures
		ITexture *NoZoneTexture;
		array<ITexture *> Textures;

		// Events
		array<IndexedEventStruct> IndexedEvents;

		// Objects
		u32 ObjectUpdateTime;
		list<ObjectClass *> Objects;
};

