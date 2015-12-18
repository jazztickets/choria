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
#include <objects/inventory.h>
#include <objects/statuseffect.h>
#include <objects/buff.h>
#include <objects/skill.h>
#include <objects/map.h>
#include <objects/battle.h>
#include <ui/element.h>
#include <constants.h>
#include <framework.h>
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
	Map = nullptr;
	Battle = nullptr;
	HUD = nullptr;
	Time = 0.0;

	Stats = new _Stats();

	Camera = new _Camera(glm::vec3(0, 0, CAMERA_DISTANCE), CAMERA_DIVISOR);
	Camera->CalculateFrustum(Graphics.AspectRatio);

	Scripting = new _Scripting();
	Scripting->InjectStats(Stats);
	Scripting->LoadScript(SCRIPTS_PATH + SCRIPTS_ITEMS);
	Scripting->LoadScript(SCRIPTS_PATH + SCRIPTS_SKILLS);
	Scripting->LoadScript(SCRIPTS_PATH + SCRIPTS_BUFFS);

	HUD = new _HUD();
	HUD->Scripting = Scripting;

	Network = new _ClientNetwork();
	Network->SetFakeLag(Config.FakeLag);
	Network->SetUpdatePeriod(Config.NetworkRate);

	ObjectManager = new _Manager<_Object>();
	AssignPlayer(nullptr);

	if(ConnectNow)
		Menu.InitConnect(true);
	else
		Menu.InitTitle();
}

// Close map
void _ClientState::Close() {
	Menu.Close();

	AssignPlayer(nullptr);
	delete ObjectManager;
	DeleteBattle();
	DeleteMap();
	delete Network;
	delete HUD;
	delete Scripting;
	delete Camera;
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
		Server = new _Server(Stats, DEFAULT_NETWORKPORT_ALT);
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
				Battle->ClientHandleInput(Action);
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
					SendActionUse((uint8_t)(Action - _Actions::SKILL1));
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
		HUD->Render(Map, BlendFactor, Time);
	}

	// Draw menu
	Menu.Render();
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

	ObjectManager->Clear();
	AssignPlayer(nullptr);

	DeleteBattle();
	DeleteMap();
}

// Handle packet from server
void _ClientState::HandlePacket(_Buffer &Data) {
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
		case PacketType::CHAT_MESSAGE:
			HandleChatMessage(Data);
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
		case PacketType::BATTLE_END:
			HandleBattleEnd(Data);
		break;
		case PacketType::ACTION_RESULTS:
			HandleActionResults(Data);
		break;
		case PacketType::STAT_CHANGE:
			HandleStatChange(Data);
		break;
		case PacketType::WORLD_HUD:
			HandleHUD(Data);
		break;
		default:
			Menu.HandlePacket(Data, Type);
		break;
	}
}

// Called once to synchronize your stats with the servers
void _ClientState::HandleObjectStats(_Buffer &Data) {
	if(!Player)
		return;

	Player->UnserializeStats(Data);
	Player->RestoreHealthMana();

	HUD->SetActionBarSize(Player->ActionBar.size());
}

// Called when the player changes maps
void _ClientState::HandleChangeMaps(_Buffer &Data) {
	Menu.InitPlay();

	// Load map
	NetworkIDType MapID = (NetworkIDType)Data.Read<uint32_t>();
	double Clock = Data.Read<double>();

	// Delete old map and create new
	if(!Map || Map->NetworkID != MapID) {
		if(Map)
			DeleteMap();

		Map = new _Map();
		Map->Clock = Clock;
		Map->NetworkID = MapID;
		Map->Load(Stats->GetMap(MapID)->File);
		AssignPlayer(nullptr);
	}
}

