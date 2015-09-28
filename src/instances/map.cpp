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
#include <instances/map.h>
#include <engine/globals.h>
#include <engine/graphics.h>
#include <engine/constants.h>
#include <network/network.h>
#include <network/packetstream.h>
#include <engine/filestream.h>
#include <engine/stats.h>
#include <states/playserver.h>
#include <objects/player.h>

// Constructor for the map editor: new map
MapClass::MapClass(const stringc &TFilename, int TWidth, int THeight) {
	Init();

	Filename = TFilename;
	Width = TWidth;
	Height = THeight;

	AllocateMap();
}

// Constructor for the map editor: load map
MapClass::MapClass(const stringc &TFilename) {
	Init();

	Filename = TFilename;
}

// Constructor for maps already created in the database
MapClass::MapClass(int TMapID) {
	Init();

	// Set ID
	ID = TMapID;

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
void MapClass::Update(u32 TDeltaTime) {

	ObjectUpdateTime += TDeltaTime;
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
void MapClass::RenderForMapEditor(bool TDrawWall, bool TDrawZone, bool TDrawPVP) {

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
				if(TDrawWall && Tile->Wall)
					Graphics.RenderText("W", DrawPosition.X, DrawPosition.Y - 8, GraphicsClass::ALIGN_CENTER);

				// Draw zone
				if(!Tile->Wall) {
					if(TDrawZone && Tile->Zone > 0)
						Graphics.RenderText(stringc(Tile->Zone).c_str(), DrawPosition.X, DrawPosition.Y - 8, GraphicsClass::ALIGN_CENTER);

					// Draw PVP
					if(TDrawPVP && Tile->PVP)
						Graphics.RenderText("PvP", DrawPosition.X, DrawPosition.Y - 8, GraphicsClass::ALIGN_CENTER, SColor(255, 255, 0, 0));
				}

				// Draw event info
				if(Tile->EventType > 0) {
					stringc EventText = Stats.GetEvent(Tile->EventType)->ShortName + stringc(", ") + stringc(Tile->EventData);
					Graphics.RenderText(EventText.c_str(), DrawPosition.X - 16, DrawPosition.Y - 16, GraphicsClass::ALIGN_LEFT, SColor(255, 0, 255, 255));
				}
			}
			else {
				Graphics.DrawCenteredImage(irrDriver->getTexture(WorkingDirectory + "textures/editor/nozone.png"), DrawPosition.X, DrawPosition.Y);
			}
		}
	}
}

