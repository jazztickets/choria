/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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
#include <objects/object.h>
#include <objects/components/character.h>
#include <objects/components/inventory.h>
#include <objects/components/controller.h>
#include <objects/components/light.h>
#include <objects/components/prop.h>
#include <objects/object.h>
#include <objects/battle.h>
#include <hud/hud.h>
#include <ae/buffer.h>
#include <ae/texture.h>
#include <ae/assets.h>
#include <ae/texture_array.h>
#include <ae/tilemap.h>
#include <ae/font.h>
#include <ae/framebuffer.h>
#include <ae/program.h>
#include <ae/camera.h>
#include <ae/servernetwork.h>
#include <ae/peer.h>
#include <ae/graphics.h>
#include <ae/random.h>
#include <server.h>
#include <constants.h>
#include <stats.h>
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

// Colors of each time cycle
const std::vector<glm::vec4> DayCycles = {
	{ 0.05f, 0.05f, 0.3f,  1 },
	{ 0.10f, 0.10f, 0.1f,  1 },
	{ 0.6f,  0.6f,  0.45f, 1 },
	{ 0.55f, 0.45f, 0.30f, 1 },
	{ 0.5,   0.4f,  0.3f,  1 },
};

// Time of each cycle change
const std::vector<double> DayCyclesTime = {
	0.0  * 60.0,
	6.0  * 60.0,
	12.5 * 60.0,
	16.5 * 60.0,
	18.0 * 60.0,
};

// Constructor
_Map::_Map() :
	Loaded(false),
	Tiles(nullptr),
	Size(0, 0),
	Headless(false),
	AmbientLight(MAP_AMBIENT_LIGHT),
	IsOutside(true),
	Clock(0),
	LightCount(0),
	PropCount(0),
	BackgroundOffset(0.0f),
	BackgroundMap(nullptr),
	ObjectUpdateTime(0),
	Stats(nullptr),
	Scripting(nullptr),
	Server(nullptr),
	Pather(nullptr),
	MapVertexBufferID(0),
	MapTextureID(0) {

}

// Destructor
_Map::~_Map() {

	// Delete path finding
	delete Pather;

	// Delete background layer
	delete BackgroundMap;

	// Delete vertex data
	if(!Headless)
		CloseVertices();

	// Delete map data
	FreeMap();

	// Update objects
	for(auto &Object : Objects)
		Object->Map = nullptr;

	// Delete objects managed by map
	for(auto &Object : StaticObjects)
		delete Object;
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

	// Delete data
	CloseVertices();
	FreeMap();

	// Init new data
	Tiles = NewTiles;
	Size = NewSize;
	InitVertices();

	// Update index
	IndexEvents();
}

