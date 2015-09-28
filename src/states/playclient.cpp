/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#include <states/playclient.h>
#include <globals.h>
#include <game.h>
#include <graphics.h>
#include <input.h>
#include <objectmanager.h>
#include <instances.h>
#include <stats.h>
#include <hud.h>
#include <network/network.h>
#include <network/packetstream.h>
#include <instances/map.h>
#include <instances/clientbattle.h>
#include <objects/player.h>
#include <objects/monster.h>
#include <states/mainmenu.h>

_PlayClientState PlayClientState;

// Constructor
_PlayClientState::_PlayClientState()
:	CharacterSlot(0) {

}

// Initializes the state
int _PlayClientState::Init() {

	ClientTime = 0;
	SentClientTime = 0;
	Player = NULL;
	Map = NULL;
	Battle = NULL;
	State = STATE_CONNECTING;

	Instances = new InstanceClass();
	ObjectManager = new ObjectManagerClass();

	// Set up the HUD system
	if(!HUD.Init())
		return 0;

	// Send character slot to play
	PacketClass Packet(NetworkClass::CHARACTERS_PLAY);
	Packet.WriteChar(CharacterSlot);
	ClientNetwork->SendPacketToHost(&Packet);

	return 1;
}

// Shuts the state down
int _PlayClientState::Close() {

	ClientNetwork->Disconnect();
	HUD.Close();
	delete ObjectManager;
	delete Instances;

	return 1;
}

// Handles a connection to the server
void _PlayClientState::HandleConnect(ENetEvent *TEvent) {

}

// Handles a disconnection from the server
void _PlayClientState::HandleDisconnect(ENetEvent *TEvent) {

	Game.ChangeState(&MainMenuState);
}

// Handles a server packet
void _PlayClientState::HandlePacket(ENetEvent *TEvent) {
	//printf("HandlePacket: type=%d\n", TEvent->packet->data[0]);

	PacketClass Packet(TEvent->packet);
	int PacketType = Packet.ReadChar();
	switch(PacketType) {
		case NetworkClass::WORLD_YOURCHARACTERINFO:
			HandleYourCharacterInfo(&Packet);
		break;
		case NetworkClass::WORLD_CHANGEMAPS:
			HandleChangeMaps(&Packet);
		break;
		case NetworkClass::WORLD_CREATEOBJECT:
			HandleCreateObject(&Packet);
		break;
		case NetworkClass::WORLD_DELETEOBJECT:
			HandleDeleteObject(&Packet);
		break;
		case NetworkClass::WORLD_OBJECTUPDATES:
			HandleObjectUpdates(&Packet);
		break;
		case NetworkClass::WORLD_STARTBATTLE:
			HandleStartBattle(&Packet);
		break;
		case NetworkClass::WORLD_HUD:
			HandleHUD(&Packet);
		break;
		case NetworkClass::WORLD_POSITION:
			HandlePlayerPosition(&Packet);
		break;
		case NetworkClass::BATTLE_TURNRESULTS:
			HandleBattleTurnResults(&Packet);
		break;
		case NetworkClass::BATTLE_END:
			HandleBattleEnd(&Packet);
		break;
		case NetworkClass::BATTLE_COMMAND:
			HandleBattleCommand(&Packet);
		break;
		case NetworkClass::EVENT_START:
			HandleEventStart(&Packet);
		break;
		case NetworkClass::INVENTORY_USE:
			HandleInventoryUse(&Packet);
		break;
		case NetworkClass::CHAT_MESSAGE:
			HandleChatMessage(&Packet);
		break;
		case NetworkClass::TRADE_REQUEST:
			HandleTradeRequest(&Packet);
		break;
		case NetworkClass::TRADE_CANCEL:
			HandleTradeCancel(&Packet);
		break;
		case NetworkClass::TRADE_ITEM:
			HandleTradeItem(&Packet);
		break;
		case NetworkClass::TRADE_GOLD:
			HandleTradeGold(&Packet);
		break;
		case NetworkClass::TRADE_ACCEPT:
			HandleTradeAccept(&Packet);
		break;
		case NetworkClass::TRADE_EXCHANGE:
			HandleTradeExchange(&Packet);
		break;
	}
}

