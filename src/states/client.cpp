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
#include <instances/battle.h>
#include <ui/element.h>
#include <constants.h>
#include <framework.h>
#include <graphics.h>
#include <menu.h>
#include <camera.h>
#include <scripting.h>
#include <assets.h>
#include <stats.h>
#include <hud.h>
#include <program.h>
#include <config.h>
#include <actions.h>
#include <buffer.h>
#include <server.h>
#include <packet.h>
#include <log.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <sstream>

_ClientState ClientState;

// Constructor
_ClientState::_ClientState() :
	IsTesting(false),
	FromEditor(false),
	ConnectNow(false),
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
	Battle = nullptr;
	Time = 0.0;
	Clock = 0.0;

	HUD = new _HUD();
	Stats = new _Stats();
	Camera = new _Camera(glm::vec3(0, 0, CAMERA_DISTANCE), CAMERA_DIVISOR);
	Camera->CalculateFrustum(Graphics.AspectRatio);

	Scripting = new _Scripting();
	Scripting->LoadScript(SCRIPTS_PATH + SCRIPTS_SKILLS);

	Network = new _ClientNetwork();
	Network->SetFakeLag(Config.FakeLag);
	Network->SetUpdatePeriod(Config.NetworkRate);

	Graphics.ChangeViewport(Graphics.WindowSize);

	Actions.ResetState();

	if(ConnectNow)
		Menu.InitConnect(true);
	else
		Menu.InitTitle();
}

// Close map
void _ClientState::Close() {
	Menu.Close();

	delete Scripting;
	delete Battle;
	delete Camera;
	delete HUD;
	delete Map;
	delete Network;
	delete Server;
	delete Stats;
}

// Connect to a server
void _ClientState::Connect(bool IsLocal) {
	if(Network->GetConnectionState() != _ClientNetwork::State::DISCONNECTED)
		return;

	// Start a local server
	if(IsLocal) {
		StartLocalServer();
		if(Server)
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
	try {
		Server = new _Server(DEFAULT_NETWORKPORT_ALT);
		Server->Stats = Stats;
		Server->StartThread();
	}
	catch(std::exception &Error) {
		Menu.SetTitleMessage(Error.what());
	}
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

	// Check for player
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

	// Battle
	if(Battle) {
		switch(Action) {
			case _Actions::MENU:
				HUD->ToggleInGameMenu();
			break;
			default: {
				bool BattleFinished = Battle->ClientHandleInput(Action);
				if(BattleFinished) {
					delete Battle;
					Player->Battle = Battle = nullptr;
				}
			} break;
		}
	}
	else {

		// Currently typing
		if(FocusedElement != nullptr) {
			if(Action == _Actions::MENU)
				FocusedElement = nullptr;
		}
		else {

			// Handle HUD keys
			switch(Action) {
				case _Actions::SKILL1:
				case _Actions::SKILL2:
				case _Actions::SKILL3:
				case _Actions::SKILL4:
				case _Actions::SKILL5:
				case _Actions::SKILL6:
				case _Actions::SKILL7:
				case _Actions::SKILL8:
					SendActionBarUse((uint8_t)(Action - _Actions::SKILL1));
				break;
				case _Actions::MENU:
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
				case _Actions::UP:
				case _Actions::DOWN:
				case _Actions::LEFT:
				case _Actions::RIGHT:
					if(!Player->WaitForServer)
						HUD->CloseWindows();
				break;
			}
		}
	}

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

	// Toggle network stats
	if(KeyEvent.Pressed && KeyEvent.Scancode == SDL_SCANCODE_F2)
		HUD->ShowStats = !HUD->ShowStats;
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
	//if(Graphics.Element->HitElement)
		//std::cout << Graphics.Element->HitElement->Identifier << std::endl;

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

	// Check for objects
	if(!Player || !Map)
		return;

	// Set input
	if(Player->AcceptingMoveInput() && !HUD->IsChatting() && FocusedElement == nullptr && Menu.State == _Menu::STATE_NONE) {
		Player->InputState = 0;
		if(Actions.GetState(_Actions::UP) > 0.0f)
			Player->InputState |= _Object::MOVE_UP;
		if(Actions.GetState(_Actions::DOWN) > 0.0f)
			Player->InputState |= _Object::MOVE_DOWN;
		if(Actions.GetState(_Actions::LEFT) > 0.0f)
			Player->InputState |= _Object::MOVE_LEFT;
		if(Actions.GetState(_Actions::RIGHT) > 0.0f)
			Player->InputState |= _Object::MOVE_RIGHT;
	}

	// Update objects
	Map->Clock = Clock;
	Map->Update(FrameTime);
	if(Player->Moved) {
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_MOVECOMMAND);
		Packet.Write<char>(Player->Moved);
		Network->SendPacket(Packet);

		if(!Player->WaitForServer)
			HUD->CloseWindows();
	}

	// Update battle system
	if(Battle)
		Battle->Update(FrameTime);

	// Update camera
	Camera->Set2DPosition(glm::vec2(Player->Position) + glm::vec2(0.5f, 0.5f));
	Camera->Update(FrameTime);

	// Update the HUD
	HUD->Update(FrameTime);

	Time += FrameTime;

	// Update clock
	Clock += FrameTime * MAP_CLOCK_SPEED;
	if(Clock >= MAP_DAY_LENGTH)
		Clock -= MAP_DAY_LENGTH;
}

