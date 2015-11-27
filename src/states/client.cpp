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
#include <instances/clientbattle.h>
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
	IsTesting(true),
	FromEditor(false),
	Stats(nullptr),
	Server(nullptr),
	HostAddress("127.0.0.1"),
	ConnectPort(DEFAULT_NETWORKPORT) {
}

// Load level and set up objects
void _ClientState::Init() {
	Server = nullptr;
	Player = nullptr;
	Map = nullptr;

	HUD = new _HUD();
	Stats = new _Stats();
	Camera = new _Camera(glm::vec3(0, 0, CAMERA_DISTANCE), CAMERA_DIVISOR);
	Camera->CalculateFrustum(Graphics.AspectRatio);

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
	delete Stats;
}

// Connect to a server
void _ClientState::Connect(bool IsLocal) {

	// Start a local server
	if(IsLocal) {
		StartLocalServer();
		Network->Connect("127.0.0.1", DEFAULT_NETWORKPORT_ALT);
	}
	else {
		Network->Connect(HostAddress.c_str(), ConnectPort);
	}
}

// Stops the server thread
void _ClientState::StopLocalServer() {

	// Kill existing server
	if(Server) {
		Server->StopServer();
		Server->JoinThread();
		delete Server;
		Server = nullptr;
	}
}

// Start local server in thread
void _ClientState::StartLocalServer() {

	// Stop existing server
	StopLocalServer();

	// Start server in thread
	Server = new _Server(DEFAULT_NETWORKPORT_ALT);
	Server->Stats = Stats;
	Server->StartThread();
}

// Action handler
bool _ClientState::HandleAction(int InputType, int Action, int Value) {
	if(Value == 0)
		return true;

	// Pass to menu
	if(Menu.State != _Menu::STATE_NONE) {
		Menu.HandleAction(InputType, Action, Value);
		return true;
	}

	if(!Player)
		return true;

	// Handle enter key
	if(Action == _Actions::CHAT) {
		HUD->HandleEnter();
		return true;
	}

	// Grab all actions except escape
	if(HUD->IsChatting()) {
		if(Action == _Actions::MENU)
			HUD->CloseChat();

		return true;
	}

	// Handle HUD keys
	switch(Action) {
		case _Actions::MENU:
			if(IsTesting)
				//Framework.Done = true;
				Network->Disconnect();
			else
				HUD->ToggleInGameMenu();
		break;
		case _Actions::INVENTORY:
			HUD->ToggleInventory();
		break;
		case _Actions::TELEPORT:
			HUD->ToggleTeleport();
		break;
		case _Actions::TRADE:
			HUD->ToggleTrade();
		break;
		case _Actions::SKILLS:
			HUD->ToggleSkills();
		break;
		case _Actions::ATTACK:
			//SendAttackPlayer();
		break;
	}
/*
	switch(Player->State) {
		case _Object::STATE_BATTLE:
			Player->Battle->HandleAction(Action);
		break;
		case _Object::STATE_TELEPORT:
			switch(Action) {
				case _Actions::UP:
				case _Actions::DOWN:
				case _Actions::LEFT:
				case _Actions::RIGHT:
				case _Actions::MENU:
				case _Actions::TELEPORT:
					HUD->ToggleTeleport();
				break;
			}
		break;
		case _Object::STATE_VENDOR:
			switch(Action) {
				case _Actions::UP:
				case _Actions::DOWN:
				case _Actions::LEFT:
				case _Actions::RIGHT:
				case _Actions::MENU:
				case _Actions::INVENTORY:
					HUD->CloseWindows();
				break;
			}
		break;
		case _Object::STATE_TRADER:
			switch(Action) {
				case _Actions::UP:
				case _Actions::DOWN:
				case _Actions::LEFT:
				case _Actions::RIGHT:
				case _Actions::MENU:
				case _Actions::INVENTORY:
					HUD->CloseWindows();
				break;
			}
		break;
		case _Object::STATE_TRADE:
			switch(Action) {
				case _Actions::UP:
				case _Actions::DOWN:
				case _Actions::LEFT:
				case _Actions::RIGHT:
				case _Actions::MENU:
				case _Actions::INVENTORY:
				case _Actions::TRADE:
					HUD->CloseWindows();
				break;
			}
		break;
		case _Object::STATE_BUSY:
			HUD->CloseWindows();
		break;
	}
*/
	return true;
}

