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
#include <states/server.h>
#include <objects/player.h>
#include <texture.h>
#include <assets.h>
#include <font.h>
#include <program.h>
#include <camera.h>
#include <fstream>
#include <limits>
#include <glm/vec3.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

// Constructor for the map editor: new map
_Map::_Map(const std::string &Filename, const glm::ivec2 &Size) {
	Init();

	this->Filename = Filename;
	this->Size = Size;

	AllocateMap();
}

// Constructor for the map editor: load map
_Map::_Map(const std::string &Filename) {
	Init();

	this->Filename = Filename;
}

// Constructor for maps already created in the database
_Map::_Map(int ID) {
	Init();

	// Set ID
	this->ID = ID;

	// Get map info
	const _MapStat *Map = Stats.GetMap(ID);

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
}

// Free memory used by the tiles
void _Map::FreeMap() {
	if(Tiles) {
		for(int i = 0; i < Size.x; i++)
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

	Tiles = new _Tile*[Size.x];
	for(int i = 0; i < Size.x; i++) {
		Tiles[i] = new _Tile[Size.y];
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
void _Map::Render(_Camera *Camera, int RenderFlags) {
	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv"]->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));
	Graphics.SetColor(COLOR_WHITE);
	Graphics.SetVBO(VBO_QUAD);

	Graphics.SetDepthTest(false);
	Graphics.SetDepthMask(false);
	glm::vec4 Bounds = Camera->GetAABB();
	Bounds[0] = glm::clamp(Bounds[0], 0.0f, (float)Size.x);
	Bounds[1] = glm::clamp(Bounds[1], 0.0f, (float)Size.y);
	Bounds[2] = glm::clamp(Bounds[2], 0.0f, (float)Size.x);
	Bounds[3] = glm::clamp(Bounds[3], 0.0f, (float)Size.y);
	for(int j = Bounds[1]; j < Bounds[3]; j++) {
		for(int i = Bounds[0]; i < Bounds[2]; i++) {
			_Tile *Tile = &Tiles[i][j];

			glm::vec3 DrawPosition = glm::vec3(i, j, 0) + glm::vec3(0.5f, 0.5f, 0);
			if(NoZoneTexture)
				Graphics.DrawSprite(DrawPosition, NoZoneTexture);

			if(Tile->Texture)
				Graphics.DrawSprite(DrawPosition, Tile->Texture);

		}
	}

	// Check for flags
	if(!RenderFlags)
		return;

	// Draw text overlay
	for(int j = Bounds[1]; j < Bounds[3]; j++) {
		for(int i = Bounds[0]; i < Bounds[2]; i++) {
			_Tile *Tile = &Tiles[i][j];

			glm::vec3 DrawPosition = glm::vec3(i, j, 0) + glm::vec3(0.5f, 0.5f, 0);
			if(NoZoneTexture)
				Graphics.DrawSprite(DrawPosition, NoZoneTexture);

			if(Tile->Texture)
				Graphics.DrawSprite(DrawPosition, Tile->Texture);

			// Draw wall
			if(Tile->Wall) {
				if(RenderFlags & FILTER_WALL)
					Assets.Fonts["hud_small"]->DrawText("W", glm::vec2(DrawPosition), COLOR_WHITE, CENTER_MIDDLE, 1.0f / 32.0f);
			}
			else {

				// Draw zone number
				if((RenderFlags & FILTER_ZONE) && Tile->Zone > 0)
					Assets.Fonts["hud_small"]->DrawText(std::to_string(Tile->Zone).c_str(), glm::vec2(DrawPosition), COLOR_WHITE, CENTER_MIDDLE, 1.0f / 32.0f);

				// Draw PVP
				if((RenderFlags & FILTER_PVP) && Tile->PVP)
					Assets.Fonts["hud_small"]->DrawText("PVP", glm::vec2(DrawPosition), COLOR_RED, CENTER_MIDDLE, 1.0f / 32.0f);
			}

			// Draw event info
			if(Tile->EventType > 0) {
				std::string EventText = Stats.Events[Tile->EventType].ShortName + std::string(", ") + std::to_string(Tile->EventData);
				Assets.Fonts["hud_small"]->DrawText(EventText, glm::vec2(DrawPosition), COLOR_CYAN, CENTER_MIDDLE, 1.0f / 32.0f);
			}
		}
	}
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
	File.write((char *)&Size.x, sizeof(Size.x));
	File.write((char *)&Size.y, sizeof(Size.y));

	// Write texture list
	int32_t TextureCount = (int32_t)(TextureList.size());
	File.write((char *)&TextureCount, sizeof(TextureCount));
	for(int32_t i = 0; i < TextureCount; i++) {
		if(TextureList[i] == nullptr) {
			File.write("map/none", 8);
			File.put(0);
		}
		else {
			std::string TexturePath = TextureList[i]->Identifier;
			File.write(TexturePath.c_str(), TexturePath.size());
			File.put(0);
		}
	}

	// Write no-zone texture
	if(NoZoneTexture == nullptr) {
		File.write("map/none", 8);
		File.put(0);
	}
	else {
		std::string TexturePath = NoZoneTexture->Identifier;
		File.write(TexturePath.c_str(), TexturePath.size());
		File.put(0);
	}

	// Write map data
	_Tile *Tile;
	for(int i = 0; i < Size.x; i++) {
		for(int j = 0; j < Size.y; j++) {
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
void _Map::LoadMap() {

	// Open file
	std::ifstream File(Filename.c_str(), std::ios::binary);
	if(!File)
		throw std::runtime_error("_Map::LoadMap: Unable to open file for reading: " + Filename);

	// Read header
	int32_t MapVersion;
	File.read((char *)&MapVersion, sizeof(MapVersion));
	File.read((char *)&Size.x, sizeof(Size.x));
	File.read((char *)&Size.y, sizeof(Size.y));
	if(Size.x < 5 || Size.x > 255 || Size.y < 5 || Size.y > 255 || MapVersion != MAP_VERSION)
		throw std::runtime_error("_Map::LoadMap: Bad size header");

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

		if(TextureFile.substr(0, 4) != "map/")
			TextureFile = std::string("map/") + TextureFile;

		if(TextureFile == "map/none")
			Textures.push_back(nullptr);
		else
			Textures.push_back(Assets.Textures[TextureFile]);
	}

	// Get no zone texture
	File.get(String, std::numeric_limits<std::streamsize>::max(), 0);
	File.get();
	TextureFile = String;
	if(TextureFile.substr(0, 4) != "map/")
		TextureFile = std::string("map/") + TextureFile;

	if(TextureFile == "map/none")
		NoZoneTexture = nullptr;
	else
		NoZoneTexture = Assets.Textures[TextureFile];

	// Read map data
	_Tile *Tile;
	for(int i = 0; i < Size.x; i++) {
		for(int j = 0; j < Size.y; j++) {
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
			if(Stats.Events[Tile->EventType].Indexed) {
				IndexedEvents.push_back(_IndexedEvent(Tile, glm::ivec2(i, j)));
			}
		}
	}

	// Close file
	File.close();
}

// Builds an array of textures that are used in the map
void _Map::GetTextureListFromMap(std::vector<const _Texture *> &TTextures) {

	TTextures.clear();

	// Go through map
	for(int i = 0; i < Size.x; i++) {
		for(int j = 0; j < Size.y; j++) {

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
bool _Map::CanMoveTo(const glm::ivec2 &Position) {

	// Bounds
	if(Position.x < 0 || Position.x >= Size.x || Position.y < 0 || Position.y >= Size.y)
		return false;

	return !Tiles[Position.x][Position.y].Wall;
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
			Packet.WriteString(NewPlayer->Name.c_str());
			Packet.Write<char>(NewPlayer->PortraitID);
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
_Player *_Map::GetClosestPlayer(const _Player *TPlayer, float MaxDistanceSquared, int TState) {

	_Player *ClosestPlayer = nullptr;
	float ClosestDistanceSquared = 1e10;
	for(std::list<_Object *>::iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		if((*Iterator)->GetType() == _Object::PLAYER) {
			_Player *Player = static_cast<_Player *>(*Iterator);
			if(Player != TPlayer && Player->State == TState) {
				int XDelta = Player->GetPosition().x - TPlayer->GetPosition().x;
				int YDelta = Player->GetPosition().y - TPlayer->GetPosition().y;
				float DistanceSquared = (float)(XDelta * XDelta + YDelta * YDelta);
				if(DistanceSquared <= MaxDistanceSquared && DistanceSquared < ClosestDistanceSquared) {
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
			State = Player->State;
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
void _Map::SendPacketToPlayers(_Buffer *Packet, _Player *ExceptionPlayer, _Network::SendType Type) {

	// Send the packet out
	for(std::list<_Object *>::iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		if((*Iterator)->GetType() == _Object::PLAYER) {
			_Player *Player = static_cast<_Player *>(*Iterator);

			if(Player != ExceptionPlayer)
				ServerNetwork->SendPacketToPeer(Packet, Player->Peer, Type, Type == _Network::UNSEQUENCED);
		}
	}
}

// Finds an event that matches the criteria
_IndexedEvent *_Map::GetIndexedEvent(int EventType, int EventData) {

	for(size_t i = 0; i < IndexedEvents.size(); i++) {
		_IndexedEvent *IndexedEvent = &IndexedEvents[i];
		if(IndexedEvent->Tile->EventType == EventType && IndexedEvent->Tile->EventData == EventData) {
			return IndexedEvent;
		}
	}

	return nullptr;
}

// Get a valid position within the grid
glm::vec2 _Map::GetValidPosition(const glm::vec2 &Position) {
	return glm::clamp(Position, glm::vec2(0.0f), glm::vec2(Size));
}