// Handle object list
void _ClientState::HandleObjectList(_Buffer &Data) {
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
		if(Object->NetworkID == ClientNetworkID) {
			AssignPlayer(Object);
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
void _ClientState::HandleObjectCreate(_Buffer &Data) {
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
void _ClientState::HandleObjectDelete(_Buffer &Data) {
	if(!Player)
		return;

	NetworkIDType NetworkID = Data.Read<NetworkIDType>();

	// Get object
	_Object *Object = ObjectManager->IDMap[NetworkID];
	if(Object && Object != Player) {
		Object->Deleted = true;
	}
}

// Handles position updates from the server
void _ClientState::HandleObjectUpdates(_Buffer &Data) {
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
		_Object *Object = ObjectManager->IDMap[NetworkID];
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
	if(!Player)
		return;

	size_t Slot = Data.Read<uint8_t>();
	Player->Inventory->DecrementItemCount(Slot, -1);
}

// Handles a inventory swap
void _ClientState::HandleInventorySwap(_Buffer &Data) {
	if(!Player)
		return;

	Player->Inventory->UnserializeSlot(Data, Stats);
	Player->Inventory->UnserializeSlot(Data, Stats);

	HUD->ResetAcceptButton();
	Player->CalculateStats();
}

// Handle an inventory update
void _ClientState::HandleInventoryUpdate(_Buffer &Data) {
	if(!Player)
		return;

	uint8_t Count = Data.Read<uint8_t>();
	for(uint8_t i = 0; i < Count; i++)
		Player->Inventory->UnserializeSlot(Data, Stats);

	Player->CalculateStats();
}

// Handle gold update
void _ClientState::HandleInventoryGold(_Buffer &Data) {
	if(!Player)
		return;

	Player->Gold = Data.Read<int32_t>();
	Player->CalculateStats();
}

// Handles a trade request
void _ClientState::HandleTradeRequest(_Buffer &Data) {
	if(!Player || !Map)
		return;

	// Read packet
	NetworkIDType NetworkID = Data.Read<NetworkIDType>();

	// Get trading player
	Player->TradePlayer = ObjectManager->IDMap[NetworkID];
	if(!Player->TradePlayer)
		return;

	// Get gold offer
	Player->TradePlayer->TradeGold = Data.Read<int32_t>();
	for(size_t i = InventoryType::TRADE; i < InventoryType::COUNT; i++)
		Player->TradePlayer->Inventory->UnserializeSlot(Data, Stats);
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

	// Get slot updates
	Player->TradePlayer->Inventory->UnserializeSlot(Data, Stats);
	Player->TradePlayer->Inventory->UnserializeSlot(Data, Stats);

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
	if(!Player)
		return;

	// Get gold offer
	Player->Gold = Data.Read<int32_t>();
	Player->Inventory->Unserialize(Data, Stats);
	Player->CalculateStats();

	// Close window
	HUD->CloseTrade(false);
}

// Handles the start of a battle
void _ClientState::HandleBattleStart(_Buffer &Data) {

	// Already in a battle
	if(Battle)
		return;

	// Reset hud
	HUD->CloseWindows();

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
void _ClientState::HandleBattleAction(_Buffer &Data) {
	if(!Player || !Battle)
		return;

	Battle->ClientHandlePlayerAction(Data);
}

// Handles the end of a battle
void _ClientState::HandleBattleEnd(_Buffer &Data) {
	if(!Player || !Battle)
		return;

	Player->WaitForServer = false;
	Battle->ClientEndBattle(Data);
	DeleteBattle();
}

// Handles the result of a turn in battle
void _ClientState::HandleActionResults(_Buffer &Data) {
	if(!Player)
		return;

	// Create result
	_ActionResult ActionResult;
	uint32_t SkillID = Data.Read<uint32_t>();
	uint32_t ItemID = Data.Read<uint32_t>();
	ActionResult.SkillUsed = Stats->Skills[SkillID];
	ActionResult.ItemUsed = Stats->Items[ItemID];

	// Set texture
	if(ActionResult.SkillUsed)
		ActionResult.Texture = ActionResult.SkillUsed->Texture;
	else if(ActionResult.ItemUsed)
		ActionResult.Texture = ActionResult.ItemUsed->Texture;
	else
		ActionResult.Texture = Assets.Textures["skills/attack.png"];

	// Get source change
	ActionResult.Source.Unserialize(Data, ObjectManager);
	int SourceFighterHealth = Data.Read<int32_t>();
	int SourceFighterMana = Data.Read<int32_t>();

	// Update source fighter
	if(ActionResult.Source.Object) {
		ActionResult.Source.Object->Health = SourceFighterHealth;
		ActionResult.Source.Object->Mana = SourceFighterMana;
		ActionResult.Source.Object->TurnTimer = 0.0;
		ActionResult.Source.Object->Action.Unset();
		ActionResult.Source.Object->Targets.clear();

		// Use item on client
		if(Player == ActionResult.Source.Object && ActionResult.ItemUsed) {
			size_t Index;
			if(Player->Inventory->FindItem(ActionResult.ItemUsed, Index)) {
				Player->Inventory->DecrementItemCount(Index, -1);
				Player->RefreshActionBarCount();
			}
		}
	}

	// Update targets
	uint8_t TargetCount = Data.Read<uint8_t>();
	for(uint8_t i = 0; i < TargetCount; i++) {
		ActionResult.Target.Unserialize(Data, ObjectManager);
		int TargetFighterHealth = Data.Read<int32_t>();
		int TargetFighterMana = Data.Read<int32_t>();

		// Read status effect
		uint32_t BuffID = Data.Read<uint32_t>();

		// Update target fighter
		if(ActionResult.Target.Object) {
			ActionResult.Target.Object->Health = TargetFighterHealth;
			ActionResult.Target.Object->Mana = TargetFighterMana;

			// Create status effect
			_StatusEffect *StatusEffect = nullptr;
			if(BuffID > 0) {
				StatusEffect = new _StatusEffect();
				StatusEffect->Buff = Stats->Buffs[BuffID];
				StatusEffect->Level = Data.Read<int>();
				StatusEffect->Count = Data.Read<int>();
			}

			// Add status effect
			if(ActionResult.Target.Object->AddStatusEffect(StatusEffect)) {
				if(ActionResult.Target.Object->BattleElement)
					StatusEffect->BattleElement = StatusEffect->CreateUIElement(ActionResult.Target.Object->BattleElement);

				// Create hud element
				if(ActionResult.Target.Object == Player) {
					StatusEffect->HUDElement = StatusEffect->CreateUIElement(Assets.Elements["element_hud_statuseffects"]);
				}
			}
			else
				delete StatusEffect;
		}

		if(Battle) {
			if(ActionResult.Target.IsChanged())
				HUD->AddStatChange(ActionResult.Target);
			Battle->ActionResults.push_back(ActionResult);
		}
	}
}

// Handles a stat change
void _ClientState::HandleStatChange(_Buffer &Data) {
	if(!Player)
		return;

	// Get stats
	_StatChange StatChange;
	StatChange.Unserialize(Data, ObjectManager);

	// Add to list
	if(StatChange.Object) {
		StatChange.Object->UpdateStats(StatChange);

		HUD->AddStatChange(StatChange);
	}
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
	_Object *Object = ObjectManager->CreateWithID(NetworkID);
	Object->HUD = HUD;
	Object->Scripting = Scripting;
	Object->Stats = Stats;
	Object->Map = Map;
	Object->UnserializeCreate(Data);

	// Add to map
	Map->AddObject(Object);

	return Object;
}

// Send action to server
void _ClientState::SendActionUse(uint8_t Slot) {
	if(!Player)
		return;

	if(Slot >= Player->ActionBar.size())
		return;

	if(!Player->ActionBar[Slot].IsSet())
		return;

	// Send use to server
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::ACTION_USE);
	Packet.Write<uint8_t>(Slot);
	Packet.Write<uint8_t>(1);
	Packet.Write<uint32_t>(Player->NetworkID);
	Network->SendPacket(Packet);
}

// Send status to server
void _ClientState::SendStatus(uint8_t Status) {
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::PLAYER_STATUS);
	Packet.Write<uint8_t>(Status);
	Network->SendPacket(Packet);
}

// Assigns the client player pointer
void _ClientState::AssignPlayer(_Object *Object) {
	Player = Object;
	if(HUD) {
		HUD->SetPlayer(Player);
		HUD->StatChanges.clear();
	}

	if(Battle) {
		Battle->ClientPlayer = Player;
	}
}

// Delete battle instance
void _ClientState::DeleteBattle() {
	delete Battle;
	Battle = nullptr;
}

// Delete map
void _ClientState::DeleteMap() {
	delete Map;
	Map = nullptr;
}