// Key handler
void _ClientState::KeyEvent(const _KeyEvent &KeyEvent) {
	bool Handled = Graphics.Element->HandleKeyEvent(KeyEvent);

	// Pass to menu
	if(!Handled)
		Menu.KeyEvent(KeyEvent);

	if(Menu.State != _Menu::STATE_NONE)
		return;

	if(!HUD->IsChatting())
		HUD->ValidateTradeGold();
}

// Mouse handler
void _ClientState::MouseEvent(const _MouseEvent &MouseEvent) {
	FocusedElement = nullptr;
	Graphics.Element->HandleInput(MouseEvent.Pressed);

	// Pass to menu
	Menu.MouseEvent(MouseEvent);
	if(Menu.State != _Menu::STATE_NONE)
		return;

	HUD->MouseEvent(MouseEvent);
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
				HandleDisconnect();
			break;
			case _NetworkEvent::PACKET:
				HandlePacket(*NetworkEvent.Data);
				delete NetworkEvent.Data;
			break;
		}
	}

	// Update menu
	Menu.Update(FrameTime);

	if(!Player || !Map)
		return;

	// Set input
	Player->InputState = 0;
	if(Actions.GetState(_Actions::UP))
		Player->InputState |= _Object::MOVE_UP;
	if(Actions.GetState(_Actions::DOWN))
		Player->InputState |= _Object::MOVE_DOWN;
	if(Actions.GetState(_Actions::LEFT))
		Player->InputState |= _Object::MOVE_LEFT;
	if(Actions.GetState(_Actions::RIGHT))
		Player->InputState |= _Object::MOVE_RIGHT;

	// Update objects
	Map->Update(FrameTime);
	if(Player->Moved) {
		_Buffer Packet;
		Packet.Write<char>(Packet::WORLD_MOVECOMMAND);
		Packet.Write<char>(Player->Moved);
		Network->SendPacket(Packet);
	}

	// Update camera
	Camera->Set2DPosition(glm::vec2(Player->Position) + glm::vec2(0.5f, 0.5f));
	Camera->Update(FrameTime);

	// Update the HUD
	HUD->Update(FrameTime);

	// Handle input
	/*
	if(Menu.GetState() == _Menu::STATE_NONE) {
		switch(Player->State) {
			case _Object::STATE_BATTLE: {

				// Send key input
				if(!HUD->IsChatting()) {
					for(int i = 0; i < ACTIONBAR_SIZE; i++) {
						if(Actions.GetState(_Actions::SKILL1+i)) {
							Player->Battle->HandleAction(_Actions::SKILL1+i);
							break;
						}
					}
				}
			} break;
		}
	}*/
}

// Render the state
void _ClientState::Render(double BlendFactor) {
	Menu.Render();

	if(!Player || !Map)
		return;

	Graphics.Setup3D();
	glm::vec3 LightPosition(glm::vec3(Player->Position, 1) + glm::vec3(0.5f, 0.5f, 0));
	glm::vec3 LightAttenuation(0.0f, 1.0f, 0.0f);

	Assets.Programs["pos_uv"]->LightAttenuation = LightAttenuation;
	Assets.Programs["pos_uv"]->LightPosition = LightPosition;
	Assets.Programs["pos_uv"]->AmbientLight = Map->AmbientLight;
	Assets.Programs["pos_uv_norm"]->LightAttenuation = LightAttenuation;
	Assets.Programs["pos_uv_norm"]->LightPosition = LightPosition;
	Assets.Programs["pos_uv_norm"]->AmbientLight = Map->AmbientLight;

	// Setup the viewing matrix
	Graphics.Setup3D();
	Camera->Set3DProjection(BlendFactor);
	Graphics.SetProgram(Assets.Programs["pos"]);
	glUniformMatrix4fv(Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["pos_uv_norm"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv_norm"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["text"]);
	glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

	// Draw map and objects
	Map->Render(Camera, Stats, Player);

	Graphics.Setup2D();
	Graphics.SetProgram(Assets.Programs["text"]);
	glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Graphics.Ortho));

	// Draw HUD
	HUD->Render();

	// Draw states
	if(Player->Battle)
		Player->Battle->Render(BlendFactor);

	// Draw menu
	Menu.Render();
}

