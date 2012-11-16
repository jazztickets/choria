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
#include "map.h"
#include "../engine/globals.h"
#include "../engine/graphics.h"
#include "../network/network.h"
#include "../network/packetstream.h"
#include "../engine/filestream.h"
#include "../engine/stats.h"
#include "../server.h"
#include "../objects/player.h"

// Constants
const int SCROLLMIN_X = 2;
const int SCROLLMIN_Y = 2;

// Constructor for the map editor: new map
MapClass::MapClass(const stringc &Filename, int Width, int Height) {
	Init();

	this->Filename = Filename;
	this->Width = Width;
	this->Height = Height;

	AllocateMap();
}

// Constructor for the map editor: load map
MapClass::MapClass(const stringc &Filename) {
	Init();
	
	this->Filename = Filename;
}

// Constructor for maps already created in the database
MapClass::MapClass(int MapID) {
	Init();

	// Set ID
	this->ID = MapID;

	// Get map info
	const MapStruct *Map = Stats.GetMap(ID);
	ViewSize.Width = Map->ViewWidth;
	ViewSize.Height = Map->ViewHeight;

	// Load map
	Filename = Map->File;
	LoadMap();
}

// Destructor
MapClass::~MapClass() {

	// Delete map data
	FreeMap();	
}

// Initialize variables
void MapClass::Init() {
	ID = 0;
	NoZoneTexture = NULL;
	Tiles = NULL;
	ViewSize.Width = 25;
	ViewSize.Height = 19;
	CameraScroll.X = SCROLLMIN_X;
	CameraScroll.Y = SCROLLMIN_Y;
}

// Free memory used by the tiles
void MapClass::FreeMap() {
	if(Tiles) {
		for(int i = 0; i < Width; i++)
			delete[] Tiles[i];
		delete[] Tiles;

		Tiles = NULL;
	}

	IndexedEvents.clear();
}

// Allocates memory for the map
void MapClass::AllocateMap() {
	if(Tiles)
		return;

	Tiles = new TileStruct*[Width];
	for(int i = 0; i < Width; i++) {
		Tiles[i] = new TileStruct[Height];
	}

	// Delete textures
	for(u32 i = 0; i < Textures.size(); i++)
		irrDriver->removeTexture(Textures[i]);
	Textures.clear();
}

// Updates the map and sends object updates
void MapClass::Update(u32 FrameTime) {

	ObjectUpdateTime += FrameTime;
	if(ObjectUpdateTime > 200) {
		ObjectUpdateTime = 0;

		SendObjectUpdates();
	}
}

// Renders the map
void MapClass::Render() {

	position2di GridPosition, DrawPosition;
	for(int i = 0; i < ViewSize.Width; i++) {
		for(int j = 0; j < ViewSize.Height; j++) {

			// Get the actual grid coordinate
			GridPosition.X = i + CameraScroll.X - ViewSize.Width / 2;
			GridPosition.Y = j + CameraScroll.Y - ViewSize.Height / 2;
			DrawPosition = position2di((i - ViewSize.Width / 2) * MAP_TILE_WIDTH + 400, (j - ViewSize.Height / 2) * MAP_TILE_HEIGHT + 300);
			if(NoZoneTexture)
				Graphics.DrawCenteredImage(NoZoneTexture, DrawPosition.X, DrawPosition.Y);

			// Validate coordinate
			if(GridPosition.X >= 0 && GridPosition.X < Width && GridPosition.Y >= 0 && GridPosition.Y < Height) {
				TileStruct *Tile = &Tiles[GridPosition.X][GridPosition.Y];

				if(Tile->Texture)
					Graphics.DrawCenteredImage(Tile->Texture, DrawPosition.X, DrawPosition.Y);
			}
		}
	}
}