// Initialize vbo and vertex data
void _Map::InitVertices(bool Static) {

	// Build quad for map
	float Vertices[] = {
		0, 1,
		1, 1,
		0, 0,
		1, 0,
		0, 1,
		1, 1,
		0, 0,
		1, 0,
	};

	// Generate vertex buffer and bind
	glGenBuffers(1, &MapVertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, MapVertexBufferID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

	// Allocate texture array for tile lookups
	glGenTextures(1, &MapTextureID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, MapTextureID);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, Size.x, Size.y, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Build lookup table of transition index to texture index
	TransitionLookup[0] = 0;
	TransitionLookup[1] = 1;
	TransitionLookup[2] = 2;
	TransitionLookup[4] = 3;
	TransitionLookup[5] = 4;
	TransitionLookup[8] = 5;
	TransitionLookup[10] = 6;
	TransitionLookup[12] = 7;
	TransitionLookup[16] = 8;
	TransitionLookup[17] = 9;
	TransitionLookup[18] = 10;
	TransitionLookup[24] = 11;
	TransitionLookup[26] = 12;
	TransitionLookup[32] = 13;
	TransitionLookup[33] = 14;
	TransitionLookup[34] = 15;
	TransitionLookup[36] = 16;
	TransitionLookup[37] = 17;
	TransitionLookup[48] = 18;
	TransitionLookup[49] = 19;
	TransitionLookup[50] = 20;
	TransitionLookup[64] = 21;
	TransitionLookup[65] = 22;
	TransitionLookup[66] = 23;
	TransitionLookup[68] = 24;
	TransitionLookup[69] = 25;
	TransitionLookup[72] = 26;
	TransitionLookup[74] = 27;
	TransitionLookup[76] = 28;
	TransitionLookup[80] = 29;
	TransitionLookup[81] = 30;
	TransitionLookup[82] = 31;
	TransitionLookup[88] = 32;
	TransitionLookup[90] = 33;
	TransitionLookup[128] = 34;
	TransitionLookup[129] = 35;
	TransitionLookup[130] = 36;
	TransitionLookup[132] = 37;
	TransitionLookup[133] = 38;
	TransitionLookup[136] = 39;
	TransitionLookup[138] = 40;
	TransitionLookup[140] = 41;
	TransitionLookup[160] = 42;
	TransitionLookup[161] = 43;
	TransitionLookup[162] = 44;
	TransitionLookup[164] = 45;
	TransitionLookup[165] = 46;
}

// Free memory used by rendering
void _Map::CloseVertices() {
	if(MapVertexBufferID)
		glDeleteBuffers(1, &MapVertexBufferID);
	MapVertexBufferID = 0;

	if(MapTextureID)
		glDeleteTextures(1, &MapTextureID);
	MapTextureID = 0;
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

// Updates the map
void _Map::Update(double FrameTime) {
	if(!Loaded)
		return;

	// Update clock
	Clock += FrameTime * MAP_CLOCK_SPEED;
	if(Clock >= MAP_DAY_LENGTH)
		Clock -= MAP_DAY_LENGTH;

	// Update static objects
	for(const auto &Object : StaticObjects) {
		Object->UpdateStatic(FrameTime);
	}
}

// Check for events
void _Map::CheckEvents(_Object *Object) {

	// Check for teleporting
	if(Server && Object->Character->TeleportTime == 0.0) {
		Object->Character->TeleportTime = -1.0;
		Object->Character->Status = _Character::STATUS_NONE;
		Server->SpawnPlayer(Object, Object->Character->SpawnMap, EventType::SPAWN);
		return;
	}

	//TODO fix
	// Handle events
	const _Tile *Tile = &Tiles[(int)Object->Position.x][(int)Object->Position.y];
	switch(Tile->Event.Type) {
		case EventType::SPAWN:
			if(Server && !(Object->Character->SpawnMap == this && Object->Character->SpawnPoint == Tile->Event.Data))
				Server->SendMessage(Object->Peer, "Spawn point set", "yellow");

			Object->Character->SpawnMap = this;
			Object->Character->SpawnPoint = Tile->Event.Data;
		break;
		case EventType::MAPENTRANCE:
		case EventType::MAPCHANGE:
			/*
			if(Server)
				Server->SpawnPlayer(Object, (ae::NetworkIDType)Tile->Event.Data, EventType::MAPENTRANCE);
			else
				Object->Controller->WaitForServer = true;
				*/
		break;
		case EventType::VENDOR:
		case EventType::TRADER:
		case EventType::BLACKSMITH:
		case EventType::MINIGAME: {
			if(Server)
				StartEvent(Object, Tile->Event);
			else
				Object->Controller->WaitForServer = true;
		} break;
		case EventType::SCRIPT: {
			if(Server)
				Server->RunEventScript(Tile->Event.Data, Object);
		} break;
		case EventType::PORTAL: {
			if(Server) {

				// Find matching even/odd event
				//FindEvent(_Event(Tile->Event.Type, Tile->Event.OldData ^ 1), Object->Position);
				Server->SendPlayerPosition(Object->Peer);
			}
			else
				Object->Controller->WaitForServer = true;
		} break;
		case EventType::JUMP: {
			if(Server) {

				// Find next jump
				//FindEvent(_Event(Tile->Event.Type, Tile->Event.OldData + 1), Object->Position);
				Server->SendPlayerPosition(Object->Peer);
			}
			else
				Object->Controller->WaitForServer = true;
		} break;
		default:
			if(Server) {
				Object->Character->Vendor = nullptr;
				Object->Character->Trader = nullptr;

				if(Object->Character->NextBattle <= 0) {
					Server->QueueBattle(Object, Tile->ZoneID, false, false, 0.0f, 0.0f);
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
			if(Tile.Event.Type != EventType::NONE) {
				IndexedEvents[Tile.Event].push_back(glm::ivec2(i, j));
			}
		}
	}
}

// Convert clock time to text
void _Map::GetClockAsString(std::stringstream &Buffer) const {

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
	if(!IsOutside)
		return;

	// Find index by time
	size_t NextCycle = DayCyclesTime.size();
	for(size_t i = 0; i < DayCyclesTime.size(); i++) {
		if(Clock < DayCyclesTime[i]) {
			NextCycle = i;
			break;
		}
	}

	// Get indices for current and next cycle
	size_t CurrentCycle = NextCycle - 1;
	if(CurrentCycle >= DayCyclesTime.size())
		CurrentCycle = 0;
	if(NextCycle >= DayCyclesTime.size())
		NextCycle = 0;

	// Get current time diff
	double Diff = Clock - DayCyclesTime[CurrentCycle];
	if(Diff < 0)
		Diff += MAP_DAY_LENGTH;

	// Get length of cycle
	double Length = DayCyclesTime[NextCycle] - DayCyclesTime[CurrentCycle];
	if(Length < 0)
		Length += MAP_DAY_LENGTH;

	// Get percent to next cycle
	float Percent = (float)(Diff / Length);

	// Set color
	AmbientLight = glm::mix(DayCycles[CurrentCycle], DayCycles[NextCycle], Percent);
}

// Start event for an object and send packet
void _Map::StartEvent(_Object *Object, _Event Event) const {
	if(!Server)
		return;

	// Handle event types
	try {
		switch(Event.Type) {
			case EventType::TRADER:
				Object->Character->Trader = &Server->Stats->Traders.at(Event.Data);
				if(!Object->Character->Trader)
					return;
			break;
			case EventType::VENDOR:
				Object->Character->Vendor = &Server->Stats->Vendors.at(Event.Data);
				if(!Object->Character->Vendor)
					return;
			break;
			case EventType::BLACKSMITH:
				Object->Character->Blacksmith = &Server->Stats->Blacksmiths.at(Event.Data);
				if(!Object->Character->Blacksmith)
					return;
			break;
			case EventType::MINIGAME: {
				Object->Character->Minigame = &Server->Stats->Minigames.at(Event.Data);
				if(!Object->Character->Minigame)
					return;
			} break;
			default:
				return;
			break;
		}
	}
	catch(std::exception &Error) {
		return;
	}

	// Notify client
	if(Object->Peer->ENetPeer) {
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::EVENT_START);
		Packet.Write<EventType>(Event.Type);
		Packet.WriteString(Event.Data.c_str());
		Packet.Write<glm::ivec2>(Object->Position);
		Server->Network->SendPacket(Packet, Object->Peer);
	}

	// Generate seed
	if(Event.Type == EventType::MINIGAME) {
		Object->SendSeed(false);
	}
}

// Determine if a position is in a pvp zone
bool _Map::IsPVPZone(const glm::ivec2 &Position) const {
	if(!IsValidPosition(Position))
		return false;

	return GetTile(Position)->PVP;
}

// Set layer texture indexes for each tile
void _Map::BuildLayers(const glm::ivec4 &Bounds, bool ShowTransitions) {
	glm::ivec2 Start = GetValidCoord(glm::ivec2(Bounds[0], Bounds[1]));
	glm::ivec2 End = GetValidCoord(glm::ivec2(Bounds[2], Bounds[3]));

	// Update map lookup texture
	glBindTexture(GL_TEXTURE_2D_ARRAY, MapTextureID);
	for(int j = Start.y; j <= End.y; j++) {
		for(int i = Start.x; i <= End.x; i++) {
			_Tile &Tile = Tiles[i + 0][j + 0];
			Tile.TextureIndex[(int)MapLayerType::BASE] = Tile.BaseTextureIndex;
			Tile.TextureIndex[(int)MapLayerType::FIRST_TRANS] = 0;
			Tile.TextureIndex[(int)MapLayerType::FIRST_LAYER] = 0;
			Tile.TextureIndex[(int)MapLayerType::SECOND_TRANS] = 0;
			Tile.TextureIndex[(int)MapLayerType::SECOND_LAYER] = 0;
			Tile.TextureIndex[(int)MapLayerType::THIRD_TRANS] = 0;
			Tile.TextureIndex[(int)MapLayerType::THIRD_LAYER] = 0;
			if(ShowTransitions && Tile.Hierarchy != -1) {
				std::map<uint32_t, std::pair<int, uint32_t> > Pairs;

				// Check corners
				GetTransition(Tile, glm::ivec2(i - 1, j - 1), Pairs, 1);
				GetTransition(Tile, glm::ivec2(i + 1, j - 1), Pairs, 4);
				GetTransition(Tile, glm::ivec2(i - 1, j + 1), Pairs, 32);
				GetTransition(Tile, glm::ivec2(i + 1, j + 1), Pairs, 128);

				// Check edges
				GetTransition(Tile, glm::ivec2(i + 0, j - 1), Pairs, 2);
				GetTransition(Tile, glm::ivec2(i - 1, j + 0), Pairs, 8);
				GetTransition(Tile, glm::ivec2(i + 1, j + 0), Pairs, 16);
				GetTransition(Tile, glm::ivec2(i + 0, j + 1), Pairs, 64);

				// Create sorted map that makes hierarchy the key, and texture index the first in the pair
				std::map<int, std::pair<uint32_t, uint32_t> > SortedPairs;
				for(const auto &Pair : Pairs) {
					SortedPairs[Pair.second.first].first = Pair.first;
					SortedPairs[Pair.second.first].second = Pair.second.second;
				}

				// Build list of textures in order of base to top
				int TextureIndex = (int)MapLayerType::FIRST_TRANS;
				for(const auto &Pair : SortedPairs) {

					// Too many transitions
					if(TextureIndex >= (int)MapLayerType::COUNT)
						break;

					// Set transition for the layer
					Tile.TextureIndex[TextureIndex] = TransitionLookup[Pair.second.second];

					// Set layer texture
					Tile.TextureIndex[TextureIndex + 1] = Pair.second.first;
					TextureIndex += 2;
				}
			}

			// Set texture indexes in map lookup texture
			GLuint Pixel;
			Pixel =
				(Tile.TextureIndex[(int)MapLayerType::BASE] << 0) |
				(Tile.TextureIndex[(int)MapLayerType::FIRST_TRANS] << 8) |
				(Tile.TextureIndex[(int)MapLayerType::FIRST_LAYER] << 16) |
				(Tile.TextureIndex[(int)MapLayerType::SECOND_TRANS] << 24);

			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, i, j, 0, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &Pixel);

			Pixel =
				(Tile.TextureIndex[(int)MapLayerType::SECOND_LAYER] << 0) |
				(Tile.TextureIndex[(int)MapLayerType::THIRD_TRANS] << 8) |
				(Tile.TextureIndex[(int)MapLayerType::THIRD_LAYER] << 16);

			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, i, j, 1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &Pixel);
		}
	}
}

