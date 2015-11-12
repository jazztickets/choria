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
#include <states/playclient.h>
#include <globals.h>
#include <framework.h>
#include <graphics.h>
#include <input.h>
#include <objectmanager.h>
#include <instances.h>
#include <stats.h>
#include <hud.h>
#include <buffer.h>
#include <network/network.h>
#include <instances/map.h>
#include <instances/clientbattle.h>
#include <objects/player.h>
#include <objects/monster.h>
#include <states/mainmenu.h>
#include <IGUIEnvironment.h>

_PlayClientState PlayClientState;

using namespace irr;

// Constructor
_PlayClientState::_PlayClientState()
:	CharacterSlot(0) {

}

// Initializes the state
void _PlayClientState::Init() {

	ClientTime = 0;
	SentClientTime = 0;
	Player = nullptr;
	Map = nullptr;
	Battle = nullptr;
	State = STATE_CONNECTING;

	Instances = new _Instance();
	ObjectManager = new _ObjectManager();

	// Set up the HUD system
	HUD.Init();

	// Send character slot to play
	_Buffer Packet;
	Packet.Write<char>(_Network::CHARACTERS_PLAY);
	Packet.Write<char>(CharacterSlot);
	ClientNetwork->SendPacketToHost(&Packet);
}

// Shuts the state down
void _PlayClientState::Close() {

	ClientNetwork->Disconnect();
	HUD.Close();
	delete ObjectManager;
	delete Instances;
}

// Handles a connection to the server
void _PlayClientState::HandleConnect(ENetEvent *TEvent) {

}

// Handles a disconnection from the server
void _PlayClientState::HandleDisconnect(ENetEvent *TEvent) {

	Framework.ChangeState(&MainMenuState);
}

// Handles a server packet
void _PlayClientState::HandlePacket(ENetEvent *TEvent) {
	//printf("HandlePacket: type=%d\n", TEvent->packet->data[0]);

	_Buffer Packet((char *)TEvent->packet->data, TEvent->packet->dataLength);
	int PacketType = Packet.Read<char>();
	switch(PacketType) {
		case _Network::WORLD_YOURCHARACTERINFO:
			HandleYourCharacterInfo(&Packet);
		break;
		case _Network::WORLD_CHANGEMAPS:
			HandleChangeMaps(&Packet);
		break;
		case _Network::WORLD_CREATEOBJECT:
			HandleCreateObject(&Packet);
		break;
		case _Network::WORLD_DELETEOBJECT:
			HandleDeleteObject(&Packet);
		break;
		case _Network::WORLD_OBJECTUPDATES:
			HandleObjectUpdates(&Packet);
		break;
		case _Network::WORLD_STARTBATTLE:
			HandleStartBattle(&Packet);
		break;
		case _Network::WORLD_HUD:
			HandleHUD(&Packet);
		break;
		case _Network::WORLD_POSITION:
			HandlePlayerPosition(&Packet);
		break;
		case _Network::BATTLE_TURNRESULTS:
			HandleBattleTurnResults(&Packet);
		break;
		case _Network::BATTLE_END:
			HandleBattleEnd(&Packet);
		break;
		case _Network::BATTLE_COMMAND:
			HandleBattleCommand(&Packet);
		break;
		case _Network::EVENT_START:
			HandleEventStart(&Packet);
		break;
		case _Network::INVENTORY_USE:
			HandleInventoryUse(&Packet);
		break;
		case _Network::CHAT_MESSAGE:
			HandleChatMessage(&Packet);
		break;
		case _Network::TRADE_REQUEST:
			HandleTradeRequest(&Packet);
		break;
		case _Network::TRADE_CANCEL:
			HandleTradeCancel(&Packet);
		break;
		case _Network::TRADE_ITEM:
			HandleTradeItem(&Packet);
		break;
		case _Network::TRADE_GOLD:
			HandleTradeGold(&Packet);
		break;
		case _Network::TRADE_ACCEPT:
			HandleTradeAccept(&Packet);
		break;
		case _Network::TRADE_EXCHANGE:
			HandleTradeExchange(&Packet);
		break;
	}
}