// Sets the camera scroll position
void MapClass::SetCameraScroll(const position2di &TPosition) {

	CameraScroll = TPosition;
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
bool MapClass::GridToScreen(const position2di &TGridPosition, position2di &TScreenPosition) const {

	// Get delta from center
	position2di CenterDelta(TGridPosition.X - CameraScroll.X, TGridPosition.Y - CameraScroll.Y);

	TScreenPosition.X = CenterDelta.X * MAP_TILE_WIDTH + 400;
	TScreenPosition.Y = CenterDelta.Y * MAP_TILE_HEIGHT + 300;

	// Check if it's on screen
	if(abs(CenterDelta.X) > ViewSize.Width/2 || abs(CenterDelta.Y) > ViewSize.Height/2)
		return false;

	return true;
}

// Converts a screen coordinate to a map position
void MapClass::ScreenToGrid(const position2di &TScreenPosition, position2di &TGridPosition) const {
	TGridPosition.X = GetCameraScroll().X + TScreenPosition.X / MAP_TILE_WIDTH - GetViewSize().Width / 2;
	TGridPosition.Y = GetCameraScroll().Y + TScreenPosition.Y / MAP_TILE_HEIGHT - GetViewSize().Height / 2;
}

// Saves the map to a file
int MapClass::SaveMap() {

	// Open file
	FileClass File;
	int Result = File.OpenForWrite(Filename.c_str());
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
	int Result = File.OpenForRead(Filename.c_str());
	if(!Result) {
		printf("LoadMap: unable to open file for reading: %s\n", Filename.c_str());
		return 0;
	}

	// Read header
	int MapVersion = File.ReadInt();
	Width = File.ReadInt();
	Height = File.ReadInt();
	if(Width < 5 || Width > 255 || Height < 5 || Height > 255 || MapVersion != MAP_VERSION) {
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
	//stringc OldWorkingDirectory = irrFile->getWorkingDirectory();
	//irrFile->changeWorkingDirectoryTo("textures/map");

	// Read textures from map
	stringc TextureFile;
	char String[256];
	for(int i = 0; i < TextureCount; i++) {
		File.ReadString(String);

		TextureFile = String;
		if(TextureFile == "none")
			Textures.push_back(NULL);
		else
			Textures.push_back(irrDriver->getTexture(WorkingDirectory + "textures/map/" + TextureFile));
	}

	// Get no zone texture
	File.ReadString(String);
	TextureFile = String;
	if(TextureFile == "none")
		NoZoneTexture = NULL;
	else
		NoZoneTexture = irrDriver->getTexture(WorkingDirectory + "textures/map/" + TextureFile);

	// Read map data
	TileStruct *Tile;
	for(int i = 0; i < Width; i++) {
		for(int j = 0; j < Height; j++) {
			Tile = &Tiles[i][j];

			Tile->Texture = Textures[File.ReadInt()];
			Tile->Zone = File.ReadInt();
			Tile->EventType = File.ReadInt();
			Tile->EventData = File.ReadInt();
			Tile->Wall = !!File.ReadChar();
			Tile->PVP = !!File.ReadChar();

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
void MapClass::GetTextureListFromMap(array<ITexture *> &TTextures) {

	TTextures.clear();

	// Go through map
	for(int i = 0; i < Width; i++) {
		for(int j = 0; j < Height; j++) {

			// Check for new textures
			if(GetTextureIndex(TTextures, Tiles[i][j].Texture) == -1) {
				TTextures.push_back(Tiles[i][j].Texture);
			}
		}
	}
}

// Returns the index of a texture in an array
int MapClass::GetTextureIndex(array<ITexture *> &TTextures, ITexture *TTexture) {

	for(u32 i = 0; i < TTextures.size(); i++) {
		if(TTextures[i] == TTexture)
			return (int)i;
	}

	return -1;
}

// Determines if a square can be moved to
bool MapClass::CanMoveTo(const position2di &TPosition) {

	// Bounds
	if(TPosition.X < 0 || TPosition.X >= Width || TPosition.Y < 0 || TPosition.Y >= Height)
		return false;

	return !Tiles[TPosition.X][TPosition.Y].Wall;
}

// Adds an object to the map
void MapClass::AddObject(ObjectClass *TObject) {

	// Create packet for the new object
	PacketClass Packet(NetworkClass::WORLD_CREATEOBJECT);
	Packet.WriteChar(TObject->GetNetworkID());
	Packet.WriteChar(TObject->GetPosition().X);
	Packet.WriteChar(TObject->GetPosition().Y);
	Packet.WriteChar(TObject->GetType());
	switch(TObject->GetType()) {
		case ObjectClass::PLAYER: {
			PlayerClass *NewPlayer = static_cast<PlayerClass *>(TObject);
			Packet.WriteString(NewPlayer->GetName().c_str());
			Packet.WriteChar(NewPlayer->GetPortraitID());
			Packet.WriteBit(NewPlayer->IsInvisible());
		}
		break;
		default:
		break;
	}

	// Notify other players of the new object
	SendPacketToPlayers(&Packet);

	// Add object to map
	Objects.push_back(TObject);
}

// Removes an object from the map
void MapClass::RemoveObject(ObjectClass *TObject) {

	// Remove from the map
	for(list<ObjectClass *>::Iterator Iterator = Objects.begin(); Iterator != Objects.end(); ) {
		if(*Iterator == TObject)
			Iterator = Objects.erase(Iterator);
		else
			++Iterator;
	}

	// Create delete packet
	PacketClass Packet(NetworkClass::WORLD_DELETEOBJECT);
	Packet.WriteChar(TObject->GetNetworkID());

	// Send to everyone
	SendPacketToPlayers(&Packet);
}

// Returns the list of objects
const list<ObjectClass *> &MapClass::GetObjects() const {

	return Objects;
}

// Returns a list of players close to a player
void MapClass::GetClosePlayers(const PlayerClass *TPlayer, float TDistanceSquared, list<PlayerClass *> &TPlayers) {

	for(list<ObjectClass *>::Iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		if((*Iterator)->GetType() == ObjectClass::PLAYER) {
			PlayerClass *Player = static_cast<PlayerClass *>(*Iterator);
			if(Player != TPlayer) {
				int XDelta = Player->GetPosition().X - TPlayer->GetPosition().X;
				int YDelta = Player->GetPosition().Y - TPlayer->GetPosition().Y;
				if((float)(XDelta * XDelta + YDelta * YDelta) <= TDistanceSquared) {
					TPlayers.push_back(Player);
				}
			}
		}
	}
}

// Returns the closest player
PlayerClass *MapClass::GetClosestPlayer(const PlayerClass *TPlayer, float TMaxDistanceSquared, int TState) {

	PlayerClass *ClosestPlayer = NULL;
	float ClosestDistanceSquared = 1e10;
	for(list<ObjectClass *>::Iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		if((*Iterator)->GetType() == ObjectClass::PLAYER) {
			PlayerClass *Player = static_cast<PlayerClass *>(*Iterator);
			if(Player != TPlayer && Player->GetState() == TState) {
				int XDelta = Player->GetPosition().X - TPlayer->GetPosition().X;
				int YDelta = Player->GetPosition().Y - TPlayer->GetPosition().Y;
				float DistanceSquared = (float)(XDelta * XDelta + YDelta * YDelta);
				if(DistanceSquared <= TMaxDistanceSquared && DistanceSquared < ClosestDistanceSquared) {
					ClosestDistanceSquared = DistanceSquared;
					ClosestPlayer = Player;
				}
			}
		}
	}

	return ClosestPlayer;
}

// Sends object position information to all the clients in the map
void MapClass::SendObjectUpdates() {
	PacketClass Packet(NetworkClass::WORLD_OBJECTUPDATES, ENET_PACKET_FLAG_UNSEQUENCED, 1);

	// Get object count
	int ObjectCount = Objects.getSize();
	Packet.WriteChar(ObjectCount);

	for(list<ObjectClass *>::Iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		ObjectClass *Object = *Iterator;
		int State = 0;
		bool Invisible = false;
		if(Object->GetType() == ObjectClass::PLAYER) {
			PlayerClass *Player = static_cast<PlayerClass *>(Object);
			State = Player->GetState();
			Invisible = Player->IsInvisible();
		}

		Packet.WriteChar(Object->GetNetworkID());
		Packet.WriteChar(State);
		Packet.WriteChar(Object->GetPosition().X);
		Packet.WriteChar(Object->GetPosition().Y);
		Packet.WriteBit(Invisible);
	}

	SendPacketToPlayers(&Packet);
}

// Sends a packet to all of the players in the map
void MapClass::SendPacketToPlayers(PacketClass *TPacket, PlayerClass *ExceptionPlayer) {

	// Send the packet out
	for(list<ObjectClass *>::Iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		if((*Iterator)->GetType() == ObjectClass::PLAYER) {
			PlayerClass *Player = static_cast<PlayerClass *>(*Iterator);

			if(Player != ExceptionPlayer)
				ServerNetwork->SendPacketToPeer(TPacket, Player->GetPeer());
		}
	}
}

// Finds an event that matches the criteria
IndexedEventStruct *MapClass::GetIndexedEvent(int TEventType, int TEventData) {

	for(u32 i = 0; i < IndexedEvents.size(); i++) {
		IndexedEventStruct *IndexedEvent = &IndexedEvents[i];
		if(IndexedEvent->Tile->EventType == TEventType && IndexedEvent->Tile->EventData == TEventData) {
			return IndexedEvent;
		}
	}

	return NULL;
}