// Render the state
void _ClientState::Render(double BlendFactor) {

	// Render in game
	if(Player && Map) {
		Graphics.Setup3D();
		Camera->Set3DProjection(BlendFactor);

		// Setup the viewing matrix
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
		Graphics.SetStaticUniforms();
		Graphics.SetProgram(Assets.Programs["text"]);
		glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Graphics.Ortho));

		// Draw states
		if(Battle)
			Battle->Render(BlendFactor);

		// Draw HUD
		HUD->Render(Time);
	}

	// Draw menu
	Menu.Render();
}

// Handle packet from server
void _ClientState::HandlePacket(_Buffer &Data) {
	PacketType Type = Data.Read<PacketType>();

	switch(Type) {
		case PacketType::WORLD_YOURCHARACTERINFO:
			HandleYourCharacterInfo(Data);
		break;
		case PacketType::WORLD_CHANGEMAPS:
			HandleChangeMaps(Data);
		break;
		case PacketType::WORLD_OBJECTLIST:
			HandleObjectList(Data);
		break;
		case PacketType::WORLD_CREATEOBJECT:
			HandleCreateObject(Data);
		break;
		case PacketType::WORLD_DELETEOBJECT:
			HandleDeleteObject(Data);
		break;
		case PacketType::WORLD_OBJECTUPDATES:
			HandleObjectUpdates(Data);
		break;
		case PacketType::WORLD_POSITION:
			HandlePlayerPosition(Data);
		break;
		case PacketType::WORLD_TELEPORTSTART:
			HandleTeleportStart(Data);
		break;
		case PacketType::EVENT_START:
			HandleEventStart(Data);
		break;
		case PacketType::CHAT_MESSAGE:
			HandleChatMessage(Data);
		break;
		case PacketType::INVENTORY_USE:
			HandleInventoryUse(Data);
		break;
		case PacketType::TRADE_REQUEST:
			HandleTradeRequest(Data);
		break;
		case PacketType::TRADE_CANCEL:
			HandleTradeCancel(Data);
		break;
		case PacketType::TRADE_ITEM:
			HandleTradeItem(Data);
		break;
		case PacketType::TRADE_GOLD:
			HandleTradeGold(Data);
		break;
		case PacketType::TRADE_ACCEPT:
			HandleTradeAccept(Data);
		break;
		case PacketType::TRADE_EXCHANGE:
			HandleTradeExchange(Data);
		break;
		case PacketType::BATTLE_START:
			HandleBattleStart(Data);
		break;
		case PacketType::BATTLE_ACTION:
			HandleBattleAction(Data);
		break;
		case PacketType::BATTLE_ACTIONRESULTS:
			HandleBattleTurnResults(Data);
		break;
		case PacketType::BATTLE_END:
			HandleBattleEnd(Data);
		break;
		case PacketType::WORLD_HUD:
			HandleHUD(Data);
		break;
		default:
			Menu.HandlePacket(Data, Type);
		break;
	}
}

// Handle connection to server
void _ClientState::HandleConnect() {

	if(Server) {
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::ACCOUNT_LOGININFO);
		Packet.WriteBit(0);
		Packet.WriteString("choria_singleplayer");
		Packet.WriteString("choria_singleplayer");
		Network->SendPacket(Packet);
	}

	Menu.HandleConnect();
}

// Handle disconnects
void _ClientState::HandleDisconnect() {
	Menu.HandleDisconnect(Server != nullptr);
	ClientState.StopLocalServer();
	if(Player) {
		if(Battle) {
			delete Battle;
			Battle = nullptr;
		}
		if(Map) {
			delete Map;
			Map = nullptr;
		}
		Player = nullptr;
		HUD->SetPlayer(nullptr);
	}
}