// Updates the current state
void _PlayClientState::Update(double FrameTime) {

	ClientTime += FrameTime;

	switch(State) {
		case STATE_CONNECTING:
		break;
		case STATE_WALK:

			// Send move input
			if(!HUD.IsChatting()) {
				if(OldInput.GetMouseState(_OldInput::MOUSE_LEFT) && !(OldInput.GetMousePosition().X >= 656 && OldInput.GetMousePosition().Y >= 575)) {
					core::position2di MoveTarget;
					Map->ScreenToGrid(OldInput.GetMousePosition(), MoveTarget);
					core::position2di Delta = MoveTarget - Player->GetPosition();

					if(abs(Delta.X) > abs(Delta.Y)) {
						if(Delta.X < 0)
							SendMoveCommand(_Player::MOVE_LEFT);
						else if(Delta.X > 0)
							SendMoveCommand(_Player::MOVE_RIGHT);
					}
					else {
						if(Delta.Y < 0)
							SendMoveCommand(_Player::MOVE_UP);
						else if(Delta.Y > 0)
							SendMoveCommand(_Player::MOVE_DOWN);
					}
				}

				if(OldInput.GetKeyState(KEY_LEFT))
					SendMoveCommand(_Player::MOVE_LEFT);
				else if(OldInput.GetKeyState(KEY_UP))
					SendMoveCommand(_Player::MOVE_UP);
				else if(OldInput.GetKeyState(KEY_RIGHT))
					SendMoveCommand(_Player::MOVE_RIGHT);
				else if(OldInput.GetKeyState(KEY_DOWN))
					SendMoveCommand(_Player::MOVE_DOWN);
			}
		break;
		case STATE_BATTLE:

			// Send key input
			if(!HUD.IsChatting()) {
				for(int i = 0; i < 8; i++) {
					EKEY_CODE Key = (EKEY_CODE)(KEY_KEY_1 + i);
					if(OldInput.GetKeyState(Key)) {
						 Battle->HandleInput(Key);
						 break;
					}
				}
			}

			// Singleplayer check
			if(Battle) {

				// Update the battle
				Battle->Update(FrameTime);

				// Done with the battle
				if(Battle->GetState() == _ClientBattle::STATE_DELETE) {
					Instances->DeleteBattle(Battle);
					Battle = nullptr;
					State = STATE_WALK;
				}
			}
		break;
	}

	HUD.Update(FrameTime);
	ObjectManager->Update(FrameTime);
}

// Draws the current state
void _PlayClientState::Draw() {
	if(State == STATE_CONNECTING)
		return;

	// Draw map and objects
	Map->SetCameraScroll(Player->GetPosition());
	Map->Render();
	ObjectManager->Render(Map, Player);

	// Draw states
	switch(State) {
		case STATE_WALK:
		break;
		case STATE_BATTLE:
			Battle->Render();
		break;
	}

	// Draw before GUI
	HUD.PreGUIDraw();

	// Draw GUI
	irrGUI->drawAll();

	// Draw HUD
	HUD.Draw();
}

