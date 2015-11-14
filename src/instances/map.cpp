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
#include <instances/map.h>
#include <globals.h>
#include <graphics.h>
#include <constants.h>
#include <stats.h>
#include <network/network.h>
#include <buffer.h>
#include <states/playserver.h>
#include <objects/player.h>
#include <texture.h>
#include <assets.h>
#include <fstream>
#include <limits>

// Constructor for the map editor: new map
_Map::_Map(const std::string &TFilename, int TWidth, int THeight) {
	Init();

	Filename = TFilename;
	Width = TWidth;
	Height = THeight;

	AllocateMap();
}

// Constructor for the map editor: load map
_Map::_Map(const std::string &TFilename) {
	Init();

	Filename = TFilename;
}

// Constructor for maps already created in the database
_Map::_Map(int TMapID) {
	Init();

	// Set ID
	ID = TMapID;

	// Get map info
	const _MapStat *Map = Stats.GetMap(ID);
	ViewSize.x = Map->ViewWidth;
	ViewSize.y = Map->ViewHeight;

	// Load map
	Filename = Map->File;
	LoadMap();
}

// Destructor
_Map::~_Map() {

	// Delete map data
	FreeMap();
}

// Initialize variables
void _Map::Init() {
	ID = 0;
	NoZoneTexture = nullptr;
	DefaultNoZoneTexture = Assets.Textures["editor/nozone.png"];
	Tiles = nullptr;
	ViewSize.x = 25;
	ViewSize.y = 19;
	CameraScroll.x = CAMERA_SCROLLMIN_X;
	CameraScroll.y = CAMERA_SCROLLMIN_Y;
}

// Free memory used by the tiles
void _Map::FreeMap() {
	if(Tiles) {
		for(int i = 0; i < Width; i++)
			delete[] Tiles[i];
		delete[] Tiles;

		Tiles = nullptr;
	}

	IndexedEvents.clear();
}

// Allocates memory for the map
void _Map::AllocateMap() {
	if(Tiles)
		return;

	Tiles = new _Tile*[Width];
	for(int i = 0; i < Width; i++) {
		Tiles[i] = new _Tile[Height];
	}

	// Delete textures
	Textures.clear();
}

// Updates the map and sends object updates
void _Map::Update(double FrameTime) {

	ObjectUpdateTime += FrameTime;
	if(ObjectUpdateTime > NETWORK_UPDATEPERIOD) {
		ObjectUpdateTime = 0;

		SendObjectUpdates();
	}
}

// Renders the map
void _Map::Render() {

	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	Graphics.SetVBO(VBO_NONE);

	glm::ivec2 GridPosition, DrawPosition;
	for(int i = 0; i < ViewSize.x; i++) {
		for(int j = 0; j < ViewSize.y; j++) {

			// Get the actual grid coordinate
			GridPosition.x = i + CameraScroll.x - ViewSize.x / 2;
			GridPosition.y = j + CameraScroll.y - ViewSize.y / 2;
			DrawPosition = glm::ivec2((i - ViewSize.x / 2) * MAP_TILE_WIDTH + Graphics.ViewportSize.x/2, (j - ViewSize.y / 2) * MAP_TILE_HEIGHT + Graphics.ViewportSize.y/2);
			if(NoZoneTexture)
				Graphics.DrawCenteredImage(DrawPosition, NoZoneTexture);

			// Validate coordinate
			if(GridPosition.x >= 0 && GridPosition.x < Width && GridPosition.y >= 0 && GridPosition.y < Height) {
				_Tile *Tile = &Tiles[GridPosition.x][GridPosition.y];

				if(Tile->Texture)
					Graphics.DrawCenteredImage(DrawPosition, Tile->Texture);
			}
		}
	}
}

