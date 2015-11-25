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
#include <states/client.h>
#include <network/clientnetwork.h>
#include <objects/object.h>
#include <instances/map.h>
#include <ui/element.h>
#include <constants.h>
#include <framework.h>
#include <graphics.h>
#include <menu.h>
#include <camera.h>
#include <assets.h>
#include <stats.h>
#include <hud.h>
#include <config.h>
#include <actions.h>
#include <buffer.h>
#include <server.h>
#include <packet.h>
#include <log.h>
#include <iostream>
#include <sstream>

#include <font.h>
#include <program.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

_ClientState ClientState;

// Constructor
_ClientState::_ClientState() :
	Level(""),
	SaveFilename("test.save"),
	IsTesting(true),
	FromEditor(false),
	Stats(nullptr),
	Server(nullptr),
	HostAddress("127.0.0.1"),
	ConnectPort(DEFAULT_NETWORKPORT) {
}

// Load level and set up objects
void _ClientState::Init() {
	Player = nullptr;
	Camera = nullptr;
	HUD = nullptr;
	Map = nullptr;
	Network = nullptr;
	Server = nullptr;

	Stats = new _Stats();
	Stats->Init();

	Network = new _ClientNetwork();
	Network->SetFakeLag(Config.FakeLag);
	Network->SetUpdatePeriod(Config.NetworkRate);

	Graphics.ChangeViewport(Graphics.WindowSize);

	Actions.ResetState();

	Menu.InitTitle();
}

// Close map
void _ClientState::Close() {
	Menu.Close();

	delete Camera;
	delete HUD;
	delete Map;
	delete Network;
	delete Server;
}

// Connect to a server
void _ClientState::Connect(bool IsLocal) {

	// Start a local server
	if(IsLocal) {
		StartLocalServer();
		Network->Connect("127.0.0.1", DEFAULT_NETWORKPORT);
	}
	else {
		Network->Connect(HostAddress.c_str(), ConnectPort);
	}
}

void _ClientState::StartLocalServer() {

	// Kill existing server
	if(Server) {
		Server->StopServer();
		delete Server;
	}

	// Start server in thread
	Server = new _Server(ConnectPort);
	Server->Stats = Stats;
	Server->StartThread();

	// Connect
	Network->Connect(HostAddress.c_str(), ConnectPort);
}

// Action handler
bool _ClientState::HandleAction(int InputType, int Action, int Value) {
	if(!Player)
		return false;

	return false;
}

// Key handler
void _ClientState::KeyEvent(const _KeyEvent &KeyEvent) {
	bool Handled = Graphics.Element->HandleKeyEvent(KeyEvent);
	Menu.KeyEvent(KeyEvent);

	if(!Handled) {
		if(Menu.GetState() != _Menu::STATE_NONE) {
			Menu.KeyEvent(KeyEvent);
			return;
		}
	}
	else {
		//if(!HUD.IsChatting())
		//	HUD.ValidateTradeGold();
	}
	/*
	if(KeyEvent.Pressed) {
		switch(KeyEvent.Scancode) {
			case SDL_SCANCODE_ESCAPE:
				if(Server)
					Server->StopServer();
				else
					Network->Disconnect();
			break;
			case SDL_SCANCODE_F1:
			break;
			case SDL_SCANCODE_GRAVE:
			break;
		}
	}*/
}

// Mouse handler
void _ClientState::MouseEvent(const _MouseEvent &MouseEvent) {
	FocusedElement = nullptr;
	Graphics.Element->HandleInput(MouseEvent.Pressed);
	Menu.MouseEvent(MouseEvent);
}

void _ClientState::WindowEvent(uint8_t Event) {
	if(Camera && Event == SDL_WINDOWEVENT_SIZE_CHANGED)
		Camera->CalculateFrustum(Graphics.AspectRatio);
}