// Key presses
bool _PlayClientState::HandleKeyPress(EKEY_CODE TKey) {

	// Start/stop chat
	if(TKey == KEY_RETURN && !HUD.IsTypingGold()) {
		HUD.ToggleChat();
		return true;
	}

	// Check chatting
	if(HUD.IsChatting() || HUD.IsTypingGold()) {
		HUD.HandleKeyPress(TKey);
		return false;
	}

	// Open character sheet
	if(TKey == KEY_KEY_B) {
		HUD.InitCharacter();
	}

	switch(State) {
		case STATE_WALK:
			switch(TKey) {
				case KEY_ESCAPE:
					#ifdef _DEBUG
						ClientNetwork->Disconnect();
					#else
						HUD.InitMenu();
					#endif
				break;
				case KEY_KEY_Q:
					HUD.ToggleTownPortal();
				break;
				case KEY_KEY_C:
				case KEY_KEY_I:
					HUD.InitInventory();
				break;
				case KEY_KEY_T:
					HUD.InitTrade();
				break;
				case KEY_KEY_S:
				case KEY_KEY_K:
					HUD.InitSkills();
				break;
				case KEY_KEY_A:
					SendAttackPlayer();
				break;
				default:
				break;
			}
		break;
		case STATE_TOWNPORTAL: {
			HUD.ToggleTownPortal();
		}
		break;
		case STATE_MAINMENU:
			switch(TKey) {
				case KEY_ESCAPE:
					HUD.CloseWindows();
				break;
				default:
				break;
			}
		break;
		case STATE_BATTLE:
			Battle->HandleInput(TKey);
		break;
		case STATE_INVENTORY:
			switch(TKey) {
				case KEY_ESCAPE:
				case KEY_KEY_I:
				case KEY_KEY_C:
				case KEY_SPACE:
				case KEY_LEFT:
				case KEY_UP:
				case KEY_RIGHT:
				case KEY_DOWN:
					HUD.CloseWindows();
				break;
				default:
				break;
			}
		break;
		case STATE_TRADE:
			switch(TKey) {
				case KEY_ESCAPE:
				case KEY_KEY_T:
				case KEY_SPACE:
				case KEY_LEFT:
				case KEY_UP:
				case KEY_RIGHT:
				case KEY_DOWN:
					HUD.CloseWindows();
				break;
				default:
				break;
			}
		break;
		case STATE_VENDOR:
			switch(TKey) {
				case KEY_ESCAPE:
				case KEY_KEY_I:
				case KEY_KEY_C:
				case KEY_SPACE:
				case KEY_LEFT:
				case KEY_UP:
				case KEY_RIGHT:
				case KEY_DOWN:
					HUD.CloseWindows();
				break;
				default:
				break;
			}
		break;
		case STATE_TRADER:
			switch(TKey) {
				case KEY_ESCAPE:
				case KEY_SPACE:
				case KEY_LEFT:
				case KEY_UP:
				case KEY_RIGHT:
				case KEY_DOWN:
					HUD.CloseWindows();
				break;
				default:
				break;
			}
		break;
		case STATE_SKILLS:
			switch(TKey) {
				case KEY_ESCAPE:
				case KEY_KEY_S:
				case KEY_KEY_K:
				case KEY_SPACE:
				case KEY_LEFT:
				case KEY_UP:
				case KEY_RIGHT:
				case KEY_DOWN:
					HUD.CloseWindows();
				break;
				default:
				break;
			}
		break;
		default:
		break;
	}

	return true;
}

// Mouse movement
void _PlayClientState::HandleMouseMotion(int TMouseX, int TMouseY) {
	HUD.HandleMouseMotion(TMouseX, TMouseY);
}

// Mouse buttons
bool _PlayClientState::HandleMousePress(int TButton, int TMouseX, int TMouseY) {

	return HUD.HandleMousePress(TButton, TMouseX, TMouseY);
}

// Mouse releases
void _PlayClientState::HandleMouseRelease(int TButton, int TMouseX, int TMouseY) {

	HUD.HandleMouseRelease(TButton, TMouseX, TMouseY);
}

// Handles GUI presses
void _PlayClientState::HandleGUI(gui::EGUI_EVENT_TYPE TEventType, gui::IGUIElement *TElement) {

	switch(State) {
		case STATE_BATTLE:
			Battle->HandleGUI(TEventType, TElement);
		break;
	}

	HUD.HandleGUI(TEventType, TElement);
}

