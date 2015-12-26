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
#include <objects/map.h>
#include <states/editor.h>
#include <network/servernetwork.h>
#include <network/peer.h>
#include <objects/object.h>
#include <server.h>
#include <graphics.h>
#include <constants.h>
#include <stats.h>
#include <buffer.h>
#include <manager.h>
#include <texture.h>
#include <assets.h>
#include <atlas.h>
#include <font.h>
#include <program.h>
#include <camera.h>
#include <packet.h>
#include <glm/vec3.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <zlib/zfstream.h>
#include <iostream>
#include <fstream>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <iomanip>

// Constructor
_Map::_Map() :
	Tiles(nullptr),
	Size(0, 0),
	TileAtlas(nullptr),
	AmbientLight(MAP_AMBIENT_LIGHT),
	IsOutside(true),
	Clock(0),
	ObjectUpdateTime(0),
	Server(nullptr),
	TileVertexBufferID(0),
	TileElementBufferID(0),
	TileVertices(nullptr),
	TileFaces(nullptr) {

}

// Destructor
_Map::~_Map() {
	if(!Server) {
		CloseAtlas();
	}

	// Delete map data
	FreeMap();

	for(auto &Object : Objects)
		Object->Map = nullptr;
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

// Resize tile data
void _Map::ResizeMap(glm::ivec2 Offset, glm::ivec2 NewSize) {

	// Create new map
	_Tile **NewTiles = new _Tile*[NewSize.x];
	for(int i = 0; i < NewSize.x; i++) {
		NewTiles[i] = new _Tile[NewSize.y];
	}

	// Copy data
	glm::ivec2 TileIndex;
	for(int j = 0; j < Size.y; j++) {
		TileIndex.y = j - Offset.y;
		if(TileIndex.y < 0 || TileIndex.y >= NewSize.y)
			continue;

		for(int i = 0; i < Size.x; i++) {
			TileIndex.x = i - Offset.x;
			if(TileIndex.x < 0 || TileIndex.x >= NewSize.x)
				continue;

			NewTiles[TileIndex.x][TileIndex.y] = Tiles[i][j];
		}
	}

	// Save old texture atlas name
	std::string OldTextureAtlas = "";
	if(TileAtlas)
		OldTextureAtlas = TileAtlas->Texture->Identifier;

	// Delete data
	CloseAtlas();
	FreeMap();

	// Init new data
	Tiles = NewTiles;
	Size = NewSize;
	if(OldTextureAtlas != "")
		InitAtlas(OldTextureAtlas);

	// Update index
	IndexEvents();
}

// Initialize the texture atlas
void _Map::InitAtlas(const std::string AtlasPath) {
	const _Texture *AtlasTexture = Assets.Textures[AtlasPath];
	if(!AtlasTexture)
		throw std::runtime_error("Can't find atlas: " + AtlasPath);

	TileAtlas = new _Atlas(AtlasTexture, glm::ivec2(MAP_TILE_WIDTH, MAP_TILE_HEIGHT), 1);

	GLuint TileVertexCount = (GLuint)(4 * Size.x * Size.y);
	GLuint TileFaceCount = (GLuint)(2 * Size.x * Size.y);

	TileVertices = new glm::vec4[TileVertexCount];
	TileFaces = new glm::u32vec3[TileFaceCount];

	int FaceIndex = 0;
	int VertexIndex = 0;
	for(int j = 0; j < Size.y; j++) {
		for(int i = 0; i < Size.x; i++) {
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
}

// Free memory used by texture atlas
void _Map::CloseAtlas() {
	delete TileAtlas;
	delete[] TileVertices;
	delete[] TileFaces;
	glDeleteBuffers(1, &TileVertexBufferID);
	glDeleteBuffers(1, &TileElementBufferID);

	TileVertexBufferID = 0;
	TileElementBufferID = 0;
	TileAtlas = nullptr;
	TileVertices = nullptr;
	TileFaces = nullptr;
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

// Updates the map and sends object updates
void _Map::Update(double FrameTime) {

	// Update clock
	Clock += FrameTime * MAP_CLOCK_SPEED;
	if(Clock >= MAP_DAY_LENGTH)
		Clock -= MAP_DAY_LENGTH;
}

// Check for events
void _Map::CheckEvents(_Object *Object) {

	// Check for teleporting
	if(Server && Object->TeleportTime == 0.0) {
		Object->TeleportTime = -1.0;
		Object->Status = _Object::STATUS_NONE;
		Server->SpawnPlayer(Object, Object->SpawnMapID, _Map::EVENT_SPAWN);
		return;
	}

	// Handle events
	const _Tile *Tile = &Tiles[Object->Position.x][Object->Position.y];
	switch(Tile->Event.Type) {
		case _Map::EVENT_SPAWN:
			//Server->Log << Object->Map->ID << " " << Tile->Event.Data << std::endl;
			//Object->SpawnMapID = Object->Map->ID;
			//Object->SpawnPoint = Tile->Event.Data;
			//Object->RestoreHealthMana();
			//SendHUD(Player);
			//Object->Save();
		break;
		case _Map::EVENT_MAPCHANGE:
			if(Server)
				Server->SpawnPlayer(Object, Tile->Event.Data, Tile->Event.Type);
			else
				Object->WaitForServer = true;
		break;
		case _Map::EVENT_VENDOR: {
			if(Server) {
				Object->Vendor = Server->Stats->GetVendor(Tile->Event.Data);

				// Notify client
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::EVENT_START);
				Packet.Write<uint32_t>(Tile->Event.Type);
				Packet.Write<uint32_t>(Tile->Event.Data);
				Packet.Write<glm::ivec2>(Object->Position);
				Server->Network->SendPacket(Packet, Object->Peer);
			}
			else
				Object->WaitForServer = true;
		} break;
		case _Map::EVENT_TRADER: {
			if(Server) {
				Object->Trader = Server->Stats->GetTrader(Tile->Event.Data);

				// Notify client
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::EVENT_START);
				Packet.Write<uint32_t>(Tile->Event.Type);
				Packet.Write<uint32_t>(Tile->Event.Data);
				Packet.Write<glm::ivec2>(Object->Position);
				Server->Network->SendPacket(Packet, Object->Peer);
			}
			else
				Object->WaitForServer = true;
		} break;
		default:
			if(Server) {
				Object->Vendor = nullptr;
				Object->Trader = nullptr;

				if(Object->NextBattle <= 0) {
					Server->StartBattle(Object, Tile->Zone);
				}
			}
		break;
	}
}

// Build indexed events list
void _Map::IndexEvents() {
	IndexedEvents.clear();

	// Build event index
	for(int j = 0; j < Size.y; j++) {
		for(int i = 0; i < Size.x; i++) {
			const _Tile &Tile = Tiles[i][j];
			if(Tile.Event.Type != EVENT_NONE) {
				IndexedEvents[Tile.Event] = glm::ivec2(i, j);
			}
		}
	}
}

// Convert clock time to text
void _Map::GetClockAsString(std::stringstream &Buffer) {

	int Hours = (int)(Clock / 60.0);
	if(Hours == 0)
		Hours = 12;
	else if(Hours > 12)
		Hours -= 12;

	int Minutes = (int)std::fmod(Clock, 60.0);

	if(Clock < MAP_DAY_LENGTH / 2)
		Buffer << Hours << ":" << std::setfill('0') << std::setw(2) << Minutes << " AM";
	else
		Buffer << Hours << ":" << std::setfill('0') << std::setw(2) << Minutes << " PM";
}

// Set ambient light for map
void _Map::SetAmbientLightByClock() {

	std::vector<glm::vec4> Cycles = {
		{0.05f, 0.05f, 0.3f, 1},
		{0.10f, 0.10f, 0.1f, 1},
		{0.6f, 0.6f, 0.45f, 1},
		{0.55f, 0.45f, 0.30f, 1},
		{0.5, 0.4f, 0.3f, 1},
	};

	std::vector<double> CyclesTime = {
		0.0 * 60.0,
		6.0 * 60.0,
		12.5 * 60.0,
		16.5 * 60.0,
		18.0 * 60.0,
	};

	// Find index by time
	size_t NextCycle = CyclesTime.size();
	for(size_t i = 0; i < CyclesTime.size(); i++) {
		if(Clock < CyclesTime[i]) {
			NextCycle = i;
			break;
		}
	}

	// Get indices for current and next cycle
	size_t CurrentCycle = NextCycle - 1;
	if(CurrentCycle >= CyclesTime.size())
		CurrentCycle = 0;
	if(NextCycle >= CyclesTime.size())
		NextCycle = 0;

	// Get current time diff
	double Diff = Clock - CyclesTime[CurrentCycle];
	if(Diff < 0)
		Diff += MAP_DAY_LENGTH;

	// Get length of cycle
	double Length = CyclesTime[NextCycle] - CyclesTime[CurrentCycle];
	if(Length < 0)
		Length += MAP_DAY_LENGTH;

	// Get percent to next cycle
	float Percent = (float)(Diff / Length);

	// Set color
	AmbientLight = glm::mix(Cycles[CurrentCycle], Cycles[NextCycle], Percent);
}

// Renders the map
void _Map::Render(_Camera *Camera, _Stats *Stats, _Object *ClientPlayer, int RenderFlags) {

	// Set lights for editor
	if(!ClientPlayer) {
		glm::vec4 AmbientLightEditor(1.0f, 1.0f, 1.0f, 1.0f);
		Assets.Programs["pos_uv"]->AmbientLight = AmbientLightEditor;
		Assets.Programs["pos_uv"]->LightPosition = glm::vec3(0, 0, 0);
		Assets.Programs["pos_uv_norm"]->AmbientLight = AmbientLightEditor;
		Assets.Programs["pos_uv_norm"]->LightPosition = glm::vec3(0, 0, 0);
	}
	else {

		// Setup day night cycle
		if(IsOutside)
			SetAmbientLightByClock();

		// Setup lights
		glm::vec3 LightPosition(glm::vec3(ClientPlayer->Position, 1) + glm::vec3(0.5f, 0.5f, 0));
		glm::vec3 LightAttenuation(0.0f, 1.0f, 0.0f);

		Assets.Programs["pos_uv"]->LightAttenuation = LightAttenuation;
		Assets.Programs["pos_uv"]->LightPosition = LightPosition;
		Assets.Programs["pos_uv"]->AmbientLight = AmbientLight;
		Assets.Programs["pos_uv_norm"]->LightAttenuation = LightAttenuation;
		Assets.Programs["pos_uv_norm"]->LightPosition = LightPosition;
		Assets.Programs["pos_uv_norm"]->AmbientLight = AmbientLight;
	}

	// Render bounds
	glm::vec4 Bounds = Camera->GetAABB();
	Bounds[0] = glm::clamp(Bounds[0], 0.0f, (float)Size.x);
	Bounds[1] = glm::clamp(Bounds[1], 0.0f, (float)Size.y);
	Bounds[2] = glm::clamp(Bounds[2], 0.0f, (float)Size.x);
	Bounds[3] = glm::clamp(Bounds[3], 0.0f, (float)Size.y);

	RenderLayer(Bounds, 0);
	RenderLayer(Bounds, 1);

	// Render objects
	for(const auto &Object : Objects) {
		Object->Render(ClientPlayer);
	}

	// Check for flags
	if(!RenderFlags)
		return;

	// Draw map boundaries
	if(RenderFlags & FILTER_BOUNDARY) {
		Graphics.SetProgram(Assets.Programs["pos"]);
		Graphics.SetVBO(VBO_NONE);
		Graphics.SetColor(COLOR_RED);
		Graphics.DrawRectangle(glm::vec2(-0.51f, -0.51f), glm::vec2(Size.x - 0.49f, Size.y - 0.49f));
	}

	// Draw text overlay
	for(int j = (int)Bounds[1]; j < Bounds[3]; j++) {
		for(int i = (int)Bounds[0]; i < Bounds[2]; i++) {
			_Tile *Tile = &Tiles[i][j];
			glm::vec3 DrawPosition = glm::vec3(i, j, 0) + glm::vec3(0.5f, 0.5f, 0);

			// Draw wall
			if(Tile->Wall) {
				if(RenderFlags & FILTER_WALL)
					Assets.Fonts["hud_small"]->DrawText("W", glm::vec2(DrawPosition), COLOR_WHITE, CENTER_MIDDLE, 1.0f / 64.0f);
			}
			else {

				// Draw zone number
				if((RenderFlags & FILTER_ZONE) && Tile->Zone > 0)
					Assets.Fonts["hud_small"]->DrawText(std::to_string(Tile->Zone).c_str(), glm::vec2(DrawPosition), COLOR_WHITE, CENTER_MIDDLE, 1.0f / 64.0f);

				// Draw PVP
				if((RenderFlags & FILTER_PVP) && Tile->PVP)
					Assets.Fonts["hud_small"]->DrawText("PVP", glm::vec2(DrawPosition), COLOR_RED, CENTER_MIDDLE, 1.0f / 64.0f);
			}

			// Draw event info
			if(Tile->Event.Type > 0) {
				std::string EventText = Stats->EventNames[Tile->Event.Type].ShortName + std::string(" ") + std::to_string(Tile->Event.Data);
				Assets.Fonts["hud_small"]->DrawText(EventText, glm::vec2(DrawPosition), COLOR_CYAN, CENTER_MIDDLE, 1.0f / 64.0f);
			}
		}
	}
}

// Render either floor or foreground texture tiles
void _Map::RenderLayer(glm::vec4 &Bounds, int Layer) {
	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv"]->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));
	Graphics.SetColor(COLOR_WHITE);

	uint32_t VertexIndex = 0;
	int FaceIndex = 0;

	for(int j = (int)Bounds[1]; j < Bounds[3]; j++) {
		for(int i = (int)Bounds[0]; i < Bounds[2]; i++) {
			glm::vec4 TextureCoords = TileAtlas->GetTextureCoords(Tiles[i][j].TextureIndex[Layer]);
			TileVertices[VertexIndex++] = { i + 0.0f, j + 0.0f, TextureCoords[0], TextureCoords[1] };
			TileVertices[VertexIndex++] = { i + 1.0f, j + 0.0f, TextureCoords[2], TextureCoords[1] };
			TileVertices[VertexIndex++] = { i + 0.0f, j + 1.0f, TextureCoords[0], TextureCoords[3] };
			TileVertices[VertexIndex++] = { i + 1.0f, j + 1.0f, TextureCoords[2], TextureCoords[3] };

			FaceIndex += 2;
		}
	}

	GLsizeiptr VertexBufferSize = VertexIndex * sizeof(glm::vec4);
	GLsizeiptr ElementBufferSize = FaceIndex * (int)sizeof(glm::u32vec3);

	Graphics.SetTextureID(TileAtlas->Texture->ID);
	Graphics.EnableAttribs(2);

	glBindBuffer(GL_ARRAY_BUFFER, TileVertexBufferID);
	glBufferSubData(GL_ARRAY_BUFFER, 0, VertexBufferSize, TileVertices);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void *)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void *)sizeof(glm::vec2));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TileElementBufferID);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, ElementBufferSize, TileFaces);
	glDrawElements(GL_TRIANGLES, FaceIndex * 3, GL_UNSIGNED_INT, 0);

	Graphics.DirtyState();
}