// Renders the map for editor
void MapClass::RenderForMapEditor(bool DrawWall, bool DrawZone, bool DrawPVP) {
	
	position2di GridPosition, DrawPosition;
	for(int i = 0; i < ViewSize.Width; i++) {
		for(int j = 0; j < ViewSize.Height; j++) {

			// Get the actual grid coordinate
			GridPosition.X = i + CameraScroll.X - ViewSize.Width / 2;
			GridPosition.Y = j + CameraScroll.Y - ViewSize.Height / 2;
			DrawPosition = position2di((i - ViewSize.Width / 2) * MAP_TILE_WIDTH + 400, (j - ViewSize.Height / 2) * MAP_TILE_HEIGHT + 300);
			if(NoZoneTexture)
				Graphics.DrawCenteredImage(NoZoneTexture, DrawPosition.X, DrawPosition.Y);

			// Validate coordinate
			if(GridPosition.X >= 0 && GridPosition.X < Width && GridPosition.Y >= 0 && GridPosition.Y < Height) {
				TileStruct *Tile = &Tiles[GridPosition.X][GridPosition.Y];

				// Draw texture
				if(Tile->Texture)
					Graphics.DrawCenteredImage(Tile->Texture, DrawPosition.X, DrawPosition.Y);
				else if(NoZoneTexture)
					Graphics.DrawCenteredImage(NoZoneTexture, DrawPosition.X, DrawPosition.Y);

				// Draw wall
				if(DrawWall && Tile->Wall)
					Graphics.RenderText("W", DrawPosition.X, DrawPosition.Y - 8, GraphicsClass::ALIGN_CENTER);

				// Draw zone
				if(!Tile->Wall) {
					if(DrawZone && Tile->Zone > 0)
						Graphics.RenderText(stringc(Tile->Zone).c_str(), DrawPosition.X, DrawPosition.Y - 8, GraphicsClass::ALIGN_CENTER);

					// Draw PVP
					if(DrawPVP && Tile->PVP)
						Graphics.RenderText("PvP", DrawPosition.X, DrawPosition.Y - 8, GraphicsClass::ALIGN_CENTER, SColor(255, 255, 0, 0));
				}

				// Draw event info
				if(Tile->EventType > 0) {
					stringc EventText = Stats.GetEvent(Tile->EventType)->ShortName + stringc(", ") + stringc(Tile->EventData);
					Graphics.RenderText(EventText.c_str(), DrawPosition.X - 16, DrawPosition.Y - 16, GraphicsClass::ALIGN_LEFT, SColor(255, 0, 255, 255));
				}
			}
			else {
				Graphics.DrawCenteredImage(irrDriver->getTexture("textures/editor/nozone.png"), DrawPosition.X, DrawPosition.Y);
			}
		}
	}
}

// Sets the camera scroll position
void MapClass::SetCameraScroll(const position2di &Position) {

	CameraScroll = Position;
	if(CameraScroll.X < SCROLLMIN_X)
		CameraScroll.X = SCROLLMIN_X;
	if(CameraScroll.Y < SCROLLMIN_Y)
		CameraScroll.Y = SCROLLMIN_Y;
	if(CameraScroll.X >= Width - SCROLLMIN_X)
		CameraScroll.X = Width - SCROLLMIN_X;
	if(CameraScroll.Y >= Height - SCROLLMIN_Y)
		CameraScroll.Y = Height - SCROLLMIN_Y;
}

// Converts a grid position on the map to a screen coordinate
bool MapClass::GridToScreen(const position2di &GridPosition, position2di &ScreenPosition) const {

	// Get delta from center
	position2di CenterDelta(GridPosition.X - CameraScroll.X, GridPosition.Y - CameraScroll.Y);

	ScreenPosition.X = CenterDelta.X * MAP_TILE_WIDTH + 400;
	ScreenPosition.Y = CenterDelta.Y * MAP_TILE_HEIGHT + 300;

	// Check if it's on screen
	if(abs(CenterDelta.X) > ViewSize.Width/2 || abs(CenterDelta.Y) > ViewSize.Height/2)
		return false;

	return true;
}

// Saves the map to a file
int MapClass::SaveMap() {

	// Open file
	FileClass File;
	int Result = File.OpenForWrite((stringc("maps/") + Filename).c_str());
	if(!Result) {
		printf("SaveMap: unable to open file for writing\n");
		return 0;
	}

	// Generate a list of textures used by the map
	array<ITexture *> TextureList;
	GetTextureListFromMap(TextureList);

	// Write header
	File.WriteInt(MAP_VERSION);
	File.WriteInt(Width);
	File.WriteInt(Height);

	// Write texture list
	File.WriteInt(TextureList.size());
	for(u32 i = 0; i < TextureList.size(); i++) {
		if(TextureList[i] == NULL)
			File.WriteString("none");
		else {
			
			// TODO: Clean up
			// Strip path from texture name
			stringc TexturePath = TextureList[i]->getName();
			int SlashIndex = TexturePath.findLastChar("/\\", 2);
			TexturePath = TexturePath.subString(SlashIndex + 1, TexturePath.size() - SlashIndex - 1);

			File.WriteString(TexturePath.c_str());
		}
	}

	// Write no-zone texture
	if(NoZoneTexture == NULL)
		File.WriteString("none");
	else {
		
		// TODO: Clean up
		// Strip path from texture name
		stringc TexturePath = NoZoneTexture->getName();
		int SlashIndex = TexturePath.findLastChar("/\\", 2);
		TexturePath = TexturePath.subString(SlashIndex + 1, TexturePath.size() - SlashIndex - 1);

		File.WriteString(TexturePath.c_str());
	}

	// Write map data
	TileStruct *Tile;
	for(int i = 0; i < Width; i++) {
		for(int j = 0; j < Height; j++) {
			Tile = &Tiles[i][j];

			// Write texture
			File.WriteInt(GetTextureIndex(TextureList, Tile->Texture));
			File.WriteInt(Tile->Zone);
			File.WriteInt(Tile->EventType);
			File.WriteInt(Tile->EventData);
			File.WriteChar(Tile->Wall);
			File.WriteChar(Tile->PVP);
		}
	}

	// Close file
	File.Close();

	return 1;
}