// Called once to synchronize your stats with the servers
void _PlayClientState::HandleYourCharacterInfo(_Buffer *TPacket) {

	// Get pack info
	int NetworkID = TPacket->Read<char>();

	Player = new _Player();
	Player->SetName(TPacket->ReadString());
	Player->SetPortraitID(TPacket->Read<int32_t>());
	Player->SetExperience(TPacket->Read<int32_t>());
	Player->SetGold(TPacket->Read<int32_t>());
	Player->SetPlayTime(TPacket->Read<int32_t>());
	Player->SetDeaths(TPacket->Read<int32_t>());
	Player->SetMonsterKills(TPacket->Read<int32_t>());
	Player->SetPlayerKills(TPacket->Read<int32_t>());
	Player->SetBounty(TPacket->Read<int32_t>());
	HUD.SetPlayer(Player);

	// Read items
	int ItemCount = TPacket->Read<char>();
	for(int i = 0; i < ItemCount; i++) {
		int Slot = TPacket->Read<char>();
		int Count = (u8)TPacket->Read<char>();
		int ItemID = TPacket->Read<int32_t>();
		Player->SetInventory(Slot, ItemID, Count);
	}

	// Read skills
	int SkillCount = TPacket->Read<char>();
	for(int i = 0; i < SkillCount; i++) {
		int Points = TPacket->Read<int32_t>();
		int Slot = TPacket->Read<char>();
		Player->SetSkillLevel(Slot, Points);
	}

	// Read skill bar
	for(int i = 0; i < FIGHTER_MAXSKILLS; i++) {
		int SkillID = TPacket->Read<char>();
		Player->SetSkillBar(i, Stats.GetSkill(SkillID));
	}

	Player->CalculateSkillPoints();
	Player->CalculatePlayerStats();
	Player->RestoreHealthMana();
	ObjectManager->AddObjectWithNetworkID(Player, NetworkID);
}

// Called when the player changes maps
void _PlayClientState::HandleChangeMaps(_Buffer *TPacket) {

	// Load map
	int NewMapID = TPacket->Read<int32_t>();
	_Map *NewMap = Instances->GetMap(NewMapID);
	if(NewMap != Map) {

		// Clear out other objects
		ObjectManager->DeletesObjectsExcept(Player);

		Player->SetMap(NewMap);
		Map = NewMap;

		// Get player count for map
		int PlayerCount = TPacket->Read<int32_t>();

		// Spawn players
		int NetworkID;
		_Player *NewPlayer;
		core::position2di GridPosition;
		for(int i = 0; i < PlayerCount; i++) {
			NetworkID = TPacket->Read<char>();
			GridPosition.X = TPacket->Read<char>();
			GridPosition.Y = TPacket->Read<char>();
			int Type = TPacket->Read<char>();

			switch(Type) {
				case _Object::PLAYER: {
					core::stringc Name(TPacket->ReadString());
					int PortraitID = TPacket->Read<char>();
					int Invisible = TPacket->ReadBit();

					// Information for your player
					if(NetworkID == Player->GetNetworkID()) {
						Player->SetPosition(GridPosition);
					}
					else {

						NewPlayer = new _Player();
						NewPlayer->SetPosition(GridPosition);
						NewPlayer->SetName(Name);
						NewPlayer->SetPortraitID(PortraitID);
						NewPlayer->SetInvisPower(Invisible);
						ObjectManager->AddObjectWithNetworkID(NewPlayer, NetworkID);
					}
				}
				break;
			}
		}

	}

	// Delete the battle
	if(Battle) {
		Instances->DeleteBattle(Battle);
		Battle = nullptr;
	}

	State = STATE_WALK;
}

// Creates an object
void _PlayClientState::HandleCreateObject(_Buffer *TPacket) {

	// Read packet
	core::position2di Position;
	int NetworkID = TPacket->Read<char>();
	Position.X = TPacket->Read<char>();
	Position.Y = TPacket->Read<char>();
	int Type = TPacket->Read<char>();

	// Create the object
	_Object *NewObject = nullptr;
	switch(Type) {
		case _Object::PLAYER: {
			core::stringc Name(TPacket->ReadString());
			int PortraitID = TPacket->Read<char>();
			int Invisible = TPacket->ReadBit();

			NewObject = new _Player();
			_Player *NewPlayer = static_cast<_Player *>(NewObject);
			NewPlayer->SetName(Name);
			NewPlayer->SetPortraitID(PortraitID);
			NewPlayer->SetInvisPower(Invisible);
		}
		break;
	}

	if(NewObject) {
		NewObject->SetPosition(core::position2di(Position.X, Position.Y));

		// Add it to the manager
		ObjectManager->AddObjectWithNetworkID(NewObject, NetworkID);
	}

	//printf("HandleCreateObject: NetworkID=%d, Type=%d\n", NetworkID, Type);
}