// Update
void _ClientState::Update(double FrameTime) {
	Graphics.Element->Update(FrameTime, Input.GetMouse());

	// Update network
	Network->Update(FrameTime);

	// Get events
	_NetworkEvent NetworkEvent;
	while(Network->GetNetworkEvent(NetworkEvent)) {

		switch(NetworkEvent.Type) {
			case _NetworkEvent::CONNECT: {
				HandleConnect();
			} break;
			case _NetworkEvent::DISCONNECT:
				//if(FromEditor)
					//Framework.ChangeState(&EditorState);
				//else
					Menu.InitTitle();
			break;
			case _NetworkEvent::PACKET:
				HandlePacket(*NetworkEvent.Data);
				delete NetworkEvent.Data;
			break;
		}
	}

	// Update menu
	Menu.Update(FrameTime);

	// Process input
	if(Player && Map) {
	}

	// Update objects
	if(Map)
		Map->Update(FrameTime);

	// Update camera
	if(Camera && Player) {
		Camera->Set2DPosition(glm::vec2(Player->Position));
		Camera->Update(FrameTime);
	}

	// Update the HUD
	if(HUD)
		HUD->Update(FrameTime);

	// Send network updates to server
	if(Player && Network->IsConnected()) {

		// Reset timer
		Network->ResetUpdateTimer();
	}

	if(Player)
		TimeSteps++;
}

// Render the state
void _ClientState::Render(double BlendFactor) {
	Menu.Render();

	if(!Map || !Player)
		return;
}

// Handle packet from server
void _ClientState::HandlePacket(_Buffer &Buffer) {
	Menu.HandlePacket(Buffer);

	char PacketType = Buffer.Read<char>();

	switch(PacketType) {
		/*
		case Packet::WORLD_YOURCHARACTERINFO:
			HandleYourCharacterInfo(&Packet);
		break;
		case Packet::WORLD_CHANGEMAPS:
			HandleChangeMaps(&Packet);
		break;
		case Packet::WORLD_CREATEOBJECT:
			HandleCreateObject(&Packet);
		break;
		case Packet::WORLD_DELETEOBJECT:
			HandleDeleteObject(&Packet);
		break;
		case Packet::WORLD_OBJECTUPDATES:
			HandleObjectUpdates(&Packet);
		break;
		case Packet::WORLD_STARTBATTLE:
			HandleStartBattle(&Packet);
		break;
		case Packet::WORLD_HUD:
			HandleHUD(&Packet);
		break;
		case Packet::WORLD_POSITION:
			HandlePlayerPosition(&Packet);
		break;
		case Packet::BATTLE_TURNRESULTS:
			HandleBattleTurnResults(&Packet);
		break;
		case Packet::BATTLE_END:
			HandleBattleEnd(&Packet);
		break;
		case Packet::BATTLE_COMMAND:
			HandleBattleCommand(&Packet);
		break;
		case Packet::EVENT_START:
			HandleEventStart(&Packet);
		break;
		case Packet::INVENTORY_USE:
			HandleInventoryUse(&Packet);
		break;
		case Packet::CHAT_MESSAGE:
			HandleChatMessage(&Packet);
		break;
		case Packet::TRADE_REQUEST:
			HandleTradeRequest(&Packet);
		break;
		case Packet::TRADE_CANCEL:
			HandleTradeCancel(&Packet);
		break;
		case Packet::TRADE_ITEM:
			HandleTradeItem(&Packet);
		break;
		case Packet::TRADE_GOLD:
			HandleTradeGold(&Packet);
		break;
		case Packet::TRADE_ACCEPT:
			HandleTradeAccept(&Packet);
		break;
		case Packet::TRADE_EXCHANGE:
			HandleTradeExchange(&Packet);
		break;*/
	}
}

// Handle connection to server
void _ClientState::HandleConnect() {

	if(Server) {
		_Buffer Packet;
		Packet.Write<char>(Packet::ACCOUNT_LOGININFO);
		Packet.WriteBit(0);
		Packet.WriteString("singleplayer");
		Packet.WriteString("singleplayer");
		Network->SendPacket(Packet);
	}

	//HUD = nullptr;

	//Log << " -- CONNECT" << std::endl;

	//if(Level == "")
	//	Level = "test.map";

	//_Buffer Buffer;
	//Buffer.Write<char>(Packet::CLIENT_JOIN);
	//Buffer.WriteString(Level.c_str());
	//Network->SendPacket(&Buffer);

	// Initialize hud
	//HUD = new _HUD();

	// Set up graphics
	//Camera = new _Camera(glm::vec3(0, 0, CAMERA_DISTANCE), CAMERA_DIVISOR);
	//Camera->CalculateFrustum(Graphics.AspectRatio);
}