// Loads a map
int MapClass::LoadMap() {

	// Open file
	FileClass File;
	int Result = File.OpenForRead((stringc("maps/") + Filename).c_str());
	if(!Result) {
		printf("LoadMap: unable to open file for reading\n");
		return 0;
	}

	// Read header
	int MapVersion = File.ReadInt();
	Width = File.ReadInt();
	Height = File.ReadInt();
	if(Width < 5 || Width > 255 || Height < 5 || Height > 255) {
		printf("LoadMap: bad size header\n");
		return 0;
	}

	// Allocate memory
	FreeMap();
	AllocateMap();

	// Get count of textures
	int TextureCount = File.ReadInt();
	Textures.clear();

	// Change directories
	stringc OldWorkingDirectory = irrFile->getWorkingDirectory();
	irrFile->changeWorkingDirectoryTo("textures/map");

	// Read textures from map
	stringc TextureFile;
	char String[256];
	for(int i = 0; i < TextureCount; i++) {
		File.ReadString(String);

		TextureFile = String;
		if(TextureFile == "none")
			Textures.push_back(NULL);
		else
			Textures.push_back(irrDriver->getTexture(TextureFile.c_str()));
	}
	
	// Get no zone texture
	File.ReadString(String);
	TextureFile = String;
	if(TextureFile == "none")
		NoZoneTexture = NULL;
	else
		NoZoneTexture = irrDriver->getTexture(TextureFile.c_str());

	irrFile->changeWorkingDirectoryTo(OldWorkingDirectory.c_str());

	// Read map data
	TileStruct *Tile;
	for(int i = 0; i < Width; i++) {
		for(int j = 0; j < Height; j++) {
			Tile = &Tiles[i][j];

			Tile->Texture = Textures[File.ReadInt()];
			Tile->Zone = File.ReadInt();
			Tile->EventType = File.ReadInt();
			Tile->EventData = File.ReadInt();
			Tile->Wall = File.ReadChar();
			Tile->PVP = File.ReadChar();

			// Save off events that need to be indexed
			if(Stats.GetEvent(Tile->EventType)->Indexed) {
				IndexedEvents.push_back(IndexedEventStruct(Tile, position2di(i, j)));
			}
		}
	}

	// Close file
	File.Close();

	return 1;
}

// Builds an array of textures that are used in the map
void MapClass::GetTextureListFromMap(array<ITexture *> &Textures) {

	Textures.clear();

	// Go through map
	for(int i = 0; i < Width; i++) {
		for(int j = 0; j < Height; j++) {

			// Check for new textures
			if(GetTextureIndex(Textures, Tiles[i][j].Texture) == -1) {
				Textures.push_back(Tiles[i][j].Texture);
			}			
		}
	}
}

// Returns the index of a texture in an array
int MapClass::GetTextureIndex(array<ITexture *> &Textures, ITexture *Texture) {

	for(u32 i = 0; i < Textures.size(); i++) {
		if(Textures[i] == Texture)
			return (int)i;
	}

	return -1;
}

// Determines if a square can be moved to
bool MapClass::CanMoveTo(const position2di &Position) {

	// Bounds
	if(Position.X < 0 || Position.X >= Width || Position.Y < 0 || Position.Y >= Height)
		return false;

	return !Tiles[Position.X][Position.Y].Wall;
}

// Adds an object to the map
void MapClass::AddObject(ObjectClass *Object) {

	// Create packet for the new object
	PacketClass Packet(NetworkClass::WORLD_CREATEOBJECT);
	Packet.WriteInt(Object->NetworkID);
	Packet.WriteChar(Object->Position.X);
	Packet.WriteChar(Object->Position.Y);
	Packet.WriteChar(Object->Type);
	switch(Object->Type) {
		case ObjectClass::PLAYER: {
			PlayerClass *NewPlayer = static_cast<PlayerClass *>(Object);
			Packet.WriteString(NewPlayer->GetName().c_str());
			Packet.WriteChar(NewPlayer->GetPortraitID());
		}
		break;
	}

	// Notify other players of the new object
	SendPacketToPlayers(&Packet);

	// Add object to map
	Objects.push_back(Object);
}