// Return 'Bit' if 'Tile' has a lower precedence than the tile in CheckPosition
void _Map::GetTransition(_Tile &Tile, const glm::ivec2 &CheckPosition, std::map<uint32_t, std::pair<int, uint32_t> > &Pairs, uint32_t Bit) {

	// Get valid tile to check
	glm::ivec2 CheckCoord = GetValidCoord(CheckPosition);
	_Tile &TileCheck = Tiles[CheckCoord.x][CheckCoord.y];

	// Ignore tiles with no hierarchy
	if(TileCheck.Hierarchy == -1)
		return;

	// Check hierarchy
	if(Tile.Hierarchy >= TileCheck.Hierarchy)
		return;

	// Create pair
	if(Pairs.find(TileCheck.BaseTextureIndex) == Pairs.end()) {
		Pairs[TileCheck.BaseTextureIndex].first = TileCheck.Hierarchy;
		Pairs[TileCheck.BaseTextureIndex].second = 0;
	}

	// Unset corner bits
	Pairs[TileCheck.BaseTextureIndex].second |= Bit;
	switch(Bit) {
		// Top
		case 2:
			Pairs[TileCheck.BaseTextureIndex].second &= ~5U;
		break;
		// Left
		case 8:
			Pairs[TileCheck.BaseTextureIndex].second &= ~33U;
		break;
		// Right
		case 16:
			Pairs[TileCheck.BaseTextureIndex].second &= ~132U;
		break;
		// Bottom
		case 64:
			Pairs[TileCheck.BaseTextureIndex].second &= ~160U;
		break;
	}
}

