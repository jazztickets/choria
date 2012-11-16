/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#include "client.h"
#include "engine/globals.h"
#include "engine/game.h"
#include "engine/graphics.h"
#include "engine/input.h"
#include "engine/objectmanager.h"
#include "engine/instances.h"
#include "engine/stats.h"
#include "engine/hud.h"
#include "network/network.h"
#include "network/packetstream.h"
#include "instances/map.h"
#include "instances/clientbattle.h"
#include "objects/player.h"
#include "objects/monster.h"
#include "mainmenu.h"

ClientStateClass ClientState;

// Constructor
ClientStateClass::ClientStateClass()
:	CharacterSlot(0) {

}

// Initializes the state
int ClientStateClass::Init() {

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
int ClientStateClass::Close() {

	ClientNetwork->Disconnect();
	HUD.Close();
	delete ObjectManager;
	delete Instances;

	return 1;
}

// Handles a connection to the server
void ClientStateClass::HandleConnect(ENetEvent *Event) {

}

// Handles a disconnection from the server
void ClientStateClass::HandleDisconnect(ENetEvent *Event) {

	Game.ChangeState(&MainMenuState);
}

// Handles a server packet
void ClientStateClass::HandlePacket(ENetEvent *Event) {
	//printf("HandlePacket: type=%d\n", Event->packet->data[0]);

	PacketClass Packet(Event->packet);
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
		case NetworkClass::BATTLE_UPDATE:
			HandleBattleUpdate(&Packet);
		break;
		case NetworkClass::BATTLE_END:
			HandleBattleEnd(&Packet);
		break;
		case NetworkClass::BATTLE_ACTION:
			HandleBattleAction(&Packet);
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
void ClientStateClass::Update(u32 FrameTime) {

	ClientTime += FrameTime;

	switch(State) {
		case STATE_CONNECTING:
		break;
		case STATE_WALK:
			
			// Send move input
			if(!HUD.IsChatting()) {
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

			// Singleplayer check
			if(Battle) {

				// Update the battle
				Battle->Update(FrameTime);

				// Done with the battle
				if(Battle->GetState() == ClientBattleClass::STATE_DELETE) {
					Instances->DeleteBattle(Battle);
					Battle = NULL;
					State = STATE_WALK;
				}
			}
		break;
	}

	HUD.Update(FrameTime);
	ObjectManager->Update(FrameTime);
}

// Draws the current state
void ClientStateClass::Draw() {
	if(State == STATE_CONNECTING)
		return;

	// Draw map and objects
	Map->SetCameraScroll(Player->Position);
	Map->Render();
	ObjectManager->Render();

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
bool ClientStateClass::HandleKeyPress(EKEY_CODE Key) {

	// Start/stop chat
	if(Key == KEY_RETURN && !HUD.IsTypingGold()) {
		HUD.ToggleChat();
		return true;
	}

	// Check chatting
	if(HUD.IsChatting() || HUD.IsTypingGold()) {
		HUD.HandleKeyPress(Key);
		return false;
	}

	// Open character sheet
	if(Key == KEY_KEY_C) {
		HUD.InitCharacter();
	}

	switch(State) {
		case STATE_WALK:
			switch(Key) {
				case KEY_KEY_1:
				case KEY_KEY_2:
				case KEY_KEY_3:
				case KEY_KEY_4:
				case KEY_KEY_5:
				case KEY_KEY_6:
				case KEY_KEY_7:
				case KEY_KEY_8:
					Player->UseActionWorld(Key - KEY_KEY_1);
				break;
				case KEY_ESCAPE:
					#ifdef _DEBUG
						ClientNetwork->Disconnect();
					#else
						HUD.InitMenu();
					#endif
				break;
				case KEY_KEY_S:
					HUD.ToggleTownPortal();
				break;
				case KEY_KEY_I:
					HUD.InitInventory();
				break;
				case KEY_KEY_T:
					HUD.InitTrade();
				break;
				case KEY_KEY_K:
					HUD.InitSkills();
				break;
				case KEY_KEY_A:
					SendAttackPlayer();					
				break;
			}
		break;
		case STATE_TOWNPORTAL: {
			HUD.ToggleTownPortal();
		}
		break;
		case STATE_MAINMENU:
			switch(Key) {
				case KEY_ESCAPE:
					HUD.CloseWindows();
				break;
			}
		break;
		case STATE_BATTLE:
			Battle->HandleInput(Key);
		break;
		case STATE_INVENTORY:
			switch(Key) {
				case KEY_ESCAPE:
				case KEY_KEY_I:
				case KEY_SPACE:
				case KEY_LEFT:
				case KEY_UP:
				case KEY_RIGHT:
				case KEY_DOWN:
					HUD.CloseWindows();
				break;
			}			
		break;
		case STATE_TRADE:
			switch(Key) {
				case KEY_ESCAPE:
				case KEY_KEY_T:
				case KEY_SPACE:
				case KEY_LEFT:
				case KEY_UP:
				case KEY_RIGHT:
				case KEY_DOWN:
					HUD.CloseWindows();
				break;
			}			
		break;
		case STATE_VENDOR:
			switch(Key) {
				case KEY_ESCAPE:
				case KEY_KEY_I:
				case KEY_SPACE:
				case KEY_LEFT:
				case KEY_UP:
				case KEY_RIGHT:
				case KEY_DOWN:
					HUD.CloseWindows();
				break;
			}
		break;
		case STATE_TRADER:
			switch(Key) {
				case KEY_ESCAPE:
				case KEY_SPACE:
				case KEY_LEFT:
				case KEY_UP:
				case KEY_RIGHT:
				case KEY_DOWN:
					HUD.CloseWindows();
				break;
			}
		break;
		case STATE_SKILLS:
			switch(Key) {
				case KEY_ESCAPE:
				case KEY_KEY_K:
				case KEY_SPACE:
				case KEY_LEFT:
				case KEY_UP:
				case KEY_RIGHT:
				case KEY_DOWN:
					HUD.CloseWindows();
				break;
			}
		break;
	}

	return true;
}

// Mouse movement
void ClientStateClass::HandleMouseMotion(int MouseX, int MouseY) {
	HUD.HandleMouseMotion(MouseX, MouseY);
}

// Mouse buttons
bool ClientStateClass::HandleMousePress(int Button, int MouseX, int MouseY) {

	return HUD.HandleMousePress(Button, MouseX, MouseY);
}

// Mouse releases
void ClientStateClass::HandleMouseRelease(int Button, int MouseX, int MouseY) {

	HUD.HandleMouseRelease(Button, MouseX, MouseY);
}

// Handles GUI presses
void ClientStateClass::HandleGUI(EGUI_EVENT_TYPE EventType, IGUIElement *Element) {

	switch(State) {
		case STATE_BATTLE:
			Battle->HandleGUI(EventType, Element);
		break;
	}

	HUD.HandleGUI(EventType, Element);
}

// Called once to synchronize your stats with the servers
void ClientStateClass::HandleYourCharacterInfo(PacketClass *Packet) {

	// Get pack info
	int NetworkID = Packet->ReadInt();

	Player = new PlayerClass();
	Player->SetName(Packet->ReadString());
	Player->SetPortraitID(Packet->ReadInt());
	Player->SetActionBarLength(Packet->ReadInt());
	Player->SetExperience(Packet->ReadInt());
	Player->SetGold(Packet->ReadInt());
	Player->SetPlayTime(Packet->ReadInt());
	Player->SetDeaths(Packet->ReadInt());
	Player->SetMonsterKills(Packet->ReadInt());
	Player->SetPlayerKills(Packet->ReadInt());
	Player->SetBounty(Packet->ReadInt());
	HUD.SetPlayer(Player);

	// Read items
	int ItemCount = Packet->ReadChar();
	for(int i = 0; i < ItemCount; i++) {
		int Slot = Packet->ReadChar();
		int Count = (u8)Packet->ReadChar();
		int ItemID = Packet->ReadInt();
		Player->SetInventory(Slot, ItemID, Count);
	}

	// Read skills
	int SkillCount = Packet->ReadChar();
	for(int i = 0; i < SkillCount; i++) {
		int ID = Packet->ReadChar();
		Player->AddSkillUnlocked(Stats.GetSkill(ID));
	}

	// Read action bar
	int ActionBarCount = Packet->ReadChar();
	for(int i = 0; i < ActionBarCount; i++) {
		int Type = Packet->ReadChar();
		int ActionID = Packet->ReadChar();
		int Slot = Packet->ReadChar();

		const ActionClass *Action;
		if(Type == ActionClass::YPE_SKILL)
			Action = Stats.GetSkill(ActionID);
		else
			Action = Stats.GetItem(ActionID);

		Player->SetActionBar(Slot, Action);
	}

	Player->CalculatePlayerStats();
	Player->RestoreHealthMana();
	ObjectManager->AddObjectWithNetworkID(Player, NetworkID);
	HUD.RefreshActionBar();
}

// Called when the player changes maps
void ClientStateClass::HandleChangeMaps(PacketClass *Packet) {

	// Load map
	int NewMapID = Packet->ReadInt();
	MapClass *NewMap = Instances->GetMap(NewMapID);
	if(NewMap != Map) {

		// Clear out other objects
		ObjectManager->DeletesObjectsExcept(Player);

		Player->Map = NewMap;
		Map = NewMap;

		// Get player count for map
		int PlayerCount = Packet->ReadInt();

		// Spawn players
		int NetworkID;
		PlayerClass *NewPlayer;
		position2di GridPosition;
		for(int i = 0; i < PlayerCount; i++) {
			NetworkID = Packet->ReadInt();
			GridPosition.X = Packet->ReadChar();
			GridPosition.Y = Packet->ReadChar();
			int Type = Packet->ReadChar();

			switch(Type) {
				case ObjectClass::PLAYER: {
					stringc Name(Packet->ReadString());
					int PortraitID = Packet->ReadChar();

					// Information for your player
					if(NetworkID == Player->NetworkID) {
						Player->Position = GridPosition;
					}
					else {

						NewPlayer = new PlayerClass();
						NewPlayer->Position = GridPosition;
						NewPlayer->SetName(Name);
						NewPlayer->SetPortraitID(PortraitID);
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
void ClientStateClass::HandleCreateObject(PacketClass *Packet) {

	// Read packet
	position2di Position;
	int NetworkID = Packet->ReadInt();
	Position.X = Packet->ReadChar();
	Position.Y = Packet->ReadChar();
	int Type = Packet->ReadChar();

	// Create the object
	ObjectClass *NewObject;
	switch(Type) {
		case ObjectClass::PLAYER: {
			stringc Name(Packet->ReadString());
			int PortraitID = Packet->ReadChar();

			NewObject = new PlayerClass();
			PlayerClass *NewPlayer = static_cast<PlayerClass *>(NewObject);
			NewPlayer->SetName(Name);
			NewPlayer->SetPortraitID(PortraitID);
		}
		break;
	}
	NewObject->Position = position2di(Position.X, Position.Y);

	// Add it to the manager
	ObjectManager->AddObjectWithNetworkID(NewObject, NetworkID);

	//printf("HandleCreateObject: NetworkID=%d, Type=%d\n", NetworkID, Type);
}

// Deletes an object
void ClientStateClass::HandleDeleteObject(PacketClass *Packet) {

	int NetworkID = Packet->ReadInt();

	ObjectClass *Object = ObjectManager->GetObjectFromNetworkID(NetworkID);
	if(Object) {
		if(Object->Type == ObjectClass::PLAYER) {
			PlayerClass *DeletedPlayer = static_cast<PlayerClass *>(Object);
			switch(State) {
				case STATE_BATTLE:
					Battle->RemovePlayer(DeletedPlayer);
				break;
			}
		}
		Object->Deleted = true;
	}
	else
		printf("failed to delete object with networkid=%d\n", NetworkID);

	//printf("HandleDeleteObject: NetworkID=%d\n", NetworkID);
}

// Handles position updates from the server
void ClientStateClass::HandleObjectUpdates(PacketClass *Packet) {

	// Get server time
	u32 ServerTime = Packet->ReadInt() - 1;

	// Get object Count
	int ObjectCount = Packet->ReadInt();

	//printf("HandleObjectUpdates: ServerTime=%d, ClientTime=%d, ObjectCount=%d\n", ServerTime, ClientTime, ObjectCount);

	position2di Position;
	int NetworkID;
	int PlayerState;
	for(int i = 0; i < ObjectCount; i++) {

		NetworkID = Packet->ReadInt();
		PlayerState = Packet->ReadChar();
		Position.X = Packet->ReadChar();
		Position.Y = Packet->ReadChar();

		PlayerClass *OtherPlayer = static_cast<PlayerClass *>(ObjectManager->GetObjectFromNetworkID(NetworkID));
		if(OtherPlayer) {
			OtherPlayer->SetState(PlayerState);
			if(Player == OtherPlayer) {

				// Return from town portal state
				if(PlayerState == PlayerClass::STATE_WALK && State == STATE_TOWNPORTAL) {
					State = STATE_WALK;
				}
			}
			else
				OtherPlayer->Position = Position;
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

// Handles the start of a battle
void ClientStateClass::HandleStartBattle(PacketClass *Packet) {
	//printf("HandleStartBattle: \n");

	// Already in a battle
	if(Battle)
		return;

	// Create a new battle instance
	Battle = Instances->CreateClientBattle();

	// Get fighter count
	int FighterCount = Packet->ReadChar();

	// Get fighter information
	for(int i = 0; i < FighterCount; i++) {

		// Get fighter type
		int Type = Packet->ReadBit();
		int Side = Packet->ReadBit();

		if(Type == FighterClass::TYPE_PLAYER) {

			// Network ID
			int NetworkID = Packet->ReadInt();

			// Player stats
			int Health = Packet->ReadInt();
			int MaxHealth = Packet->ReadInt();
			int Mana = Packet->ReadInt();
			int MaxMana = Packet->ReadInt();

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
			int MonsterID = Packet->ReadInt();
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
void ClientStateClass::HandleBattleUpdate(PacketClass *Packet) {

	// Check for a battle in progress
	if(!Battle)
		return;

	Battle->HandleBattleUpdate(Packet);
}

// Handles the end of a battle
void ClientStateClass::HandleBattleEnd(PacketClass *Packet) {

	// Check for a battle in progress
	if(!Battle)
		return;

	Battle->EndBattle(Packet);
}

// Handles a battle command from other players
void ClientStateClass::HandleBattleAction(PacketClass *Packet) {

	// Check for a battle in progress
	if(!Battle)
		return;

	int Slot = Packet->ReadChar();
	int ActionType = Packet->ReadChar();
	int ActionID = Packet->ReadChar();
	Battle->HandleBattleAction(Slot, ActionType, ActionID);
}

// Handles HUD updates
void ClientStateClass::HandleHUD(PacketClass *Packet) {

	Player->SetExperience(Packet->ReadInt());
	Player->SetGold(Packet->ReadInt());
	Player->SetHealth(Packet->ReadInt());
	Player->SetMana(Packet->ReadInt());

	float HealthAccumulator = Packet->ReadFloat();
	float ManaAccumulator = Packet->ReadFloat();
	Player->SetRegenAccumulators(HealthAccumulator, ManaAccumulator);
	Player->CalculatePlayerStats();
}

// Handles player position
void ClientStateClass::HandlePlayerPosition(PacketClass *Packet) {
	position2di GridPosition;
	GridPosition.X = Packet->ReadChar();
	GridPosition.Y = Packet->ReadChar();
	Player->Position = GridPosition;
}

// Handles the start of an event
void ClientStateClass::HandleEventStart(PacketClass *Packet) {

	int Type = Packet->ReadChar();
	int Data = Packet->ReadInt();

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
void ClientStateClass::HandleInventoryUse(PacketClass *Packet) {

	int Slot = Packet->ReadChar();
	Player->UpdateInventory(Slot, -1);	
}

// Handles a chat message
void ClientStateClass::HandleChatMessage(PacketClass *Packet) {

	// Read packet
	int NetworkID = Packet->ReadInt();
	stringc Message(Packet->ReadString());

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
void ClientStateClass::HandleTradeRequest(PacketClass *Packet) {

	// Read packet
	int NetworkID = Packet->ReadInt();

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
	TradePlayer->SetTradeGold(Packet->ReadInt());
	for(int i = PlayerClass::INVENTORY_TRADE; i < PlayerClass::INVENTORY_COUNT; i++) {
		int ItemID = Packet->ReadInt();
		int Count = 0;
		if(ItemID != 0)
			Count = Packet->ReadChar();

		TradePlayer->SetInventory(i, ItemID, Count);
	}
}

// Handles a trade cancel
void ClientStateClass::HandleTradeCancel(PacketClass *Packet) {
	Player->SetTradePlayer(NULL);

	// Reset agreement
	HUD.ResetAcceptButton();
}

// Handles a trade item update
void ClientStateClass::HandleTradeItem(PacketClass *Packet) {
	
	// Get trading player
	PlayerClass *TradePlayer = Player->GetTradePlayer();
	if(!TradePlayer)
		return;

	// Get old slot information
	int OldItemID = Packet->ReadInt();
	int OldSlot = Packet->ReadChar();
	int OldCount = 0;
	if(OldItemID > 0)
		OldCount = Packet->ReadChar();

	// Get new slot information
	int NewItemID = Packet->ReadInt();
	int NewSlot = Packet->ReadChar();
	int NewCount = 0;
	if(NewItemID > 0)
		NewCount = Packet->ReadChar();

	// Update player
	TradePlayer->SetInventory(OldSlot, OldItemID, OldCount);
	TradePlayer->SetInventory(NewSlot, NewItemID, NewCount);

	// Reset agreement
	TradePlayer->SetTradeAccepted(false);
	HUD.ResetAcceptButton();	
}

// Handles a gold update from the trading player
void ClientStateClass::HandleTradeGold(PacketClass *Packet) {

	// Get trading player
	PlayerClass *TradePlayer = Player->GetTradePlayer();
	if(!TradePlayer)
		return;

	// Set gold
	int Gold = Packet->ReadInt();
	TradePlayer->SetTradeGold(Gold);

	// Reset agreement
	TradePlayer->SetTradeAccepted(false);
	HUD.ResetAcceptButton();	
}

// Handles a trade accept
void ClientStateClass::HandleTradeAccept(PacketClass *Packet) {

	// Get trading player
	PlayerClass *TradePlayer = Player->GetTradePlayer();
	if(!TradePlayer)
		return;

	// Set state
	bool Accepted = Packet->ReadChar();
	TradePlayer->SetTradeAccepted(Accepted);
}

// Handles a trade exchange
void ClientStateClass::HandleTradeExchange(PacketClass *Packet) {

	// Get gold offer
	int Gold = Packet->ReadInt();
	Player->SetGold(Gold);
	for(int i = PlayerClass::INVENTORY_TRADE; i < PlayerClass::INVENTORY_COUNT; i++) {
		int ItemID = Packet->ReadInt();
		int Count = 0;
		if(ItemID != 0)
			Count = Packet->ReadChar();

		Player->SetInventory(i, ItemID, Count);
	}

	// Move traded items to backpack
	Player->MoveTradeToInventory();

	// Close window
	HUD.CloseTrade(false);
}

// Sends a move command to the server
void ClientStateClass::SendMoveCommand(int Direction) {

	if(Player->CanMove()) {

		// Move player locally
		if(Player->MovePlayer(Direction)) {

			// Send move
			PacketClass Packet(NetworkClass::WORLD_MOVECOMMAND);
			Packet.WriteChar(Direction);
			ClientNetwork->SendPacketToHost(&Packet);
		}
	}
}

// Requests an attack to another a player
void ClientStateClass::SendAttackPlayer() {
	if(Player->CanAttackPlayer()) {
		Player->ResetAttackPlayerTime();
		PacketClass Packet(NetworkClass::WORLD_ATTACKPLAYER);
		ClientNetwork->SendPacketToHost(&Packet);
	}
}
