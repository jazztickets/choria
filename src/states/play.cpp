/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2018  Alan Witkowski
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
#include <states/play.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <objects/components/inventory.h>
#include <objects/components/record.h>
#include <objects/components/fighter.h>
#include <objects/components/controller.h>
#include <objects/statuseffect.h>
#include <objects/buff.h>
#include <objects/map.h>
#include <objects/battle.h>
#include <objects/minigame.h>
#include <ae/manager.h>
#include <ae/clientnetwork.h>
#include <ae/program.h>
#include <ae/actions.h>
#include <ae/buffer.h>
#include <ae/audio.h>
#include <ae/random.h>
#include <ae/ui.h>
#include <ae/graphics.h>
#include <ae/manager.h>
#include <ae/camera.h>
#include <ae/assets.h>
#include <ae/log.h>
#include <constants.h>
#include <framework.h>
#include <save.h>
#include <menu.h>
#include <scripting.h>
#include <stats.h>
#include <hud.h>
#include <config.h>
#include <actiontype.h>
#include <server.h>
#include <packet.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <iostream>
#include <sstream>

_PlayState PlayState;

// Constructor
_PlayState::_PlayState() :
	DevMode(false),
	IsHardcore(false),
	FromEditor(false),
	ConnectNow(false),
	Stats(nullptr),
	Server(nullptr),
	HostAddress("127.0.0.1"),
	ConnectPort(DEFAULT_NETWORKPORT) {
}

// Load level and set up objects
void _PlayState::Init() {
	Server = nullptr;
	Map = nullptr;
	Battle = nullptr;
	HUD = nullptr;
	Time = 0.0;
	IgnoreFirstChar = false;
	CoinSoundPlayed = false;

	ae::Graphics.Element->SetActive(false);
	ae::Graphics.Element->Active = true;

	Stats = new _Stats();

	Camera = new ae::_Camera(glm::vec3(0, 0, CAMERA_DISTANCE), CAMERA_DIVISOR, CAMERA_FOVY, CAMERA_NEAR, CAMERA_FAR);
	Camera->CalculateFrustum(ae::Graphics.AspectRatio);

	Scripting = new _Scripting();
	Scripting->Setup(Stats, SCRIPTS_GAME);

	HUD = new _HUD();
	HUD->Scripting = Scripting;

	Network = new ae::_ClientNetwork();
	Network->SetFakeLag(Config.FakeLag);
	Network->SetUpdatePeriod(Config.NetworkRate);

	ObjectManager = new ae::_Manager<_Object>();
	AssignPlayer(nullptr);

	if(ConnectNow)
		Menu.InitConnect(true, true);
	else
		Menu.InitTitle();
}

// Close map
void _PlayState::Close() {
	Menu.Close();

	AssignPlayer(nullptr);
	delete ObjectManager;
	DeleteBattle();
	DeleteMap();
	delete HUD;
	delete Scripting;
	delete Camera;
	delete Server;
	delete Stats;
	delete Network;
}

// Delete objects and return to menu
void _PlayState::StopGame() {
	ae::Audio.Stop();
	ObjectManager->Clear();
	AssignPlayer(nullptr);
	DeleteBattle();
	DeleteMap();
}