// Updates the current state
void _PlayClientState::Update(u32 TDeltaTime) {

	ClientTime += TDeltaTime;

	switch(State) {
		case STATE_CONNECTING:
		break;
		case STATE_WALK:

			// Send move input
			if(!HUD.IsChatting()) {
				if(Input.GetMouseState(InputClass::MOUSE_LEFT) && !(Input.GetMousePosition().X >= 656 && Input.GetMousePosition().Y >= 575)) {
					position2di MoveTarget;
					Map->ScreenToGrid(Input.GetMousePosition(), MoveTarget);
					position2di Delta = MoveTarget - Player->GetPosition();

					if(abs(Delta.X) > abs(Delta.Y)) {
						if(Delta.X < 0)
							SendMoveCommand(PlayerClass::MOVE_LEFT);
						else if(Delta.X > 0)
							SendMoveCommand(PlayerClass::MOVE_RIGHT);
					}
					else {
						if(Delta.Y < 0)
							SendMoveCommand(PlayerClass::MOVE_UP);
						else if(Delta.Y > 0)
							SendMoveCommand(PlayerClass::MOVE_DOWN);
					}
				}

				if(Input.GetKeyState(KEY_LEFT))
					SendMoveCommand(PlayerClass::MOVE_LEFT);
				else if(Input.GetKeyState(KEY_UP))
					SendMoveCommand(PlayerClass::MOVE_UP);
				else if(Input.GetKeyState(KEY_RIGHT))
					SendMoveCommand(PlayerClass::MOVE_RIGHT);
				else if(Input.GetKeyState(KEY_DOWN))
					SendMoveCommand(PlayerClass::MOVE_DOWN);
			}
		break;
		case STATE_BATTLE:

			// Send key input
			if(!HUD.IsChatting()) {
				for(int i = 0; i < 8; i++) {
					EKEY_CODE Key = (EKEY_CODE)(KEY_KEY_1 + i);
					if(Input.GetKeyState(Key)) {
						 Battle->HandleInput(Key);
						 break;
					}
				}
			}

			// Singleplayer check
			if(Battle) {

				// Update the battle
				Battle->Update(TDeltaTime);

				// Done with the battle
				if(Battle->GetState() == ClientBattleClass::STATE_DELETE) {
					Instances->DeleteBattle(Battle);
					Battle = NULL;
					State = STATE_WALK;
				}
			}
		break;
	}

	HUD.Update(TDeltaTime);
	ObjectManager->Update(TDeltaTime);
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
void _PlayClientState::HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement) {

	switch(State) {
		case STATE_BATTLE:
			Battle->HandleGUI(TEventType, TElement);
		break;
	}

	HUD.HandleGUI(TEventType, TElement);
}

// Called once to synchronize your stats with the servers
void _PlayClientState::HandleYourCharacterInfo(PacketClass *TPacket) {

	// Get pack info
	int NetworkID = TPacket->ReadChar();

	Player = new PlayerClass();
	Player->SetName(TPacket->ReadString());
	Player->SetPortraitID(TPacket->ReadInt());
	Player->SetExperience(TPacket->ReadInt());
	Player->SetGold(TPacket->ReadInt());
	Player->SetPlayTime(TPacket->ReadInt());
	Player->SetDeaths(TPacket->ReadInt());
	Player->SetMonsterKills(TPacket->ReadInt());
	Player->SetPlayerKills(TPacket->ReadInt());
	Player->SetBounty(TPacket->ReadInt());
	HUD.SetPlayer(Player);

	// Read items
	int ItemCount = TPacket->ReadChar();
	for(int i = 0; i < ItemCount; i++) {
		int Slot = TPacket->ReadChar();
		int Count = (u8)TPacket->ReadChar();
		int ItemID = TPacket->ReadInt();
		Player->SetInventory(Slot, ItemID, Count);
	}

	// Read skills
	int SkillCount = TPacket->ReadChar();
	for(int i = 0; i < SkillCount; i++) {
		int Points = TPacket->ReadInt();
		int Slot = TPacket->ReadChar();
		Player->SetSkillLevel(Slot, Points);
	}

	// Read skill bar
	for(int i = 0; i < FIGHTER_MAXSKILLS; i++) {
		int SkillID = TPacket->ReadChar();
		Player->SetSkillBar(i, Stats.GetSkill(SkillID));
	}

	Player->CalculateSkillPoints();
	Player->CalculatePlayerStats();
	Player->RestoreHealthMana();
	ObjectManager->AddObjectWithNetworkID(Player, NetworkID);
}

