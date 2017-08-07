/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#include <network/clientnetwork.h>
#include <objects/object.h>
#include <objects/inventory.h>
#include <objects/statuseffect.h>
#include <objects/buff.h>
#include <objects/map.h>
#include <objects/battle.h>
#include <ui.h>
#include <constants.h>
#include <framework.h>
#include <save.h>
#include <graphics.h>
#include <manager.h>
#include <menu.h>
#include <camera.h>
#include <scripting.h>
#include <assets.h>
#include <stats.h>
#include <hud.h>
#include <program.h>
#include <config.h>
#include <actions.h>
#include <actiontype.h>
#include <buffer.h>
#include <server.h>
#include <packet.h>
#include <audio.h>
#include <random.h>
#include <log.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <sstream>

_PlayState PlayState;

// Constructor
_PlayState::_PlayState() :
	IsTesting(false),
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

	Graphics.Element->SetVisible(false);
	Graphics.Element->Visible = true;

	Stats = new _Stats();

	Camera = new _Camera(glm::vec3(0, 0, CAMERA_DISTANCE), CAMERA_DIVISOR);
	Camera->CalculateFrustum(Graphics.AspectRatio);

	Scripting = new _Scripting();
	Scripting->Setup(Stats, SCRIPTS_PATH + SCRIPTS_GAME);

	HUD = new _HUD();
	HUD->Scripting = Scripting;

	Network = new _ClientNetwork();
	Network->SetFakeLag(Config.FakeLag);
	Network->SetUpdatePeriod(Config.NetworkRate);

	ObjectManager = new _Manager<_Object>();
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
	Audio.Stop();
	ObjectManager->Clear();
	AssignPlayer(nullptr);
	DeleteBattle();
	DeleteMap();
}

// Connect to a server
void _PlayState::Connect(bool IsLocal) {
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
		Server = new _Server(Stats, DEFAULT_NETWORKPORT_ALT);
		Server->IsTesting = IsTesting;
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
	if(!Player->IsAlive()) {
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
				Battle->ClientHandleInput(Action);
			} break;
		}
	}
	else {

		// Currently typing
		if(FocusedElement != nullptr) {
			if(Action == Action::MENU_BACK)
				FocusedElement = nullptr;
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
					if(!Player->WaitForServer)
						HUD->CloseWindows(true);
				break;
			}
		}
	}

	return true;
}

// Key handler
void _PlayState::HandleKey(const _KeyEvent &KeyEvent) {

	// Ignore keys if opening and focusing a textbox with a hotkey
	if(IgnoreFirstChar) {
		IgnoreFirstChar = false;
		return;
	}

	bool Handled = Graphics.Element->HandleKey(KeyEvent);

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
void _PlayState::HandleMouseButton(const _MouseEvent &MouseEvent) {
	FocusedElement = nullptr;
	if(MouseEvent.Button == SDL_BUTTON_LEFT)
		Graphics.Element->HandleInput(MouseEvent.Pressed);

	// Pass to menu
	Menu.HandleMouseButton(MouseEvent);
	if(Menu.State != _Menu::STATE_NONE)
		return;

	HUD->HandleMouseButton(MouseEvent);
}

// Mouse movement handler
void _PlayState::HandleMouseMove(const glm::ivec2 &Position) {

	// Enable mouse during combat
	if(HUD && Player && Player->Battle)
		HUD->EnableMouseCombat = true;
}

void _PlayState::HandleWindow(uint8_t Event) {
	if(Camera && Event == SDL_WINDOWEVENT_SIZE_CHANGED)
		Camera->CalculateFrustum(Graphics.AspectRatio);
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
	Graphics.Element->Update(FrameTime, Input.GetMouse());
	//if(Graphics.Element->HitElement)
	//	std::cout << Graphics.Element->HitElement->ID << std::endl;

	//if(std::abs(std::fmod(Time, 1.0)) >= 0.99)
	//	std::cout << "Client: O=" << ObjectManager->Objects.size() << " B=" << (int)(Battle != nullptr) << std::endl;

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
		int InputState = 0;
		if(Actions.GetState(Action::GAME_UP) > 0.0f)
			InputState |= _Object::MOVE_UP;
		if(Actions.GetState(Action::GAME_DOWN) > 0.0f)
			InputState |= _Object::MOVE_DOWN;
		if(Actions.GetState(Action::GAME_LEFT) > 0.0f)
			InputState |= _Object::MOVE_LEFT;
		if(Actions.GetState(Action::GAME_RIGHT) > 0.0f)
			InputState |= _Object::MOVE_RIGHT;

		// Get player direction
		glm::ivec2 Direction(0, 0);
		Player->GetDirectionFromInput(InputState, Direction);

		// Append input state if moving
		Player->InputStates.clear();
		if(Direction.x != 0 || Direction.y != 0)
			Player->InputStates.push_back(InputState);
	}

	// Update objects
	ObjectManager->Update(FrameTime);

	// Update map
	Map->Update(FrameTime);

	// Send input to server
	if(Player->Moved) {
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_MOVECOMMAND);
		Packet.Write<char>((char)Player->Moved);
		Network->SendPacket(Packet);

		if(!Player->WaitForServer)
			HUD->CloseWindows(true);
	}

	// Update battle system
	if(Battle) {
		if(!Player->Battle)
			DeleteBattle();
		else
			Battle->Update(FrameTime);
	}

	// Update camera
	Camera->Set2DPosition(glm::vec2(Player->Position) + glm::vec2(0.5f, 0.5f));
	Camera->Update(FrameTime);

	// Set HUD message
	if(Player->ClientMessage.length())
		HUD->SetMessage(Player->ClientMessage);

	// Update the HUD
	HUD->Update(FrameTime);

	Time += FrameTime;
}

