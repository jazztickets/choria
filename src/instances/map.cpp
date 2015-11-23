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
#include <zlib/zfstream.h>
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
	Load();
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
}

// Allocates memory for the map
void _Map::AllocateMap() {
	if(Tiles)
		return;

	Tiles = new _Tile*[Size.x];
	for(int i = 0; i < Size.x; i++) {
		Tiles[i] = new _Tile[Size.y];
	}
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

// Load map
void _Map::Load() {
	/*this->Stats = Stats;
	this->ID = ID;
	this->Filename = Path;
	this->ServerNetwork = ServerNetwork;
	std::string AtlasPath = TEXTURES_TILES + MAP_DEFAULT_TILESET;
	*/
	//bool TilesInitialized = false;

	// Load file
	gzifstream File(Filename.c_str());
	try {
		_Tile *Tile = nullptr;
		while(!File.eof() && File.peek() != EOF) {

			// Read chunk type
			char ChunkType;
			File >> ChunkType;

			switch(ChunkType) {
				// Map version
				case 'V': {
					int FileVersion;
					File >> FileVersion;
					if(FileVersion != MAP_VERSION)
						throw std::runtime_error("Level version mismatch: ");
				} break;
				// Map size
				case 'S': {
					File >> Size.x >> Size.y;
					FreeMap();
					AllocateMap();
					//TilesInitialized = true;
				} break;
				// No-zone textuer
				case 'N': {
					std::string TextureIdentifier;
					File >> TextureIdentifier;
					if(TextureIdentifier == "none")
						NoZoneTexture = nullptr;
					else
						NoZoneTexture = Assets.Textures[TextureIdentifier];
				} break;
				// Atlas texture
				case 'a': {
					//File >> AtlasPath;
				} break;
				// Tile
				case 'T': {
					glm::ivec2 Coordinate;
					File >> Coordinate.x >> Coordinate.y;
					Tile = &Tiles[Coordinate.x][Coordinate.y];
				} break;
				case 't': {
					std::string TextureIdentifier;
					File >> TextureIdentifier;
					if(TextureIdentifier == "none")
						Tile->Texture = nullptr;
					else
						Tile->Texture = Assets.Textures[TextureIdentifier];
				} break;
				case 'z': {
					File >> Tile->Zone;
				} break;
				case 'e': {
					File >> Tile->EventType >> Tile->EventData;
				} break;
				case 'w': {
					File >> Tile->Wall;
				} break;
				case 'p': {
					File >> Tile->PVP;
				} break;
			}
		}

		File.close();
	}
	catch(std::exception &Error) {
		std::cout << Error.what() << std::endl;
	}

	//if(!TilesInitialized)
	//	Grid->InitTiles();

	// Initialize 2d tile rendering
	if(!ServerNetwork) {
		/*
		TileAtlas = new _Atlas(Assets.Textures[AtlasPath], glm::ivec2(64, 64), 1);

		GLuint TileVertexCount = 4 * Grid->Size.x * Grid->Size.y;
		GLuint TileFaceCount = 2 * Grid->Size.x * Grid->Size.y;

		TileVertices = new glm::vec4[TileVertexCount];
		TileFaces = new glm::u32vec3[TileFaceCount];

		int FaceIndex = 0;
		int VertexIndex = 0;
		for(int j = 0; j < Grid->Size.y; j++) {
			for(int i = 0; i < Grid->Size.x; i++) {
				TileFaces[FaceIndex++] = { VertexIndex + 2, VertexIndex + 1, VertexIndex + 0 };
				TileFaces[FaceIndex++] = { VertexIndex + 2, VertexIndex + 3, VertexIndex + 1 };
				VertexIndex += 4;
			}
		}

		glGenBuffers(1, &TileVertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, TileVertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * TileVertexCount, nullptr, GL_DYNAMIC_DRAW);

		glGenBuffers(1, &TileElementBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TileElementBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glm::u32vec3) * TileFaceCount, nullptr, GL_DYNAMIC_DRAW);
		*/
	}
}

// Saves the level to a file
void _Map::Save(const std::string &String) {
	if(String == "")
		throw std::runtime_error("Empty file name");

	Filename = String;

	// Open file
	gzofstream Output(Filename.c_str());
	if(!Output)
		throw std::runtime_error("Cannot create file: " + Filename);

	// Header
	Output << "V " << MAP_VERSION << '\n';
	Output << "S " << Size.x << " " << Size.y << '\n';
	if(NoZoneTexture)
		Output << "N " << NoZoneTexture->Identifier << '\n';
	else
		Output << "N none\n";

	// Write tile map
	for(int j = 0; j < Size.y; j++) {
		for(int i = 0; i < Size.x; i++) {
			Output << "T " << i << " " << j << '\n';
			if(Tiles[i][j].Texture)
				Output << "t " << Tiles[i][j].Texture->Identifier << '\n';
			else
				Output << "t none\n";
			Output << "z " << Tiles[i][j].Zone << '\n';
			Output << "e " << Tiles[i][j].EventType << " " << Tiles[i][j].EventData << '\n';
			Output << "w " << Tiles[i][j].Wall << '\n';
			Output << "p " << Tiles[i][j].PVP << '\n';
		}
		Output << '\n';
	}

	Output.close();
}

// Builds an array of textures that are used in the map
void _Map::GetTextureListFromMap(std::vector<const _Texture *> &SearchTextures) {

	SearchTextures.clear();

	// Go through map
	for(int i = 0; i < Size.x; i++) {
		for(int j = 0; j < Size.y; j++) {

			// Check for new textures
			if(GetTextureIndex(SearchTextures, Tiles[i][j].Texture) == -1) {
				SearchTextures.push_back(Tiles[i][j].Texture);
			}
		}
	}
}

// Returns the index of a texture in an array
int _Map::GetTextureIndex(std::vector<const _Texture *> &SearchTextures, const _Texture *Texture) {

	for(size_t i = 0; i < SearchTextures.size(); i++) {
		if(SearchTextures[i] == Texture)
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
void _Map::AddObject(_Object *Object) {

	// Create packet for the new object
	_Buffer Packet;
	Packet.Write<char>(_Network::WORLD_CREATEOBJECT);
	Packet.Write<char>(Object->NetworkID);
	Packet.Write<char>(Object->Position.x);
	Packet.Write<char>(Object->Position.y);
	Packet.Write<char>(Object->Type);
	switch(Object->Type) {
		case _Object::PLAYER: {
			_Player *NewPlayer = (_Player *)Object;
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
	Objects.push_back(Object);
}

// Removes an object from the map
void _Map::RemoveObject(_Object *TObject) {

	// Remove from the map
	for(auto Iterator = Objects.begin(); Iterator != Objects.end(); ) {
		if(*Iterator == TObject)
			Iterator = Objects.erase(Iterator);
		else
			++Iterator;
	}

	// Create delete packet
	_Buffer Packet;
	Packet.Write<char>(_Network::WORLD_DELETEOBJECT);
	Packet.Write<char>(TObject->NetworkID);

	// Send to everyone
	SendPacketToPlayers(&Packet);
}

// Returns a list of players close to a player
void _Map::GetClosePlayers(const _Player *Player, float DistanceSquared, std::list<_Player *> &Players) {

	for(const auto &Object : Objects) {
		if(Object->Type == _Object::PLAYER) {
			_Player *TestPlayer = (_Player *)Object;
			if(TestPlayer != Player) {
				glm::ivec2 Delta = TestPlayer->Position - Player->Position;
				if((float)(Delta.x * Delta.x + Delta.y * Delta.y) <= DistanceSquared) {
					Players.push_back(TestPlayer);
				}
			}
		}
	}
}

// Returns the closest player
_Player *_Map::GetClosestPlayer(const _Player *Player, float MaxDistanceSquared, int State) {

	_Player *ClosestPlayer = nullptr;
	float ClosestDistanceSquared = HUGE_VAL;
	for(const auto &Object : Objects) {
		if(Object->Type == _Object::PLAYER) {
			_Player *TestPlayer = (_Player *)Object;
			if(TestPlayer != Player && TestPlayer->State == State) {
				glm::ivec2 Delta = TestPlayer->Position - Player->Position;
				float DistanceSquared = (float)(Delta.x * Delta.x + Delta.y * Delta.y);
				if(DistanceSquared <= MaxDistanceSquared && DistanceSquared < ClosestDistanceSquared) {
					ClosestDistanceSquared = DistanceSquared;
					ClosestPlayer = TestPlayer;
				}
			}
		}
	}

	return ClosestPlayer;
}

// Find an event on the map, returns true on found
bool _Map::FindEvent(int EventType, int EventData, glm::ivec2 &Position) {

	for(int j = 0; j < Size.y; j++) {
		for(int i = 0; i < Size.x; i++) {
			if(Tiles[i][j].EventType == EventType && Tiles[i][j].EventData == EventData) {
				Position.x = i;
				Position.y = j;
				return true;
			}
		}
	}

	return false;
}

// Sends object position information to all the clients in the map
void _Map::SendObjectUpdates() {

	_Buffer Packet;
	Packet.Write<char>(_Network::WORLD_OBJECTUPDATES);

	// Get object count
	int ObjectCount = Objects.size();
	Packet.Write<char>(ObjectCount);

	// Iterate over objects
	for(const auto &Object : Objects) {
		int State = 0;
		bool Invisible = false;
		if(Object->Type == _Object::PLAYER) {
			_Player *Player = (_Player *)Object;
			State = Player->State;
			Invisible = Player->IsInvisible();
		}

		Packet.Write<char>(Object->NetworkID);
		Packet.Write<char>(State);
		Packet.Write<char>(Object->Position.x);
		Packet.Write<char>(Object->Position.y);
		Packet.WriteBit(Invisible);
	}

	SendPacketToPlayers(&Packet, nullptr, _Network::UNSEQUENCED);
}

// Sends a packet to all of the players in the map
void _Map::SendPacketToPlayers(_Buffer *Packet, _Player *ExceptionPlayer, _Network::SendType Type) {

	// Send the packet out
	for(auto &Object : Objects) {
		if(Object->Type == _Object::PLAYER) {
			_Player *Player = (_Player *)Object;

			if(Player != ExceptionPlayer)
				ServerNetwork->SendPacketToPeer(Packet, Player->Peer, Type, Type == _Network::UNSEQUENCED);
		}
	}
}

// Get a valid position within the grid
glm::vec2 _Map::GetValidPosition(const glm::vec2 &Position) {
	return glm::clamp(Position, glm::vec2(0.0f), glm::vec2(Size));
}