// Deletes an object
void _PlayClientState::HandleDeleteObject(_Buffer *TPacket) {

	int NetworkID = TPacket->Read<char>();

	_Object *Object = ObjectManager->GetObjectFromNetworkID(NetworkID);
	if(Object) {
		if(Object->GetType() == _Object::PLAYER) {
			_Player *DeletedPlayer = static_cast<_Player *>(Object);
			switch(State) {
				case STATE_BATTLE:
					Battle->RemovePlayer(DeletedPlayer);
				break;
			}
		}
		Object->SetDeleted(true);
	}
	else
		printf("failed to delete object with networkid=%d\n", NetworkID);

	//printf("HandleDeleteObject: NetworkID=%d\n", NetworkID);
}

// Handles position updates from the server
void _PlayClientState::HandleObjectUpdates(_Buffer *TPacket) {

	// Get object Count
	char ObjectCount = TPacket->Read<char>();

	//printf("HandleObjectUpdates: ServerTime=%d, ClientTime=%d, ObjectCount=%d\n", ServerTime, ClientTime, ObjectCount);

	core::position2di Position;
	char NetworkID;
	int PlayerState;
	int Invisible;
	for(int i = 0; i < ObjectCount; i++) {

		NetworkID = TPacket->Read<char>();
		PlayerState = TPacket->Read<char>();
		Position.X = TPacket->Read<char>();
		Position.Y = TPacket->Read<char>();
		Invisible = TPacket->ReadBit();

		//printf("NetworkID=%d invis=%d\n", NetworkID, Invisible);

		_Player *OtherPlayer = static_cast<_Player *>(ObjectManager->GetObjectFromNetworkID(NetworkID));
		if(OtherPlayer) {

			OtherPlayer->SetState(PlayerState);
			if(Player == OtherPlayer) {

				// Return from town portal state
				if(PlayerState == _Player::STATE_WALK && State == STATE_TOWNPORTAL) {
					State = STATE_WALK;
				}
			}
			else {
				OtherPlayer->SetPosition(Position);
				OtherPlayer->SetInvisPower(Invisible);
			}

			switch(PlayerState) {
				case _Player::STATE_WALK:
					OtherPlayer->SetStateImage(nullptr);
				break;
				case _Player::STATE_WAITTRADE:
					OtherPlayer->SetStateImage(Graphics.GetImage(_Graphics::IMAGE_WORLDTRADE));
				break;
				default:
					OtherPlayer->SetStateImage(Graphics.GetImage(_Graphics::IMAGE_WORLDBUSY));
				break;
			}
		}
	}
}

// Handles the start of a battle
void _PlayClientState::HandleStartBattle(_Buffer *TPacket) {
	//printf("HandleStartBattle: \n");

	// Already in a battle
	if(Battle)
		return;

	// Create a new battle instance
	Battle = Instances->CreateClientBattle();

	// Get fighter count
	int FighterCount = TPacket->Read<char>();

	// Get fighter information
	for(int i = 0; i < FighterCount; i++) {

		// Get fighter type
		int Type = TPacket->ReadBit();
		int Side = TPacket->ReadBit();

		if(Type == _Fighter::TYPE_PLAYER) {

			// Network ID
			int NetworkID = TPacket->Read<char>();

			// Player stats
			int Health = TPacket->Read<int32_t>();
			int MaxHealth = TPacket->Read<int32_t>();
			int Mana = TPacket->Read<int32_t>();
			int MaxMana = TPacket->Read<int32_t>();

			// Get player object
			_Player *NewPlayer = static_cast<_Player *>(ObjectManager->GetObjectFromNetworkID(NetworkID));
			if(NewPlayer != nullptr) {
				NewPlayer->SetHealth(Health);
				NewPlayer->SetMaxHealth(MaxHealth);
				NewPlayer->SetMana(Mana);
				NewPlayer->SetMaxMana(MaxMana);
				NewPlayer->SetBattle(Battle);

				Battle->AddFighter(NewPlayer, Side);
			}
		}
		else {

			// Monster ID
			int MonsterID = TPacket->Read<int32_t>();
			_Monster *Monster = new _Monster(MonsterID);

			Battle->AddFighter(Monster, Side);
		}
	}

	// Start the battle
	HUD.CloseWindows();
	Battle->StartBattle(Player);
	State = STATE_BATTLE;
}