// Render the state
void _PlayState::Render(double BlendFactor) {

	// Render in game
	if(Player && Map) {
		Graphics.Setup3D();
		Camera->Set3DProjection(BlendFactor);

		// Setup the viewing matrix
		Graphics.SetProgram(Assets.Programs["pos"]);
		glUniformMatrix4fv(Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
		Graphics.SetProgram(Assets.Programs["pos_uv"]);
		glUniformMatrix4fv(Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
		Graphics.SetProgram(Assets.Programs["pos_uv_static"]);
		glUniformMatrix4fv(Assets.Programs["pos_uv_static"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
		Graphics.SetProgram(Assets.Programs["text"]);
		glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

		// Draw map and objects
		Map->Render(Camera, Player, BlendFactor);

		Graphics.Setup2D();
		Graphics.SetStaticUniforms();
		Graphics.SetProgram(Assets.Programs["text"]);
		glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Graphics.Ortho));

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
	std::stringstream Buffer;
	Buffer << "coin" << GetRandomInt(0, 2) << ".ogg";
	Audio.PlaySound(Assets.Sounds[Buffer.str()]);
}

// Play death sound
void _PlayState::PlayDeathSound() {
	std::stringstream Buffer;
	Buffer << "death" << GetRandomInt(0, 2) << ".ogg";
	Audio.PlaySound(Assets.Sounds[Buffer.str()]);
}

// Handle connection to server
void _PlayState::HandleConnect() {

	if(Server) {
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::ACCOUNT_LOGININFO);
		Packet.WriteBit(0);
		Packet.WriteString("");
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
void _PlayState::HandlePacket(_Buffer &Data) {
	PacketType Type = Data.Read<PacketType>();

	switch(Type) {
		case PacketType::OBJECT_STATS:
			HandleObjectStats(Data);
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
		case PacketType::INVENTORY_USE:
			HandleInventoryUse(Data);
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
		default:
			Menu.HandlePacket(Data, Type);
		break;
	}
}

// Called once to synchronize your stats with the servers
void _PlayState::HandleObjectStats(_Buffer &Data) {
	if(!Player)
		return;

	Player->UnserializeStats(Data);

	HUD->UpdateLabels();
	HUD->SetActionBarSize(Player->ActionBar.size());
}

// Called when the player changes maps
void _PlayState::HandleChangeMaps(_Buffer &Data) {
	Menu.InitPlay();

	// Load map
	NetworkIDType MapID = (NetworkIDType)Data.Read<uint32_t>();
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
		Map->Load(Stats->GetMap(MapID));
		AssignPlayer(nullptr);

		Audio.PlayMusic(Assets.Music[Map->Music]);
	}
}

// Handle object list
void _PlayState::HandleObjectList(_Buffer &Data) {
	ObjectManager->Clear();
	AssignPlayer(nullptr);

	// Read header
	NetworkIDType ClientNetworkID = Data.Read<NetworkIDType>();
	NetworkIDType ObjectCount = Data.Read<NetworkIDType>();

	// Create objects
	for(NetworkIDType i = 0; i < ObjectCount; i++) {
		NetworkIDType NetworkID = Data.Read<NetworkIDType>();

		// Create object
		_Object *Object = CreateObject(Data, NetworkID);

		// Set player pointer
		if(Object->NetworkID == ClientNetworkID)
			AssignPlayer(Object);
		else
			Object->CalcLevelStats = false;
	}

	if(Player) {
		Camera->ForcePosition(glm::vec3(Player->Position, CAMERA_DISTANCE) + glm::vec3(0.5, 0.5, 0));
	}
	else {
		// Error
	}
}

// Creates an object
void _PlayState::HandleObjectCreate(_Buffer &Data) {
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
void _PlayState::HandleObjectDelete(_Buffer &Data) {
	if(!Player)
		return;

	NetworkIDType NetworkID = Data.Read<NetworkIDType>();

	// Get object
	_Object *Object = ObjectManager->GetObject(NetworkID);
	if(Object && Object != Player) {
		Object->Deleted = true;
	}
}

// Handles position updates from the server
void _PlayState::HandleObjectUpdates(_Buffer &Data) {
	if(!Player || !Map)
		return;

	// Check map id
	NetworkIDType MapID = Data.Read<uint8_t>();
	if(MapID != Map->NetworkID)
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
		_Object *Object = ObjectManager->GetObject(NetworkID);
		if(Object) {
			Object->Status = Status;

			if(Object == Player) {
			}
			else {
				Object->Position = Position;
				Object->Invisible = Invisible;
			}
			Object->ServerPosition = Position;

			switch(Status) {
				case _Object::STATUS_NONE:
					Object->StatusTexture = nullptr;
				break;
				case _Object::STATUS_PAUSE:
					Object->StatusTexture = Assets.Textures["status/pause.png"];
				break;
				case _Object::STATUS_INVENTORY:
					Object->StatusTexture = Assets.Textures["status/bag.png"];
				break;
				case _Object::STATUS_VENDOR:
					Object->StatusTexture = Assets.Textures["status/vendor.png"];
				break;
				case _Object::STATUS_SKILLS:
					Object->StatusTexture = Assets.Textures["status/skills.png"];
				break;
				case _Object::STATUS_TRADE:
					Object->StatusTexture = Assets.Textures["status/trade.png"];
				break;
				case _Object::STATUS_TRADER:
					Object->StatusTexture = Assets.Textures["status/vendor.png"];
				break;
				case _Object::STATUS_BLACKSMITH:
					Object->StatusTexture = Assets.Textures["status/vendor.png"];
				break;
				case _Object::STATUS_TELEPORT:
					Object->StatusTexture = Assets.Textures["status/teleport.png"];
				break;
				case _Object::STATUS_BATTLE:
					Object->StatusTexture = Assets.Textures["status/battle.png"];
				break;
				case _Object::STATUS_DEAD:
					Object->StatusTexture = Assets.Textures["status/dead.png"];
				break;
				default:
				break;
			}
		}
	}
}

// Handles player position
void _PlayState::HandlePlayerPosition(_Buffer &Data) {
	if(!Player)
		return;

	Player->Position = Data.Read<glm::ivec2>();
	Player->WaitForServer = false;
	Player->TeleportTime = -1;
	HUD->StopTeleport();
}

// Start teleport event
void _PlayState::HandleTeleportStart(_Buffer &Data) {
	if(!Player)
		return;

	Player->TeleportTime = Data.Read<double>();
	Player->WaitForServer = true;
	HUD->CloseWindows(false);
	HUD->StartTeleport();

	Audio.PlaySound(Assets.Sounds["teleport0.ogg"]);
}

// Handles the start of an event
void _PlayState::HandleEventStart(_Buffer &Data) {
	if(!Player)
		return;

	// Read packet
	uint32_t EventType = Data.Read<uint32_t>();
	uint32_t EventData = Data.Read<uint32_t>();
	Player->Position = Data.Read<glm::ivec2>();

	// Handle event
	switch(EventType) {
		case _Map::EVENT_VENDOR:
			Player->Vendor = Stats->GetVendor(EventData);
			Player->WaitForServer = false;
			HUD->InitVendor();
		break;
		case _Map::EVENT_TRADER:
			Player->Trader = Stats->GetTrader(EventData);
			Player->WaitForServer = false;
			HUD->InitTrader();
		break;
		case _Map::EVENT_BLACKSMITH:
			Player->Blacksmith = Stats->GetBlacksmith(EventData);
			Player->WaitForServer = false;
			HUD->InitBlacksmith();
		break;
	}
}

// Handle inventory sync
void _PlayState::HandleInventory(_Buffer &Data) {
	if(!Player)
		return;

	Player->Inventory->Unserialize(Data, Stats);
	Player->CalculateStats();

	// Refresh trader screen
	if(Player->Trader) {
		PlayCoinSound();
		HUD->InitTrader();

		// Update recent items
		if(HUD->RecentItems.size() && HUD->RecentItems.back().Item == Player->Trader->RewardItem) {
			HUD->RecentItems.back().Count += Player->Trader->Count;
			HUD->RecentItems.back().Time = 0.0;
		}
		else {
			_RecentItem RecentItem;
			RecentItem.Item = Player->Trader->RewardItem;
			RecentItem.Count = Player->Trader->Count;

			HUD->RecentItems.push_back(RecentItem);
		}
	}
}

// Handles a chat message
void _PlayState::HandleChatMessage(_Buffer &Data) {

	// Read packet
	_Message Chat;
	Chat.Color = Data.Read<glm::vec4>();
	Chat.Message = Data.ReadString();
	Chat.Time = Time;

	HUD->AddChatMessage(Chat);
}

// Handles the use of an inventory item
void _PlayState::HandleInventoryUse(_Buffer &Data) {
	if(!Player)
		return;
/*
	size_t Index = Data.Read<uint8_t>();
	Player->Inventory->DecrementItemCount(_Slot(_Bag::BagType::INVENTORY, Index), -1);
*/
}

// Handles a inventory swap
void _PlayState::HandleInventorySwap(_Buffer &Data) {
	if(!Player)
		return;

	Player->Inventory->UnserializeSlot(Data, Stats);
	Player->Inventory->UnserializeSlot(Data, Stats);

	HUD->ResetAcceptButton();
	Player->CalculateStats();
}

// Handle an inventory update
void _PlayState::HandleInventoryUpdate(_Buffer &Data) {
	if(!Player)
		return;

	uint8_t Count = Data.Read<uint8_t>();
	for(uint8_t i = 0; i < Count; i++)
		Player->Inventory->UnserializeSlot(Data, Stats);

	Player->CalculateStats();
}

// Handle gold update
void _PlayState::HandleInventoryGold(_Buffer &Data) {
	if(!Player)
		return;

	Player->Gold = Data.Read<int>();
	Player->CalculateStats();

	PlayCoinSound();
}

// Handle party info
void _PlayState::HandlePartyInfo(_Buffer &Data) {
	if(!Player)
		return;

	Player->PartyName = Data.ReadString();
	HUD->UpdateLabels();
}

// Handles a trade request
void _PlayState::HandleTradeRequest(_Buffer &Data) {
	if(!Player || !Map)
		return;

	// Read packet
	NetworkIDType NetworkID = Data.Read<NetworkIDType>();

	// Get trading player
	Player->TradePlayer = ObjectManager->GetObject(NetworkID);
	if(!Player->TradePlayer)
		return;

	// Get gold offer
	Player->TradePlayer->TradeGold = Data.Read<int>();
	for(size_t i = 0; i < PLAYER_TRADEITEMS; i++)
		Player->TradePlayer->Inventory->UnserializeSlot(Data, Stats);
}

// Handles a trade cancel
void _PlayState::HandleTradeCancel(_Buffer &Data) {
	Player->TradePlayer = nullptr;

	// Reset agreement
	HUD->ResetAcceptButton();
	HUD->ResetTradeTheirsWindow();
}

// Handles a trade item update
void _PlayState::HandleTradeItem(_Buffer &Data) {

	// Get trading player
	if(!Player->TradePlayer)
		return;

	// Get slot updates
	Player->TradePlayer->Inventory->UnserializeSlot(Data, Stats);
	Player->TradePlayer->Inventory->UnserializeSlot(Data, Stats);

	// Reset agreement
	Player->TradePlayer->TradeAccepted = false;
	HUD->ResetAcceptButton();
}

// Handles a gold update from the trading player
void _PlayState::HandleTradeGold(_Buffer &Data) {

	// Get trading player
	if(!Player->TradePlayer)
		return;

	// Set gold
	int Gold = Data.Read<int>();
	Player->TradePlayer->TradeGold = Gold;

	// Reset agreement
	Player->TradePlayer->TradeAccepted = false;
	HUD->ResetAcceptButton();
}

// Handles a trade accept
void _PlayState::HandleTradeAccept(_Buffer &Data) {

	// Get trading player
	if(!Player->TradePlayer)
		return;

	// Set state
	bool Accepted = !!Data.Read<char>();
	HUD->UpdateTradeStatus(Accepted);
}

// Handles a trade exchange
void _PlayState::HandleTradeExchange(_Buffer &Data) {
	if(!Player)
		return;

	// Get gold offer
	Player->Gold = Data.Read<int>();
	Player->Inventory->Unserialize(Data, Stats);
	Player->CalculateStats();

	// Close window
	HUD->CloseTrade(false);
}

// Handles the start of a battle
void _PlayState::HandleBattleStart(_Buffer &Data) {

	// Already in a battle
	if(Battle)
		return;

	// Allow player to hit menu buttons
	Player->WaitForServer = false;

	// Reset hud
	HUD->CloseWindows(true);
	HUD->EnableMouseCombat = false;
	if(Config.ShowTutorial && Player->Level == 1)
		HUD->SetMessage("Hit the " + Actions.GetInputNameForAction(Action::GAME_SKILL1) + " key to attack");

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
void _PlayState::HandleBattleAction(_Buffer &Data) {
	if(!Player || !Battle)
		return;

	Battle->ClientHandlePlayerAction(Data);
}

// Handle a fighter joining the battle
void _PlayState::HandleBattleJoin(_Buffer &Data) {
	if(!Player || !Battle)
		return;

	// Read header
	NetworkIDType NetworkID = Data.Read<NetworkIDType>();
	uint32_t DatabaseID = Data.Read<uint32_t>();

	// Get object
	_Object *Object = ObjectManager->GetObject(NetworkID);
	if(Object) {
		if(DatabaseID)
			Stats->GetMonsterStats(DatabaseID, Object);
		Object->UnserializeBattle(Data);
		Battle->AddFighter(Object, Object->BattleSide, true);
	}
}

// Handle a fighter leaving battle
void _PlayState::HandleBattleLeave(_Buffer &Data) {
	if(!Player || !Battle)
		return;

	NetworkIDType NetworkID = Data.Read<NetworkIDType>();
	_Object *Object = ObjectManager->GetObject(NetworkID);
	if(Object) {
		Battle->RemoveFighter(Object);
	}
}

// Handles the end of a battle
void _PlayState::HandleBattleEnd(_Buffer &Data) {
	if(!Player || !Battle)
		return;

	HUD->SetMessage("");
	HUD->CloseWindows(false);

	Player->WaitForServer = false;

	_StatChange StatChange;
	StatChange.Object = Player;

	// Get ending stats
	Player->PlayerKills = Data.Read<int>();
	Player->MonsterKills = Data.Read<int>();
	Player->GoldLost = Data.Read<int>();
	Player->Bounty = Data.Read<int>();
	StatChange.Values[StatType::EXPERIENCE].Integer = Data.Read<int>();
	StatChange.Values[StatType::GOLD].Integer = Data.Read<int>();
	uint8_t ItemCount = Data.Read<uint8_t>();
	for(uint8_t i = 0; i < ItemCount; i++) {
		_RecentItem RecentItem;

		uint32_t ItemID = Data.Read<uint32_t>();
		RecentItem.Item = Stats->Items[ItemID];
		int Upgrades = (int)Data.Read<uint8_t>();
		RecentItem.Count = (int)Data.Read<uint8_t>();

		// Add items
		HUD->RecentItems.push_back(RecentItem);
		Player->Inventory->AddItem(RecentItem.Item, Upgrades, RecentItem.Count);
	}

	// Update client death count
	if(!Player->IsAlive()) {
		Player->Deaths++;
		PlayDeathSound();
	}
	else {
		PlayCoinSound();
	}

	Player->Battle = nullptr;
	HUD->ClearBattleStatChanges();
	HUD->AddStatChange(StatChange);

	DeleteBattle();
}

// Clear action used and targets
void _PlayState::HandleActionClear(_Buffer &Data) {
	NetworkIDType NetworkID = Data.Read<NetworkIDType>();
	_Object *Object = ObjectManager->GetObject(NetworkID);
	if(!Object)
		return;

	Object->Action.Unset();
	Object->Targets.clear();
}

// Handles the result of a turn in battle
void _PlayState::HandleActionResults(_Buffer &Data) {
	if(!Player)
		return;

	// Create result
	_ActionResult ActionResult;
	bool DecrementItem = Data.ReadBit();
	bool SkillUnlocked = Data.ReadBit();
	bool ItemUnlocked = Data.ReadBit();
	uint32_t ItemID = Data.Read<uint32_t>();
	int InventorySlot = (int)Data.Read<char>();
	ActionResult.ActionUsed.Item = Stats->Items[ItemID];

	// Set texture
	if(ActionResult.ActionUsed.Item)
		ActionResult.Texture = ActionResult.ActionUsed.Item->Texture;

	// Get source change
	HandleStatChange(Data, ActionResult.Source);

	// Update source fighter
	if(ActionResult.Source.Object) {
		ActionResult.Source.Object->TurnTimer = 0.0;
		ActionResult.Source.Object->Action.Unset();
		ActionResult.Source.Object->Targets.clear();

		// Use item on client
		if(Player == ActionResult.Source.Object) {
			if(ActionResult.ActionUsed.Item) {

				if(DecrementItem) {
					size_t Index;
					if(Player->Inventory->FindItem(ActionResult.ActionUsed.Item, Index, (size_t)InventorySlot)) {
						Player->Inventory->DecrementItemCount(_Slot(_Bag::BagType::INVENTORY, Index), -1);
						Player->RefreshActionBarCount();
					}
				}

				if(SkillUnlocked) {
					Player->Skills[ActionResult.ActionUsed.Item->ID] = 0;
				}

				if(ItemUnlocked) {
					Player->Unlocks[ActionResult.ActionUsed.Item->UnlockID].Level = 1;
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
			HUD->AddStatChange(ActionResult.Source);
			HUD->AddStatChange(ActionResult.Target);

			// No damage dealt
			if((ActionResult.ActionUsed.GetTargetType() == TargetType::ENEMY || ActionResult.ActionUsed.GetTargetType() == TargetType::ENEMY_ALL)
			   && ((ActionResult.Target.HasStat(StatType::HEALTH) && ActionResult.Target.Values[StatType::HEALTH].Integer == 0) || ActionResult.Target.HasStat(StatType::MISS))) {
				ActionResult.Timeout = HUD_ACTIONRESULT_TIMEOUT_SHORT;
				ActionResult.Speed = HUD_ACTIONRESULT_SPEED_SHORT;

				if(ActionResult.Target.HasStat(StatType::MISS)) {
					std::stringstream Buffer;
					Buffer << "miss" << GetRandomInt(0, 2) << ".ogg";
					Audio.PlaySound(Assets.Sounds[Buffer.str()]);
				}
				else {
					Audio.PlaySound(Assets.Sounds["thud0.ogg"]);
				}
			}
			else {
				ActionResult.Timeout = HUD_ACTIONRESULT_TIMEOUT;
				ActionResult.Speed = HUD_ACTIONRESULT_SPEED;
			}

			Battle->ActionResults.push_back(ActionResult);
		}
		else if(ActionResult.Target.Object == Player) {
			HUD->AddStatChange(ActionResult.Source);
			HUD->AddStatChange(ActionResult.Target);
		}
	}

	// Play audio
	if(ActionResult.ActionUsed.Item)
		ActionResult.ActionUsed.Item->PlaySound(Scripting);
}

// Handles a stat change
void _PlayState::HandleStatChange(_Buffer &Data, _StatChange &StatChange) {
	if(!Player)
		return;

	// Check if player is alive
	bool WasAlive = Player->IsAlive();

	// Get stats
	StatChange.Unserialize(Data, ObjectManager);
	if(StatChange.Object) {

		// Update object
		_StatusEffect *StatusEffect = StatChange.Object->UpdateStats(StatChange);

		if(StatChange.Object == Player) {

			// Create hud element for status effects
			if(StatusEffect)
				StatusEffect->HUDElement = StatusEffect->CreateUIElement(Assets.Elements["element_hud_statuseffects"]);

			// Play buff sounds
			if(StatChange.HasStat(StatType::ID)) {
				const _Buff *Buff = Stats->Buffs[(uint32_t)StatChange.Values[StatType::ID].Integer];
				if(Buff && Scripting->StartMethodCall(Buff->Script, "PlaySound")) {
					Scripting->MethodCall(0, 0);
					Scripting->FinishMethodCall();
				}
			}

			// Update action bar
			if(StatChange.HasStat(StatType::ACTIONBARSIZE))
				HUD->SetActionBarSize(Player->ActionBar.size());

			// Play death sound
			if(!Player->Battle && Player->Health <= 0 && WasAlive)
				PlayDeathSound();
		}

		// Add stat change
		HUD->AddStatChange(StatChange);

		// Play sounds
		if(StatChange.HasStat(StatType::GOLD)) {
			PlayCoinSound();
		}
	}
}

// Handles HUD updates
void _PlayState::HandleHUD(_Buffer &Data) {
	if(!Player)
		return;

	int OldLevel = Player->Level;

	Player->Health = Data.Read<int>();
	Player->Mana = Data.Read<int>();
	Player->MaxHealth = Data.Read<int>();
	Player->MaxMana = Data.Read<int>();
	Player->Experience = Data.Read<int>();
	Player->Gold = Data.Read<int>();
	Player->Bounty = Data.Read<int>();
	double Clock = Data.Read<double>();

	Player->CalculateStats();

	if(Map)
		Map->Clock = Clock;

	if(Player->Level > OldLevel) {
		HUD->SetMessage("You have " + std::to_string(Player->GetSkillPointsRemaining()) + " skill point(s). Press " + Actions.GetInputNameForAction(Action::GAME_SKILLS) + " to use them.");
		Audio.PlaySound(Assets.Sounds["success0.ogg"]);

		if(Player->Level == 2) {
			Config.ShowTutorial = 0;
			Config.Save();
		}
	}
}

// Creates an object from a buffer
_Object *_PlayState::CreateObject(_Buffer &Data, NetworkIDType NetworkID) {

	// Create object
	_Object *Object = ObjectManager->CreateWithID(NetworkID);
	Object->HUD = HUD;
	Object->Scripting = Scripting;
	Object->Stats = Stats;
	Object->Map = Map;
	Object->CalcLevelStats = false;
	Object->UnserializeCreate(Data);

	// Add to map
	Map->AddObject(Object);

	return Object;
}

// Send action to server
void _PlayState::SendActionUse(uint8_t Slot) {
	if(!Player)
		return;

	if(Slot >= Player->ActionBar.size())
		return;

	if(!Player->ActionBar[Slot].IsSet())
		return;

	if(Player->WaitForServer)
		return;

	// Send use to server
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::ACTION_USE);
	Packet.Write<uint8_t>(Slot);
	Packet.Write<uint8_t>(1);
	Packet.Write<uint32_t>(Player->NetworkID);
	Network->SendPacket(Packet);
}

// Send join request to server
void _PlayState::SendJoinRequest() {
	if(!Player->AcceptingMoveInput())
		return;

	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_JOIN);
	Network->SendPacket(Packet);
}

// Send status to server
void _PlayState::SendStatus(uint8_t Status) {
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::PLAYER_STATUS);
	Packet.Write<uint8_t>(Status);
	Network->SendPacket(Packet);
}

// Assigns the client player pointer
void _PlayState::AssignPlayer(_Object *Object) {
	Player = Object;
	if(Player)
		Player->CalcLevelStats = true;

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