// Called once to synchronize your stats with the servers
void _ClientState::HandleYourCharacterInfo(_Buffer &Data) {
	if(!Player)
		return;

	//Log << "HandleYourCharacterInfo" << std::endl;

	Player->WorldTexture = Assets.Textures["players/basic.png"];
	Player->Name = Data.ReadString();
	Player->PortraitID = Data.Read<uint32_t>();
	Player->Experience = Data.Read<int32_t>();
	Player->Gold = Data.Read<int32_t>();
	Player->PlayTime = Data.Read<int32_t>();
	Player->Deaths = Data.Read<int32_t>();
	Player->MonsterKills = Data.Read<int32_t>();
	Player->PlayerKills = Data.Read<int32_t>();
	Player->Bounty = Data.Read<int32_t>();
	Player->InvisPower = Data.Read<int32_t>();

	// Read items
	uint8_t ItemCount = Data.Read<uint8_t>();
	for(uint8_t i = 0; i < ItemCount; i++) {
		uint8_t Slot = Data.Read<uint8_t>();
		uint8_t Count = (uint8_t)Data.Read<uint8_t>();
		uint32_t ItemID = Data.Read<uint32_t>();
		Player->SetInventory(Slot, ItemID, Count);
	}

	// Read skills
	uint32_t SkillCount = Data.Read<uint32_t>();
	for(uint32_t i = 0; i < SkillCount; i++) {
		uint32_t SkillID = Data.Read<uint32_t>();
		int32_t Points = Data.Read<int32_t>();
		Player->SkillLevels[SkillID] = Points;
	}

	// Read skill bar
	uint8_t ActionBarSize = Data.Read<uint8_t>();
	Player->ActionBar.resize(ActionBarSize);
	for(size_t i = 0; i < ActionBarSize; i++)
		Player->ActionBar[i].Unserialize(Data, Stats);

	Player->RefreshActionBarCount();
	Player->CalculateSkillPoints();
	Player->CalculateStats();
	Player->RestoreHealthMana();

	HUD->SetActionBarSize(Player->ActionBar.size());
}

// Called when the player changes maps
void _ClientState::HandleChangeMaps(_Buffer &Data) {
	Menu.InitPlay();

	// Load map
	uint32_t MapID = Data.Read<uint32_t>();
	Clock = Data.Read<double>();

	// Delete old map and create new
	if(!Map || Map->ID != MapID) {
		if(Map)
			delete Map;

		Map = new _Map();
		Map->ID = MapID;
		Map->Load(Stats->GetMap(MapID)->File);
		Player = nullptr;
	}
}

// Handle object list
void _ClientState::HandleObjectList(_Buffer &Data) {

	// Read header
	NetworkIDType ClientNetworkID = Data.Read<NetworkIDType>();
	NetworkIDType ObjectCount = Data.Read<NetworkIDType>();

	// Create objects
	for(NetworkIDType i = 0; i < ObjectCount; i++) {
		NetworkIDType NetworkID = Data.Read<NetworkIDType>();

		// Create object
		_Object *Object = CreateObject(Data, NetworkID);

		// Set player pointer
		if(Object->NetworkID == ClientNetworkID) {
			Player = Object;
			HUD->SetPlayer(Player);
		}
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
		CreateObject(Data, NetworkID);
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
		if(Battle)
			Battle->RemoveFighter(Object);

		Object->Deleted = true;
	}
}