// Load map
void _Map::Load(const std::string &Path) {
	std::string AtlasPath;

	// Load file
	gzifstream File(Path.c_str());
	if(!File)
		throw std::runtime_error("Cannot load map: " + Path);

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
					throw std::runtime_error("Level version mismatch: " + std::to_string(FileVersion));
			} break;
			// Map size
			case 'S': {
				File >> Size.x >> Size.y;
				FreeMap();
				AllocateMap();
			} break;
			// Ambient light
			case 'A': {
				File >> AmbientLight.r >> AmbientLight.g >> AmbientLight.b;
			} break;
			// Outside
			case 'O': {
				File >> IsOutside;
			} break;
			// Atlas texture
			case 'a': {
				File >> AtlasPath;
			} break;
			// Tile
			case 'T': {
				glm::ivec2 Coordinate;
				File >> Coordinate.x >> Coordinate.y;
				if(Coordinate.x < Size.x && Coordinate.y < Size.y)
					Tile = &Tiles[Coordinate.x][Coordinate.y];
				else
					Tile = nullptr;
			} break;
			// Texture index
			case 'b': {
				if(Tile)
					File >> Tile->TextureIndex[0];
			} break;
			// Foreground texture index
			case 'f': {
				if(Tile)
					File >> Tile->TextureIndex[1];
			} break;
			// Zone
			case 'z': {
				if(Tile)
					File >> Tile->Zone;
			} break;
			// Event
			case 'e': {
				if(Tile)
					File >> Tile->Event.Type >> Tile->Event.Data;
			} break;
			// Wall
			case 'w': {
				if(Tile)
					File >> Tile->Wall;
			} break;
			// PVP
			case 'p': {
				if(Tile)
					File >> Tile->PVP;
			} break;
			default:
				File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			break;
		}
	}

	File.close();

	// Index events
	IndexEvents();

	// Initialize 2d tile rendering
	if(!Server) {
		InitAtlas(AtlasPath);
	}
}