// Handle packet from server
void _ClientState::HandlePacket(_Buffer &Data) {
	char PacketType = Data.Read<char>();

	switch(PacketType) {
		case Packet::WORLD_YOURCHARACTERINFO:
			HandleYourCharacterInfo(Data);
		break;
		case Packet::WORLD_CHANGEMAPS:
			HandleChangeMaps(Data);
		break;
		case Packet::WORLD_OBJECTLIST:
			HandleObjectList(Data);
		break;
		case Packet::WORLD_CREATEOBJECT:
			HandleCreateObject(Data);
		break;
		case Packet::WORLD_DELETEOBJECT:
			HandleDeleteObject(Data);
		break;
		case Packet::WORLD_POSITION:
			HandlePlayerPosition(Data);
		break;
		case Packet::WORLD_OBJECTUPDATES:
			HandleObjectUpdates(Data);
		break;
		case Packet::EVENT_START:
			HandleEventStart(Data);
		break;
		/*
		case Packet::WORLD_STARTBATTLE:
			HandleStartBattle(Data);
		break;
		case Packet::WORLD_HUD:
			HandleHUD(Data);
		break;
		case Packet::BATTLE_TURNRESULTS:
			HandleBattleTurnResults(Data);
		break;
		case Packet::BATTLE_END:
			HandleBattleEnd(Data);
		break;
		case Packet::BATTLE_COMMAND:
			HandleBattleCommand(Data);
		break;
		case Packet::INVENTORY_USE:
			HandleInventoryUse(Data);
		break;
		case Packet::CHAT_MESSAGE:
			HandleChatMessage(Data);
		break;
		case Packet::TRADE_REQUEST:
			HandleTradeRequest(Data);
		break;
		case Packet::TRADE_CANCEL:
			HandleTradeCancel(Data);
		break;
		case Packet::TRADE_ITEM:
			HandleTradeItem(Data);
		break;
		case Packet::TRADE_GOLD:
			HandleTradeGold(Data);
		break;
		case Packet::TRADE_ACCEPT:
			HandleTradeAccept(Data);
		break;
		case Packet::TRADE_EXCHANGE:
			HandleTradeExchange(Data);
		break;*/
		default:
			Menu.HandlePacket(Data, PacketType);
		break;
	}
}

// Handle connection to server
void _ClientState::HandleConnect() {

	if(Server) {
		_Buffer Packet;
		Packet.Write<char>(Packet::ACCOUNT_LOGININFO);
		Packet.WriteBit(0);
		Packet.WriteString("choria_singleplayer");
		Packet.WriteString("choria_singleplayer");
		Network->SendPacket(Packet);
	}

	Menu.HandleConnect();
}

// Handle disconnects
void _ClientState::HandleDisconnect() {
	ClientState.StopLocalServer();
	Menu.HandleDisconnect();
	if(Player) {
		if(Map) {
			delete Map;
			Map = nullptr;
		}
	}
}

// Called once to synchronize your stats with the servers
void _ClientState::HandleYourCharacterInfo(_Buffer &Data) {
	if(!Player)
		return;

	//Log << "HandleYourCharacterInfo" << std::endl;

	Player->WorldImage = Assets.Textures["players/basic.png"];
	Player->Stats = Stats;
	Player->Name = Data.ReadString();
	Player->PortraitID = Data.Read<int32_t>();
	Player->Experience = Data.Read<int32_t>();
	Player->Gold = Data.Read<int32_t>();
	Player->PlayTime = Data.Read<int32_t>();
	Player->Deaths = Data.Read<int32_t>();
	Player->MonsterKills = Data.Read<int32_t>();
	Player->PlayerKills = Data.Read<int32_t>();
	Player->Bounty = Data.Read<int32_t>();
	HUD->SetPlayer(Player);

	// Read items
	int ItemCount = Data.Read<char>();
	for(int i = 0; i < ItemCount; i++) {
		int Slot = Data.Read<char>();
		int Count = (uint8_t)Data.Read<char>();
		int ItemID = Data.Read<int32_t>();
		Player->SetInventory(Slot, ItemID, Count);
	}

	// Read skills
	int SkillCount = Data.Read<char>();
	for(int i = 0; i < SkillCount; i++) {
		uint32_t SkillID = Data.Read<uint32_t>();
		int32_t Points = Data.Read<int32_t>();
		Player->SkillLevels[SkillID] = Points;
	}

	// Read skill bar
	for(int i = 0; i < ACTIONBAR_SIZE; i++) {
		uint32_t SkillID = Data.Read<uint32_t>();
		Player->ActionBar[i] = Stats->Skills[SkillID];
	}

	Player->CalculateSkillPoints();
	Player->CalculatePlayerStats();
	Player->RestoreHealthMana();
}

