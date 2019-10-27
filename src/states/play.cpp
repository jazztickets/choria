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
#include <states/play.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <objects/components/inventory.h>
#include <objects/components/fighter.h>
#include <objects/components/controller.h>
#include <objects/components/light.h>
#include <objects/statuseffect.h>
#include <objects/buff.h>
#include <objects/map.h>
#include <objects/battle.h>
#include <objects/minigame.h>
#include <hud/hud.h>
#include <hud/character_screen.h>
#include <hud/inventory_screen.h>
#include <hud/vendor_screen.h>
#include <hud/trade_screen.h>
#include <hud/trader_screen.h>
#include <hud/blacksmith_screen.h>
#include <hud/skill_screen.h>
#include <hud/stash_screen.h>
#include <ae/manager.h>
#include <ae/clientnetwork.h>
#include <ae/database.h>
#include <ae/program.h>
#include <ae/actions.h>
#include <ae/framelimit.h>
#include <ae/audio.h>
#include <ae/random.h>
#include <ae/ui.h>
#include <ae/graphics.h>
#include <ae/framebuffer.h>
#include <ae/manager.h>
#include <ae/camera.h>
#include <ae/assets.h>
#include <ae/console.h>
#include <ae/log.h>
#include <ae/util.h>
#include <constants.h>
#include <framework.h>
#include <save.h>
#include <menu.h>
#include <scripting.h>
#include <stats.h>
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
#include <iomanip>

_PlayState PlayState;

// Constructor
_PlayState::_PlayState() :
	DevMode(false),
	IsHardcore(false),
	FromEditor(false),
	ConnectNow(false),
	Stats(nullptr),
	Framebuffer(nullptr),
	Server(nullptr),
	HostAddress("127.0.0.1"),
	ConnectPort(DEFAULT_NETWORKPORT),
	PongPacket(1024) {
}

// Load level and set up objects
void _PlayState::Init() {
	Server = nullptr;
	Map = nullptr;
	Battle = nullptr;
	HUD = nullptr;
	MenuMap = nullptr;
	Time = 0.0;
	CoinSoundPlayed = false;

	ae::Graphics.Element->SetActive(false);
	ae::Graphics.Element->Active = true;

	Stats = new _Stats();

	Camera = new ae::_Camera(glm::vec3(0, 0, CAMERA_DISTANCE), CAMERA_DIVISOR, CAMERA_FOVY, CAMERA_NEAR, CAMERA_FAR);
	Camera->CalculateFrustum(ae::Graphics.AspectRatio);

	Framebuffer = new ae::_Framebuffer(ae::Graphics.CurrentSize);

	Scripting = new _Scripting();
	Scripting->Setup(Stats, GAME_SCRIPTS);

	HUD = new _HUD();
	HUD->Scripting = Scripting;

	Network = new ae::_ClientNetwork();
	Network->SetFakeLag(Config.FakeLag);
	Network->SetUpdatePeriod(Config.NetworkRate);

	ObjectManager = new ae::_Manager<_Object>();
	AssignPlayer(nullptr);

	// Load menu map
	try {
		MenuMap = new _Map();
		MenuMap->Stats = Stats;
		MenuMap->Scripting = Scripting;
		MenuMap->Clock = ae::GetRandomInt(0, MAP_DAY_LENGTH);
		MenuMap->Load("maps/start.map.gz");
	}
	catch(std::exception &Error) {
		delete MenuMap;
		MenuMap = nullptr;
	}
	AssignPlayer(nullptr);

	// Set position of menu map camera
	MenuCamera = new ae::_Camera(GetRandomMapPosition(), 1.0f, CAMERA_FOVY, CAMERA_NEAR, CAMERA_FAR);
	MenuCamera->CalculateFrustum(ae::Graphics.AspectRatio);
	MenuCameraTargetPosition = GetRandomMapPosition();

	if(ConnectNow)
		Menu.InitBrowseServers(true, true);
	else
		Menu.InitTitle();
}