// Load the map
void _ClientState::HandleMapInfo(_Buffer &Buffer) {

	// Read packet
	//uint8_t MapID = Buffer.Read<uint8_t>();
	//std::string NewMap = Buffer.ReadString();

	// Create new map
	//delete Map;
	//Map = new _Map(NewMap, Stats, false, MapID);
	//Map->SetCamera(Camera);
	//Player = nullptr;
	//HUD->SetPlayer(nullptr);
}

// Handle a complete list of objects from a map
void _ClientState::HandleObjectList(_Buffer &Buffer) {
/*
	// Check map id
	uint8_t MapID = Buffer.Read<uint8_t>();
	if(MapID != Map->ID)
		return;

	TimeSteps = Buffer.Read<uint16_t>();
	uint16_t ClientID = Buffer.Read<uint16_t>();
	LastServerTimeSteps = TimeSteps - 1;

	// Clear out old objects
	//Map->DeleteObjects();

	// Read object list
	uint16_t ObjectCount = Buffer.Read<uint16_t>();
	for(uint16_t i = 0; i < ObjectCount; i++) {
		//std::string Identifier = Buffer.ReadString();
		//uint16_t NetworkID = Buffer.Read<uint16_t>();

		// Create object
		//_Object *Object = Stats->CreateObject(Identifier, false);
		//Object->Map = Map;
		//Object->ID = NetworkID;
		//Object->NetworkUnserialize(Buffer);

		// Add to map
		//Map->AddObject(Object);

		// Keep track of player id
		//if(ClientID == NetworkID)
		//	Player = Object;
	}

	if(Player) {
		//HUD->SetPlayer(Player);
		//Camera->ForcePosition(glm::vec3(Player->Physics->Position.x, Player->Physics->Position.y, CAMERA_DISTANCE));
	}*/
}

// Handle incremental updates from a map
void _ClientState::HandleObjectUpdates(_Buffer &Buffer) {

	// Check map id
	uint8_t MapID = Buffer.Read<uint8_t>();
	if(MapID != Map->ID)
		return;

	// Discard out of order packets
	uint16_t ServerTimeSteps = Buffer.Read<uint16_t>();
	if(!_Network::MoreRecentAck(LastServerTimeSteps, ServerTimeSteps, uint16_t(-1)))
		return;

	// Update objects
	//Map->UpdateObjectsFromBuffer(Buffer, ServerTimeSteps);
	//if(Controller)
	//	Controller->ReplayInput();

	LastServerTimeSteps = ServerTimeSteps;
}

// Handle a create packet
void _ClientState::HandleObjectCreate(_Buffer &Buffer) {

	// Check map id
	uint8_t MapID = Buffer.Read<uint8_t>();
	if(MapID != Map->ID)
		return;

	// Get object properties
	//std::string Identifier = Buffer.ReadString();
	//uint16_t ID = Buffer.Read<uint16_t>();

	// Create object
	/*
	_Object *Object = Stats->CreateObject(Identifier, false);
	Object->ID = ID;
	Object->Map = Map;
	Object->NetworkUnserialize(Buffer);

	// Add to map
	Map->AddObject(Object);
	if(Object->Shape)
		Map->Grid->AddObject(Object);

	Object->Log = Log;*/
}

// Handle a delete packet
void _ClientState::HandleObjectDelete(_Buffer &Buffer) {

	// Check map id
	uint8_t MapID = Buffer.Read<uint8_t>();
	if(MapID != Map->ID)
		return;

	// Delete object by id
	//uint16_t ID = Buffer.Read<uint16_t>();
	//_Object *Object = Map->GetObjectByID(ID);
	//if(Object)
	//	Object->Deleted = true;
}