// Saves the level to a file
bool _Map::Save(const std::string &Path) {
	if(Path == "")
		return false;

	// Open file
	gzofstream Output(Path.c_str());
	if(!Output)
		throw std::runtime_error("Cannot create file: " + Path);

	// Header
	Output << "V " << MAP_VERSION << '\n';
	Output << "S " << Size.x << " " << Size.y << '\n';
	Output << "A " << AmbientLight.r << " " << AmbientLight.g << " " << AmbientLight.b << '\n';
	Output << "O " << IsOutside << '\n';
	Output << "a " << TileAtlas->Texture->Identifier << '\n';

	// Write tile map
	for(int j = 0; j < Size.y; j++) {
		for(int i = 0; i < Size.x; i++) {
			const _Tile &Tile = Tiles[i][j];
			Output << "T " << i << " " << j << '\n';
			if(Tile.TextureIndex[0])
				Output << "b " << Tile.TextureIndex[0] << '\n';
			if(Tile.TextureIndex[1])
				Output << "f " << Tile.TextureIndex[1] << '\n';
			if(Tile.Zone)
				Output << "z " << Tile.Zone << '\n';
			if(Tile.Event.Type)
				Output << "e " << Tile.Event.Type << " " << Tiles[i][j].Event.Data << '\n';
			if(Tile.Wall)
				Output << "w " << Tile.Wall << '\n';
			if(Tile.PVP)
				Output << "p " << Tile.PVP << '\n';
		}
		Output << '\n';
	}

	Output.close();

	return true;
}