// Removes an object from the map
void MapClass::RemoveObject(ObjectClass *Object) {

	// Remove from the map
	for(list<ObjectClass *>::Iterator Iterator = Objects.begin(); Iterator != Objects.end(); ) {
		if(*Iterator == Object)
			Iterator = Objects.erase(Iterator);
		else
			++Iterator;
	}

	// Create delete packet
	PacketClass Packet(NetworkClass::WORLD_DELETEOBJECT);
	Packet.WriteInt(Object->NetworkID);

	// Send to everyone
	SendPacketToPlayers(&Packet);
}

// Returns the list of objects
const list<ObjectClass *> &MapClass::GetObjects() const {

	return Objects;
}

// Returns a list of players close to a player
void MapClass::GetClosePlayers(const PlayerClass *Player, float DistanceSquared, list<PlayerClass *> &PlayerList) {
	
	for(list<ObjectClass *>::Iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		if((*Iterator)->Type == ObjectClass::PLAYER) {
			PlayerClass *PlayerIterator = static_cast<PlayerClass *>(*Iterator);
			if(PlayerIterator != Player) {
				int XDelta = PlayerIterator->Position.X - Player->Position.X;
				int YDelta = PlayerIterator->Position.Y - Player->Position.Y;
				if((float)(XDelta * XDelta + YDelta * YDelta) <= DistanceSquared) {
					PlayerList.push_back(PlayerIterator);
				}
			}
		}
	}
}

// Returns the closest player
PlayerClass *MapClass::GetClosestPlayer(const PlayerClass *Player, float MaxDistanceSquared, int State) {
	
	PlayerClass *ClosestPlayer = NULL;
	float ClosestDistanceSquared = 1e10;
	for(list<ObjectClass *>::Iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		if((*Iterator)->Type == ObjectClass::PLAYER) {
			PlayerClass *PlayerIterator = static_cast<PlayerClass *>(*Iterator);
			if(PlayerIterator != Player && PlayerIterator->GetState() == State) {
				int XDelta = PlayerIterator->Position.X - Player->Position.X;
				int YDelta = PlayerIterator->Position.Y - Player->Position.Y;
				float DistanceSquared = (float)(XDelta * XDelta + YDelta * YDelta);
				if(DistanceSquared <= MaxDistanceSquared && DistanceSquared < ClosestDistanceSquared) {
					ClosestDistanceSquared = DistanceSquared;
					ClosestPlayer = PlayerIterator;
				}
			}
		}
	}

	return ClosestPlayer;
}

// Sends object position information to all the clients in the map
void MapClass::SendObjectUpdates() {
	PacketClass Packet(NetworkClass::WORLD_OBJECTUPDATES, ENET_PACKET_FLAG_UNSEQUENCED, 1);
	
	// Write time
	Packet.WriteInt(ServerState.GetServerTime());

	// Get object count
	int ObjectCount = Objects.getSize();
	Packet.WriteInt(ObjectCount);

	for(list<ObjectClass *>::Iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		ObjectClass *Object = *Iterator;
		int State = 0;
		if(Object->Type == ObjectClass::PLAYER) {
			PlayerClass *Player = static_cast<PlayerClass *>(Object);
			State = Player->GetState();
		}

		Packet.WriteInt(Object->NetworkID);
		Packet.WriteChar(State);
		Packet.WriteChar(Object->Position.X);
		Packet.WriteChar(Object->Position.Y);
	}

	SendPacketToPlayers(&Packet);
}

// Sends a packet to all of the players in the map
void MapClass::SendPacketToPlayers(PacketClass *Packet, PlayerClass *ExceptionPlayer) {

	// Send the packet out
	for(list<ObjectClass *>::Iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		if((*Iterator)->Type == ObjectClass::PLAYER) {
			PlayerClass *Player = static_cast<PlayerClass *>(*Iterator);

			if(Player != ExceptionPlayer)
				ServerNetwork->SendPacketToPeer(Packet, Player->GetPeer());
		}
	}
}

// Finds an event that matches the criteria
IndexedEventStruct *MapClass::GetIndexedEvent(int EventType, int EventData) {

	for(u32 i = 0; i < IndexedEvents.size(); i++) {
		IndexedEventStruct *IndexedEvent = &IndexedEvents[i];
		if(IndexedEvent->Tile->EventType == EventType && IndexedEvent->Tile->EventData == EventData) {
			return IndexedEvent;
		}
	}

	return NULL;
}