// Handles the result of a turn in battle
void _PlayClientState::HandleBattleTurnResults(_Buffer *TPacket) {

	// Check for a battle in progress
	if(!Battle)
		return;

	Battle->ResolveTurn(TPacket);
}

// Handles the end of a battle
void _PlayClientState::HandleBattleEnd(_Buffer *TPacket) {

	// Check for a battle in progress
	if(!Battle)
		return;

	Battle->EndBattle(TPacket);
}

// Handles a battle command from other players
void _PlayClientState::HandleBattleCommand(_Buffer *TPacket) {

	// Check for a battle in progress
	if(!Battle)
		return;

	int Slot = TPacket->Read<char>();
	int SkillID = TPacket->Read<char>();
	Battle->HandleCommand(Slot, SkillID);
}

// Handles HUD updates
void _PlayClientState::HandleHUD(_Buffer *TPacket) {
	Player->SetExperience(TPacket->Read<int32_t>());
	Player->SetGold(TPacket->Read<int32_t>());
	Player->SetHealth(TPacket->Read<int32_t>());
	Player->SetMana(TPacket->Read<int32_t>());
	float HealthAccumulator = TPacket->Read<float>();
	float ManaAccumulator = TPacket->Read<float>();
	Player->SetRegenAccumulators(HealthAccumulator, ManaAccumulator);
	Player->CalculatePlayerStats();
}

// Handles player position
void _PlayClientState::HandlePlayerPosition(_Buffer *TPacket) {
	core::position2di GridPosition;
	GridPosition.X = TPacket->Read<char>();
	GridPosition.Y = TPacket->Read<char>();
	Player->SetPosition(GridPosition);
}

// Handles the start of an event
void _PlayClientState::HandleEventStart(_Buffer *TPacket) {
	core::position2di GridPosition;
	int Type = TPacket->Read<char>();
	int Data = TPacket->Read<int32_t>();
	GridPosition.X = TPacket->Read<char>();
	GridPosition.Y = TPacket->Read<char>();
	Player->SetPosition(GridPosition);

	switch(Type) {
		case _Map::EVENT_VENDOR:
			HUD.InitVendor(Data);
			State = STATE_VENDOR;
		break;
		case _Map::EVENT_TRADER:
			HUD.InitTrader(Data);
			State = STATE_TRADER;
		break;
	}
}

// Handles the use of an inventory item
void _PlayClientState::HandleInventoryUse(_Buffer *TPacket) {

	int Slot = TPacket->Read<char>();
	Player->UpdateInventory(Slot, -1);
}

// Handles a chat message
void _PlayClientState::HandleChatMessage(_Buffer *TPacket) {

	// Read packet
	int NetworkID = TPacket->Read<char>();
	core::stringc Message(TPacket->ReadString());

	// Get player that sent packet
	_Player *MessagePlayer = static_cast<_Player *>(ObjectManager->GetObjectFromNetworkID(NetworkID));
	if(!MessagePlayer)
		return;

	// Create chat message
	_ChatMessage Chat;
	Chat.Message = MessagePlayer->GetName() + core::stringc(": ") + Message;
	HUD.AddChatMessage(Chat);

	printf("%s\n", Chat.Message.c_str());
}