// Connect to a server
void _PlayState::Connect(bool IsLocal) {
	if(Network->GetConnectionState() != ae::_ClientNetwork::State::DISCONNECTED)
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
void _PlayState::StopLocalServer() {

	// Kill existing server
	if(Server) {
		Server->StopServer();
		Server->JoinThread();
		delete Server;
		Server = nullptr;
	}
}

// Start local server in thread
void _PlayState::StartLocalServer() {

	// Stop existing server
	StopLocalServer();

	// Start server in thread
	try {
		Server = new _Server(DEFAULT_NETWORKPORT_ALT);
		Server->IsTesting = DevMode;
		Server->Hardcore = IsHardcore;
		Server->StartThread();
	}
	catch(std::exception &Error) {
		Menu.SetTitleMessage(Error.what());
	}
}

// Action handler, return true to stop handling same input
bool _PlayState::HandleAction(int InputType, size_t Action, int Value) {
	if(Value == 0)
		return true;

	// Pass to menu
	if(Menu.State != _Menu::STATE_NONE)
		return Menu.HandleAction(InputType, Action, Value);

	// Check for player
	if(!Player)
		return true;

	// Handle enter key
	if(Action == Action::GAME_CHAT) {
		HUD->HandleEnter();
		return true;
	}

	// Grab all actions except escape when chatting
	if(HUD->IsChatting()) {
		if(Action == Action::MENU_BACK)
			HUD->CloseChat();

		return true;
	}

	// Grab all actions except escape when typing party
	if(HUD->IsTypingParty()) {
		if(Action == Action::MENU_BACK)
			HUD->CloseWindows(true, false);

		return true;
	}

	// Toggle debug stats
	if(Action == Action::MISC_DEBUG)
		HUD->ShowDebug = !HUD->ShowDebug;

	// Respawn
	if(!Player->Character->IsAlive()) {
		if(Action == Action::MENU_BACK || Action == Action::MENU_PAUSE) {
			HUD->ToggleInGameMenu(true);
			return true;
		}
	}

	// Battle
	if(Battle) {
		switch(Action) {
			case Action::MENU_BACK:
			case Action::MENU_PAUSE:
				HUD->ToggleInGameMenu(Action == Action::MENU_PAUSE);
			break;
			case Action::GAME_INVENTORY:
				HUD->ToggleCharacterStats();
			break;
			default: {
				if(Battle->ClientHandleInput(Action))
					HUD->EnableMouseCombat = false;
			} break;
		}
	}
	else {

		// Currently typing
		if(ae::FocusedElement != nullptr) {
			if(Action == Action::MENU_BACK)
				ae::FocusedElement = nullptr;
		}
		else {

			// Handle HUD keys
			switch(Action) {
				case Action::GAME_SKILL1:
				case Action::GAME_SKILL2:
				case Action::GAME_SKILL3:
				case Action::GAME_SKILL4:
				case Action::GAME_SKILL5:
				case Action::GAME_SKILL6:
				case Action::GAME_SKILL7:
				case Action::GAME_SKILL8:
					SendActionUse((uint8_t)(Action - Action::GAME_SKILL1));
				break;
				case Action::MENU_BACK:
				case Action::MENU_PAUSE:
					HUD->ToggleInGameMenu(Action == Action::MENU_PAUSE);
				break;
				case Action::GAME_INVENTORY:
					HUD->ToggleInventory();
				break;
				case Action::GAME_TRADE:
					HUD->ToggleTrade();
				break;
				case Action::GAME_SKILLS:
					HUD->ToggleSkills();
				break;
				case Action::GAME_JOIN:
					SendJoinRequest();
				break;
				case Action::GAME_PARTY:
					HUD->ToggleParty();
					IgnoreFirstChar = true;
				break;
				case Action::GAME_UP:
				case Action::GAME_DOWN:
				case Action::GAME_LEFT:
				case Action::GAME_RIGHT:
					if(!Player->Controller->WaitForServer)
						HUD->CloseWindows(true);
				break;
				case Action::GAME_USE:
					if(!Player->Controller->WaitForServer)
						SendUseCommand();
				break;
			}
		}
	}

	return true;
}

// Key handler
void _PlayState::HandleKey(const ae::_KeyEvent &KeyEvent) {

	// Ignore keys if opening and focusing a textbox with a hotkey
	if(IgnoreFirstChar) {
		IgnoreFirstChar = false;
		return;
	}

	bool Handled = ae::Graphics.Element->HandleKey(KeyEvent);

	// Message history handling
	if(HUD->IsChatting() && KeyEvent.Pressed) {
		if(KeyEvent.Scancode == SDL_SCANCODE_UP)
			HUD->UpdateSentHistory(-1);
		else if(KeyEvent.Scancode == SDL_SCANCODE_DOWN)
			HUD->UpdateSentHistory(1);
	}

	// Pass to menu
	if(!Handled)
		Menu.HandleKey(KeyEvent);

	if(Menu.State != _Menu::STATE_NONE)
		return;

	if(!HUD->IsChatting())
		HUD->ValidateTradeGold();
}

// Mouse handler
void _PlayState::HandleMouseButton(const ae::_MouseEvent &MouseEvent) {
	ae::FocusedElement = nullptr;
	if(MouseEvent.Button == SDL_BUTTON_LEFT)
		ae::Graphics.Element->HandleMouseButton(MouseEvent.Pressed);

	// Pass to menu
	Menu.HandleMouseButton(MouseEvent);
	if(Menu.State != _Menu::STATE_NONE)
		return;

	// Enable mouse during combat
	if(HUD && Player && Player->Character->Battle)
		HUD->EnableMouseCombat = true;

	HUD->HandleMouseButton(MouseEvent);
}

// Mouse movement handler
void _PlayState::HandleMouseMove(const glm::ivec2 &Position) {

	// Enable mouse during combat
	if(HUD && Player && Player->Character->Battle)
		HUD->EnableMouseCombat = true;
}

// Window size updates
void _PlayState::HandleWindow(uint8_t Event) {
	if(Event == SDL_WINDOWEVENT_SIZE_CHANGED) {
		if(Camera)
			Camera->CalculateFrustum(ae::Graphics.AspectRatio);

		if(HUD && HUD->Minigame && HUD->Minigame->Camera)
			HUD->Minigame->Camera->CalculateFrustum(ae::Graphics.AspectRatio);
	}
}

// Handle quit events
void _PlayState::HandleQuit() {
	if(Network && Network->CanDisconnect())
		Network->Disconnect();
	else
		Framework.Done = true;
}

// Update
void _PlayState::Update(double FrameTime) {
	CoinSoundPlayed = false;
	//if(std::abs(std::fmod(Time, 1.0)) >= 0.99)
	//	std::cout << "Client: O=" << ObjectManager->Objects.size() << " B=" << (int)(Battle != nullptr) << std::endl;

	// Update network
	Network->Update(FrameTime);

	// Get events
	ae::_NetworkEvent NetworkEvent;
	while(Network->GetNetworkEvent(NetworkEvent)) {

		switch(NetworkEvent.Type) {
			case ae::_NetworkEvent::CONNECT: {
				HandleConnect();
			} break;
			case ae::_NetworkEvent::DISCONNECT:
				HandleDisconnect();
			break;
			case ae::_NetworkEvent::PACKET:
				HandlePacket(*NetworkEvent.Data);
				delete NetworkEvent.Data;
			break;
		}
	}

	// Update UI
	ae::Graphics.Element->Update(FrameTime, ae::Input.GetMouse());
	//if(ae::Graphics.Element->HitElement)
	//	std::cout << ae::Graphics.Element->HitElement->Name << std::endl;

	// Update menu
	Menu.Update(FrameTime);

	// Check for objects
	if(!Player || !Map)
		return;

	// Set input
	if(Player->Character->AcceptingMoveInput() && !HUD->IsChatting() && ae::FocusedElement == nullptr && Menu.State == _Menu::STATE_NONE) {
		int InputState = 0;
		if(ae::Actions.State[Action::GAME_UP].Value > 0.0f)
			InputState |= _Object::MOVE_UP;
		if(ae::Actions.State[Action::GAME_DOWN].Value > 0.0f)
			InputState |= _Object::MOVE_DOWN;
		if(ae::Actions.State[Action::GAME_LEFT].Value > 0.0f)
			InputState |= _Object::MOVE_LEFT;
		if(ae::Actions.State[Action::GAME_RIGHT].Value > 0.0f)
			InputState |= _Object::MOVE_RIGHT;

		// Get player direction
		glm::ivec2 Direction(0, 0);
		Player->GetDirectionFromInput(InputState, Direction);

		// Append input state if moving
		Player->Controller->InputStates.clear();
		if(Direction.x != 0 || Direction.y != 0)
			Player->Controller->InputStates.push_back(InputState);
	}

	// Update objects
	ObjectManager->Update(FrameTime);

	// Update map
	Map->Update(FrameTime);

	// Send input to server
	if(Player->Controller->DirectionMoved) {
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_MOVECOMMAND);
		Packet.Write<char>((char)Player->Controller->DirectionMoved);
		Network->SendPacket(Packet);

		if(!Player->Controller->WaitForServer)
			HUD->CloseWindows(true);
	}

	// Update battle system
	if(Battle) {
		if(!Player->Character->Battle)
			DeleteBattle();
		else
			Battle->Update(FrameTime);
	}

	// Update camera
	Camera->Set2DPosition(glm::vec2(Player->Position) + glm::vec2(0.5f, 0.5f));
	Camera->Update(FrameTime);

	// Update the HUD
	HUD->Update(FrameTime);

	Time += FrameTime;
}