// Renders the map for editor
void _Map::RenderForMapEditor(bool TDrawWall, bool TDrawZone, bool TDrawPVP) {

	glm::ivec2 GridPosition, DrawPosition;
	for(int i = 0; i < ViewSize.x; i++) {
		for(int j = 0; j < ViewSize.y; j++) {

			// Get the actual grid coordinate
			GridPosition.x = i + CameraScroll.x - ViewSize.x / 2;
			GridPosition.y = j + CameraScroll.y - ViewSize.y / 2;
			DrawPosition = glm::ivec2((i - ViewSize.x / 2) * MAP_TILE_WIDTH + Graphics.ViewportSize.x/2, (j - ViewSize.y / 2) * MAP_TILE_HEIGHT + Graphics.ViewportSize.y/2);
			if(NoZoneTexture)
				Graphics.DrawCenteredImage(DrawPosition, NoZoneTexture);

			// Validate coordinate
			if(GridPosition.x >= 0 && GridPosition.x < Width && GridPosition.y >= 0 && GridPosition.y < Height) {
				_Tile *Tile = &Tiles[GridPosition.x][GridPosition.y];

				// Draw texture
				if(Tile->Texture)
					Graphics.DrawCenteredImage(DrawPosition, Tile->Texture);
				else if(NoZoneTexture)
					Graphics.DrawCenteredImage(DrawPosition, NoZoneTexture);

				// Draw wall
				if(TDrawWall && Tile->Wall)
					//Graphics.RenderText("W", DrawPosition.x, DrawPosition.y - 8, _Graphics::ALIGN_CENTER);

				// Draw zone
				if(!Tile->Wall) {
					if(TDrawZone && Tile->Zone > 0)
						//Graphics.RenderText(std::string(Tile->Zone).c_str(), DrawPosition.x, DrawPosition.y - 8, _Graphics::ALIGN_CENTER);

					// Draw PVP
					if(TDrawPVP && Tile->PVP) {
						//Graphics.RenderText("PvP", DrawPosition.x, DrawPosition.y - 8, _Graphics::ALIGN_CENTER, video::SColor(255, 255, 0, 0));
					}
				}

				// Draw event info
				if(Tile->EventType > 0) {
					std::string EventText = Stats.GetEvent(Tile->EventType)->ShortName + std::string(", ") + std::to_string(Tile->EventData);
					//Graphics.RenderText(EventText.c_str(), DrawPosition.x - 16, DrawPosition.y - 16, _Graphics::ALIGN_LEFT, video::SColor(255, 0, 255, 255));
				}
			}
			else {
				Graphics.DrawCenteredImage(DrawPosition, DefaultNoZoneTexture);
			}
		}
	}
}

// Sets the camera scroll position
void _Map::SetCameraScroll(const glm::ivec2 &TPosition) {

	CameraScroll = TPosition;
	if(CameraScroll.x < CAMERA_SCROLLMIN_X)
		CameraScroll.x = CAMERA_SCROLLMIN_X;
	if(CameraScroll.y < CAMERA_SCROLLMIN_Y)
		CameraScroll.y = CAMERA_SCROLLMIN_Y;
	if(CameraScroll.x >= Width - CAMERA_SCROLLMIN_X)
		CameraScroll.x = Width - CAMERA_SCROLLMIN_X;
	if(CameraScroll.y >= Height - CAMERA_SCROLLMIN_Y)
		CameraScroll.y = Height - CAMERA_SCROLLMIN_Y;
}

// Converts a grid position on the map to a screen coordinate
bool _Map::GridToScreen(const glm::ivec2 &TGridPosition, glm::ivec2 &TScreenPosition) const {

	// Get delta from center
	glm::ivec2 CenterDelta(TGridPosition.x - CameraScroll.x, TGridPosition.y - CameraScroll.y);

	TScreenPosition.x = CenterDelta.x * MAP_TILE_WIDTH + Graphics.ViewportSize.x/2;
	TScreenPosition.y = CenterDelta.y * MAP_TILE_HEIGHT + Graphics.ViewportSize.y/2;

	// Check if it's on screen
	if(abs(CenterDelta.x) > ViewSize.x/2 || abs(CenterDelta.y) > ViewSize.y/2)
		return false;

	return true;
}

// Converts a screen coordinate to a map position
void _Map::ScreenToGrid(const glm::ivec2 &TScreenPosition, glm::ivec2 &TGridPosition) const {
	TGridPosition.x = GetCameraScroll().x + TScreenPosition.x / MAP_TILE_WIDTH - GetViewSize().x / 2;
	TGridPosition.y = GetCameraScroll().y + TScreenPosition.y / MAP_TILE_HEIGHT - GetViewSize().y / 2;
}