// Close
void _PlayState::Close() {
	Menu.Close();

	AssignPlayer(nullptr);
	delete ObjectManager;
	DeleteBattle();
	DeleteMap();
	delete MenuMap;
	delete HUD;
	delete Scripting;
	delete Camera;
	delete MenuCamera;
	delete Server;
	delete Stats;
	delete Network;
	delete Framebuffer;
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
			Network->Connect("127.0.0.1", DEFAULT_NETWORKPORT);
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
		Server = new _Server(DEFAULT_NETWORKPORT);
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

	// Handle console toggling
	if(Action == Action::MISC_CONSOLE) {
		Framework.Console->Toggle();
		Framework.IgnoreNextInputEvent = true;
	}

	// Ignore actions when console is open
	if(Framework.Console->IsOpen())
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
				HUD->CharacterScreen->Toggle();
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
					HUD->InventoryScreen->Toggle();
				break;
				case Action::GAME_TRADE:
					HUD->TradeScreen->Toggle();
				break;
				case Action::GAME_SKILLS:
					HUD->SkillScreen->Toggle();
				break;
				case Action::GAME_JOIN:
					SendJoinRequest();
				break;
				case Action::GAME_PARTY:
					HUD->ToggleParty();
					Framework.IgnoreNextInputEvent = true;
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
bool _PlayState::HandleKey(const ae::_KeyEvent &KeyEvent) {
	bool Handled = ae::Graphics.Element->HandleKey(KeyEvent);

	// Message history handling
	if(HUD->IsChatting() && KeyEvent.Pressed) {
		if(KeyEvent.Scancode == SDL_SCANCODE_UP)
			HUD->UpdateSentHistory(-1);
		else if(KeyEvent.Scancode == SDL_SCANCODE_DOWN)
			HUD->UpdateSentHistory(1);
	}

	// Pass to menu
	bool SendAction = true;
	if(!Handled)
		SendAction = Menu.HandleKey(KeyEvent);

	if(Menu.State != _Menu::STATE_NONE)
		return SendAction;

	if(!HUD->IsChatting())
		HUD->TradeScreen->ValidateTradeGold();

	return SendAction;
}

// Mouse handler
void _PlayState::HandleMouseButton(const ae::_MouseEvent &MouseEvent) {
	ae::FocusedElement = nullptr;
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

// Handle console command
bool _PlayState::HandleCommand(ae::_Console *Console) {

	// Get parameters
	std::vector<std::string> Parameters;
	ae::TokenizeString(Console->Parameters, Parameters);

	// Start packet
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::COMMAND);
	Packet.WriteString(Console->Command.c_str());

	// Handle normal commands
	if(Console->Command == "quit") {
		HandleQuit();
		return true;
	}

	// Handle dev commands
	if(DevMode) {
		if(Console->Command == "battle") {
			if(Parameters.size() == 1) {
				if(Network && Network->IsConnected()) {
					Packet.WriteString(Parameters[0].c_str());
					Network->SendPacket(Packet);
				}
			}
			else
				Console->AddMessage("usage: battle [zone]");
		}
		else if(Console->Command == "bounty" || Console->Command == "experience" || Console->Command == "gold") {
			if(Parameters.size() == 1) {
				if(Network && Network->IsConnected()) {
					if(Parameters[0][0] == '+' || Parameters[0][0] == '-')
						Packet.WriteBit(1);
					else
						Packet.WriteBit(0);
					Packet.Write<int>(ae::ToNumber<int>(Parameters[0]));
					Network->SendPacket(Packet);
				}
			}
			else
				Console->AddMessage("usage: " + Console->Command + " [+-][amount]");
		}
		else if(Console->Command == "clock") {
			if(Parameters.size() == 1) {
				if(Network && Network->IsConnected()) {
					Packet.Write<int>(ae::ToNumber<int>(Parameters[0]));
					Network->SendPacket(Packet);
				}
			}
			else
				Console->AddMessage("usage: clock [time]");
		}
		else if(Console->Command == "event") {
			if(Parameters.size() == 2) {
				if(Network && Network->IsConnected()) {
					Packet.Write<EventType>((EventType)ae::ToNumber<int>(Parameters[0]));
					Packet.WriteString(Parameters[1].c_str());
					Network->SendPacket(Packet);
				}
			}
			else
				Console->AddMessage("usage: event [type] [data]");
		}
		else if(Console->Command == "give") {
			if(Parameters.size() == 2) {
				if(Network && Network->IsConnected()) {
					Packet.WriteString(Parameters[0].c_str());
					Packet.Write<int>(ae::ToNumber<int>(Parameters[1]));
					Network->SendPacket(Packet);
				}
			}
			else
				Console->AddMessage("usage: give [item_id] [count]");
		}
		else if(Console->Command == "map") {
			if(Parameters.size() == 1) {
				if(Network && Network->IsConnected()) {
					Packet.WriteString(Parameters[0].c_str());
					Network->SendPacket(Packet);
				}
			}
			else
				Console->AddMessage("usage: map [name]");
		}
		else if(Console->Command == "move") {
			if(Parameters.size() == 2) {
				if(Network && Network->IsConnected()) {
					Packet.Write<uint8_t>(ae::ToNumber<int>(Parameters[0]));
					Packet.Write<uint8_t>(ae::ToNumber<int>(Parameters[1]));
					Network->SendPacket(Packet);
				}
			}
			else
				Console->AddMessage("usage: move [x] [y]");
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}

	return true;
}

// Window size updates
void _PlayState::HandleWindow(uint8_t Event) {
	if(Event == SDL_WINDOWEVENT_SIZE_CHANGED) {
		if(Camera)
			Camera->CalculateFrustum(ae::Graphics.AspectRatio);

		if(MenuCamera)
			MenuCamera->CalculateFrustum(ae::Graphics.AspectRatio);

		if(HUD && HUD->Minigame && HUD->Minigame->Camera)
			HUD->Minigame->Camera->CalculateFrustum(ae::Graphics.AspectRatio);

		if(Framebuffer) {
			Framebuffer->Resize(ae::Graphics.CurrentSize);
			ae::Graphics.DirtyState();
		}
	}
}

// Handle quit events
void _PlayState::HandleQuit() {
	if(Network && Network->IsConnected())
		Network->Disconnect();
	else
		Framework.Done = true;
}

// Update
void _PlayState::Update(double FrameTime) {
	CoinSoundPlayed = false;
	//if(std::abs(std::fmod(Time, 1.0)) >= 0.99)
	//	std::cout << "Client: O=" << ObjectManager->Objects.size() << " B=" << (int)(Battle != nullptr) << std::endl;

	// Handle pongs
	ae::_NetworkAddress PongAddress;
	while(Network->CheckPings(PongPacket, PongAddress)) {

		// Read header
		char IP[16];
		PongAddress.GetIP(IP);
		PingType Type = PongPacket.Read<PingType>();

		// Handle ping types
		switch(Type) {
			case PingType::SERVER_INFO_RESPONSE: {
				_ConnectServer ConnectServer;
				ConnectServer.IP = IP;
				ConnectServer.Port = PongPacket.Read<uint16_t>();
				ConnectServer.Players = PongPacket.Read<uint16_t>();
				ConnectServer.MaxPlayers = PongPacket.Read<uint16_t>();
				ConnectServer.Hardcore = PongPacket.ReadBit();
				Menu.AddConnectServer(ConnectServer);
			} break;
			default:
			break;
		}

		// Reset packet
		PongPacket.StartRead();
	}

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

	// Check for objects, otherwise display menu map
	if(!Player || !Map) {

		// Set direction of camera
		glm::vec3 Direction = MenuCameraTargetPosition - MenuCamera->GetPosition();
		if(glm::length(Direction) > 1.0f) {
			Direction = glm::normalize(Direction);
			MenuCamera->UpdatePosition(Direction * MENU_MAP_SCROLL_SPEED * 1.0f);
		}
		else
			MenuCameraTargetPosition = GetRandomMapPosition();

		// Update camera movement
		MenuCamera->Update(FrameTime);
		if(MenuMap)
			MenuMap->Update(FrameTime);

		return;
	}

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
		glm::vec2 Direction(0, 0);
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
	Camera->Set2DPosition(Player->Position);
	Camera->Update(FrameTime);

	// Update the HUD
	HUD->Update(FrameTime);

	Time += FrameTime;
}

// Render the state
void _PlayState::Render(double BlendFactor) {

	// Setup transforms
	ae::Graphics.Setup3D();
	Camera->Set3DProjection(BlendFactor);
	MenuCamera->Set3DProjection(BlendFactor);
	SetViewProjection(Camera);

	// Render in game
	if(Player && Map) {

		// Draw map and objects
		Map->Render(Camera, Framebuffer, Player, BlendFactor);

		ae::Graphics.Setup2D();
		ae::Graphics.SetStaticUniforms();
		ae::Graphics.SetProgram(ae::Assets.Programs["text"]);
		glUniformMatrix4fv(ae::Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(ae::Graphics.Ortho));

		Map->Render2D(Camera);
		HUD->DrawRecentItems();

		// Draw states
		if(Battle)
			Battle->Render(BlendFactor);

		// Draw HUD
		HUD->Render(Map, BlendFactor, Time);
	}
	else {

		// Setup the viewing matrix
		SetViewProjection(MenuCamera);

		// Render background map
		if(MenuMap)
			MenuMap->Render(MenuCamera, Framebuffer, nullptr, BlendFactor);
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

// Get a random location in the background map
glm::vec3 _PlayState::GetRandomMapPosition() {
	if(!MenuMap)
		return glm::vec3(0, 0, CAMERA_DISTANCE);

	return glm::vec3(ae::GetRandomInt(15, MenuMap->Size.x - 15), ae::GetRandomInt(10, MenuMap->Size.y - 10), CAMERA_DISTANCE);
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
		case PacketType::TRADE_INVENTORY:
			HandleTradeInventory(Data);
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
		case PacketType::ACTION_START:
			HandleActionStart(Data);
		break;
		case PacketType::ACTION_APPLY:
			HandleActionApply(Data);
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
		Map->Scripting = Scripting;
		Map->Clock = Clock;
		Map->NetworkID = MapID;
		for(const auto &Iterator : Stats->MapsIndex) {
			if(Iterator.second == MapID) {
				Map->Load(MAPS_PATH + Iterator.first + ".map.gz");
				break;
			}
		}
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
	}

	if(Player) {
		Camera->ForcePosition(glm::vec3(Player->Position, CAMERA_DISTANCE));
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
		bool HasLight = Data.ReadBit();
		int Invisible = Data.ReadBit();
		int Bounty = Data.ReadBit();
		if(Bounty)
			Bounty = Data.Read<int>();

		uint8_t LightTypeID = 0;
		if(HasLight)
			LightTypeID = Data.Read<uint8_t>();

		// Find object
		_Object *Object = ObjectManager->GetObject(NetworkID);
		if(Object) {
			Object->Character->Status = Status;

			// Set stats
			if(Object != Player) {
				Object->SetPositionFromCoords(Position);
				Object->Character->Invisible = Invisible;
				Object->Character->Bounty = Bounty;
			}
			Object->ServerPosition = Position;
			Object->Light->LightTypeID = LightTypeID;

			// Check for valid light
			const auto &Iterator = Stats->Lights.find(LightTypeID);
			if(Iterator != Stats->Lights.end()) {
				Object->Light->Texture = Iterator->second.Texture;
				Object->Shape.HalfSize = Iterator->second.HalfSize;
				Object->Light->Color = Iterator->second.Color;
			}

			// Set status texture
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
				case _Character::STATUS_STASH:
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
			}
		}
	}
}

// Handles player position
void _PlayState::HandlePlayerPosition(ae::_Buffer &Data) {
	if(!Player)
		return;

	Player->SetPositionFromCoords(Data.Read<glm::ivec2>());
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
	EventType Event = Data.Read<EventType>();
	std::string EventData = Data.ReadString();
	Player->SetPositionFromCoords(Data.Read<glm::ivec2>());

	// Handle event
	switch(Event) {
		case EventType::STASH:
			Player->Character->ViewingStash = true;
			Player->Controller->WaitForServer = false;
			HUD->StashScreen->Init();
		break;
		case EventType::VENDOR:
			Player->Character->Vendor = &Stats->Vendors.at(EventData);
			Player->Controller->WaitForServer = false;
			HUD->VendorScreen->Init();
		break;
		case EventType::TRADER:
			Player->Character->Trader = &Stats->Traders.at(EventData);
			Player->Controller->WaitForServer = false;
			HUD->TraderScreen->Init();
		break;
		case EventType::BLACKSMITH:
			Player->Character->Blacksmith = &Stats->Blacksmiths.at(EventData);
			Player->Controller->WaitForServer = false;
			HUD->BlacksmithScreen->Init();
		break;
		case EventType::MINIGAME:
			Player->Character->Minigame = &Stats->Minigames.at(EventData);
			Player->Controller->WaitForServer = false;
			HUD->InitMinigame();
		break;
		default:
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
		HUD->TraderScreen->Init();

		// Update recent items
		if(HUD->RecentItems.size() && HUD->RecentItems.back().Item == Player->Character->Trader->RewardItem) {
			HUD->RecentItems.back().Count += Player->Character->Trader->RewardCount;
			HUD->RecentItems.back().Time = 0.0;
		}
		else {
			_RecentItem RecentItem;
			RecentItem.Item = Player->Character->Trader->RewardItem;
			RecentItem.Count = Player->Character->Trader->RewardCount;

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
	RecentItem.Item = Stats->ItemsIndex.at(Data.Read<uint16_t>());
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

	HUD->TradeScreen->ResetAcceptButton();
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
	for(size_t i = 0; i < INVENTORY_MAX_TRADE_ITEMS; i++)
		Player->Character->TradePlayer->Inventory->UnserializeSlot(Data, Stats);
}

// Handles a trade cancel
void _PlayState::HandleTradeCancel(ae::_Buffer &Data) {
	Player->Character->TradePlayer = nullptr;

	// Reset agreement
	HUD->TradeScreen->ResetAcceptButton();
	HUD->TradeScreen->ResetTradeTheirsWindow();
}

// Handles a trade item update
void _PlayState::HandleTradeInventory(ae::_Buffer &Data) {

	// Get trading player
	if(!Player->Character->TradePlayer)
		return;

	// Get slot updates
	Player->Character->TradePlayer->Inventory->GetBag(BagType::TRADE).Unserialize(Data, Stats);

	// Reset agreement
	Player->Character->TradePlayer->Character->TradeAccepted = false;
	HUD->TradeScreen->ResetAcceptButton();
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
	HUD->TradeScreen->ResetAcceptButton();
}

// Handles a trade accept
void _PlayState::HandleTradeAccept(ae::_Buffer &Data) {

	// Get trading player
	if(!Player->Character->TradePlayer)
		return;

	// Set state
	bool Accepted = !!Data.Read<char>();
	HUD->TradeScreen->UpdateTradeStatus(Accepted);
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
	HUD->TradeScreen->Close(false);
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
	uint16_t DatabaseID = Data.Read<uint16_t>();

	// Get object
	_Object *Object = ObjectManager->GetObject(NetworkID);
	if(Object) {
		if(DatabaseID)
			Stats->GetMonsterStats(Stats->MonstersIndex.at(DatabaseID), Object);
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
	Player->Character->PlayerKills = Data.Read<int>();
	Player->Character->MonsterKills = Data.Read<int>();
	Player->Character->GoldLost = Data.Read<int>();
	Player->Character->Bounty = Data.Read<int>();
	StatChange.Values[StatType::EXPERIENCE].Integer = Data.Read<int>();
	StatChange.Values[StatType::GOLD].Integer = Data.Read<int>();
	uint8_t ItemCount = Data.Read<uint8_t>();
	for(uint8_t i = 0; i < ItemCount; i++) {
		_RecentItem RecentItem;

		uint16_t ItemID = Data.Read<uint16_t>();
		RecentItem.Item = Stats->ItemsIndex.at(ItemID);
		int Upgrades = (int)Data.Read<uint8_t>();
		RecentItem.Count = (int)Data.Read<uint8_t>();

		// Add items
		HUD->RecentItems.push_back(RecentItem);
		Player->Inventory->AddItem(RecentItem.Item, Upgrades, RecentItem.Count);
	}

	// Update client death count
	if(!Player->Character->IsAlive()) {
		Player->Character->Deaths++;
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

// Handles the start of an action use
void _PlayState::HandleActionStart(ae::_Buffer &Data) {
	if(!Player)
		return;

	// Create result
	_ActionResult ActionResult;
	ActionResultFlag ActionFlags = Data.Read<ActionResultFlag>();

	// Read data
	_Slot Slot;
	Slot.Unserialize(Data);
	uint16_t ItemID = Data.Read<uint16_t>();
	float ReactTime = Data.Read<float>();
	float FlyTime = Data.Read<float>();

	// Get usable
	if(ActionFlags & ActionResultFlag::SKILL)
		ActionResult.ActionUsed.Usable = Stats->SkillsIndex.at(ItemID);
	else
		ActionResult.ActionUsed.Usable = Stats->ItemsIndex.at(ItemID);

	// Get source change
	HandleStatChange(Data, ActionResult.Source);

	// Create battle action
	_BattleAction BattleAction;
	BattleAction.Usable = ActionResult.ActionUsed.Usable;
	BattleAction.Source = ActionResult.Source.Object;
	BattleAction.AttackDelay = ReactTime;
	BattleAction.AttackTime = FlyTime;

	// Use item on client
	if(Player == ActionResult.Source.Object) {
		if(ActionResult.ActionUsed.Usable) {

			// Spend item
			if(ActionFlags & ActionResultFlag::DECREMENT) {
				_Slot FoundSlot;
				if(Player->Inventory->FindItem(ActionResult.ActionUsed.Usable->AsItem(), FoundSlot, Slot)) {
					Player->Inventory->UpdateItemCount(FoundSlot, -1);
					Player->Character->RefreshActionBarCount();
				}
			}

			if(ActionFlags & ActionResultFlag::UNLOCK)
				Player->Character->Unlocks[ActionResult.ActionUsed.Usable->ID].Level = 1;

			if(ActionFlags & ActionResultFlag::KEY)
				Player->Inventory->GetBag(BagType::KEYS).Slots.push_back(_InventorySlot(ActionResult.ActionUsed.Usable->AsItem(), 1));
		}
	}

	// Read targets
	uint8_t TargetCount = Data.Read<uint8_t>();
	for(uint8_t i = 0; i < TargetCount; i++) {

		// Get object id
		ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();
		ActionResult.Target.Object = ObjectManager->GetObject(NetworkID);
		if(!ActionResult.Target.Object)
			continue;

		// Add action result to battle
		if(Battle) {
			BattleAction.Target = ActionResult.Target.Object;
			Battle->BattleActions.push_back(BattleAction);
		}
	}
}

// Handle action apply
void _PlayState::HandleActionApply(ae::_Buffer &Data) {
	_ActionResult ActionResult;

	// Update targets
	uint8_t TargetCount = Data.Read<uint8_t>();
	for(uint8_t i = 0; i < TargetCount; i++) {
		HandleStatChange(Data, ActionResult.Source);
		HandleStatChange(Data, ActionResult.Target);

		ActionResult.Source.Object->Character->Action.Unset();
		ActionResult.Source.Object->Character->Targets.clear();

		/*
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
		*/
	}

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
				const _Buff *Buff = Stats->BuffsIndex.at((uint32_t)StatChange.Values[StatType::ID].Integer);
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
	Player->Character->Bounty = Data.Read<int>();
	double Clock = Data.Read<double>();

	Player->Character->CalculateStats();

	if(Map)
		Map->Clock = Clock;

	if(Player->Character->Level > OldLevel) {
		//HUD->SetMessage("You have " + std::to_string(Player->Character->GetSkillPointsAvailable()) + " skill point(s). Press " + ae::Actions.GetInputNameForAction(Action::GAME_SKILLS) + " to use them.");
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

	if(!Player->Character->ActionBar[Slot].Usable)
		return;

	if(Player->Character->Action.IsSet())
		return;

	if(Player->Controller->WaitForServer)
		return;

	if(Player->Character->Minigame)
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

// Set view projection matrix in shaders
void _PlayState::SetViewProjection(ae::_Camera *CameraUsed) {
	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(CameraUsed->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(CameraUsed->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv_static"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos_uv_static"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(CameraUsed->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["text"]);
	glUniformMatrix4fv(ae::Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(CameraUsed->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["map"]);
	glUniformMatrix4fv(ae::Assets.Programs["map"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(CameraUsed->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["map_object"]);
	glUniformMatrix4fv(ae::Assets.Programs["map_object"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(CameraUsed->Transform));
}