// Render the state
void _PlayState::Render(double BlendFactor) {

	// Render in game
	if(Player && Map) {
		ae::Graphics.Setup3D();
		Camera->Set3DProjection(BlendFactor);

		// Setup the viewing matrix
		ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
		glUniformMatrix4fv(ae::Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
		ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
		glUniformMatrix4fv(ae::Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
		ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv_static"]);
		glUniformMatrix4fv(ae::Assets.Programs["pos_uv_static"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
		ae::Graphics.SetProgram(ae::Assets.Programs["text"]);
		glUniformMatrix4fv(ae::Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

		// Draw map and objects
		Map->Render(Camera, Player, BlendFactor);

		ae::Graphics.Setup2D();
		ae::Graphics.SetStaticUniforms();
		ae::Graphics.SetProgram(ae::Assets.Programs["text"]);
		glUniformMatrix4fv(ae::Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(ae::Graphics.Ortho));

		HUD->DrawRecentItems();

		// Draw states
		if(Battle)
			Battle->Render(BlendFactor);

		// Draw HUD
		HUD->Render(Map, BlendFactor, Time);
	}

	// Draw menu
	Menu.Render();
}

// Play coin sound
void _PlayState::PlayCoinSound() {
	if(CoinSoundPlayed)
		return;

	std::stringstream Buffer;
	Buffer << "coin" << ae::GetRandomInt(0, 2) << ".ogg";
	ae::Audio.PlaySound(ae::Assets.Sounds[Buffer.str()]);
	CoinSoundPlayed = true;
}

// Play death sound
void _PlayState::PlayDeathSound() {
	std::stringstream Buffer;
	Buffer << "death" << ae::GetRandomInt(0, 2) << ".ogg";
	ae::Audio.PlaySound(ae::Assets.Sounds[Buffer.str()]);
}

// Handle connection to server
void _PlayState::HandleConnect() {

	if(Server) {
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::ACCOUNT_LOGININFO);
		Packet.WriteBit(0);
		if(DevMode && ae::Input.ModKeyDown(KMOD_CTRL))
			Packet.WriteString("bots");
		else
			Packet.WriteString("singleplayer");
		Packet.WriteString("");
		Packet.Write<uint64_t>(Server->Save->Secret);
		Network->SendPacket(Packet);
	}

	Menu.HandleConnect();
}

// Handle disconnects
void _PlayState::HandleDisconnect() {
	Menu.HandleDisconnect(Server != nullptr);
	PlayState.StopLocalServer();

	HUD->Reset();
	ObjectManager->Clear();
	AssignPlayer(nullptr);

	DeleteBattle();
	DeleteMap();
}

// Handle packet from server
void _PlayState::HandlePacket(ae::_Buffer &Data) {
	PacketType Type = Data.Read<PacketType>();

	switch(Type) {
		case PacketType::OBJECT_STATS:
			HandleObjectStats(Data);
		break;
		case PacketType::WORLD_CLOCK:
			HandleClock(Data);
		break;
		case PacketType::WORLD_CHANGEMAPS:
			HandleChangeMaps(Data);
		break;
		case PacketType::WORLD_OBJECTLIST:
			HandleObjectList(Data);
		break;
		case PacketType::WORLD_CREATEOBJECT:
			HandleObjectCreate(Data);
		break;
		case PacketType::WORLD_DELETEOBJECT:
			HandleObjectDelete(Data);
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
		case PacketType::PARTY_INFO:
			HandlePartyInfo(Data);
		break;
		case PacketType::CHAT_MESSAGE:
			HandleChatMessage(Data);
		break;
		case PacketType::INVENTORY:
			HandleInventory(Data);
		break;
		case PacketType::INVENTORY_ADD:
			HandleInventoryAdd(Data);
		break;
		case PacketType::INVENTORY_SWAP:
			HandleInventorySwap(Data);
		break;
		case PacketType::INVENTORY_UPDATE:
			HandleInventoryUpdate(Data);
		break;
		case PacketType::INVENTORY_GOLD:
			HandleInventoryGold(Data);
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
		case PacketType::BATTLE_JOIN:
			HandleBattleJoin(Data);
		break;
		case PacketType::BATTLE_LEAVE:
			HandleBattleLeave(Data);
		break;
		case PacketType::BATTLE_END:
			HandleBattleEnd(Data);
		break;
		case PacketType::ACTION_CLEAR:
			HandleActionClear(Data);
		break;
		case PacketType::ACTION_RESULTS:
			HandleActionResults(Data);
		break;
		case PacketType::STAT_CHANGE: {
			_StatChange StatChange;
			HandleStatChange(Data, StatChange);
		} break;
		case PacketType::WORLD_HUD:
			HandleHUD(Data);
		break;
		case PacketType::MINIGAME_SEED:
			HandleMinigameSeed(Data);
		break;
		default:
			Menu.HandlePacket(Data, Type);
		break;
	}
}

// Called once to synchronize your stats with the servers
void _PlayState::HandleObjectStats(ae::_Buffer &Data) {
	if(!Player)
		return;

	Player->UnserializeStats(Data);

	HUD->UpdateLabels();
	HUD->SetActionBarSize(Player->Character->ActionBar.size());
}

// Handle world clock change
void _PlayState::HandleClock(ae::_Buffer &Data) {
	double Clock = Data.Read<float>();

	if(Map)
		Map->Clock = Clock;
}

// Called when the player changes maps
void _PlayState::HandleChangeMaps(ae::_Buffer &Data) {
	Menu.InitPlay();

	// Load map
	ae::NetworkIDType MapID = (ae::NetworkIDType)Data.Read<uint32_t>();
	double Clock = Data.Read<double>();

	// Delete old map and create new
	if(!Map || Map->NetworkID != MapID) {
		if(Map)
			DeleteMap();

		Map = new _Map();
		Map->Stats = Stats;
		Map->UseAtlas = true;
		Map->Clock = Clock;
		Map->NetworkID = MapID;
		Map->Load(&Stats->Maps.at(MapID));
		AssignPlayer(nullptr);

		ae::Audio.PlayMusic(ae::Assets.Music[Map->Music]);
	}
}

// Handle object list
void _PlayState::HandleObjectList(ae::_Buffer &Data) {
	ObjectManager->Clear();
	AssignPlayer(nullptr);

	// Read header
	ae::NetworkIDType ClientNetworkID = Data.Read<ae::NetworkIDType>();
	ae::NetworkIDType ObjectCount = Data.Read<ae::NetworkIDType>();

	// Create objects
	for(ae::NetworkIDType i = 0; i < ObjectCount; i++) {
		ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();

		// Create object
		_Object *Object = CreateObject(Data, NetworkID);

		// Set player pointer
		if(Object->NetworkID == ClientNetworkID)
			AssignPlayer(Object);
		else
			Object->Character->CalcLevelStats = false;
	}

	if(Player) {
		Camera->ForcePosition(glm::vec3(Player->Position, CAMERA_DISTANCE) + glm::vec3(0.5, 0.5, 0));
	}
	else {
		// Error
	}
}

// Creates an object
void _PlayState::HandleObjectCreate(ae::_Buffer &Data) {
	if(!Map || !Player)
		return;

	// Read packet
	ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();

	// Check id
	if(NetworkID != Player->NetworkID) {

		// Create object
		CreateObject(Data, NetworkID);
	}
}

// Deletes an object
void _PlayState::HandleObjectDelete(ae::_Buffer &Data) {
	if(!Player)
		return;

	ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();

	// Get object
	_Object *Object = ObjectManager->GetObject(NetworkID);
	if(Object && Object != Player) {
		Object->Deleted = true;
	}
}

// Handles position updates from the server
void _PlayState::HandleObjectUpdates(ae::_Buffer &Data) {
	if(!Player || !Map)
		return;

	// Check map id
	ae::NetworkIDType MapID = Data.Read<uint8_t>();
	if(MapID != Map->NetworkID)
		return;

	// Get object count
	ae::NetworkIDType ObjectCount = Data.Read<ae::NetworkIDType>();

	// Iterate over objects
	for(ae::NetworkIDType i = 0; i < ObjectCount; i++) {

		// Read packet
		ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();
		glm::ivec2 Position = Data.Read<glm::ivec2>();
		uint8_t Status = Data.Read<uint8_t>();
		int Light = Data.ReadBit();
		int Invisible = Data.ReadBit();
		int Bounty = Data.ReadBit();
		if(Bounty)
			Bounty = Data.Read<int>();
		if(Light)
			Light = Data.Read<uint8_t>();

		// Find object
		_Object *Object = ObjectManager->GetObject(NetworkID);
		if(Object) {
			Object->Character->Status = Status;

			if(Object == Player) {
			}
			else {
				Object->Position = Position;
				Object->Character->Invisible = Invisible;
				Object->Record->Bounty = Bounty;
			}
			Object->Light = Light;
			Object->ServerPosition = Position;

			switch(Status) {
				case _Character::STATUS_NONE:
					Object->Character->StatusTexture = nullptr;
				break;
				case _Character::STATUS_MENU:
					Object->Character->StatusTexture = ae::Assets.Textures["textures/status/pause.png"];
				break;
				case _Character::STATUS_INVENTORY:
					Object->Character->StatusTexture = ae::Assets.Textures["textures/status/bag.png"];
				break;
				case _Character::STATUS_VENDOR:
					Object->Character->StatusTexture = ae::Assets.Textures["textures/status/vendor.png"];
				break;
				case _Character::STATUS_SKILLS:
					Object->Character->StatusTexture = ae::Assets.Textures["textures/status/skills.png"];
				break;
				case _Character::STATUS_TRADE:
					Object->Character->StatusTexture = ae::Assets.Textures["textures/status/trade.png"];
				break;
				case _Character::STATUS_TRADER:
					Object->Character->StatusTexture = ae::Assets.Textures["textures/status/vendor.png"];
				break;
				case _Character::STATUS_BLACKSMITH:
					Object->Character->StatusTexture = ae::Assets.Textures["textures/status/vendor.png"];
				break;
				case _Character::STATUS_MINIGAME:
					Object->Character->StatusTexture = ae::Assets.Textures["textures/status/vendor.png"];
				break;
				case _Character::STATUS_TELEPORT:
					Object->Character->StatusTexture = ae::Assets.Textures["textures/status/teleport.png"];
				break;
				case _Character::STATUS_BATTLE:
					Object->Character->StatusTexture = ae::Assets.Textures["textures/status/battle.png"];
				break;
				case _Character::STATUS_DEAD:
					Object->Character->StatusTexture = ae::Assets.Textures["textures/status/dead.png"];
				break;
				default:
				break;
			}
		}
	}
}

// Handles player position
void _PlayState::HandlePlayerPosition(ae::_Buffer &Data) {
	if(!Player)
		return;

	Player->Position = Data.Read<glm::ivec2>();
	Player->Controller->WaitForServer = false;
	Player->Character->TeleportTime = -1;
	HUD->StopTeleport();
}

// Start teleport event
void _PlayState::HandleTeleportStart(ae::_Buffer &Data) {
	if(!Player)
		return;

	Player->Character->TeleportTime = Data.Read<double>();
	Player->Controller->WaitForServer = true;
	HUD->CloseWindows(false);
	HUD->StartTeleport();

	ae::Audio.PlaySound(ae::Assets.Sounds["teleport0.ogg"]);
}

// Handles the start of an event
void _PlayState::HandleEventStart(ae::_Buffer &Data) {
	if(!Player)
		return;

	// Read packet
	uint32_t EventType = Data.Read<uint32_t>();
	uint32_t EventData = Data.Read<uint32_t>();
	Player->Position = Data.Read<glm::ivec2>();

	// Handle event
	switch(EventType) {
		case _Map::EVENT_VENDOR:
			Player->Character->Vendor = &Stats->Vendors.at(EventData);
			Player->Controller->WaitForServer = false;
			HUD->InitVendor();
		break;
		case _Map::EVENT_TRADER:
			Player->Character->Trader = &Stats->Traders.at(EventData);
			Player->Controller->WaitForServer = false;
			HUD->InitTrader();
		break;
		case _Map::EVENT_BLACKSMITH:
			Player->Character->Blacksmith = &Stats->Blacksmiths.at(EventData);
			Player->Controller->WaitForServer = false;
			HUD->InitBlacksmith();
		break;
		case _Map::EVENT_MINIGAME:
			Player->Character->Minigame = &Stats->Minigames.at(EventData);
			Player->Controller->WaitForServer = false;
			HUD->InitMinigame();
		break;
	}
}

// Handle inventory sync
void _PlayState::HandleInventory(ae::_Buffer &Data) {
	if(!Player)
		return;

	Player->Inventory->Unserialize(Data, Stats);
	Player->Character->CalculateStats();

	// Refresh trader screen
	if(Player->Character->Trader) {
		PlayCoinSound();
		HUD->InitTrader();

		// Update recent items
		if(HUD->RecentItems.size() && HUD->RecentItems.back().Item == Player->Character->Trader->RewardItem) {
			HUD->RecentItems.back().Count += Player->Character->Trader->Count;
			HUD->RecentItems.back().Time = 0.0;
		}
		else {
			_RecentItem RecentItem;
			RecentItem.Item = Player->Character->Trader->RewardItem;
			RecentItem.Count = Player->Character->Trader->Count;

			HUD->RecentItems.push_back(RecentItem);
		}
	}
}

// Handle an item being added to the inventory
void _PlayState::HandleInventoryAdd(ae::_Buffer &Data){
	if(!Player)
		return;

	_RecentItem RecentItem;
	RecentItem.Count = (int)Data.Read<uint8_t>();
	RecentItem.Item = Stats->Items.at(Data.Read<uint32_t>());
	HUD->RecentItems.push_back(RecentItem);

	Player->Inventory->AddItem(RecentItem.Item, 0, RecentItem.Count);
}

// Handles a chat message
void _PlayState::HandleChatMessage(ae::_Buffer &Data) {

	// Read packet
	_Message Chat;
	Chat.Color = ae::Assets.Colors[Data.ReadString()];
	Chat.Message = Data.ReadString();
	Chat.Time = Time;

	HUD->AddChatMessage(Chat);
}

// Handles a inventory swap
void _PlayState::HandleInventorySwap(ae::_Buffer &Data) {
	if(!Player)
		return;

	Player->Inventory->UnserializeSlot(Data, Stats);
	Player->Inventory->UnserializeSlot(Data, Stats);

	HUD->ResetAcceptButton();
	Player->Character->CalculateStats();
}

// Handle an inventory update
void _PlayState::HandleInventoryUpdate(ae::_Buffer &Data) {
	if(!Player)
		return;

	uint8_t Count = Data.Read<uint8_t>();
	for(uint8_t i = 0; i < Count; i++)
		Player->Inventory->UnserializeSlot(Data, Stats);

	Player->Character->CalculateStats();
}

// Handle gold update
void _PlayState::HandleInventoryGold(ae::_Buffer &Data) {
	if(!Player)
		return;

	Player->Character->Gold = Data.Read<int>();
	Player->Character->CalculateStats();

	PlayCoinSound();
}

// Handle party info
void _PlayState::HandlePartyInfo(ae::_Buffer &Data) {
	if(!Player)
		return;

	Player->Character->PartyName = Data.ReadString();
	HUD->UpdateLabels();
}

// Handles a trade request
void _PlayState::HandleTradeRequest(ae::_Buffer &Data) {
	if(!Player || !Map)
		return;

	// Read packet
	ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();

	// Get trading player
	Player->Character->TradePlayer = ObjectManager->GetObject(NetworkID);
	if(!Player->Character->TradePlayer)
		return;

	// Get gold offer
	Player->Character->TradePlayer->Character->TradeGold = Data.Read<int>();
	for(size_t i = 0; i < PLAYER_TRADEITEMS; i++)
		Player->Character->TradePlayer->Inventory->UnserializeSlot(Data, Stats);
}

// Handles a trade cancel
void _PlayState::HandleTradeCancel(ae::_Buffer &Data) {
	Player->Character->TradePlayer = nullptr;

	// Reset agreement
	HUD->ResetAcceptButton();
	HUD->ResetTradeTheirsWindow();
}

// Handles a trade item update
void _PlayState::HandleTradeItem(ae::_Buffer &Data) {

	// Get trading player
	if(!Player->Character->TradePlayer)
		return;

	// Get slot updates
	Player->Character->TradePlayer->Inventory->UnserializeSlot(Data, Stats);
	Player->Character->TradePlayer->Inventory->UnserializeSlot(Data, Stats);

	// Reset agreement
	Player->Character->TradePlayer->Character->TradeAccepted = false;
	HUD->ResetAcceptButton();
}

// Handles a gold update from the trading player
void _PlayState::HandleTradeGold(ae::_Buffer &Data) {

	// Get trading player
	if(!Player->Character->TradePlayer)
		return;

	// Set gold
	int Gold = Data.Read<int>();
	Player->Character->TradePlayer->Character->TradeGold = Gold;

	// Reset agreement
	Player->Character->TradePlayer->Character->TradeAccepted = false;
	HUD->ResetAcceptButton();
}

// Handles a trade accept
void _PlayState::HandleTradeAccept(ae::_Buffer &Data) {

	// Get trading player
	if(!Player->Character->TradePlayer)
		return;

	// Set state
	bool Accepted = !!Data.Read<char>();
	HUD->UpdateTradeStatus(Accepted);
}

// Handles a trade exchange
void _PlayState::HandleTradeExchange(ae::_Buffer &Data) {
	if(!Player)
		return;

	// Get gold offer
	Player->Character->Gold = Data.Read<int>();
	Player->Inventory->Unserialize(Data, Stats);
	Player->Character->CalculateStats();

	// Close window
	HUD->CloseTrade(false);
}

// Handles the start of a battle
void _PlayState::HandleBattleStart(ae::_Buffer &Data) {

	// Already in a battle
	if(Battle)
		return;

	// Allow player to hit menu buttons
	Player->Controller->WaitForServer = false;

	// Reset hud
	HUD->CloseWindows(true);
	HUD->EnableMouseCombat = false;
	if(Config.ShowTutorial && Player->Character->Level == 1)
		HUD->SetMessage("Hit the " + ae::Actions.GetInputNameForAction(Action::GAME_SKILL1) + " key to attack");

	// Create a new battle instance
	Battle = new _Battle();
	Battle->Manager = ObjectManager;
	Battle->Stats = Stats;
	Battle->Scripting = Scripting;
	Battle->ClientPlayer = Player;
	Battle->ClientNetwork = Network;

	Battle->Unserialize(Data, HUD);
}

// Handles a battle action set from another player
void _PlayState::HandleBattleAction(ae::_Buffer &Data) {
	if(!Player || !Battle)
		return;

	Battle->ClientHandlePlayerAction(Data);
}

// Handle an object joining the battle
void _PlayState::HandleBattleJoin(ae::_Buffer &Data) {
	if(!Player || !Battle)
		return;

	// Read header
	ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();
	uint32_t DatabaseID = Data.Read<uint32_t>();

	// Get object
	_Object *Object = ObjectManager->GetObject(NetworkID);
	if(Object) {
		if(DatabaseID)
			Stats->GetMonsterStats(DatabaseID, Object);
		Object->UnserializeBattle(Data, Player == Object);
		Battle->AddObject(Object, Object->Fighter->BattleSide, true);
	}
}

// Handle an object leaving battle
void _PlayState::HandleBattleLeave(ae::_Buffer &Data) {
	if(!Player || !Battle)
		return;

	ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();
	_Object *Object = ObjectManager->GetObject(NetworkID);
	if(Object) {
		Battle->RemoveObject(Object);
	}
}

// Handles the end of a battle
void _PlayState::HandleBattleEnd(ae::_Buffer &Data) {
	if(!Player || !Battle)
		return;

	HUD->SetMessage("");
	HUD->CloseWindows(false);

	Player->Controller->WaitForServer = false;

	_StatChange StatChange;
	StatChange.Object = Player;

	// Get ending stats
	Player->Record->PlayerKills = Data.Read<int>();
	Player->Record->MonsterKills = Data.Read<int>();
	Player->Record->GoldLost = Data.Read<int>();
	Player->Record->Bounty = Data.Read<int>();
	StatChange.Values[StatType::EXPERIENCE].Integer = Data.Read<int>();
	StatChange.Values[StatType::GOLD].Integer = Data.Read<int>();
	uint8_t ItemCount = Data.Read<uint8_t>();
	for(uint8_t i = 0; i < ItemCount; i++) {
		_RecentItem RecentItem;

		uint32_t ItemID = Data.Read<uint32_t>();
		RecentItem.Item = Stats->Items.at(ItemID);
		int Upgrades = (int)Data.Read<uint8_t>();
		RecentItem.Count = (int)Data.Read<uint8_t>();

		// Add items
		HUD->RecentItems.push_back(RecentItem);
		Player->Inventory->AddItem(RecentItem.Item, Upgrades, RecentItem.Count);
	}

	// Update client death count
	if(!Player->Character->IsAlive()) {
		Player->Record->Deaths++;
		PlayDeathSound();
	}

	Player->Character->Battle = nullptr;
	HUD->ClearBattleStatChanges();
	HUD->AddStatChange(StatChange);

	DeleteBattle();
}

// Clear action used and targets
void _PlayState::HandleActionClear(ae::_Buffer &Data) {
	ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();
	_Object *Object = ObjectManager->GetObject(NetworkID);
	if(!Object)
		return;

	Object->Character->Action.Unset();
	Object->Character->Targets.clear();
}

// Handles the result of a turn in battle
void _PlayState::HandleActionResults(ae::_Buffer &Data) {
	if(!Player)
		return;

	// Create result
	_ActionResult ActionResult;
	bool DecrementItem = Data.ReadBit();
	bool SkillUnlocked = Data.ReadBit();
	bool ItemUnlocked = Data.ReadBit();
	uint32_t ItemID = Data.Read<uint32_t>();
	int InventorySlot = (int)Data.Read<char>();
	ActionResult.ActionUsed.Item = Stats->Items.at(ItemID);

	// Set texture
	if(ActionResult.ActionUsed.Item)
		ActionResult.Texture = ActionResult.ActionUsed.Item->Texture;

	// Get source change
	HandleStatChange(Data, ActionResult.Source);

	// Update source object
	if(ActionResult.Source.Object) {
		ActionResult.Source.Object->Fighter->TurnTimer = 0.0;
		ActionResult.Source.Object->Character->Action.Unset();
		ActionResult.Source.Object->Character->Targets.clear();

		// Use item on client
		if(Player == ActionResult.Source.Object) {
			if(ActionResult.ActionUsed.Item) {

				if(DecrementItem) {
					size_t Index;
					if(Player->Inventory->FindItem(ActionResult.ActionUsed.Item, Index, (size_t)InventorySlot)) {
						Player->Inventory->UpdateItemCount(_Slot(_Bag::BagType::INVENTORY, Index), -1);
						Player->Character->RefreshActionBarCount();
					}
				}

				if(SkillUnlocked) {
					Player->Character->Skills[ActionResult.ActionUsed.Item->ID] = 0;
				}

				if(ItemUnlocked) {
					Player->Character->Unlocks[ActionResult.ActionUsed.Item->UnlockID].Level = 1;
				}
			}
		}
	}

	// Update targets
	uint8_t TargetCount = Data.Read<uint8_t>();
	for(uint8_t i = 0; i < TargetCount; i++) {
		HandleStatChange(Data, ActionResult.Source);
		HandleStatChange(Data, ActionResult.Target);

		if(Battle) {

			// No damage dealt
			if((ActionResult.ActionUsed.GetTargetType() == TargetType::ENEMY || ActionResult.ActionUsed.GetTargetType() == TargetType::ENEMY_ALL)
			   && ((ActionResult.Target.HasStat(StatType::HEALTH) && ActionResult.Target.Values[StatType::HEALTH].Integer == 0) || ActionResult.Target.HasStat(StatType::MISS))) {
				ActionResult.Timeout = HUD_ACTIONRESULT_TIMEOUT_SHORT;
				ActionResult.Speed = HUD_ACTIONRESULT_SPEED_SHORT;

				if(ActionResult.Target.HasStat(StatType::MISS)) {
					std::stringstream Buffer;
					Buffer << "miss" << ae::GetRandomInt(0, 2) << ".ogg";
					ae::Audio.PlaySound(ae::Assets.Sounds[Buffer.str()]);
				}
				else {
					ae::Audio.PlaySound(ae::Assets.Sounds["thud0.ogg"]);
				}
			}
			else {
				ActionResult.Timeout = HUD_ACTIONRESULT_TIMEOUT;
				ActionResult.Speed = HUD_ACTIONRESULT_SPEED;
			}

			Battle->ActionResults.push_back(ActionResult);
		}
	}

	// Play audio
	if(ActionResult.ActionUsed.Item)
		ActionResult.ActionUsed.Item->PlaySound(Scripting);
}

// Handles a stat change
void _PlayState::HandleStatChange(ae::_Buffer &Data, _StatChange &StatChange) {
	if(!Player)
		return;

	// Check if player is alive
	bool WasAlive = Player->Character->IsAlive();

	// Get stats
	StatChange.Unserialize(Data, ObjectManager);
	if(StatChange.Object) {

		// Update object
		_StatusEffect *StatusEffect = StatChange.Object->UpdateStats(StatChange);

		if(StatChange.Object == Player) {

			// Create hud element for status effects
			if(StatusEffect)
				StatusEffect->HUDElement = StatusEffect->CreateUIElement(ae::Assets.Elements["element_hud_statuseffects"]);

			// Play buff sounds
			if(StatChange.HasStat(StatType::ID)) {
				const _Buff *Buff = Stats->Buffs.at((uint32_t)StatChange.Values[StatType::ID].Integer);
				if(Buff && Scripting->StartMethodCall(Buff->Script, "PlaySound")) {
					Scripting->MethodCall(0, 0);
					Scripting->FinishMethodCall();
				}
			}

			// Update action bar
			if(StatChange.HasStat(StatType::ACTIONBARSIZE))
				HUD->SetActionBarSize(Player->Character->ActionBar.size());

			// Play death sound
			if(!Player->Character->Battle && Player->Character->Health <= 0 && WasAlive)
				PlayDeathSound();
		}

		// Add stat change
		HUD->AddStatChange(StatChange);
	}
}

// Handles HUD updates
void _PlayState::HandleHUD(ae::_Buffer &Data) {
	if(!Player)
		return;

	int OldLevel = Player->Character->Level;

	Player->Character->Health = Data.Read<int>();
	Player->Character->Mana = Data.Read<int>();
	Player->Character->MaxHealth = Data.Read<int>();
	Player->Character->MaxMana = Data.Read<int>();
	Player->Character->Experience = Data.Read<int>();
	Player->Character->Gold = Data.Read<int>();
	Player->Record->Bounty = Data.Read<int>();
	double Clock = Data.Read<double>();

	Player->Character->CalculateStats();

	if(Map)
		Map->Clock = Clock;

	if(Player->Character->Level > OldLevel) {
		HUD->SetMessage("You have " + std::to_string(Player->Character->GetSkillPointsAvailable()) + " skill point(s). Press " + ae::Actions.GetInputNameForAction(Action::GAME_SKILLS) + " to use them.");
		ae::Audio.PlaySound(ae::Assets.Sounds["success0.ogg"]);

		if(Player->Character->Level == 2) {
			Config.ShowTutorial = 0;
			Config.Save();
		}
	}
}

// Handle seed from server
void _PlayState::HandleMinigameSeed(ae::_Buffer &Data) {
	if(!Player || !Player->Character->Minigame || !HUD->Minigame)
		return;

	HUD->Minigame->StartGame(Data.Read<uint32_t>());
}

// Creates an object from a buffer
_Object *_PlayState::CreateObject(ae::_Buffer &Data, ae::NetworkIDType NetworkID) {

	// Create object
	_Object *Object = ObjectManager->CreateWithID(NetworkID);
	Object->Character->HUD = HUD;
	Object->Scripting = Scripting;
	Object->Stats = Stats;
	Object->Map = Map;
	Object->Character->CalcLevelStats = false;
	Object->UnserializeCreate(Data);

	// Add to map
	Map->AddObject(Object);

	return Object;
}

// Send action to server
void _PlayState::SendActionUse(uint8_t Slot) {
	if(!Player)
		return;

	if(Slot >= Player->Character->ActionBar.size())
		return;

	if(!Player->Character->ActionBar[Slot].IsSet())
		return;

	if(Player->Controller->WaitForServer)
		return;

	// Send use to server
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::ACTION_USE);
	Packet.Write<uint8_t>(Slot);
	Packet.Write<uint8_t>(1);
	Packet.Write<uint32_t>(Player->NetworkID);
	Network->SendPacket(Packet);
}

// Send join request to server
void _PlayState::SendJoinRequest() {
	if(!Player->Character->AcceptingMoveInput())
		return;

	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_JOIN);
	Network->SendPacket(Packet);
}

// Send use action
void _PlayState::SendUseCommand() {
	if(HUD->CloseWindows(true))
		return;

	if(!Player->Character->AcceptingMoveInput())
		return;

	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_USECOMMAND);
	Network->SendPacket(Packet);
}

// Send status to server
void _PlayState::SendStatus(uint8_t Status) {
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::PLAYER_STATUS);
	Packet.Write<uint8_t>(Status);
	Network->SendPacket(Packet);
}

// Assigns the client player pointer
void _PlayState::AssignPlayer(_Object *Object) {
	Player = Object;
	if(Player)
		Player->Character->CalcLevelStats = true;

	if(HUD) {
		HUD->SetPlayer(Player);
		HUD->StatChanges.clear();
		HUD->StopTeleport();
	}

	if(Battle)
		Battle->ClientPlayer = Player;
}

// Delete battle instance
void _PlayState::DeleteBattle() {
	delete Battle;
	Battle = nullptr;
}

// Delete map
void _PlayState::DeleteMap() {
	delete Map;
	Map = nullptr;
}