// Renders the map and all objects
void _Map::Render(ae::_Camera *Camera, ae::_Framebuffer *Framebuffer, _Object *ClientPlayer, double BlendFactor, int RenderFlags) {

	// Set lights for editor
	if(RenderFlags & MAP_RENDER_EDITOR_AMBIENT) {
		Framebuffer->Use();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glm::vec4 AmbientLightEditor(1.0f);
		ae::Assets.Programs["map"]->AmbientLight = AmbientLightEditor;
		ae::Assets.Programs["map_object"]->AmbientLight = AmbientLightEditor;
		ae::Assets.Programs["pos_uv_static"]->AmbientLight = AmbientLightEditor;
	}
	else {

		// Setup day night cycle
		SetAmbientLightByClock();

		// Setup lights
		ae::Assets.Programs["map"]->AmbientLight = AmbientLight;
		ae::Assets.Programs["map_object"]->AmbientLight = AmbientLight;

		// Add lights
		LightCount = 0;
		Framebuffer->Use();
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		AddLights(&Objects, ae::Assets.Programs["pos_uv"], Camera->GetAABB());
		AddLights(&StaticObjects, ae::Assets.Programs["pos_uv"], Camera->GetAABB());
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// Draw background map
	/*
	if(BackgroundMap) {
		BackgroundMap->Clock = Clock;
		BackgroundMap->SetAmbientLightByClock();
		ae::Assets.Programs["pos_uv_static"]->AmbientLight = BackgroundMap->AmbientLight;
		if(RenderFlags & MAP_RENDER_EDITOR_AMBIENT)
			ae::Assets.Programs["pos_uv_static"]->AmbientLight = glm::vec4(1.0f);

		// Get camera position
		glm::vec3 DrawPosition;
		Camera->GetDrawPosition(BlendFactor, DrawPosition);
		DrawPosition -= BackgroundOffset;

		float Width = DrawPosition.z * ae::Graphics.AspectRatio;
		float Height = DrawPosition.z;

		// Get render bounds of background tiles
		glm::vec4 BackgroundBounds;
		BackgroundBounds[0] = glm::clamp(-Width + DrawPosition.x, 0.0f, (float)BackgroundMap->Size.x);
		BackgroundBounds[1] = glm::clamp(-Height + DrawPosition.y, 0.0f, (float)BackgroundMap->Size.y);
		BackgroundBounds[2] = glm::clamp(Width + DrawPosition.x, 0.0f, (float)BackgroundMap->Size.x);
		BackgroundBounds[3] = glm::clamp(Height + DrawPosition.y, 0.0f, (float)BackgroundMap->Size.y);

		BackgroundMap->RenderLayer("pos_uv_static", BackgroundBounds, BackgroundOffset, 0, true);
		BackgroundMap->RenderLayer("pos_uv_static", BackgroundBounds, BackgroundOffset, 1, true);
	}
*/
	// Get render bounds
	glm::vec4 Bounds = Camera->GetAABB();
	Bounds[0] = glm::clamp(Bounds[0], 0.0f, (float)Size.x);
	Bounds[1] = glm::clamp(Bounds[1], 0.0f, (float)Size.y);
	Bounds[2] = glm::clamp(Bounds[2], 0.0f, (float)Size.x);
	Bounds[3] = glm::clamp(Bounds[3], 0.0f, (float)Size.y);

	// Set textures
	ae::Assets.Programs["map"]->Use();
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D_ARRAY, ae::Assets.TextureArrays["trans"]->ID);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D_ARRAY, MapTextureID);
	if(Framebuffer) {
		glActiveTexture(GL_TEXTURE1);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, Framebuffer->TextureID);
	}
	glActiveTexture(GL_TEXTURE0);
	ae::Graphics.DirtyState();

	// Draw layers
	RenderTiles(ae::Assets.Programs["map"], Bounds, glm::vec3(0.0f), false);

	// Set program for objects
	ae::Graphics.SetProgram(ae::Assets.Programs["map_object"]);
	glUniformMatrix4fv(ae::Assets.Programs["map_object"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	glUniformMatrix4fv(ae::Assets.Programs["map_object"]->TextureTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));
	glUniformMatrix4fv(ae::Assets.Programs["map_object"]->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));

	// Render floor props
	PropCount = 0;
	RenderProps(ae::Assets.Programs["map_object"], Bounds);

	// Render objects
	for(const auto &Object : Objects)
		Object->Render(ClientPlayer);

	// Check for flags
	if(!RenderFlags)
		return;

	// Draw map boundaries
	if(RenderFlags & MAP_RENDER_BOUNDARY) {
		ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
		ae::Graphics.SetColor(ae::Assets.Colors["red"]);
		ae::Graphics.DrawRectangle3D(glm::vec2(0), glm::vec2(Size), false);
	}

	// Draw zone overlays
	if(RenderFlags & MAP_RENDER_ZONE) {
		ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
		for(int j = (int)Bounds[1]; j < Bounds[3]; j++) {
			for(int i = (int)Bounds[0]; i < Bounds[2]; i++) {
				_Tile *Tile = &Tiles[i][j];

				// Draw zone color
				if(!Tile->Wall && !Tile->ZoneID.empty()) {
					uint16_t ZoneIndex = Stats->Zones.at(Tile->ZoneID).NetworkID % MAP_ZONE_COLORS;
					ae::_Style *Style = ae::Assets.Styles["style_editor_zone" + std::to_string(ZoneIndex)];
					ae::Graphics.SetColor(Style->BackgroundColor);
					ae::Graphics.DrawRectangle(glm::vec2(i, j), glm::vec2(i+1, j+1), true);
				}
			}
		}
	}

	// Draw text overlay
	for(int j = (int)Bounds[1]; j < Bounds[3]; j++) {
		for(int i = (int)Bounds[0]; i < Bounds[2]; i++) {
			_Tile *Tile = &Tiles[i][j];
			glm::vec3 DrawPosition = glm::vec3(i, j, 0) + glm::vec3(0.5f, 0.5f, 0);

			// Draw wall
			if(Tile->Wall) {
				if(RenderFlags & MAP_RENDER_WALL)
					ae::Assets.Fonts["hud_medium"]->DrawText("W", glm::vec2(DrawPosition), ae::CENTER_MIDDLE, glm::vec4(1.0f), 1.0f / 64.0f);
			}
			else {

				// Draw zone number
				if((RenderFlags & MAP_RENDER_ZONE) && !Tile->ZoneID.empty())
					ae::Assets.Fonts["hud_medium"]->DrawText(Tile->ZoneID, glm::vec2(DrawPosition), ae::CENTER_MIDDLE, glm::vec4(1.0f), 1.0f / 64.0f);

				// Draw PVP
				if((RenderFlags & MAP_RENDER_PVP) && Tile->PVP)
					ae::Assets.Fonts["hud_medium"]->DrawText("PVP", glm::vec2(DrawPosition), ae::CENTER_MIDDLE, ae::Assets.Colors["red"], 1.0f / 64.0f);
			}

			// Draw event info
			if(Tile->Event.Type > EventType::NONE) {
				ae::Assets.Fonts["hud_medium"]->DrawText(Stats->EventTypes.at(Tile->Event.Type).second, glm::vec2(DrawPosition) + glm::vec2(0, -0.1), ae::CENTER_MIDDLE, ae::Assets.Colors["cyan"], 1.0f / 128.0f);
				ae::Assets.Fonts["hud_medium"]->DrawText(Tile->Event.Data, glm::vec2(DrawPosition) + glm::vec2(0, 0.1), ae::CENTER_MIDDLE, ae::Assets.Colors["cyan"], 1.0f / 128.0f);
			}
		}
	}
}