// Called when the player changes maps
void _ClientState::HandleChangeMaps(_Buffer &Data) {
	Menu.InitPlay();

	// Load map
	int MapID = Data.Read<int32_t>();

	// Delete old map and create new
	if(!Map || Map->ID != MapID) {
		if(Map)
			delete Map;

		Map = new _Map(MapID, Stats);
		Player = nullptr;
	}

	/*
	// Delete the battle
	if(Player->Battle) {
		delete Player->Battle;
		Player->Battle = nullptr;
	}

	Player->State = _Object::STATE_NONE;
	*/
}

// Handle object list
void _ClientState::HandleObjectList(_Buffer &Data) {

	// Read header
	NetworkIDType ClientNetworkID = Data.Read<NetworkIDType>();
	NetworkIDType ObjectCount = Data.Read<NetworkIDType>();

	// Create objects
	for(NetworkIDType i = 0; i < ObjectCount; i++) {

		// Read data
		_Object *Object = new _Object();
		Object->NetworkID = Data.Read<NetworkIDType>();
		Object->Position = Data.Read<glm::ivec2>();
		Object->Type = Data.Read<char>();
		Object->Name = Data.ReadString();
		Object->PortraitID = Data.Read<char>();
		Object->Portrait = Stats->Portraits[Object->PortraitID].Image;
		Object->InvisPower = Data.ReadBit();
		Object->WorldImage = Assets.Textures["players/basic.png"];

		Map->AddObject(Object, Object->NetworkID);
		Object->Map = Map;

		// Set player pointer
		if(Object->NetworkID == ClientNetworkID)
			Player = Object;
	}

	if(Player) {
		Camera->ForcePosition(glm::vec3(Player->Position, CAMERA_DISTANCE) + glm::vec3(0.5, 0.5, 0));
	}
	else {
		// Error
	}
}

// Creates an object
void _ClientState::HandleCreateObject(_Buffer &Data) {
	if(!Map || !Player)
		return;

	// Read packet
	NetworkIDType NetworkID = Data.Read<NetworkIDType>();

	// Check id
	if(NetworkID != Player->NetworkID) {

		// Create object
		_Object *Object = new _Object();
		Object->NetworkID = NetworkID;
		Object->Position = Data.Read<glm::ivec2>();
		Object->Type = Data.Read<char>();
		Object->Name = Data.ReadString();
		Object->PortraitID = Data.Read<char>();
		Object->InvisPower = Data.ReadBit();

		Object->Portrait = Stats->Portraits[Object->PortraitID].Image;
		Object->Map = Map;
		Object->WorldImage = Assets.Textures["players/basic.png"];

		// Add to map
		Map->AddObject(Object, NetworkID);
	}
}

// Deletes an object
void _ClientState::HandleDeleteObject(_Buffer &Data) {
	if(!Player || !Map)
		return;

	uint8_t MapID = Data.Read<uint8_t>();
	NetworkIDType NetworkID = Data.Read<NetworkIDType>();

	// Check for same map
	if(Map->ID != MapID)
		return;

	// Get object
	_Object *Object = Map->GetObjectByID(NetworkID);
	if(Object) {
		switch(Player->State) {
			case _Object::STATE_BATTLE:
				Player->Battle->RemoveFighter(Object);
			break;
		}
		Object->Deleted = true;
	}
}

// Handles position updates from the server
void _ClientState::HandleObjectUpdates(_Buffer &Data) {
	if(!Player || !Map)
		return;

	// Check map id
	int MapID = Data.Read<char>();
	if(MapID != Map->ID)
		return;

	// Get object count
	NetworkIDType ObjectCount = Data.Read<NetworkIDType>();

	// Iterate over objects
	for(NetworkIDType i = 0; i < ObjectCount; i++) {

		// Read packet
		NetworkIDType NetworkID = Data.Read<NetworkIDType>();
		int PlayerState = Data.Read<char>();
		glm::ivec2 Position = Data.Read<glm::ivec2>();
		int Invisible = Data.ReadBit();

		// Find object
		_Object *Object = Map->GetObjectByID(NetworkID);
		if(Object) {

			if(Object == Player) {

			}
			else {
				Object->Position = Position;
				Object->State = PlayerState;
				Object->InvisPower = Invisible;
			}
			Object->ServerPosition = Position;

			switch(PlayerState) {
				case _Object::STATE_NONE:
					Object->StateImage = nullptr;
				break;
				case _Object::STATE_TRADE:
					Object->StateImage = Assets.Textures["world/trade.png"];
				break;
				default:
					Object->StateImage = Assets.Textures["world/busy.png"];
				break;
			}
		}
	}
}