// Determines if a square can be moved to
bool _Map::CanMoveTo(const glm::ivec2 &Position) {

	// Bounds
	if(Position.x < 0 || Position.x >= Size.x || Position.y < 0 || Position.y >= Size.y)
		return false;

	return !Tiles[Position.x][Position.y].Wall;
}

// Removes an object from the map
void _Map::RemoveObject(const _Object *RemoveObject) {

	// Notify peers
	if(Server) {

		// Create packet
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_DELETEOBJECT);
		Packet.Write<NetworkIDType>(RemoveObject->NetworkID);

		// Send to everyone
		BroadcastPacket(Packet);
	}

	// Remove object
	auto Iterator = std::find(Objects.begin(), Objects.end(), RemoveObject);
	if(Iterator != Objects.end())
		Objects.erase(Iterator);
}

// Adds an object to the map
void _Map::AddObject(_Object *Object) {
	if(Server) {

		// Create packet for the new object
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_CREATEOBJECT);
		Object->SerializeCreate(Packet);

		// Notify other players of the new object
		BroadcastPacket(Packet);
	}

	// Add object to map
	Objects.push_back(Object);
}

// Returns a list of players close to a player
void _Map::GetClosePlayers(const _Object *Player, float DistanceSquared, std::list<_Object *> &Players) {

	for(const auto &Object : Objects) {
		if(Object != Player) {
			glm::vec2 Delta = Object->Position - Player->Position;
			if(glm::dot(Delta, Delta) <= DistanceSquared) {
				Players.push_back(Object);
			}
		}
	}
}