// Handles position updates from the server
void _ClientState::HandleObjectUpdates(_Buffer &Data) {
	if(!Player || !Map)
		return;

	// Check map id
	uint32_t MapID = Data.Read<uint8_t>();
	if(MapID != Map->ID)
		return;

	// Get object count
	NetworkIDType ObjectCount = Data.Read<NetworkIDType>();

	// Iterate over objects
	for(NetworkIDType i = 0; i < ObjectCount; i++) {

		// Read packet
		NetworkIDType NetworkID = Data.Read<NetworkIDType>();
		glm::ivec2 Position = Data.Read<glm::ivec2>();
		uint8_t Status = Data.Read<uint8_t>();
		int Invisible = Data.ReadBit();

		// Find object
		_Object *Object = Map->GetObjectByID(NetworkID);
		if(Object) {
			Object->Status = Status;

			if(Object == Player) {
			}
			else {
				Object->Position = Position;
				Object->InvisPower = Invisible;
			}
			Object->ServerPosition = Position;

			switch(Status) {
				case _Object::STATUS_NONE:
					Object->StatusTexture = nullptr;
				break;
				case _Object::STATUS_PAUSE:
					Object->StatusTexture = Assets.Textures["hud/pause.png"];
				break;
				case _Object::STATUS_INVENTORY:
					Object->StatusTexture = Assets.Textures["hud/bag.png"];
				break;
				case _Object::STATUS_VENDOR:
					Object->StatusTexture = Assets.Textures["hud/vendor.png"];
				break;
				case _Object::STATUS_SKILLS:
					Object->StatusTexture = Assets.Textures["hud/skills.png"];
				break;
				case _Object::STATUS_TRADE:
					Object->StatusTexture = Assets.Textures["hud/trade.png"];
				break;
				case _Object::STATUS_TRADER:
					Object->StatusTexture = Assets.Textures["hud/vendor.png"];
				break;
				case _Object::STATUS_TELEPORT:
					Object->StatusTexture = Assets.Textures["hud/teleport.png"];
				break;
				case _Object::STATUS_BATTLE:
					Object->StatusTexture = Assets.Textures["hud/battle.png"];
				break;
				default:
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
	Player->WaitForServer = false;
}

// Start teleport event
void _ClientState::HandleTeleportStart(_Buffer &Data) {
	if(!Player)
		return;

	Player->TeleportTime = Data.Read<double>();
}

// Handles the start of an event
void _ClientState::HandleEventStart(_Buffer &Data) {
	if(!Player)
		return;

	// Read packet
	uint32_t EventType = Data.Read<uint32_t>();
	uint32_t EventData = Data.Read<uint32_t>();
	Player->Position = Data.Read<glm::ivec2>();

	// Handle event
	switch(EventType) {
		case _Map::EVENT_VENDOR:
			Player->Vendor = ClientState.Stats->GetVendor(EventData);
			Player->WaitForServer = false;
			HUD->InitVendor();
		break;
		case _Map::EVENT_TRADER:
			Player->Trader = ClientState.Stats->GetTrader(EventData);
			Player->WaitForServer = false;
			HUD->InitTrader();
		break;
	}
}

// Handles a chat message
void _ClientState::HandleChatMessage(_Buffer &Data) {

	// Read packet
	_ChatMessage Chat;
	Chat.Color = Data.Read<glm::vec4>();
	Chat.Message = Data.ReadString();
	Chat.Time = Time;

	HUD->AddChatMessage(Chat);
}

// Handles the use of an inventory item
void _ClientState::HandleInventoryUse(_Buffer &Data) {
	int Slot = Data.Read<char>();
	Player->UpdateInventory(Slot, -1);
}

// Handles a trade request
void _ClientState::HandleTradeRequest(_Buffer &Data) {
	if(!Player || !Map)
		return;

	// Read packet
	NetworkIDType NetworkID = Data.Read<NetworkIDType>();

	// Get trading player
	Player->TradePlayer = Map->GetObjectByID(NetworkID);
	if(!Player->TradePlayer)
		return;

	// Get gold offer
	Player->TradePlayer->TradeGold = Data.Read<int32_t>();
	for(int i = _Object::INVENTORY_TRADE; i < _Object::INVENTORY_COUNT; i++) {
		uint32_t ItemID = Data.Read<uint32_t>();
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
	HUD->ResetTradeTheirsWindow();
}

// Handles a trade item update
void _ClientState::HandleTradeItem(_Buffer &Data) {

	// Get trading player
	if(!Player->TradePlayer)
		return;

	// Get old slot information
	uint32_t OldItemID = Data.Read<uint32_t>();
	int OldSlot = Data.Read<char>();
	int OldCount = 0;
	if(OldItemID > 0)
		OldCount = Data.Read<char>();

	// Get new slot information
	uint32_t NewItemID = Data.Read<uint32_t>();
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
		uint32_t ItemID = Data.Read<uint32_t>();
		uint8_t Count = 0;
		if(ItemID != 0)
			Count = Data.Read<uint8_t>();

		Player->SetInventory(i, ItemID, Count);
	}

	// Move traded items to backpack
	Player->MoveTradeToInventory();

	// Close window
	HUD->CloseTrade(false);
}

// Handles the start of a battle
void _ClientState::HandleBattleStart(_Buffer &Data) {

	// Already in a battle
	if(Battle)
		return;

	// Create a new battle instance
	Battle = new _Battle();
	Battle->Stats = Stats;
	Battle->Scripting = Scripting;
	Battle->ClientPlayer = Player;
	Battle->ClientNetwork = Network;

	// Get fighter count
	int FighterCount = Data.Read<uint8_t>();

	// Get fighter information
	for(int i = 0; i < FighterCount; i++) {

		// Get fighter type
		uint32_t DatabaseID = Data.Read<uint32_t>();
		uint8_t Side = Data.Read<uint8_t>();
		double TurnTimer = Data.Read<double>();

		_Object *Fighter = nullptr;
		if(DatabaseID == 0) {

			// Network ID
			NetworkIDType NetworkID = Data.Read<NetworkIDType>();
			glm::ivec2 Position = Data.Read<glm::ivec2>();

			// Player stats
			int Health = Data.Read<int32_t>();
			int MaxHealth = Data.Read<int32_t>();
			int Mana = Data.Read<int32_t>();
			int MaxMana = Data.Read<int32_t>();

			// Get player object
			Fighter = Map->GetObjectByID(NetworkID);
			if(Fighter != nullptr) {
				Fighter->InputState = 0;
				Fighter->Position = Fighter->ServerPosition = Position;
				Fighter->Health = Health;
				Fighter->MaxHealth = MaxHealth;
				Fighter->Mana = Mana;
				Fighter->MaxMana = MaxMana;
				Fighter->TurnTimer = TurnTimer;

				Battle->AddFighter(Fighter, Side);
			}
		}
		else {
			Fighter = new _Object();
			Fighter->DatabaseID = DatabaseID;
			Fighter->TurnTimer = TurnTimer;
			Stats->GetMonsterStats(DatabaseID, Fighter);

			Battle->AddFighter(Fighter, Side);
		}
	}

	// Start the battle
	HUD->CloseWindows();
	Battle->ClientStartBattle();
}

// Handles a battle action set from another player
void _ClientState::HandleBattleAction(_Buffer &Data) {
	if(!Player || !Battle)
		return;

	Battle->ClientHandlePlayerAction(Data);
}

// Handles the result of a turn in battle
void _ClientState::HandleBattleTurnResults(_Buffer &Data) {
	if(!Player || !Battle)
		return;

	Battle->ClientResolveAction(Data);
}

// Handles the end of a battle
void _ClientState::HandleBattleEnd(_Buffer &Data) {
	if(!Player || !Battle)
		return;

	Player->WaitForServer = false;
	Battle->ClientEndBattle(Data);
	if(Player->Health == 0)
		Player->WaitForServer = true;
}

// Handles HUD updates
void _ClientState::HandleHUD(_Buffer &Data) {
	if(!Player)
		return;

	Player->Experience = Data.Read<int32_t>();
	Player->Gold = Data.Read<int32_t>();
	Player->Health = Data.Read<int32_t>();
	Player->Mana = Data.Read<int32_t>();
	Player->HealthAccumulator = Data.Read<float>();
	Player->ManaAccumulator = Data.Read<float>();
	Player->CalculateStats();
}

// Creates an object from a buffer
_Object *_ClientState::CreateObject(_Buffer &Data, NetworkIDType NetworkID) {

	// Create object
	_Object *Object = new _Object();
	Object->Stats = Stats;
	Object->Map = Map;
	Object->NetworkID = NetworkID;
	Object->Unserialize(Data);

	// Add to map
	Map->AddObject(Object, Object->NetworkID);

	return Object;
}

// Send action to server outside of battle
void _ClientState::SendActionBarUse(uint8_t Slot) {
	if(!Player)
		return;

	if(Slot >= Player->ActionBar.size())
		return;

	if(!Player->ActionBar[Slot].IsSet())
		return;

	// Client prediction
	if(Player->UseActionWorld(Scripting, Slot)) {

		// Send use to server
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_ACTIONBAR_USE);
		Packet.Write<uint8_t>(Slot);
		Network->SendPacket(Packet);
	}
}

// Send status to server
void _ClientState::SendStatus(uint8_t Status) {
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::PLAYER_STATUS);
	Packet.Write<uint8_t>(Status);
	Network->SendPacket(Packet);
}

// Requests an attack to another a player
void _ClientState::SendAttackPlayer() {
	if(Player->CanAttackPlayer()) {
		Player->ResetAttackPlayerTime();
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_ATTACKPLAYER);
		Network->SendPacket(Packet);
	}
}