// Handles a trade request
void _PlayClientState::HandleTradeRequest(_Buffer *TPacket) {

	// Read packet
	int NetworkID = TPacket->Read<char>();

	// Get trading player
	_Player *TradePlayer = static_cast<_Player *>(ObjectManager->GetObjectFromNetworkID(NetworkID));
	if(!TradePlayer)
		return;

	// Set the trading player
	Player->SetTradePlayer(TradePlayer);

	// Reset state
	TradePlayer->SetTradeAccepted(false);
	Player->SetTradeAccepted(false);

	// Get gold offer
	TradePlayer->SetTradeGold(TPacket->Read<int32_t>());
	for(int i = _Player::INVENTORY_TRADE; i < _Player::INVENTORY_COUNT; i++) {
		int ItemID = TPacket->Read<int32_t>();
		int Count = 0;
		if(ItemID != 0)
			Count = TPacket->Read<char>();

		TradePlayer->SetInventory(i, ItemID, Count);
	}
}

// Handles a trade cancel
void _PlayClientState::HandleTradeCancel(_Buffer *TPacket) {
	Player->SetTradePlayer(nullptr);

	// Reset agreement
	HUD.ResetAcceptButton();
}

// Handles a trade item update
void _PlayClientState::HandleTradeItem(_Buffer *TPacket) {

	// Get trading player
	_Player *TradePlayer = Player->GetTradePlayer();
	if(!TradePlayer)
		return;

	// Get old slot information
	int OldItemID = TPacket->Read<int32_t>();
	int OldSlot = TPacket->Read<char>();
	int OldCount = 0;
	if(OldItemID > 0)
		OldCount = TPacket->Read<char>();

	// Get new slot information
	int NewItemID = TPacket->Read<int32_t>();
	int NewSlot = TPacket->Read<char>();
	int NewCount = 0;
	if(NewItemID > 0)
		NewCount = TPacket->Read<char>();

	// Update player
	TradePlayer->SetInventory(OldSlot, OldItemID, OldCount);
	TradePlayer->SetInventory(NewSlot, NewItemID, NewCount);

	// Reset agreement
	TradePlayer->SetTradeAccepted(false);
	HUD.ResetAcceptButton();
}

// Handles a gold update from the trading player
void _PlayClientState::HandleTradeGold(_Buffer *TPacket) {

	// Get trading player
	_Player *TradePlayer = Player->GetTradePlayer();
	if(!TradePlayer)
		return;

	// Set gold
	int Gold = TPacket->Read<int32_t>();
	TradePlayer->SetTradeGold(Gold);

	// Reset agreement
	TradePlayer->SetTradeAccepted(false);
	HUD.ResetAcceptButton();
}

// Handles a trade accept
void _PlayClientState::HandleTradeAccept(_Buffer *TPacket) {

	// Get trading player
	_Player *TradePlayer = Player->GetTradePlayer();
	if(!TradePlayer)
		return;

	// Set state
	bool Accepted = !!TPacket->Read<char>();
	TradePlayer->SetTradeAccepted(Accepted);
}

// Handles a trade exchange
void _PlayClientState::HandleTradeExchange(_Buffer *TPacket) {

	// Get gold offer
	int Gold = TPacket->Read<int32_t>();
	Player->SetGold(Gold);
	for(int i = _Player::INVENTORY_TRADE; i < _Player::INVENTORY_COUNT; i++) {
		int ItemID = TPacket->Read<int32_t>();
		int Count = 0;
		if(ItemID != 0)
			Count = TPacket->Read<char>();

		Player->SetInventory(i, ItemID, Count);
	}

	// Move traded items to backpack
	Player->MoveTradeToInventory();

	// Close window
	HUD.CloseTrade(false);
}

// Sends a move command to the server
void _PlayClientState::SendMoveCommand(int TDirection) {

	if(Player->CanMove()) {

		// Move player locally
		if(Player->MovePlayer(TDirection)) {
			_Buffer Packet;
			Packet.Write<char>(_Network::WORLD_MOVECOMMAND);
			Packet.Write<char>(TDirection);
			ClientNetwork->SendPacketToHost(&Packet);
		}
	}
}

// Requests an attack to another a player
void _PlayClientState::SendAttackPlayer() {
	if(Player->CanAttackPlayer()) {
		Player->ResetAttackPlayerTime();
		_Buffer Packet;
		Packet.Write<char>(_Network::WORLD_ATTACKPLAYER);
		ClientNetwork->SendPacketToHost(&Packet);
	}
}