// Render map
void _Map::RenderTiles(ae::_Program *Program, glm::vec4 &Bounds, const glm::vec3 &Offset, bool Static) {

	// Set shader parameters
	ae::Graphics.SetProgram(Program);
	ae::Graphics.SetColor(glm::vec4(1.0f));
	ae::Graphics.SetTextureID(ae::Assets.TextureArrays["default"]->ID, GL_TEXTURE_2D_ARRAY);
	Program->SetUniformVec2("tile_count", glm::vec2((int)Bounds[2] - (int)Bounds[0] + 1, (int)Bounds[3] - (int)Bounds[1] + 1));
	Program->SetUniformVec2("tile_offset", glm::vec2((int)Bounds[0], (int)Bounds[1]));
	Program->SetUniformFloat("texture_scale", MAP_TILE_WIDTH / (float)MAP_TILE_PADDED_WIDTH);
	Program->SetUniformFloat("texture_offset", 1.0f / MAP_TILE_PADDED_WIDTH);

	// Draw map quad
	glBindBuffer(GL_ARRAY_BUFFER, MapVertexBufferID);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (GLvoid *)(sizeof(float) * 8));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Reset internal state
	ae::Graphics.DirtyState();
}

// Render map props
void _Map::RenderProps(const ae::_Program *Program, glm::vec4 &Bounds) {
	ae::Graphics.SetProgram(Program);

	// Iterate over objects
	for(const auto &Object : StaticObjects) {
		if(!Object->Prop)
			continue;

		// Check to see if object is in frustum
		if(!Object->CheckAABB(Bounds))
			continue;

		// Get size
		glm::vec2 Scale;
		if(Object->Shape.IsAABB())
			Scale = Object->Shape.HalfSize * 2.0f;
		else
			Scale = glm::vec2(Object->Shape.HalfSize.x * 2.0f);

		// Draw object
		ae::Graphics.SetColor(Object->Prop->Color);
		ae::Graphics.DrawSprite(glm::vec3(Object->Position, 0), Object->Prop->Texture, 0.0f, Scale);

		PropCount++;
	}
}