// Returns the closest player
_Object *_Map::FindTradePlayer(const _Object *Player, float MaxDistanceSquared) {

	_Object *ClosestPlayer = nullptr;
	float ClosestDistanceSquared = HUGE_VAL;
	for(const auto &Object : Objects) {
		if(Object != Player && Object->WaitingForTrade && Object->TradePlayer == nullptr) {
			glm::vec2 Delta = Object->Position - Player->Position;
			float DistanceSquared = glm::dot(Delta, Delta);
			if(DistanceSquared <= MaxDistanceSquared && DistanceSquared < ClosestDistanceSquared) {
				ClosestDistanceSquared = DistanceSquared;
				ClosestPlayer = Object;
			}
		}
	}

	return ClosestPlayer;
}

// Find an event on the map, returns true on found
bool _Map::FindEvent(const _Event &Event, glm::ivec2 &Position) {

	// Find event
	auto Iterator = IndexedEvents.find(Event);
	if(Iterator == IndexedEvents.end())
		return false;

	// Return position
	Position = Iterator->second;

	return true;
}

// Send complete object list to player
void _Map::SendObjectList(_Peer *Peer) {
	if(!Server)
		return;

	if(!Peer->Object)
		return;

	// Create packet
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_OBJECTLIST);
	Packet.Write<NetworkIDType>(Peer->Object->NetworkID);

	// Write object data
	Packet.Write<NetworkIDType>((NetworkIDType)Objects.size());
	for(auto &Object : Objects) {
		Object->SerializeCreate(Packet);
	}

	Server->Network->SendPacket(Packet, Peer);
}