// Saves the map to a file
int _Map::SaveMap() {

	// Open file
	std::ofstream File(Filename.c_str(), std::ios::binary);
	if(!File) {
		printf("SaveMap: unable to open file for writing\n");
		return 0;
	}

	// Generate a list of textures used by the map
	std::vector<const _Texture *> TextureList;
	GetTextureListFromMap(TextureList);

	// Write header
	File.write((char *)&MAP_VERSION, sizeof(MAP_VERSION));
	File.write((char *)&Width, sizeof(Width));
	File.write((char *)&Height, sizeof(Height));

	// Write texture list
	int32_t TextureCount = (int32_t)(TextureList.size());
	File.write((char *)&TextureCount, sizeof(TextureCount));
	for(int32_t i = 0; i < TextureCount; i++) {
		if(TextureList[i] == nullptr) {
			File.write("none", 4);
			File.put(0);
		}
		else {

			// Strip path from texture name
			//std::string TexturePath = TextureList[i]->Identifier;
			//int SlashIndex = TexturePath.findLastChar("/\\", 2);
			//TexturePath = TexturePath.subString(SlashIndex + 1, TexturePath.size() - SlashIndex - 1);

			//File.write(TexturePath.c_str(), TexturePath.size());
			//File.put(0);
		}
	}

	// Write no-zone texture
	if(NoZoneTexture == nullptr) {
		File.write("none", 4);
		File.put(0);
	}
	else {

		// Strip path from texture name
		//std::string TexturePath = NoZoneTexture->Identifier;
		//int SlashIndex = TexturePath.findLastChar("/\\", 2);
		//TexturePath = TexturePath.subString(SlashIndex + 1, TexturePath.size() - SlashIndex - 1);

		//File.write(TexturePath.c_str(), TexturePath.size());
		//File.put(0);
	}

	// Write map data
	_Tile *Tile;
	for(int i = 0; i < Width; i++) {
		for(int j = 0; j < Height; j++) {
			Tile = &Tiles[i][j];

			// Write texture
			int32_t TextureIndex = GetTextureIndex(TextureList, Tile->Texture);
			File.write((char *)&TextureIndex, sizeof(TextureIndex));
			File.write((char *)&Tile->Zone, sizeof(Tile->Zone));
			File.write((char *)&Tile->EventType, sizeof(Tile->EventType));
			File.write((char *)&Tile->EventData, sizeof(Tile->EventData));
			File.write((char *)&Tile->Wall, sizeof(Tile->Wall));
			File.write((char *)&Tile->PVP, sizeof(Tile->PVP));
		}
	}

	// Close file
	File.close();

	return 1;
}

// Loads a map
int _Map::LoadMap() {

	// Open file
	std::ifstream File(Filename.c_str(), std::ios::binary);
	if(!File) {
		printf("LoadMap: unable to open file for reading: %s\n", Filename.c_str());
		return 0;
	}

	// Read header
	int32_t MapVersion;
	File.read((char *)&MapVersion, sizeof(MapVersion));
	File.read((char *)&Width, sizeof(Width));
	File.read((char *)&Height, sizeof(Height));
	if(Width < 5 || Width > 255 || Height < 5 || Height > 255 || MapVersion != MAP_VERSION) {
		printf("LoadMap: bad size header\n");
		return 0;
	}

	// Allocate memory
	FreeMap();
	AllocateMap();

	// Get count of textures
	int32_t TextureCount;
	File.read((char *)&TextureCount, sizeof(TextureCount));
	Textures.clear();

	// Read textures from map
	std::string TextureFile;
	char String[256];
	for(int32_t i = 0; i < TextureCount; i++) {
		File.get(String, std::numeric_limits<std::streamsize>::max(), 0);
		File.get();

		TextureFile = String;
		if(TextureFile == "none")
			Textures.push_back(nullptr);
		else
			Textures.push_back(Assets.Textures[std::string("map/") + TextureFile]);
	}

	// Get no zone texture
	File.get(String, std::numeric_limits<std::streamsize>::max(), 0);
	File.get();
	TextureFile = String;
	if(TextureFile == "none")
		NoZoneTexture = nullptr;
	else
		NoZoneTexture = Assets.Textures[std::string("map/") + TextureFile];

	// Read map data
	_Tile *Tile;
	for(int i = 0; i < Width; i++) {
		for(int j = 0; j < Height; j++) {
			Tile = &Tiles[i][j];

			int32_t TextureIndex;
			File.read((char *)&TextureIndex, sizeof(int32_t));
			Tile->Texture = Textures[TextureIndex];

			File.read((char *)&Tile->Zone, sizeof(Tile->Zone));
			File.read((char *)&Tile->EventType, sizeof(Tile->EventType));
			File.read((char *)&Tile->EventData, sizeof(Tile->EventData));
			File.read((char *)&Tile->Wall, sizeof(Tile->Wall));
			File.read((char *)&Tile->PVP, sizeof(Tile->PVP));

			// Save off events that need to be indexed
			if(Stats.GetEvent(Tile->EventType)->Indexed) {
				IndexedEvents.push_back(_IndexedEvent(Tile, glm::ivec2(i, j)));
			}
		}
	}

	// Close file
	File.close();

	return 1;
}