// Add lights from objects
void _Map::AddLights(const std::list<_Object *> *ObjectList, const ae::_Program *Program, glm::vec4 AABB) {
	ae::Graphics.SetProgram(Program);
	glUniformMatrix4fv(Program->TextureTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

	// Iterate over objects
	for(const auto &Object : *ObjectList) {
		if(!Object->Light)
			continue;

		if(Object->Shape.HalfSize.x == 0.0f)
			continue;

		_Light *Light = Object->Light;
		if(!Light->Texture)
			continue;

		// Check to see if light is in frustum
		if(!Object->CheckAABB(AABB))
			continue;

		// Get size
		glm::vec2 Scale;
		if(Object->Shape.IsAABB())
			Scale = Object->Shape.HalfSize * 2.0f;
		else
			Scale = glm::vec2(Object->Shape.HalfSize.x * 2.0f);

		// Draw light
		ae::Graphics.SetColor(Light->FinalColor);
		ae::Graphics.DrawSprite(glm::vec3(Object->Position, 0), Light->Texture, 0.0f, Scale);

		LightCount++;
	}
}

// Load map
void _Map::Load(const std::string &Path, bool Static) {

	// Load file
	gzifstream File(Path.c_str());
	if(!File)
		throw std::runtime_error("Cannot load map: " + Path);

	// Get base map name
	size_t PrefixPosition = Path.find(MAPS_PATH);
	if(PrefixPosition != std::string::npos)
		Name = Path.substr(PrefixPosition + MAPS_PATH.length(), Path.find(".map.gz") - MAPS_PATH.length());

	// Load background map
	/*
	if(UseAtlas && MapStat->BackgroundMapID) {
		BackgroundOffset = MapStat->BackgroundOffset;

		BackgroundMap = new _Map();
		BackgroundMap->UseAtlas = true;
		try {
			BackgroundMap->Load(&Stats->Maps.at(MapStat->BackgroundMapID), true);
		}
		catch(std::exception &Error) {
			delete BackgroundMap;
			BackgroundMap = nullptr;
		}
	}*/

	// Load tiles
	_Tile *Tile = nullptr;
	_Object *Object = nullptr;
	int TileIndex = 0;
	while(!File.eof() && File.peek() != EOF) {

		// Read chunk type
		char ChunkType;
		File >> ChunkType;

		// Handle chunks
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
				AllocateMap();
			} break;
			// Begin new tile
			case 'T': {
				glm::ivec2 Coordinate;
				Coordinate.x = TileIndex % Size.x;
				Coordinate.y = TileIndex / Size.x;
				Tile = &Tiles[Coordinate.x][Coordinate.y];
				TileIndex++;
			} break;
			// Texture index
			case 'b': {
				if(Tile) {
					char Buffer[1024];
					File.ignore(1);
					File.getline(Buffer, 1024, '\n');
					if(!Server) {
						const ae::_TileMap::_TileData &TileData = ae::Assets.TileMaps["default"]->Data.at(Buffer);
						Tile->BaseTextureIndex = TileData.Index;
						Tile->Hierarchy = TileData.Hierarchy;
					}
				}
			} break;
			// Zone
			case 'z': {
				if(Tile)
					File >> Tile->ZoneID;
			} break;
			// Event
			case 'e': {
				if(Tile) {
					int Type;
					File >> Type;

					char Buffer[1024];
					File.ignore(1);
					File.getline(Buffer, 1024, '\n');
					Tile->Event.Type = (EventType)Type;
					Tile->Event.Data = Buffer;
				}
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
			// Object
			case 'O': {
				glm::vec2 Position;
				File >> Position.x >> Position.y;
				Object = new _Object();
				Object->Scripting = Scripting;
				Object->Position = Position;
				StaticObjects.push_back(Object);
			} break;
			// Object shape
			case 's': {
				File >> Object->Shape.HalfSize.x >> Object->Shape.HalfSize.y;
			} break;
			// light
			case 'L': {
				if(Object) {
					char SubChunkType;
					File >> SubChunkType;
					switch(SubChunkType) {
						case 't': {
							std::string TextureName;
							File >> TextureName;
							Object->Light->Texture = ae::Assets.Textures[TextureName];
						} break;
						case 's':
							File >> Object->Light->Script;
						break;
						case 'c':
							File >> Object->Light->Color.r >> Object->Light->Color.g >> Object->Light->Color.b >> Object->Light->Color.a;
						break;
					}
				}
			} break;
			// Prop
			case 'P': {
				if(Object) {
					if(!Object->Prop)
						Object->Prop = new _Prop(Object);

					char SubChunkType;
					File >> SubChunkType;
					switch(SubChunkType) {
						case 't': {
							std::string TextureName;
							File >> TextureName;
							Object->Prop->Texture = ae::Assets.Textures[TextureName];
						} break;
						case 'c':
							File >> Object->Prop->Color.r >> Object->Prop->Color.g >> Object->Prop->Color.b >> Object->Prop->Color.a;
						break;
					}
				}
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
	if(!Headless) {
		InitVertices(Static);
		BuildLayers(glm::ivec4(0, 0, Size.x, Size.y));
	}

	// Initialize path finding
	Pather = new micropather::MicroPather(this, (unsigned)(Size.x * Size.y), 4);
	Loaded = true;
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
	Output << "S " << Size.x << ' ' << Size.y << '\n';

	// Write tile map
	for(int j = 0; j < Size.y; j++) {
		for(int i = 0; i < Size.x; i++) {
			const _Tile &Tile = Tiles[i][j];
			Output << "T" << '\n';
			if(Tile.BaseTextureIndex)
				Output << "b " << ae::Assets.TileMaps["default"]->Index.at(Tile.BaseTextureIndex)->ID << '\n';
			if(!Tile.ZoneID.empty())
				Output << "z " << Tile.ZoneID << '\n';
			if(Tile.Event.Type != EventType::NONE)
				Output << "e " << (int)Tile.Event.Type << ' ' << Tiles[i][j].Event.Data << '\n';
			if(Tile.Wall)
				Output << "w " << Tile.Wall << '\n';
			if(Tile.PVP)
				Output << "p " << Tile.PVP << '\n';
		}
	}

	// Write static objects
	for(auto &Object : StaticObjects) {
		Output << "O " << Object->Position.x << ' ' << Object->Position.y << '\n';
		Output << "s " << Object->Shape.HalfSize.x << ' ' << Object->Shape.HalfSize.y << '\n';

		if(Object->Light && Object->Light->Texture) {
			Output << "Lt " << Object->Light->Texture->Name << '\n';
			Output << "Ls " << Object->Light->Script << '\n';
			Output << "Lc " << Object->Light->Color.r << ' ' << Object->Light->Color.g << ' ' << Object->Light->Color.b << ' ' << Object->Light->Color.a << '\n';
		}
		if(Object->Prop && Object->Prop->Texture) {
			Output << "Pt " << Object->Prop->Texture->Name << '\n';
			Output << "Pc " << Object->Prop->Color.r << ' ' << Object->Prop->Color.g << ' ' << Object->Prop->Color.b << ' ' << Object->Prop->Color.a << '\n';
		}
	}

	Output.close();

	return true;
}

// Determines if a square can be moved to
bool _Map::CanMoveTo(const glm::vec2 &Position, _Object *Object) {

	// Bounds
	if(Position.x < 0 || Position.x >= Size.x || Position.y < 0 || Position.y >= Size.y)
		return false;

	const _Tile *Tile = &Tiles[(int)Position.x][(int)Position.y];
	if(Tile->Event.Type == EventType::KEY) {
		if(Object->Inventory->HasItem(Tile->Event.Data))
			return true;

		// Set message for client
		if(!Server) {
			const _BaseItem *Item = &Object->Stats->Items.at(Tile->Event.Data);
			if(Item && Object->Character->HUD)
				Object->Character->HUD->SetMessage("You need a " + Item->Name);
		}

		return false;
	}

	return !Tile->Wall;
}

// Removes an object from the map
void _Map::RemoveObject(const _Object *RemoveObject) {

	// Notify peers
	if(Server) {

		// Create packet
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_DELETEOBJECT);
		Packet.Write<ae::NetworkIDType>(RemoveObject->NetworkID);

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
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_CREATEOBJECT);
		Object->SerializeCreate(Packet);

		// Notify other players of the new object
		BroadcastPacket(Packet);
	}

	// Add object to map
	Objects.push_back(Object);
}

// Returns a list of players close to a player that can battle
void _Map::GetPotentialBattlePlayers(const _Object *Player, float DistanceSquared, size_t Max, std::list<_Object *> &Players) {

	for(const auto &Object : Objects) {
		if(!Object->Character)
			continue;

		if(Object == Player)
			continue;

		glm::vec2 Delta = Object->Position - Player->Position;
		if(glm::dot(Delta, Delta) <= DistanceSquared && Object->Character->CanBattle() && Player->Character->PartyName == Object->Character->PartyName) {
			Players.push_back(Object);
			if(Players.size() >= Max)
				return;
		}
	}
}

// Returns a battle instance close to a player
_Battle *_Map::GetCloseBattle(const _Object *Player, bool &HitPrivateParty) {
	for(const auto &Object : Objects) {
		if(!Object->Character)
			continue;

		if(Object == Player)
			continue;

		if(Object->GetTilePosition() == Player->GetTilePosition() && Object->Character->IsAlive() && Object->Character->Battle && !Object->Character->Battle->PVP && Object->Character->Battle->SideCount[0] < BATTLE_MAX_OBJECTS_PER_SIDE) {
			if(Object->Character->PartyName == "" || Object->Character->PartyName == Player->Character->PartyName)
				return Object->Character->Battle;
			else
				HitPrivateParty = true;
		}
	}

	return nullptr;
}

// Returns target players appropriate for pvp
void _Map::GetPVPPlayers(const _Object *Player, std::list<_Object *> &Players, bool UsePVPZone) {

	// Attacker must be in PVP zone
	if(UsePVPZone && !IsPVPZone(Player->Position))
		return;

	for(const auto &Object : Objects) {

		// Skip self target
		if(Object == Player)
			continue;

		// Check for character
		if(!Object->Character)
			continue;

		// Check if target can PVP
		if(!Object->Character->CanPVP())
			continue;

		// Target must be in PVP zone
		if(UsePVPZone && !IsPVPZone(Object->Position))
			continue;

		// Can only bounty hunt players with a bounty
		if(!UsePVPZone && !Object->Character->Bounty)
			continue;

		// Can't attack same party member
		if(Object->Character->PartyName != "" && Object->Character->PartyName == Player->Character->PartyName)
			continue;

		// Check distance
		glm::vec2 Delta = Object->Position - Player->Position;
		if(glm::dot(Delta, Delta) <= BATTLE_PVP_DISTANCE)
			Players.push_back(Object);
	}
}

// Returns the closest player
_Object *_Map::FindTradePlayer(const _Object *Player, float MaxDistanceSquared) {

	_Object *ClosestPlayer = nullptr;
	float ClosestDistanceSquared = std::numeric_limits<float>::infinity();
	for(const auto &Object : Objects) {
		if(!Object->Character)
			continue;

		if(Object != Player && Object->Character->WaitingForTrade && Object->Character->TradePlayer == nullptr) {
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

// Find closest event on the map, returns true on found
bool _Map::FindEvent(const _Event &Event, glm::vec2 &Position) const {

	// Find event
	auto Iterator = IndexedEvents.find(Event);
	if(Iterator == IndexedEvents.end())
		return false;

	// Return closest position
	glm::vec2 StartPosition = Position;
	float ClosestDistanceSquared = std::numeric_limits<float>::infinity();
	for(const auto &CheckPosition : Iterator->second) {
		glm::vec2 Delta = StartPosition - CheckPosition;
		float DistanceSquared = glm::dot(Delta, Delta);
		if(DistanceSquared < ClosestDistanceSquared) {
			ClosestDistanceSquared = DistanceSquared;
			Position = CheckPosition;
		}
	}

	return true;
}

// Send complete object list to player
void _Map::SendObjectList(ae::_Peer *Peer) {
	if(!Server)
		return;

	if(!Peer->Object)
		return;

	// Create packet
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_OBJECTLIST);
	Packet.Write<ae::NetworkIDType>(Peer->Object->NetworkID);

	// Write object data
	Packet.Write<ae::NetworkIDType>((ae::NetworkIDType)Objects.size());
	for(auto &Object : Objects) {
		Object->SerializeCreate(Packet);
	}

	Server->Network->SendPacket(Packet, Peer);
}

// Sends object position information to all the clients in the map
void _Map::SendObjectUpdates() {

	// Create packet
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_OBJECTUPDATES);
	Packet.Write<uint8_t>((uint8_t)NetworkID);

	// Write object count
	Packet.Write<ae::NetworkIDType>((ae::NetworkIDType)Objects.size());

	// Iterate over objects
	for(const auto &Object : Objects) {
		Object->SerializeUpdate(Packet);
	}

	// Send packet to players in map
	BroadcastPacket(Packet, ae::_Network::UNSEQUENCED);
}

// Broadcast a packet to all peers in the map
void _Map::BroadcastPacket(ae::_Buffer &Buffer, ae::_Network::SendType Type) {
	if(!Server)
		return;

	// Send packet to peers
	for(auto &Object : Objects) {
		if(!Object->Deleted && Object->Peer && Object->Peer->ENetPeer)
			Server->Network->SendPacket(Buffer, Object->Peer, Type, Type == ae::_Network::UNSEQUENCED);
	}
}

// Get a valid position within the grid
glm::vec2 _Map::GetValidPosition(const glm::vec2 &Position) const {
	return glm::clamp(Position, glm::vec2(0.0f), glm::vec2(Size));
}

// Get a valid position within the grid
glm::ivec2 _Map::GetValidCoord(const glm::ivec2 &Position) const {
	return glm::clamp(Position, glm::ivec2(0), Size - 1);
}

// Distance between two points
float _Map::LeastCostEstimate(void *StateStart, void *StateEnd) {
	glm::ivec2 StartPosition;
	NodeToPosition(StateStart, StartPosition);

	glm::ivec2 EndPosition;
	NodeToPosition(StateStart, EndPosition);

	return std::abs(StartPosition.x - EndPosition.x) + std::abs(StartPosition.y - EndPosition.y);
}

// Generate successors from a state
void _Map::AdjacentCost(void *State, std::vector<micropather::StateCost> *Neighbors) {
	glm::ivec2 Position;
	NodeToPosition(State, Position);

	glm::ivec2 Directions[4] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
	for(int i = 0; i < 4; i++) {
		glm::ivec2 NewPosition = Position + Directions[i];
		float Cost = Tiles[NewPosition.x][NewPosition.y].Wall ? FLT_MAX : 1.0f;
		micropather::StateCost NodeCost = { PositionToNode(NewPosition), Cost };
		Neighbors->push_back(NodeCost);
	}
}