// Sends object position information to all the clients in the map
void _Map::SendObjectUpdates() {

	// Create packet
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_OBJECTUPDATES);
	Packet.Write<uint8_t>((uint8_t)NetworkID);

	// Write object count
	Packet.Write<NetworkIDType>((NetworkIDType)Objects.size());

	// Iterate over objects
	for(const auto &Object : Objects) {
		Object->SerializeUpdate(Packet);
	}

	// Send packet to players in map
	BroadcastPacket(Packet, _Network::UNSEQUENCED);
}

// Broadcast a packet to all peers in the map
void _Map::BroadcastPacket(_Buffer &Buffer, _Network::SendType Type) {
	if(!Server)
		return;

	// Send packet to peers
	for(auto &Object : Objects) {
		if(Object->Peer)
			Server->Network->SendPacket(Buffer, Object->Peer, Type, Type == _Network::UNSEQUENCED);
	}
}

// Get a valid position within the grid
glm::vec2 _Map::GetValidPosition(const glm::vec2 &Position) {
	return glm::clamp(Position, glm::vec2(0.0f), glm::vec2(Size));
}

// Get a valid position within the grid
glm::ivec2 _Map::GetValidCoord(const glm::ivec2 &Position) {
	return glm::clamp(Position, glm::ivec2(0), Size - 1);
}
