/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020 Alan Witkowski
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
#include <objects/object.h>
#include <objects/battle.h>
#include <hud/hud.h>
#include <ae/buffer.h>
#include <ae/texture.h>
#include <ae/assets.h>
#include <ae/atlas.h>
#include <ae/font.h>
#include <ae/framebuffer.h>
#include <ae/program.h>
#include <ae/camera.h>
#include <ae/servernetwork.h>
#include <ae/peer.h>
#include <ae/graphics.h>
#include <ae/random.h>
#include <ae/light.h>
#include <states/editor.h>
#include <server.h>
#include <scripting.h>
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

// Color overlays for zones
const glm::vec4 ZoneColors[] = {
	{ 1.0f, 0.0f, 0.0f, 0.4f },
	{ 0.0f, 1.0f, 0.0f, 0.4f },
	{ 0.0f, 0.0f, 1.0f, 0.4f },
	{ 1.0f, 1.0f, 0.0f, 0.4f },
	{ 0.0f, 1.0f, 1.0f, 0.4f },
	{ 1.0f, 0.0f, 1.0f, 0.4f },
};

// Colors of each time cycle
const std::vector<glm::vec4> DayCycles = {
	{ 0.0625f, 0.0625f, 0.375f,  1 },
	{ 0.125f,  0.125f,  0.125f,  1 },
	{ 0.75f,   0.75f,   0.5625f, 1 },
	{ 0.6875f, 0.5625f, 0.375f,  1 },
	{ 0.625f,  0.5f,    0.375f,  1 },
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
	Tiles(nullptr),
	Size(0, 0),
	TileAtlas(nullptr),
	AmbientLight(MAP_AMBIENT_LIGHT),
	AmbientBackground(1.0f),
	OutsideFlag(1),
	Clock(0),
	Headless(false),
	BackgroundOffset(0.0f),
	BackgroundMap(nullptr),
	ObjectUpdateTime(0),
	UpdateID(0),
	Stats(nullptr),
	Server(nullptr),
	MaxZoneColors(sizeof(ZoneColors) / sizeof(glm::vec4)),
	CurrentZoneColors(MaxZoneColors),
	Pather(nullptr),
	TileVertexBufferID{0, 0},
	TileElementBufferID(0),
	TileVertices{nullptr, nullptr},
	TileFaces(nullptr) {
}

// Destructor
_Map::~_Map() {

	// Delete path finding
	delete Pather;

	// Delete background layer
	delete BackgroundMap;

	// Delete atlas
	if(!Headless)
		CloseAtlas();

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

	// Save old texture atlas name
	std::string OldTextureAtlas = "";
	if(TileAtlas)
		OldTextureAtlas = TileAtlas->Texture->Name;

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

	// Update static objects
	for(auto &Object : StaticObjects)
		Object->Position -= Offset;
}

// Initialize the texture atlas
void _Map::InitAtlas(const std::string AtlasPath, bool Static) {
	const ae::_Texture *AtlasTexture = ae::Assets.Textures[AtlasPath];
	if(!AtlasTexture)
		throw std::runtime_error("Can't find atlas: " + AtlasPath);

	TileAtlas = new ae::_Atlas(AtlasTexture, glm::ivec2(MAP_TILE_WIDTH, MAP_TILE_HEIGHT), 1);

	GLuint TileVertexCount = (GLuint)(4 * Size.x * Size.y);
	GLuint TileFaceCount = (GLuint)(2 * Size.x * Size.y);

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

	// Create a static vbo
	if(Static) {
		uint32_t VertexIndex = 0;
		TileVertices[0] = new glm::vec4[TileVertexCount];
		TileVertices[1] = new glm::vec4[TileVertexCount];

		for(int k = 0; k < 2; k++) {
			VertexIndex = 0;
			for(int j = 0; j < Size.y; j++) {
				for(int i = 0; i < Size.x; i++) {
					glm::vec4 TextureCoords = TileAtlas->GetTextureCoords(Tiles[i][j].TextureIndex[k]);
					TileVertices[k][VertexIndex++] = { i + 0.0f, j + 0.0f, TextureCoords[0], TextureCoords[1] };
					TileVertices[k][VertexIndex++] = { i + 1.0f, j + 0.0f, TextureCoords[2], TextureCoords[1] };
					TileVertices[k][VertexIndex++] = { i + 0.0f, j + 1.0f, TextureCoords[0], TextureCoords[3] };
					TileVertices[k][VertexIndex++] = { i + 1.0f, j + 1.0f, TextureCoords[2], TextureCoords[3] };
				}
			}

			glGenBuffers(1, &TileVertexBufferID[k]);
			glBindBuffer(GL_ARRAY_BUFFER, TileVertexBufferID[k]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * TileVertexCount, TileVertices[k], GL_STATIC_DRAW);
		}

		glGenBuffers(1, &TileElementBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TileElementBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glm::u32vec3) * TileFaceCount, TileFaces, GL_STATIC_DRAW);
	}
	else {
		TileVertices[0] = new glm::vec4[TileVertexCount];

		glGenBuffers(1, &TileVertexBufferID[0]);
		glBindBuffer(GL_ARRAY_BUFFER, TileVertexBufferID[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * TileVertexCount, nullptr, GL_DYNAMIC_DRAW);

		glGenBuffers(1, &TileElementBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TileElementBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glm::u32vec3) * TileFaceCount, nullptr, GL_DYNAMIC_DRAW);
	}
}