// Handles player position
void _ClientState::HandlePlayerPosition(_Buffer &Data) {
	if(!Player)
		return;

	Player->Position = Data.Read<glm::ivec2>();
}

// Handles the start of an event
void _ClientState::HandleEventStart(_Buffer &Data) {
	if(!Player)
		return;

	// Read packet
	int32_t EventType = Data.Read<int32_t>();
	int32_t EventData = Data.Read<int32_t>();
	Player->Position = Data.Read<glm::ivec2>();

	// Handle event
	switch(EventType) {
		case _Map::EVENT_VENDOR:
			Player->Vendor = ClientState.Stats->GetVendor(EventData);
			HUD->InitVendor();
		break;
		case _Map::EVENT_TRADER:
			HUD->InitTrader(EventData);
			//Player->State = _Object::STATE_TRADER;
		break;
	}
}

/*
// Send the busy signal to server
void _ClientState::SendBusy(bool Value) {
	_Buffer Packet;
	Packet.Write<char>(Packet::WORLD_BUSY);
	Packet.Write<char>(Value);
	Network->SendPacket(Packet);
}

// Handles the start of a battle
void _ClientState::HandleStartBattle(_Buffer &Data) {

	// Already in a battle
	if(Player->Battle)
		return;

	// Create a new battle instance
	Player->Battle = new _ClientBattle();;

	// Get fighter count
	int FighterCount = Data.Read<char>();

	// Get fighter information
	for(int i = 0; i < FighterCount; i++) {

		// Get fighter type
		int Type = Data.ReadBit();
		int Side = Data.ReadBit();
		if(Type == _Object::PLAYER) {

			// Network ID
			NetworkIDType NetworkID = Data.Read<NetworkIDType>();

			// Player stats
			int Health = Data.Read<int32_t>();
			int MaxHealth = Data.Read<int32_t>();
			int Mana = Data.Read<int32_t>();
			int MaxMana = Data.Read<int32_t>();

			// Get player object
			_Object *NewPlayer = (_Object *)ObjectManager->GetObjectFromNetworkID(NetworkID);
			if(NewPlayer != nullptr) {
				NewPlayer->Health = Health;
				NewPlayer->MaxHealth = MaxHealth;
				NewPlayer->Mana = Mana;
				NewPlayer->MaxMana = MaxMana;
				NewPlayer->Battle = Player->Battle;

				Player->Battle->AddFighter(NewPlayer, Side);
			}
		}
		else {

			// Monster ID
			int MonsterID = Data.Read<int32_t>();
			_Object *Monster = new _Object(MonsterID);
			Monster->ID = MonsterID;
			Monster->Type = _Object::MONSTER;
			Stats->GetMonsterStats(MonsterID, Monster);

			Player->Battle->AddFighter(Monster, Side);
		}
	}

	// Start the battle
	HUD->CloseWindows();
	((_ClientBattle *)Player->Battle)->StartBattle(Player);
}

// Handles the result of a turn in battle
void _ClientState::HandleBattleTurnResults(_Buffer &Data) {

	// Check for a battle in progress
	if(!Player->Battle)
		return;

	((_ClientBattle *)Player->Battle)->ResolveTurn(Packet);
}

// Handles the end of a battle
void _ClientState::HandleBattleEnd(_Buffer &Data) {

	// Check for a battle in progress
	if(!Player->Battle)
		return;

	((_ClientBattle *)Player->Battle)->EndBattle(Packet);
}

// Handles a battle command from other players
void _ClientState::HandleBattleCommand(_Buffer &Data) {

	// Check for a battle in progress
	if(!Player->Battle)
		return;

	int Slot = Data.Read<char>();
	int SkillID = Data.Read<char>();
	((_ClientBattle *)Player->Battle)->HandleCommand(Slot, SkillID);
}

// Handles HUD updates
void _ClientState::HandleHUD(_Buffer &Data) {
	Player->Experience = Data.Read<int32_t>();
	Player->Gold = Data.Read<int32_t>();
	Player->Health = Data.Read<int32_t>();
	Player->Mana = Data.Read<int32_t>();
	Player->HealthAccumulator = Data.Read<float>();
	Player->ManaAccumulator = Data.Read<float>();
	Player->CalculatePlayerStats();
}

// Handles the use of an inventory item
void _ClientState::HandleInventoryUse(_Buffer &Data) {
	int Slot = Data.Read<char>();
	Player->UpdateInventory(Slot, -1);
}

// Handles a chat message
void _ClientState::HandleChatMessage(_Buffer &Data) {

	// Read packet
	_ChatMessage Chat;
	Chat.Color = Data.Read<glm::vec4>();
	Chat.Message = Data.ReadString();
	Chat.Time = ClientTime;

	HUD->AddChatMessage(Chat);
}

// Handles a trade request
void _ClientState::HandleTradeRequest(_Buffer &Data) {

	// Read packet
	NetworkIDType NetworkID = Data.Read<NetworkIDType>();

	// Get trading player
	Player->TradePlayer = (_Object *)ObjectManager->GetObjectFromNetworkID(NetworkID);
	if(!Player->TradePlayer)
		return;

	// Get gold offer
	Player->TradePlayer->TradeGold = Data.Read<int32_t>();
	for(int i = _Object::INVENTORY_TRADE; i < _Object::INVENTORY_COUNT; i++) {
		int ItemID = Data.Read<int32_t>();
		int Count = 0;
		if(ItemID != 0)
			Count = Data.Read<char>();

		Player->TradePlayer->SetInventory(i, ItemID, Count);
	}
}

// Handles a trade cancel
void _ClientState::HandleTradeCancel(_Buffer &Data) {
	Player->TradePlayer = nullptr;

	// Reset agreement
	HUD->ResetAcceptButton();
}

// Handles a trade item update
void _ClientState::HandleTradeItem(_Buffer &Data) {

	// Get trading player
	if(!Player->TradePlayer)
		return;

	// Get old slot information
	int OldItemID = Data.Read<int32_t>();
	int OldSlot = Data.Read<char>();
	int OldCount = 0;
	if(OldItemID > 0)
		OldCount = Data.Read<char>();

	// Get new slot information
	int NewItemID = Data.Read<int32_t>();
	int NewSlot = Data.Read<char>();
	int NewCount = 0;
	if(NewItemID > 0)
		NewCount = Data.Read<char>();

	// Update player
	Player->TradePlayer->SetInventory(OldSlot, OldItemID, OldCount);
	Player->TradePlayer->SetInventory(NewSlot, NewItemID, NewCount);

	// Reset agreement
	Player->TradePlayer->TradeAccepted = false;
	HUD->ResetAcceptButton();
}

// Handles a gold update from the trading player
void _ClientState::HandleTradeGold(_Buffer &Data) {

	// Get trading player
	if(!Player->TradePlayer)
		return;

	// Set gold
	int Gold = Data.Read<int32_t>();
	Player->TradePlayer->TradeGold = Gold;

	// Reset agreement
	Player->TradePlayer->TradeAccepted = false;
	HUD->ResetAcceptButton();
}

// Handles a trade accept
void _ClientState::HandleTradeAccept(_Buffer &Data) {

	// Get trading player
	if(!Player->TradePlayer)
		return;

	// Set state
	bool Accepted = !!Data.Read<char>();
	HUD->UpdateTradeStatus(Accepted);
}

// Handles a trade exchange
void _ClientState::HandleTradeExchange(_Buffer &Data) {

	// Get gold offer
	int Gold = Data.Read<int32_t>();
	Player->Gold = Gold;
	for(int i = _Object::INVENTORY_TRADE; i < _Object::INVENTORY_COUNT; i++) {
		int ItemID = Data.Read<int32_t>();
		int Count = 0;
		if(ItemID != 0)
			Count = Data.Read<char>();

		Player->SetInventory(i, ItemID, Count);
	}

	// Move traded items to backpack
	Player->MoveTradeToInventory();

	// Close window
	HUD->CloseTrade(false);
}

// Requests an attack to another a player
void _ClientState::SendAttackPlayer() {
	if(Player->CanAttackPlayer()) {
		Player->ResetAttackPlayerTime();
		_Buffer Packet;
		Packet.Write<char>(Packet::WORLD_ATTACKPLAYER);
		Network->SendPacket(Packet);
	}
}
*/