// Called when the player changes maps
void _PlayClientState::HandleChangeMaps(PacketClass *TPacket) {

	// Load map
	int NewMapID = TPacket->ReadInt();
	MapClass *NewMap = Instances->GetMap(NewMapID);
	if(NewMap != Map) {

		// Clear out other objects
		ObjectManager->DeletesObjectsExcept(Player);

		Player->SetMap(NewMap);
		Map = NewMap;

		// Get player count for map
		int PlayerCount = TPacket->ReadInt();

		// Spawn players
		int NetworkID;
		PlayerClass *NewPlayer;
		position2di GridPosition;
		for(int i = 0; i < PlayerCount; i++) {
			NetworkID = TPacket->ReadChar();
			GridPosition.X = TPacket->ReadChar();
			GridPosition.Y = TPacket->ReadChar();
			int Type = TPacket->ReadChar();

			switch(Type) {
				case ObjectClass::PLAYER: {
					stringc Name(TPacket->ReadString());
					int PortraitID = TPacket->ReadChar();
					int Invisible = TPacket->ReadBit();

					// Information for your player
					if(NetworkID == Player->GetNetworkID()) {
						Player->SetPosition(GridPosition);
					}
					else {

						NewPlayer = new PlayerClass();
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
		Battle = NULL;
	}

	State = STATE_WALK;
}

// Creates an object
void _PlayClientState::HandleCreateObject(PacketClass *TPacket) {

	// Read packet
	position2di Position;
	int NetworkID = TPacket->ReadChar();
	Position.X = TPacket->ReadChar();
	Position.Y = TPacket->ReadChar();
	int Type = TPacket->ReadChar();

	// Create the object
	ObjectClass *NewObject = nullptr;
	switch(Type) {
		case ObjectClass::PLAYER: {
			stringc Name(TPacket->ReadString());
			int PortraitID = TPacket->ReadChar();
			int Invisible = TPacket->ReadBit();

			NewObject = new PlayerClass();
			PlayerClass *NewPlayer = static_cast<PlayerClass *>(NewObject);
			NewPlayer->SetName(Name);
			NewPlayer->SetPortraitID(PortraitID);
			NewPlayer->SetInvisPower(Invisible);
		}
		break;
	}

	if(NewObject) {
		NewObject->SetPosition(position2di(Position.X, Position.Y));

		// Add it to the manager
		ObjectManager->AddObjectWithNetworkID(NewObject, NetworkID);
	}

	//printf("HandleCreateObject: NetworkID=%d, Type=%d\n", NetworkID, Type);
}

// Deletes an object
void _PlayClientState::HandleDeleteObject(PacketClass *TPacket) {

	int NetworkID = TPacket->ReadChar();

	ObjectClass *Object = ObjectManager->GetObjectFromNetworkID(NetworkID);
	if(Object) {
		if(Object->GetType() == ObjectClass::PLAYER) {
			PlayerClass *DeletedPlayer = static_cast<PlayerClass *>(Object);
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
void _PlayClientState::HandleObjectUpdates(PacketClass *TPacket) {

	// Get object Count
	char ObjectCount = TPacket->ReadChar();

	//printf("HandleObjectUpdates: ServerTime=%d, ClientTime=%d, ObjectCount=%d\n", ServerTime, ClientTime, ObjectCount);

	position2di Position;
	char NetworkID;
	int PlayerState;
	int Invisible;
	for(int i = 0; i < ObjectCount; i++) {

		NetworkID = TPacket->ReadChar();
		PlayerState = TPacket->ReadChar();
		Position.X = TPacket->ReadChar();
		Position.Y = TPacket->ReadChar();
		Invisible = TPacket->ReadBit();

		//printf("NetworkID=%d invis=%d\n", NetworkID, Invisible);

		PlayerClass *OtherPlayer = static_cast<PlayerClass *>(ObjectManager->GetObjectFromNetworkID(NetworkID));
		if(OtherPlayer) {

			OtherPlayer->SetState(PlayerState);
			if(Player == OtherPlayer) {

				// Return from town portal state
				if(PlayerState == PlayerClass::STATE_WALK && State == STATE_TOWNPORTAL) {
					State = STATE_WALK;
				}
			}
			else {
				OtherPlayer->SetPosition(Position);
				OtherPlayer->SetInvisPower(Invisible);
			}

			switch(PlayerState) {
				case PlayerClass::STATE_WALK:
					OtherPlayer->SetStateImage(NULL);
				break;
				case PlayerClass::STATE_WAITTRADE:
					OtherPlayer->SetStateImage(Graphics.GetImage(GraphicsClass::IMAGE_WORLDTRADE));
				break;
				default:
					OtherPlayer->SetStateImage(Graphics.GetImage(GraphicsClass::IMAGE_WORLDBUSY));
				break;
			}
		}
	}
}

// Handles the start of a battle
void _PlayClientState::HandleStartBattle(PacketClass *TPacket) {
	//printf("HandleStartBattle: \n");

	// Already in a battle
	if(Battle)
		return;

	// Create a new battle instance
	Battle = Instances->CreateClientBattle();

	// Get fighter count
	int FighterCount = TPacket->ReadChar();

	// Get fighter information
	for(int i = 0; i < FighterCount; i++) {

		// Get fighter type
		int Type = TPacket->ReadBit();
		int Side = TPacket->ReadBit();

		if(Type == FighterClass::TYPE_PLAYER) {

			// Network ID
			int NetworkID = TPacket->ReadChar();

			// Player stats
			int Health = TPacket->ReadInt();
			int MaxHealth = TPacket->ReadInt();
			int Mana = TPacket->ReadInt();
			int MaxMana = TPacket->ReadInt();

			// Get player object
			PlayerClass *NewPlayer = static_cast<PlayerClass *>(ObjectManager->GetObjectFromNetworkID(NetworkID));
			if(NewPlayer != NULL) {
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
			int MonsterID = TPacket->ReadInt();
			MonsterClass *Monster = new MonsterClass(MonsterID);

			Battle->AddFighter(Monster, Side);
		}
	}

	// Start the battle
	HUD.CloseWindows();
	Battle->StartBattle(Player);
	State = STATE_BATTLE;
}

// Handles the result of a turn in battle
void _PlayClientState::HandleBattleTurnResults(PacketClass *TPacket) {

	// Check for a battle in progress
	if(!Battle)
		return;

	Battle->ResolveTurn(TPacket);
}

// Handles the end of a battle
void _PlayClientState::HandleBattleEnd(PacketClass *TPacket) {

	// Check for a battle in progress
	if(!Battle)
		return;

	Battle->EndBattle(TPacket);
}

// Handles a battle command from other players
void _PlayClientState::HandleBattleCommand(PacketClass *TPacket) {

	// Check for a battle in progress
	if(!Battle)
		return;

	int Slot = TPacket->ReadChar();
	int SkillID = TPacket->ReadChar();
	Battle->HandleCommand(Slot, SkillID);
}

// Handles HUD updates
void _PlayClientState::HandleHUD(PacketClass *TPacket) {
	Player->SetExperience(TPacket->ReadInt());
	Player->SetGold(TPacket->ReadInt());
	Player->SetHealth(TPacket->ReadInt());
	Player->SetMana(TPacket->ReadInt());
	float HealthAccumulator = TPacket->ReadFloat();
	float ManaAccumulator = TPacket->ReadFloat();
	Player->SetRegenAccumulators(HealthAccumulator, ManaAccumulator);
	Player->CalculatePlayerStats();
}

// Handles player position
void _PlayClientState::HandlePlayerPosition(PacketClass *TPacket) {
	position2di GridPosition;
	GridPosition.X = TPacket->ReadChar();
	GridPosition.Y = TPacket->ReadChar();
	Player->SetPosition(GridPosition);
}

// Handles the start of an event
void _PlayClientState::HandleEventStart(PacketClass *TPacket) {
	position2di GridPosition;
	int Type = TPacket->ReadChar();
	int Data = TPacket->ReadInt();
	GridPosition.X = TPacket->ReadChar();
	GridPosition.Y = TPacket->ReadChar();
	Player->SetPosition(GridPosition);

	switch(Type) {
		case MapClass::EVENT_VENDOR:
			HUD.InitVendor(Data);
			State = STATE_VENDOR;
		break;
		case MapClass::EVENT_TRADER:
			HUD.InitTrader(Data);
			State = STATE_TRADER;
		break;
	}
}

// Handles the use of an inventory item
void _PlayClientState::HandleInventoryUse(PacketClass *TPacket) {

	int Slot = TPacket->ReadChar();
	Player->UpdateInventory(Slot, -1);
}

// Handles a chat message
void _PlayClientState::HandleChatMessage(PacketClass *TPacket) {

	// Read packet
	int NetworkID = TPacket->ReadChar();
	stringc Message(TPacket->ReadString());

	// Get player that sent packet
	PlayerClass *MessagePlayer = static_cast<PlayerClass *>(ObjectManager->GetObjectFromNetworkID(NetworkID));
	if(!MessagePlayer)
		return;

	// Create chat message
	ChatStruct Chat;
	Chat.Message = MessagePlayer->GetName() + stringc(": ") + Message;
	HUD.AddChatMessage(Chat);

	printf("%s\n", Chat.Message.c_str());
}

// Handles a trade request
void _PlayClientState::HandleTradeRequest(PacketClass *TPacket) {

	// Read packet
	int NetworkID = TPacket->ReadChar();

	// Get trading player
	PlayerClass *TradePlayer = static_cast<PlayerClass *>(ObjectManager->GetObjectFromNetworkID(NetworkID));
	if(!TradePlayer)
		return;

	// Set the trading player
	Player->SetTradePlayer(TradePlayer);

	// Reset state
	TradePlayer->SetTradeAccepted(false);
	Player->SetTradeAccepted(false);

	// Get gold offer
	TradePlayer->SetTradeGold(TPacket->ReadInt());
	for(int i = PlayerClass::INVENTORY_TRADE; i < PlayerClass::INVENTORY_COUNT; i++) {
		int ItemID = TPacket->ReadInt();
		int Count = 0;
		if(ItemID != 0)
			Count = TPacket->ReadChar();

		TradePlayer->SetInventory(i, ItemID, Count);
	}
}

// Handles a trade cancel
void _PlayClientState::HandleTradeCancel(PacketClass *TPacket) {
	Player->SetTradePlayer(NULL);

	// Reset agreement
	HUD.ResetAcceptButton();
}

// Handles a trade item update
void _PlayClientState::HandleTradeItem(PacketClass *TPacket) {

	// Get trading player
	PlayerClass *TradePlayer = Player->GetTradePlayer();
	if(!TradePlayer)
		return;

	// Get old slot information
	int OldItemID = TPacket->ReadInt();
	int OldSlot = TPacket->ReadChar();
	int OldCount = 0;
	if(OldItemID > 0)
		OldCount = TPacket->ReadChar();

	// Get new slot information
	int NewItemID = TPacket->ReadInt();
	int NewSlot = TPacket->ReadChar();
	int NewCount = 0;
	if(NewItemID > 0)
		NewCount = TPacket->ReadChar();

	// Update player
	TradePlayer->SetInventory(OldSlot, OldItemID, OldCount);
	TradePlayer->SetInventory(NewSlot, NewItemID, NewCount);

	// Reset agreement
	TradePlayer->SetTradeAccepted(false);
	HUD.ResetAcceptButton();
}

// Handles a gold update from the trading player
void _PlayClientState::HandleTradeGold(PacketClass *TPacket) {

	// Get trading player
	PlayerClass *TradePlayer = Player->GetTradePlayer();
	if(!TradePlayer)
		return;

	// Set gold
	int Gold = TPacket->ReadInt();
	TradePlayer->SetTradeGold(Gold);

	// Reset agreement
	TradePlayer->SetTradeAccepted(false);
	HUD.ResetAcceptButton();
}

// Handles a trade accept
void _PlayClientState::HandleTradeAccept(PacketClass *TPacket) {

	// Get trading player
	PlayerClass *TradePlayer = Player->GetTradePlayer();
	if(!TradePlayer)
		return;

	// Set state
	bool Accepted = !!TPacket->ReadChar();
	TradePlayer->SetTradeAccepted(Accepted);
}

// Handles a trade exchange
void _PlayClientState::HandleTradeExchange(PacketClass *TPacket) {

	// Get gold offer
	int Gold = TPacket->ReadInt();
	Player->SetGold(Gold);
	for(int i = PlayerClass::INVENTORY_TRADE; i < PlayerClass::INVENTORY_COUNT; i++) {
		int ItemID = TPacket->ReadInt();
		int Count = 0;
		if(ItemID != 0)
			Count = TPacket->ReadChar();

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
			PacketClass Packet(NetworkClass::WORLD_MOVECOMMAND);
			Packet.WriteChar(TDirection);
			ClientNetwork->SendPacketToHost(&Packet);
		}
	}
}

// Requests an attack to another a player
void _PlayClientState::SendAttackPlayer() {
	if(Player->CanAttackPlayer()) {
		Player->ResetAttackPlayerTime();
		PacketClass Packet(NetworkClass::WORLD_ATTACKPLAYER);
		ClientNetwork->SendPacketToHost(&Packet);
	}
}