// Builds an array of textures that are used in the map
void _Map::GetTextureListFromMap(std::vector<const _Texture *> &TTextures) {

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
int _Map::GetTextureIndex(std::vector<const _Texture *> &TTextures, const _Texture *TTexture) {

	for(size_t i = 0; i < TTextures.size(); i++) {
		if(TTextures[i] == TTexture)
			return (int)i;
	}

	return -1;
}

// Determines if a square can be moved to
bool _Map::CanMoveTo(const glm::ivec2 &TPosition) {

	// Bounds
	if(TPosition.x < 0 || TPosition.x >= Width || TPosition.y < 0 || TPosition.y >= Height)
		return false;

	return !Tiles[TPosition.x][TPosition.y].Wall;
}

// Adds an object to the map
void _Map::AddObject(_Object *TObject) {

	// Create packet for the new object
	_Buffer Packet;
	Packet.Write<char>(_Network::WORLD_CREATEOBJECT);
	Packet.Write<char>(TObject->GetNetworkID());
	Packet.Write<char>(TObject->GetPosition().x);
	Packet.Write<char>(TObject->GetPosition().y);
	Packet.Write<char>(TObject->GetType());
	switch(TObject->GetType()) {
		case _Object::PLAYER: {
			_Player *NewPlayer = static_cast<_Player *>(TObject);
			Packet.WriteString(NewPlayer->GetName().c_str());
			Packet.Write<char>(NewPlayer->GetPortraitID());
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
void _Map::RemoveObject(_Object *TObject) {

	// Remove from the map
	for(std::list<_Object *>::iterator Iterator = Objects.begin(); Iterator != Objects.end(); ) {
		if(*Iterator == TObject)
			Iterator = Objects.erase(Iterator);
		else
			++Iterator;
	}

	// Create delete packet
	_Buffer Packet;
	Packet.Write<char>(_Network::WORLD_DELETEOBJECT);
	Packet.Write<char>(TObject->GetNetworkID());

	// Send to everyone
	SendPacketToPlayers(&Packet);
}

// Returns the list of objects
const std::list<_Object *> &_Map::GetObjects() const {

	return Objects;
}

// Returns a list of players close to a player
void _Map::GetClosePlayers(const _Player *TPlayer, float TDistanceSquared, std::list<_Player *> &TPlayers) {

	for(std::list<_Object *>::iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		if((*Iterator)->GetType() == _Object::PLAYER) {
			_Player *Player = static_cast<_Player *>(*Iterator);
			if(Player != TPlayer) {
				int XDelta = Player->GetPosition().x - TPlayer->GetPosition().x;
				int YDelta = Player->GetPosition().y - TPlayer->GetPosition().y;
				if((float)(XDelta * XDelta + YDelta * YDelta) <= TDistanceSquared) {
					TPlayers.push_back(Player);
				}
			}
		}
	}
}

// Returns the closest player
_Player *_Map::GetClosestPlayer(const _Player *TPlayer, float TMaxDistanceSquared, int TState) {

	_Player *ClosestPlayer = nullptr;
	float ClosestDistanceSquared = 1e10;
	for(std::list<_Object *>::iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		if((*Iterator)->GetType() == _Object::PLAYER) {
			_Player *Player = static_cast<_Player *>(*Iterator);
			if(Player != TPlayer && Player->GetState() == TState) {
				int XDelta = Player->GetPosition().x - TPlayer->GetPosition().x;
				int YDelta = Player->GetPosition().y - TPlayer->GetPosition().y;
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
void _Map::SendObjectUpdates() {

	// unsequenced
	_Buffer Packet;
	Packet.Write<char>(_Network::WORLD_OBJECTUPDATES);

	// Get object count
	int ObjectCount = Objects.size();
	Packet.Write<char>(ObjectCount);

	for(std::list<_Object *>::iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		_Object *Object = *Iterator;
		int State = 0;
		bool Invisible = false;
		if(Object->GetType() == _Object::PLAYER) {
			_Player *Player = static_cast<_Player *>(Object);
			State = Player->GetState();
			Invisible = Player->IsInvisible();
		}

		Packet.Write<char>(Object->GetNetworkID());
		Packet.Write<char>(State);
		Packet.Write<char>(Object->GetPosition().x);
		Packet.Write<char>(Object->GetPosition().y);
		Packet.WriteBit(Invisible);
	}

	SendPacketToPlayers(&Packet, nullptr, _Network::UNSEQUENCED);
}

// Sends a packet to all of the players in the map
void _Map::SendPacketToPlayers(_Buffer *TPacket, _Player *ExceptionPlayer, _Network::SendType Type) {

	// Send the packet out
	for(std::list<_Object *>::iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		if((*Iterator)->GetType() == _Object::PLAYER) {
			_Player *Player = static_cast<_Player *>(*Iterator);

			if(Player != ExceptionPlayer)
				ServerNetwork->SendPacketToPeer(TPacket, Player->GetPeer(), Type, Type == _Network::UNSEQUENCED);
		}
	}
}

// Finds an event that matches the criteria
_IndexedEvent *_Map::GetIndexedEvent(int TEventType, int TEventData) {

	for(size_t i = 0; i < IndexedEvents.size(); i++) {
		_IndexedEvent *IndexedEvent = &IndexedEvents[i];
		if(IndexedEvent->Tile->EventType == TEventType && IndexedEvent->Tile->EventData == TEventData) {
			return IndexedEvent;
		}
	}

	return nullptr;
}
