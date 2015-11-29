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
#include <network/servernetwork.h>
#include <network/peer.h>
#include <server.h>
#include <graphics.h>
#include <constants.h>
#include <stats.h>
#include <buffer.h>
#include <objects/object.h>
#include <texture.h>
#include <assets.h>
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
#include <stdexcept>

// Constructor for the map editor: new map
_Map::_Map()
:	Server(nullptr) {

}

_Map::_Map(const std::string &Filename, const glm::ivec2 &Size) : _Map() {
	Init();

	this->Filename = Filename;
	this->Size = Size;

	AllocateMap();
}

// Constructor for the map editor: load map
_Map::_Map(const std::string &Filename) : _Map() {
	Init();

	this->Filename = Filename;
}

// Constructor for maps already created in the database
_Map::_Map(int ID, _Stats *Stats) : _Map() {
	Init();

	// Set ID
	this->ID = ID;

	// Get map info
	const _MapStat *Map = Stats->GetMap(ID);

	// Load map
	Filename = Map->File;
	Load();
}

// Destructor
_Map::~_Map() {
	DeleteObjects();

	// Delete map data
	FreeMap();
}

// Initialize variables
void _Map::Init() {
	NextObjectID = 0;
	ObjectUpdateTime = 0;
	ID = 0;
	NoZoneTexture = nullptr;
	Tiles = nullptr;
	AmbientLight = MAP_AMBIENT_LIGHT;
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
	ObjectUpdateCount = 0;

	// Update objects
	for(auto Iterator = Objects.begin(); Iterator != Objects.end(); ) {
		_Object *Object = *Iterator;

		// Update the object
		Object->Update(FrameTime);

		// Check for events
		if(Object->Moved)
			CheckEvents(Object);

		// Delete old objects
		if(Object->Deleted) {
			if(Object->Peer) {
				RemovePeer(Object->Peer);
			}
			RemoveObject(Object);
			ObjectIDs[Object->NetworkID] = false;

			delete Object;
			Iterator = Objects.erase(Iterator);
		}
		else {
			//if(Object->SendUpdate)
			ObjectUpdateCount++;

			++Iterator;
		}
	}
}