// Free memory used by texture atlas
void _Map::CloseAtlas() {
	delete TileAtlas;
	delete[] TileVertices[0];
	delete[] TileVertices[1];
	delete[] TileFaces;
	glDeleteBuffers(1, &TileVertexBufferID[0]);
	glDeleteBuffers(1, &TileVertexBufferID[1]);
	glDeleteBuffers(1, &TileElementBufferID);

	TileVertexBufferID[0] = 0;
	TileVertexBufferID[1] = 0;
	TileElementBufferID = 0;
	TileAtlas = nullptr;
	TileVertices[0] = nullptr;
	TileVertices[1] = nullptr;
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
void _Map::CheckEvents(_Object *Object, _Scripting *Scripting) const {

	// Check for teleporting
	if(Server && Object->Character->TeleportTime == 0.0) {
		Object->Character->TeleportTime = -1.0;
		Server->SpawnPlayer(Object, Object->Character->SpawnMapID, _Map::EVENT_SPAWN);
		return;
	}

	// Handle events
	const _Tile *Tile = &Tiles[Object->Position.x][Object->Position.y];
	switch(Tile->Event.Type) {
		case _Map::EVENT_SPAWN:
			if(Server && !(Object->Character->SpawnMapID == NetworkID && Object->Character->SpawnPoint == Tile->Event.Data))
				Server->SendMessage(Object->Peer, "Spawn point set", "yellow");

			Object->Character->SpawnMapID = NetworkID;
			Object->Character->SpawnPoint = Tile->Event.Data;
		break;
		case _Map::EVENT_MAPCHANGE:
			if(Server)
				Server->SpawnPlayer(Object, (ae::NetworkIDType)Tile->Event.Data, _Map::EVENT_MAPENTRANCE);
			else
				Object->Controller->WaitForServer = true;
		break;
		case _Map::EVENT_VENDOR:
		case _Map::EVENT_TRADER:
		case _Map::EVENT_BLACKSMITH:
		case _Map::EVENT_ENCHANTER:
		case _Map::EVENT_MINIGAME: {
			if(Server)
				StartEvent(Object, Tile->Event);
			else
				Object->Controller->WaitForServer = true;
		} break;
		case _Map::EVENT_SCRIPT: {
			if(Server) {
				Server->RunEventScript(Tile->Event.Data, Object);
				CheckBattle(Object, Tile);
			}
			else {

				// Find script
				auto Iterator = Stats->Scripts.find(Tile->Event.Data);
				if(Iterator != Stats->Scripts.end()) {
					const _Script &Script = Iterator->second;
					if(Scripting->StartMethodCall(Script.Name, "PlaySound")) {
						Scripting->PushObject(Object);
						Scripting->MethodCall(1, 0);
						Scripting->FinishMethodCall();
					}
				}
			}
		} break;
		case _Map::EVENT_PORTAL: {
			if(Server) {

				// Find matching even/odd event
				FindEvent(_Event(Tile->Event.Type, Tile->Event.Data ^ 1), Object->Position);
				Server->SendPlayerPosition(Object->Peer);
			}
			else
				Object->Controller->WaitForServer = true;
		} break;
		case _Map::EVENT_JUMP: {
			if(Server) {

				// Find next jump
				FindEvent(_Event(Tile->Event.Type, Tile->Event.Data + 1), Object->Position);
				Server->SendPlayerPosition(Object->Peer);
			}
			else
				Object->Controller->WaitForServer = true;
		} break;
		default:
			if(Server) {
				Object->Character->Vendor = nullptr;
				Object->Character->Trader = nullptr;
				CheckBattle(Object, Tile);
			}
		break;
	}

	if(Server && Object->QueuedMapChange) {
		Server->SpawnPlayer(Object, Object->QueuedMapChange, _Map::EVENT_MAPENTRANCE);
		Object->QueuedMapChange = 0;
	}
}

// Check for next battle
void _Map::CheckBattle(_Object *Object, const _Tile *Tile) const {
	if(!Server)
		return;

	if(Object->Character->NextBattle <= 0)
		Server->QueueBattle(Object, Tile->Zone, false, false, 0.0f, 0.0f);
}

// Build indexed events list
void _Map::IndexEvents() {
	IndexedEvents.clear();

	// Build event index
	for(int j = 0; j < Size.y; j++) {
		for(int i = 0; i < Size.x; i++) {
			const _Tile &Tile = Tiles[i][j];
			if(Tile.Event.Type != EVENT_NONE) {
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
	if(!OutsideFlag)
		return;

	// Find index by time
	std::size_t NextCycle = DayCyclesTime.size();
	for(std::size_t i = 0; i < DayCyclesTime.size(); i++) {
		if(Clock < DayCyclesTime[i]) {
			NextCycle = i;
			break;
		}
	}

	// Get indices for current and next cycle
	std::size_t CurrentCycle = NextCycle - 1;
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
	glm::vec4 OutdoorLight = glm::mix(DayCycles[CurrentCycle], DayCycles[NextCycle], Percent);
	if(OutsideFlag == 2)
		AmbientLight = LightFilter * OutdoorLight;
	else
		AmbientLight = OutdoorLight;
}

// Start event for an object and send packet
void _Map::StartEvent(_Object *Object, _Event Event) const {
	if(!Server)
		return;

	// Handle event types
	try {
		switch(Event.Type) {
			case _Map::EVENT_TRADER:
				Object->Character->Trader = &Server->Stats->Traders.at(Event.Data);
				if(!Object->Character->Trader->ID)
					return;
			break;
			case _Map::EVENT_VENDOR:
				Object->Character->Vendor = &Server->Stats->Vendors.at(Event.Data);
				if(!Object->Character->Vendor->ID)
					return;
			break;
			case _Map::EVENT_BLACKSMITH:
				Object->Character->Blacksmith = &Server->Stats->Blacksmiths.at(Event.Data);
				if(!Object->Character->Blacksmith->ID)
					return;
			break;
			case _Map::EVENT_ENCHANTER:
				Object->Character->Enchanter = &Server->Stats->Enchanters.at(Event.Data);
				if(!Object->Character->Enchanter->ID)
					return;
			break;
			case _Map::EVENT_MINIGAME: {
				Object->Character->Minigame = &Server->Stats->Minigames.at(Event.Data);
				if(!Object->Character->Minigame->ID)
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
		Packet.Write<uint32_t>(Event.Type);
		Packet.Write<uint32_t>(Event.Data);
		Packet.Write<glm::ivec2>(Object->Position);
		Server->Network->SendPacket(Packet, Object->Peer);
	}

	// Generate seed
	if(Event.Type == _Map::EVENT_MINIGAME) {
		Object->SendSeed(false);
	}
}

// Determine if a position is in a pvp zone
bool _Map::IsPVPZone(const glm::ivec2 &Position) const {
	if(!IsValidPosition(Position))
		return false;

	return GetTile(Position)->PVP;
}

// Renders the map
void _Map::Render(ae::_Camera *Camera, ae::_Framebuffer *Framebuffer, _Object *ClientPlayer, double BlendFactor, int RenderFlags) {

	// Set lights for editor
	if(RenderFlags & MAP_RENDER_EDITOR_AMBIENT) {
		Framebuffer->Use();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glm::vec4 AmbientLightEditor(1.0f);
		ae::Assets.Programs["map"]->AmbientLight = AmbientLightEditor;
		ae::Assets.Programs["pos_uv_static"]->AmbientLight = AmbientLightEditor;
	}
	else {

		// Setup day night cycle
		SetAmbientLightByClock();

		// Setup lights
		ae::Assets.Programs["map"]->AmbientLight = AmbientLight;

		// Add lights
		int LightCount = 0;
		Framebuffer->Use();
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		LightCount += AddLights(ClientPlayer, &Objects, ae::Assets.Programs["pos_uv"], Camera->GetAABB());
		LightCount += AddLights(nullptr, &StaticObjects, ae::Assets.Programs["pos_uv"], Camera->GetAABB());
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// Draw background map
	if(!(RenderFlags & MAP_RENDER_NOBACKGROUND) && BackgroundMap) {
		BackgroundMap->Clock = Clock;
		BackgroundMap->SetAmbientLightByClock();
		ae::Assets.Programs["pos_uv_static"]->AmbientLight = BackgroundMap->AmbientLight * BackgroundMap->AmbientBackground;
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

	// Get render bounds
	glm::vec4 Bounds = Camera->GetAABB();
	Bounds[0] = glm::clamp(Bounds[0], 0.0f, (float)Size.x);
	Bounds[1] = glm::clamp(Bounds[1], 0.0f, (float)Size.y);
	Bounds[2] = glm::clamp(Bounds[2], 0.0f, (float)Size.x);
	Bounds[3] = glm::clamp(Bounds[3], 0.0f, (float)Size.y);

	// Set framebuffer texture
	if(Framebuffer) {
		ae::Assets.Programs["map"]->Use();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Framebuffer->TextureID);
		glActiveTexture(GL_TEXTURE0);
	}

	// Draw layers
	RenderLayer("map", Bounds, glm::vec3(0.0f), 0);
	RenderLayer("map", Bounds, glm::vec3(0.0f), 1);

	// Render static objects
	for(const auto &Object : StaticObjects) {
		if(Object->Light || Object->Character)
			continue;

		Object->Render(Bounds, ClientPlayer);
	}

	// Render objects
	for(const auto &Object : Objects) {
		Object->Render(Bounds, ClientPlayer);
	}

	// Check for flags
	if(!RenderFlags)
		return;

	// Draw map boundaries
	if(RenderFlags & MAP_RENDER_BOUNDARY) {
		ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
		ae::Graphics.SetColor(ae::Assets.Colors["red"]);
		ae::Graphics.DrawRectangle3D(glm::vec2(0), glm::vec2(Size), false);
	}

	// Draw boundary where map edge is almost visible
	if((RenderFlags & MAP_RENDER_EDGE_BOUNDARY) && Size.x > 32 && Size.y > 20) {
		ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
		ae::Graphics.SetColor(ae::Assets.Colors["editor_edge"]);
		ae::Graphics.DrawRectangle3D(glm::vec2(16, 10), glm::vec2(Size.x - 16, Size.y - 10), false);
	}

	// Draw zone overlays
	if(RenderFlags & MAP_RENDER_ZONE) {
		ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
		for(int j = (int)Bounds[1]; j < Bounds[3]; j++) {
			for(int i = (int)Bounds[0]; i < Bounds[2]; i++) {
				_Tile *Tile = &Tiles[i][j];

				// Draw zone color
				if(!Tile->Wall && Tile->Zone > 0) {
					ae::Graphics.SetColor(ZoneColors[Tile->Zone % CurrentZoneColors]);
					ae::Graphics.DrawRectangle(glm::vec2(i, j), glm::vec2(i+1, j+1), true);
				}
			}
		}
	}

	if(RenderFlags & MAP_RENDER_MAP_VIEW)
		return;

	// Draw text overlay
	for(int j = (int)Bounds[1]; j < Bounds[3]; j++) {
		for(int i = (int)Bounds[0]; i < Bounds[2]; i++) {
			_Tile *Tile = &Tiles[i][j];
			glm::vec3 DrawPosition = glm::vec3(i, j, 0) + glm::vec3(0.5f, 0.5f, 0);

			// Draw wall
			if(Tile->Wall) {
				if(RenderFlags & MAP_RENDER_WALL)
					ae::Assets.Fonts["hud_medium"]->DrawText("W", glm::vec2(DrawPosition), ae::CENTER_MIDDLE, glm::vec4(1.0f), 1.0f / (UI_TILE_SIZE.x * ae::_Element::GetUIScale()));
			}
			else {

				// Draw zone number
				if((RenderFlags & MAP_RENDER_ZONE) && Tile->Zone > 0)
					ae::Assets.Fonts["hud_medium"]->DrawText(std::to_string(Tile->Zone), glm::vec2(DrawPosition), ae::CENTER_MIDDLE, glm::vec4(1.0f), 1.0f / (UI_TILE_SIZE.x * ae::_Element::GetUIScale()));

				// Draw PVP
				if((RenderFlags & MAP_RENDER_PVP) && Tile->PVP)
					ae::Assets.Fonts["hud_medium"]->DrawText("PVP", glm::vec2(DrawPosition) - glm::vec2(0, 0.25), ae::CENTER_MIDDLE, ae::Assets.Colors["red"], 1.0f / (UI_TILE_SIZE.x * ae::_Element::GetUIScale()));
			}

			// Draw event info
			if(Tile->Event.Type > 0) {
				std::string EventText = Stats->EventNames[Tile->Event.Type].ShortName + std::string(" ") + std::to_string(Tile->Event.Data);
				ae::Assets.Fonts["hud_medium"]->DrawText(EventText, glm::vec2(DrawPosition) - glm::vec2(0, -0.25), ae::CENTER_MIDDLE, ae::Assets.Colors["cyan"], 1.0f / (UI_TILE_SIZE.x * ae::_Element::GetUIScale()));
			}
		}
	}
}

// Render either floor or foreground texture tiles
void _Map::RenderLayer(const std::string &Program, glm::vec4 &Bounds, const glm::vec3 &Offset, int Layer, bool Static) {
	ae::Graphics.SetProgram(ae::Assets.Programs[Program]);
	ae::Graphics.SetColor(glm::vec4(1.0f));
	glUniformMatrix4fv(ae::Assets.Programs[Program]->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::translate(glm::mat4(1.0f), Offset)));
	glUniformMatrix4fv(ae::Assets.Programs[Program]->TextureTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

	uint32_t VertexIndex = 0;
	int FaceIndex = 0;

	if(!Static) {
		for(int j = (int)Bounds[1]; j < Bounds[3]; j++) {
			for(int i = (int)Bounds[0]; i < Bounds[2]; i++) {
				glm::vec4 TextureCoords = TileAtlas->GetTextureCoords(Tiles[i][j].TextureIndex[Layer]);
				TileVertices[0][VertexIndex++] = { i + 0.0f, j + 0.0f, TextureCoords[0], TextureCoords[1] };
				TileVertices[0][VertexIndex++] = { i + 1.0f, j + 0.0f, TextureCoords[2], TextureCoords[1] };
				TileVertices[0][VertexIndex++] = { i + 0.0f, j + 1.0f, TextureCoords[0], TextureCoords[3] };
				TileVertices[0][VertexIndex++] = { i + 1.0f, j + 1.0f, TextureCoords[2], TextureCoords[3] };

				FaceIndex += 2;
			}
		}
		Layer = 0;
	}
	else {
		VertexIndex = (uint32_t)(Size.x * Size.y * 4);
		FaceIndex = Size.x * Size.y * 2;
	}

	GLsizeiptr VertexBufferSize = VertexIndex * sizeof(glm::vec4);
	GLsizeiptr ElementBufferSize = FaceIndex * (int)sizeof(glm::u32vec3);

	ae::Graphics.SetTextureID(TileAtlas->Texture->ID);
	ae::Graphics.EnableAttribs(2);

	glBindBuffer(GL_ARRAY_BUFFER, TileVertexBufferID[Layer]);
	if(!Static)
		glBufferSubData(GL_ARRAY_BUFFER, 0, VertexBufferSize, TileVertices[Layer]);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void *)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void *)sizeof(glm::vec2));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TileElementBufferID);
	if(!Static)
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, ElementBufferSize, TileFaces);
	glDrawElements(GL_TRIANGLES, FaceIndex * 3, GL_UNSIGNED_INT, 0);

	ae::Graphics.ResetState();
}

// Add lights from objects
int _Map::AddLights(_Object *ClientPlayer, const std::list<_Object *> *ObjectList, const ae::_Program *Program, glm::vec4 AABB) {
	ae::Graphics.SetProgram(Program);
	glUniformMatrix4fv(Program->TextureTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

	// Iterate over objects
	int LightCount = 0;
	for(const auto &Object : *ObjectList) {
		if(!Object->Light)
			continue;

		if(ClientPlayer) {
			if(ClientPlayer->Character->Offline && ClientPlayer != Object)
				continue;

			if(ClientPlayer != Object && Object->Character->Offline)
				continue;
		}

		// Check for valid light
		const auto &Iterator = Stats->Lights.find(Object->Light);
		if(Iterator == Stats->Lights.end())
		   continue;

		const _LightType &LightType = Iterator->second;

		// Check to see if light is in frustum
		glm::vec2 Point(Object->Position.x + 0.5f, Object->Position.y + 0.5f);
		if(Point.x < AABB[0])
			Point.x = AABB[0];
		if(Point.y < AABB[1])
			Point.y = AABB[1];
		if(Point.x > AABB[2])
			Point.x = AABB[2];
		if(Point.y > AABB[3])
			Point.y = AABB[3];

		// Compare distances
		float DistanceSquared = glm::distance2(Point, glm::vec2(Object->Position) + glm::vec2(0.5f));
		if(DistanceSquared >= (LightType.Radius + 0.5f) * (LightType.Radius + 0.5f))
			continue;

		// Draw light
		glm::vec3 Position = glm::vec3(Object->Position, 0) + glm::vec3(0.5f, 0.5f, 0);
		glm::vec4 Color = glm::vec4(LightType.Color, 1);
		glm::vec2 Scale(LightType.Radius * 2.0f);
		ae::Graphics.SetColor(Color);
		ae::Graphics.DrawSprite(Position, ae::Assets.Textures["textures/lights/light0.png"], 0.0f, Scale);

		LightCount++;
	}

	return LightCount;
}

// Load map
void _Map::Load(const _MapStat *MapStat, bool Static) {

	// Load file
	gzifstream File(MapStat->File.c_str());
	if(!File)
		throw std::runtime_error("Cannot load map: " + MapStat->File);

	// Save map stats
	if(Server)
		Headless = true;
	AmbientLight = LightFilter = MapStat->AmbientLight;
	AmbientBackground = MapStat->AmbientBackground;
	Music = MapStat->Music;

	// 0: static ambient light 1: use daytime cycle 2: multiply ambient with daytime cycle
	OutsideFlag = MapStat->Outside;

	// Load background map
	if(!Headless && MapStat->BackgroundMapID) {
		BackgroundOffset = MapStat->BackgroundOffset;

		BackgroundMap = new _Map();
		try {
			BackgroundMap->Load(&Stats->Maps.at(MapStat->BackgroundMapID), true);
		}
		catch(std::exception &Error) {
			delete BackgroundMap;
			BackgroundMap = nullptr;
		}
	}

	// Load tiles
	_Tile *Tile = nullptr;
	_Object *Object = nullptr;
	glm::ivec2 TileCoordinate;
	int TileIndex = 0;
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
			// Tile
			case 'T': {
				TileCoordinate.x = TileIndex % Size.x;
				TileCoordinate.y = TileIndex / Size.x;
				Tile = &Tiles[TileCoordinate.x][TileCoordinate.y];
				TileIndex++;
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
				if(Tile) {
					File >> Tile->Event.Type >> Tile->Event.Data;

					// Create static objects for boss events
					if(!Headless && Stats && Tile->Event.Type == EVENT_SCRIPT) {
						const _Script &Script = Stats->Scripts.at(Tile->Event.Data);
						if(Script.Name == "Script_Boss") {
							_Object *BossObject = new _Object();
							BossObject->Position = TileCoordinate;
							BossObject->BossZoneID = Script.Level;
							if(!Script.Data.empty())
								BossObject->ModelTexture = ae::Assets.Textures.at(Script.Data);
							StaticObjects.push_back(BossObject);
						}
					}
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
				glm::ivec2 Coordinate;
				File >> Coordinate.x >> Coordinate.y;
				if(!Headless) {
					Object = new _Object();
					Object->Position = Coordinate;
					StaticObjects.push_back(Object);
				}
				else
					Object = nullptr;
			} break;
			// Object light
			case 'l': {
				if(Object)
					File >> Object->Light;
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
	if(!Headless)
		InitAtlas(MapStat->Atlas, Static);

	// Initialize path finding
	Pather = new micropather::MicroPather(this, (unsigned)(Size.x * Size.y), 4);
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

	// Write tile map
	for(int j = 0; j < Size.y; j++) {
		for(int i = 0; i < Size.x; i++) {
			const _Tile &Tile = Tiles[i][j];
			Output << "T" << '\n';
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
	}

	// Write static objects
	for(auto &Object : StaticObjects) {
		Output << "O " << Object->Position.x << ' ' << Object->Position.y << '\n';
		Output << "l " << Object->Light << '\n';
	}

	Output.close();

	return true;
}

// Determines if a square can be moved to
bool _Map::CanMoveTo(const glm::ivec2 &Position, _Object *Object) {

	// Bounds
	if(Position.x < 0 || Position.x >= Size.x || Position.y < 0 || Position.y >= Size.y)
		return false;

	const _Tile *Tile = &Tiles[Position.x][Position.y];
	if(Tile->Event.Type == _Map::EVENT_KEY) {
		if(Object->Inventory->HasItemID(Tile->Event.Data))
			return true;

		// Set message for client
		if(!Server) {
			const _Item *Item = Object->Stats->Items.at(Tile->Event.Data);
			if(Item && Object->Character->HUD && Item->Name.size())
				Object->Character->HUD->SetMessage("You need the " + Item->Name);
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
void _Map::GetPotentialBattlePlayers(const _Object *Player, float DistanceSquared, std::size_t Max, std::vector<_Object *> &Players) {
	if(Player && Player->Character->Offline)
		return;

	bool HitLevelRestriction = false;
	for(const auto &Object : Objects) {

		// Check interaction
		if(!Player->CanInteractWith(Object, BATTLE_LEVEL_RANGE, HitLevelRestriction))
			continue;

		// Check party name
		if(Player->Character->PartyName != Object->Character->PartyName)
			continue;

		glm::vec2 Delta = Object->Position - Player->Position;
		if(glm::dot(Delta, Delta) <= DistanceSquared && Object->Character->CanBattle()) {
			Players.push_back(Object);
			if(Players.size() >= Max)
				return;
		}
	}
}

// Returns a battle instance close to a player
_Battle *_Map::GetCloseBattle(const _Object *Player, bool &HitPrivateParty, bool &HitFullBattle, bool &HitLevelRestriction) {
	if(Player && Player->Character->Offline)
		return nullptr;

	for(const auto &Object : Objects) {

		// Check interaction
		if(!Player->CanInteractWith(Object, BATTLE_LEVEL_RANGE, HitLevelRestriction))
			continue;

		if(!Object->Character->IsAlive())
			continue;

		if(!Object->Character->Battle)
			continue;

		if(Object->Character->Battle->PVP)
			continue;

		glm::vec2 Delta = Object->Position - Player->Position;
		if(glm::dot(Delta, Delta) > BATTLE_JOIN_DISTANCE)
			continue;

		if(Object->Character->Battle->SideCount[0] >= BATTLE_MAX_OBJECTS_PER_SIDE) {
			HitFullBattle = true;
			continue;
		}

		if(Object->Character->PartyName == "" || Object->Character->PartyName == Player->Character->PartyName)
			return Object->Character->Battle;
		else
			HitPrivateParty = true;
	}

	return nullptr;
}

// Returns target players appropriate for pvp
void _Map::GetPVPPlayers(const _Object *Attacker, std::vector<_Object *> &Players, bool UsePVPZone) {
	if(Attacker && Attacker->Character->Offline)
		return;

	// Attacker must be in PVP zone
	if(UsePVPZone && !IsPVPZone(Attacker->Position))
		return;

	bool HitLevelRestriction = false;
	for(const auto &Object : Objects) {

		// Check interaction
		if(!Attacker->CanInteractWith(Object, BATTLE_LEVEL_RANGE, HitLevelRestriction))
			continue;

		// Check if target can PVP
		if(!Object->Character->CanPVP())
			continue;

		// Target must be in PVP zone
		if(UsePVPZone && !IsPVPZone(Object->Position))
			continue;

		// Can only bounty hunt players with a bounty
		if(!UsePVPZone && !Object->Character->Attributes["Bounty"].Int64)
			continue;

		// Can't attack same party member
		if(Object->Character->PartyName != "" && Object->Character->PartyName == Attacker->Character->PartyName)
			continue;

		// Check distance
		glm::vec2 Delta = Object->Position - Attacker->Position;
		if(glm::dot(Delta, Delta) <= BATTLE_PVP_DISTANCE)
			Players.push_back(Object);
	}
}

// Returns the closest player
_Object *_Map::FindTradePlayer(const _Object *Player, float MaxDistanceSquared) {
	if(Player && Player->Character->Offline)
		return nullptr;

	_Object *ClosestPlayer = nullptr;
	float ClosestDistanceSquared = HUGE_VAL;
	bool HitLevelRestriction = false;
	for(const auto &Object : Objects) {

		// Check interaction
		if(!Player->CanInteractWith(Object, GAME_TRADING_LEVEL_RANGE, HitLevelRestriction))
			continue;

		if(!Object->Character->WaitingForTrade)
			continue;

		if(Object->Character->TradePlayer)
			continue;

		glm::vec2 Delta = Object->Position - Player->Position;
		float DistanceSquared = glm::dot(Delta, Delta);
		if(DistanceSquared <= MaxDistanceSquared && DistanceSquared < ClosestDistanceSquared) {
			ClosestDistanceSquared = DistanceSquared;
			ClosestPlayer = Object;
		}
	}

	return ClosestPlayer;
}

// Find a player that's dead in the world
_Object *_Map::FindDeadPlayer(const _Object *Player, float MaxDistanceSquared) {
	if(Player && Player->Character->Offline)
		return nullptr;

	_Object *ClosestPlayer = nullptr;
	float ClosestDistanceSquared = HUGE_VAL;
	bool HitLevelRestriction = false;
	for(const auto &Object : Objects) {

		// Check interaction
		if(!Player->CanInteractWith(Object, -1, HitLevelRestriction))
			continue;

		if(Object->Character->IsAlive())
			continue;

		if(Object->Character->Hardcore)
			continue;

		if(Object->Character->Battle)
			continue;

		glm::vec2 Delta = Object->Position - Player->Position;
		float DistanceSquared = glm::dot(Delta, Delta);
		if(DistanceSquared <= MaxDistanceSquared && DistanceSquared < ClosestDistanceSquared) {
			ClosestDistanceSquared = DistanceSquared;
			ClosestPlayer = Object;
		}
	}

	return ClosestPlayer;
}

// Find closest event on the map, returns true on found
bool _Map::FindEvent(const _Event &Event, glm::ivec2 &Position) const {

	// Find event
	auto Iterator = IndexedEvents.find(Event);
	if(Iterator == IndexedEvents.end())
		return false;

	// Return closest position
	glm::ivec2 StartPosition = Position;
	float ClosestDistanceSquared = HUGE_VAL;
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

// Delete static objects at the position
void _Map::DeleteStaticObject(const glm::ivec2 &Position) {

	// Update objects
	for(auto Iterator = StaticObjects.begin(); Iterator != StaticObjects.end(); ) {
		_Object *Object = *Iterator;
		if(Position == Object->Position) {
			Iterator = StaticObjects.erase(Iterator);
			delete Object;
		}
		else {
			++Iterator;
		}
	}
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

	// See if at least one object was changed
	bool Changed = false;
	for(auto &Object : Objects) {
		if(Object->Changed) {
			Object->Changed = false;
			Changed = true;
		}
	}

	// Increment update id
	if(Changed)
		UpdateID++;

	// Create packet
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_OBJECTUPDATES);
	Packet.Write<uint8_t>(UpdateID);
	Packet.Write<uint8_t>((uint8_t)NetworkID);

	// Write object count
	Packet.Write<ae::NetworkIDType>((ae::NetworkIDType)Objects.size());

	// Iterate over objects
	for(const auto &Object : Objects) {
		Object->SerializeUpdate(Packet);
	}

	// Send packet to players in map
	for(auto &Object : Objects) {
		if(!Object->Deleted && Object->Peer && Object->Peer->ENetPeer && Object->UpdateID != UpdateID) {
			Server->Network->SendPacket(Packet, Object->Peer, ae::_Network::UNSEQUENCED, 1);
		}
	}
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
