/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef MAP_H
#define MAP_H

// Libraries
#include <irrlicht.h>

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

const int MAP_TILE_WIDTH = 32;
const int MAP_TILE_HEIGHT = 32;

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
	IndexedEventStruct(const TileStruct *Tile, const position2di &Position) : Tile(Tile), Position(Position) { }
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

		MapClass(const stringc &Filename, int Width, int Height);
		MapClass(const stringc &Filename);
		MapClass(int MapID);
		~MapClass();

		void Update(u32 FrameTime);

		// States
		int GetID() const { return ID; }
		const stringc &GetFilename() const { return Filename; }

		// Graphics
		void Render();
		void RenderForMapEditor(bool DrawWall, bool DrawZone, bool DrawPVP);

		void SetCameraScroll(const position2di &Position);
		const position2di &GetCameraScroll() const { return CameraScroll; }
		const dimension2di &GetViewSize() const { return ViewSize; }

		bool GridToScreen(const position2di &GridPosition, position2di &ScreenPosition) const;

		// Collision
		bool CanMoveTo(const position2di &Position);

		// Object management
		void AddObject(ObjectClass *Object);
		void RemoveObject(ObjectClass *Object);
		const list<ObjectClass *> &GetObjects() const;
		void GetClosePlayers(const PlayerClass *Player, float DistanceSquared, list<PlayerClass *> &PlayerList);
		PlayerClass *GetClosestPlayer(const PlayerClass *Player, float MaxDistanceSquared, int State);

		void SendPacketToPlayers(PacketClass *Packet, PlayerClass *ExceptionPlayer=NULL);

		// Events
		IndexedEventStruct *GetIndexedEvent(int EventType, int EventData);

		// Map editing
		int GetWidth() const { return Width; }
		int GetHeight() const { return Height; }
		bool IsValidPosition(int X, int Y) const { return X >= 0 && Y >= 0 && X < Width && Y < Height; }
		void SetNoZoneTexture(ITexture *Texture) { NoZoneTexture = Texture; }

		void GetTile(int X, int Y, TileStruct &Tile) const { Tile = Tiles[X][Y]; }
		const TileStruct *GetTile(int X, int Y) const { return &Tiles[X][Y]; }
		void SetTile(int X, int Y, const TileStruct *Tile) { Tiles[X][Y] = *Tile; }

		// File IO
		int SaveMap();
		int LoadMap();

	private:

		void Init();

		void AllocateMap();
		void FreeMap();

		void SendObjectUpdates();

		void GetTextureListFromMap(array<ITexture *> &Textures);
		int GetTextureIndex(array<ITexture *> &Textures, ITexture *Texture);

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

#endif