// Check for events
void _Map::CheckEvents(_Object *Object) {

	// Handle events
	const _Tile *Tile = &Tiles[Object->Position.x][Object->Position.y];
	switch(Tile->EventType) {
		case _Map::EVENT_SPAWN:
			//Server->Log << Object->Map->ID << " " << Tile->EventData << std::endl;
			//Object->SpawnMapID = Object->Map->ID;
			//Object->SpawnPoint = Tile->EventData;
			//Object->RestoreHealthMana();
			//SendHUD(Player);
			//Object->Save();
		break;
		case _Map::EVENT_MAPCHANGE:
			//Object->GenerateNextBattle();
			if(Server)
				Server->SpawnPlayer(Object->Peer, Tile->EventData, Tile->EventType);
			else
				Object->WaitForServer = true;
		break;
		case _Map::EVENT_VENDOR: {
			if(Server) {
				Object->Vendor = Server->Stats->GetVendor(Tile->EventData);

				// Notify client
				_Buffer Packet;
				Packet.Write<char>(Packet::EVENT_START);
				Packet.Write<int32_t>(Tile->EventType);
				Packet.Write<int32_t>(Tile->EventData);
				Packet.Write<glm::ivec2>(Object->Position);
				Server->Network->SendPacket(Packet, Object->Peer);
			}
			else
				Object->WaitForServer = true;
		} break;
		case _Map::EVENT_TRADER: {
			if(Server) {
				Object->Trader = Server->Stats->GetTrader(Tile->EventData);

				// Notify client
				_Buffer Packet;
				Packet.Write<char>(Packet::EVENT_START);
				Packet.Write<int32_t>(Tile->EventType);
				Packet.Write<int32_t>(Tile->EventData);
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
			}
/*
			// Start a battle
			if(Object->NextBattle <= 0) {

				// Get monsters
				std::vector<int> Monsters;
				Stats->GenerateMonsterListFromZone(Object->GetCurrentZone(), Monsters);
				size_t MonsterCount = Monsters.size();
				if(MonsterCount > 0) {

					// Create a new battle instance
					_ServerBattle *Battle = new _ServerBattle();
					Battles.push_back(Battle);

					// Add players
					Battle->AddFighter(Player, 0);
					if(1) {

						// Get a list of players
						std::list<_Object *> Players;
						Object->Map->GetClosePlayers(Player, 7*7, Players);

						// Add players to battle
						int PlayersAdded = 0;
						for(std::list<_Object *>::iterator Iterator = Players.begin(); Iterator != Players.end(); ++Iterator) {
							_Object *PartyPlayer = *Iterator;
							if(PartyPlayer->State == _Object::STATE_NONE && !PartyPlayer->IsInvisible()) {
								SendPlayerPosition(PartyPlayer);
								Battle->AddFighter(PartyPlayer, 0);
								PlayersAdded++;
								if(PlayersAdded == 2)
									break;
							}
						}
					}

					// Add monsters
					for(size_t i = 0; i < Monsters.size(); i++) {
						_Object *Monster = new _Object(Monsters[i]);
						Monster->ID = Monsters[i];
						Monster->Type = _Object::MONSTER;
						Stats->GetMonsterStats(Monsters[i], Monster);
						Battle->AddFighter(Monster, 1);
					}

					Battle->StartBattle();
				}
			}
		*/
		break;
	}
}

// Renders the map
void _Map::Render(_Camera *Camera, _Stats *Stats, _Object *ClientPlayer, int RenderFlags) {
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

	// Render objects
	for(const auto &Object : Objects) {
		Object->Render(ClientPlayer);
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
				std::string EventText = Stats->Events[Tile->EventType].ShortName + std::string(", ") + std::to_string(Tile->EventData);
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
				// Ambient light
				case 'A': {
					File >> AmbientLight.r >> AmbientLight.g >> AmbientLight.b;
				}
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
	//if(!OldServerNetwork) {
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
	//}
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

// Generate network id
NetworkIDType _Map::GenerateObjectID() {

	// Search for an empty slot
	for(NetworkIDType i = 0; i <= std::numeric_limits<NetworkIDType>::max(); i++) {
		if(ObjectIDs[NextObjectID] == false) {
			ObjectIDs[NextObjectID] = true;
			return NextObjectID;
		}

		NextObjectID++;
	}

	throw std::runtime_error("Ran out of object ids");
}

// Removes an object from the map
void _Map::RemoveObject(const _Object *RemoveObject) {

	// Notify peers
	if(Server) {

		// Create packet
		_Buffer Packet;
		Packet.Write<char>(Packet::WORLD_DELETEOBJECT);
		Packet.Write<uint8_t>(ID);
		Packet.Write<NetworkIDType>(RemoveObject->NetworkID);

		// Send to everyone
		BroadcastPacket(Packet);
	}
}

// Remove a peer
void _Map::RemovePeer(const _Peer *Peer) {
	for(auto Iterator = Peers.begin(); Iterator != Peers.end(); ++Iterator) {
		if(*Iterator == Peer) {
			Peers.erase(Iterator);
			return;
		}
	}
}

// Delete all objects
void _Map::DeleteObjects() {

	// Delete objects
	for(auto &Object : Objects) {
		delete Object;
	}

	Objects.clear();
	ObjectIDs.clear();
	NextObjectID = 0;
}

// Get object by id
_Object *_Map::GetObjectByID(NetworkIDType ObjectID) {
	for(auto &Object : Objects) {
		if(Object->NetworkID == ObjectID)
			return Object;
	}

	return nullptr;
}

// Adds an object to the map
void _Map::AddObject(_Object *Object) {
	if(!Server)
		return;

	// Create packet for the new object
	_Buffer Packet;
	Packet.Write<char>(Packet::WORLD_CREATEOBJECT);
	Packet.Write<NetworkIDType>(Object->NetworkID);
	Packet.Write<glm::ivec2>(Object->Position);
	Packet.Write<char>(Object->Type);
	Packet.WriteString(Object->Name.c_str());
	Packet.Write<uint32_t>(Object->PortraitID);
	Packet.WriteBit(Object->IsInvisible());

	// Notify other players of the new object
	BroadcastPacket(Packet);

	// Add object to map
	Objects.push_back(Object);
}

// Add object with network
void _Map::AddObject(_Object *Object, NetworkIDType NetworkID) {
	if(ObjectIDs[NetworkID])
		throw std::runtime_error(std::string("NetworkID already taken: ") + std::to_string(NetworkID));

	ObjectIDs[NetworkID] = true;
	Objects.push_back(Object);
}
/*
// Returns a list of players close to a player
void _Map::GetClosePlayers(const _Object *Player, float DistanceSquared, std::list<_Object *> &Players) {

	for(const auto &Object : Objects) {
		if(Object->Type == _Object::PLAYER) {
			_Object *TestPlayer = (_Object *)Object;
			if(TestPlayer != Player) {
				glm::ivec2 Delta = TestPlayer->Position - Player->Position;
				if((float)(Delta.x * Delta.x + Delta.y * Delta.y) <= DistanceSquared) {
					Players.push_back(TestPlayer);
				}
			}
		}
	}
}
*/
// Returns the closest player
_Object *_Map::FindTradePlayer(const _Object *Player, float MaxDistanceSquared) {

	_Object *ClosestPlayer = nullptr;
	float ClosestDistanceSquared = HUGE_VAL;
	for(const auto &Object : Objects) {
		if(Object != Player && Object->State == _Object::STATE_TRADE && Object->TradePlayer == nullptr) {
			glm::ivec2 Delta = Object->Position - Player->Position;
			float DistanceSquared = (float)(Delta.x * Delta.x + Delta.y * Delta.y);
			if(DistanceSquared <= MaxDistanceSquared && DistanceSquared < ClosestDistanceSquared) {
				ClosestDistanceSquared = DistanceSquared;
				ClosestPlayer = Object;
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

// Send complete object list to player
void _Map::SendObjectList(_Peer *Peer) {
	if(!Server)
		return;

	if(!Peer->Object)
		return;

	// Create packet
	_Buffer Packet;
	Packet.Write<char>(Packet::WORLD_OBJECTLIST);
	Packet.Write<NetworkIDType>(Peer->Object->NetworkID);

	// Write object data
	Packet.Write<NetworkIDType>(Objects.size());
	for(auto &Object : Objects) {
		Packet.Write<NetworkIDType>(Object->NetworkID);
		Packet.Write<glm::ivec2>(Object->Position);
		Packet.Write<char>(Object->Type);
		Packet.WriteString(Object->Name.c_str());
		Packet.Write<uint32_t>(Object->PortraitID);
		Packet.WriteBit((Object->IsInvisible()));
	}

	Server->Network->SendPacket(Packet, Peer);
}

// Sends object position information to all the clients in the map
void _Map::SendObjectUpdates() {

	// Create packet
	_Buffer Packet;
	Packet.Write<char>(Packet::WORLD_OBJECTUPDATES);
	Packet.Write<char>(ID);

	// Write object count
	Packet.Write<NetworkIDType>(Objects.size());

	// Iterate over objects
	for(const auto &Object : Objects) {

		// Write object data
		Packet.Write<NetworkIDType>(Object->NetworkID);
		Packet.Write<char>(Object->State);
		Packet.Write<glm::ivec2>(Object->Position);
		Packet.WriteBit(Object->IsInvisible());
	}

	// Send packet to players in map
	BroadcastPacket(Packet, _Network::UNSEQUENCED);
}

// Broadcast a packet to all peers in the map
void _Map::BroadcastPacket(_Buffer &Buffer, _Network::SendType Type) {
	if(!Server)
		return;

	// Send packet to peers
	for(auto &Peer : Peers)
		Server->Network->SendPacket(Buffer, Peer, Type, Type == _Network::UNSEQUENCED);
}

// Get a valid position within the grid
glm::vec2 _Map::GetValidPosition(const glm::vec2 &Position) {
	return glm::clamp(Position, glm::vec2(0.0f), glm::vec2(Size));
}
