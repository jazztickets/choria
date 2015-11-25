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
#include <states/oldserver.h>
#include <objects/object.h>
#include <database.h>
#include <stats.h>
#include <globals.h>
#include <config.h>
#include <framework.h>
#include <constants.h>
#include <packet.h>
#include <ui/textbox.h>
#include <network/oldnetwork.h>
#include <buffer.h>
#include <instances/map.h>
#include <instances/serverbattle.h>
#include <iostream>
#include <string>

#include <network/servernetwork.h>
/*

_Peer *Peer;

_OldServerState OldServerState;
static void ObjectDeleted(_Object *Object);

// Loop to run server commands
void HandleCommands(void *Arguments) {
	std::string Input;
	bool Done = false;

	std::this_thread::sleep_for(std::chrono::milliseconds(300));

	Framework.Log << "Listening on port " << OldServerNetwork->GetPort() << std::endl;
	Framework.Log << "Type stop to stop the server" << std::endl;
	while(!Done) {
		std::getline(std::cin, Input);
		if(Input == "stop")
			Done = true;
		else
			std::cout << "Command not recognized" << std::endl;
	}

	OldServerState.StopServer();
}

// Initializes the state
void _OldServerState::Init() {
	ServerTime = 0;
	StopRequested = false;
	CommandThread = nullptr;

	ObjectManager = new _ObjectManager();
	ObjectManager->SetObjectDeletedCallback(ObjectDeleted);
}

// Shuts the state down
void _OldServerState::Close() {
	if(CommandThread) {
		CommandThread->join();
		delete CommandThread;
	}

	// Disconnect peers
	std::list<_Object *> Objects = ObjectManager->GetObjects();
	for(auto &Object : Objects) {
		if(Object->Type == _Object::PLAYER) {
			Object->Save();
			OldServerNetwork->Disconnect(Object->OldPeer);
		}
	}

	delete Database;
	delete ObjectManager;
}

// Handles a client disconnect
void _Server::HandleDisconnect(ENetEvent *Event) {
	char Buffer[16];
	enet_address_get_host_ip(&Event->peer->address, Buffer, 16);
	Framework.Log << "HandleDisconnect: " << Buffer << ":" << Event->peer->address.port << std::endl;

	_Object *Player = (_Object *)Event->peer->data;
	if(!Player)
		return;

	// Leave trading screen
	_Object *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {
		TradePlayer->TradePlayer = nullptr;

		_Buffer Packet;
		Packet.Write<char>(Packet::TRADE_CANCEL);
		OldServerNetwork->SendPacketToPeer(&Packet, TradePlayer->OldPeer);
	}

	// Remove from battle
	RemovePlayerFromBattle(Player);

	// Save character info
	Player->Save();

	// Delete object
	ObjectManager->DeleteObject(Player);
}


// Updates the current state
void _OldServerState::Update(double FrameTime) {
	ServerTime += FrameTime;

	// Update maps
	for(auto &Map : Maps)
		Map->Update(FrameTime);

	// Update battles
	for(auto &Battle : Battles)
		Battle->Update(FrameTime);

	// Update objects
	ObjectManager->Update(FrameTime);

	if(StopRequested) {
		Framework.Done = true;
	}
}


// Handles move commands from a client
void _Server::HandleMoveCommand(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	int Direction = Data.Read<char>();
	if(Player->MovePlayer(Direction)) {

		// Handle events
		const _Tile *Tile = Player->GetTile();
		switch(Tile->EventType) {
			case _Map::EVENT_SPAWN:
				Player->SpawnMapID = Player->Map->ID;
				Player->SpawnPoint = Tile->EventData;
				Player->RestoreHealthMana();
				SendHUD(Player);
				Player->Save();
			break;
			case _Map::EVENT_MAPCHANGE:
				Player->GenerateNextBattle();
				SpawnPlayer(Player, Tile->EventData, _Map::EVENT_MAPCHANGE, Player->Map->ID);
			break;
			case _Map::EVENT_VENDOR:
				Player->State = _Object::STATE_VENDOR;
				Player->Vendor = Stats->GetVendor(Tile->EventData);
				SendEvent(Player, Tile->EventType, Tile->EventData);
			break;
			case _Map::EVENT_TRADER:
				Player->State = _Object::STATE_TRADER;
				Player->Trader = Stats->GetTrader(Tile->EventData);
				SendEvent(Player, Tile->EventType, Tile->EventData);
			break;
			default:

				// Start a battle
				if(Player->NextBattle <= 0) {

					// Get monsters
					std::vector<int> Monsters;
					Stats->GenerateMonsterListFromZone(Player->GetCurrentZone(), Monsters);
					size_t MonsterCount = Monsters.size();
					if(MonsterCount > 0) {

						// Create a new battle instance
						_ServerBattle *Battle = new _ServerBattle();
						Battles.push_back(Battle);

						// Add players
						Battle->AddFighter(Player, 0);
						if(1) {

							// Get a list of players
							std::list<_Object *> Players;
							Player->Map->GetClosePlayers(Player, 7*7, Players);

							// Add players to battle
							int PlayersAdded = 0;
							for(std::list<_Object *>::iterator Iterator = Players.begin(); Iterator != Players.end(); ++Iterator) {
								_Object *PartyPlayer = *Iterator;
								if(PartyPlayer->State == _Object::STATE_WALK && !PartyPlayer->IsInvisible()) {
									SendPlayerPosition(PartyPlayer);
									Battle->AddFighter(PartyPlayer, 0);
									PlayersAdded++;
									if(PlayersAdded == 2)
										break;
								}
							}
						}

						// Add monsters
						for(size_t i = 0; i < Monsters.size(); i++) {
							_Object *Monster = new _Object(Monsters[i]);
							Monster->ID = Monsters[i];
							Monster->Type = _Object::MONSTER;
							Stats->GetMonsterStats(Monsters[i], Monster);
							Battle->AddFighter(Monster, 1);
						}

						Battle->StartBattle();
					}
				}
			break;
		}
	}
}

// Handles battle commands from a client
void _Server::HandleBattleCommand(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	_ServerBattle *Battle = (_ServerBattle *)Player->Battle;
	if(!Battle)
		return;

	int Command = Data.Read<char>();
	int Target = Data.Read<char>();
	Battle->HandleInput(Player, Command, Target);

	//printf("HandleBattleCommand: %d\n", Command);
}

// The client is done with the battle results screen
void _Server::HandleBattleFinished(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	_ServerBattle *Battle = (_ServerBattle *)Player->Battle;
	if(!Battle)
		return;

	// Check for the last player leaving the battle
	RemovePlayerFromBattle(Player);

	// Check for death
	if(Player->Health == 0) {
		Player->RestoreHealthMana();
		SpawnPlayer(Player, Player->SpawnMapID, _Map::EVENT_SPAWN, Player->SpawnPoint);
		Player->Save();
	}

	// Send updates
	SendHUD(Player);
}

// Handles a player's inventory move
void _Server::HandleInventoryMove(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	int OldSlot = Data.Read<char>();
	int NewSlot = Data.Read<char>();

	// Move items
	Player->MoveInventory(OldSlot, NewSlot);
	Player->CalculatePlayerStats();

	// Check for trading players
	_Object *TradePlayer = Player->TradePlayer;
	if(Player->State == _Object::STATE_TRADE && TradePlayer && (_Object::IsSlotTrade(OldSlot) || _Object::IsSlotTrade(NewSlot))) {

		// Reset agreement
		Player->TradeAccepted = false;
		TradePlayer->TradeAccepted = false;

		// Send item updates to trading player
		_InventorySlot *OldSlotItem = &Player->Inventory[OldSlot];
		_InventorySlot *NewSlotItem = &Player->Inventory[NewSlot];

		// Get item information
		int OldItemID = 0, NewItemID = 0, OldItemCount = 0, NewItemCount = 0;
		if(OldSlotItem->Item) {
			OldItemID = OldSlotItem->Item->ID;
			OldItemCount = OldSlotItem->Count;
		}
		if(NewSlotItem->Item) {
			NewItemID = NewSlotItem->Item->ID;
			NewItemCount = NewSlotItem->Count;
		}

		// Build packet
		_Buffer NewPacket;
		NewPacket.Write<char>(Packet::TRADE_ITEM);
		NewPacket.Write<int32_t>(OldItemID);
		NewPacket.Write<char>(OldSlot);
		if(OldItemID > 0)
			NewPacket.Write<char>(OldItemCount);
		NewPacket.Write<int32_t>(NewItemID);
		NewPacket.Write<char>(NewSlot);
		if(NewItemID > 0)
			NewPacket.Write<char>(NewItemCount);

		// Send updates
		OldServerNetwork->SendPacketToPeer(&NewPacket, TradePlayer->OldPeer);
	}
}

// Handle a player's inventory use request
void _Server::HandleInventoryUse(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	// Use an item
	Player->UseInventory(Packet->Read<char>());
}

// Handle a player's inventory split stack request
void _Server::HandleInventorySplit(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	int Slot = Data.Read<char>();
	int Count = Data.Read<char>();

	// Inventory only
	if(!_Object::IsSlotInventory(Slot))
		return;

	Player->SplitStack(Slot, Count);
}

// Handles a player's event end message
void _Server::HandleEventEnd(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	Player->Vendor = nullptr;
	Player->State = _Object::STATE_WALK;
}

// Handles a vendor exchange message
void _Server::HandleVendorExchange(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	// Get vendor
	const _Vendor *Vendor = Player->Vendor;
	if(!Vendor)
		return;

	// Get info
	bool Buy = Data.ReadBit();
	int Amount = Data.Read<char>();
	int Slot = Data.Read<char>();
	if(Slot < 0)
		return;

	// Update player
	if(Buy) {
		if(Slot >= (int)Vendor->Items.size())
			return;

		// Get optional inventory slot
		int TargetSlot = Data.Read<char>();

		// Get item info
		const _Item *Item = Vendor->Items[Slot];
		int Price = Item->GetPrice(Vendor, Amount, Buy);

		// Update player
		Player->UpdateGold(-Price);
		Player->AddItem(Item, Amount, TargetSlot);
		Player->CalculatePlayerStats();
	}
	else {
		if(Slot >= _Object::INVENTORY_COUNT)
			return;

		// Get item info
		_InventorySlot *Item = &Player->Inventory[Slot];
		if(Item && Item->Item) {
			int Price = Item->Item->GetPrice(Vendor, Amount, Buy);
			Player->UpdateGold(Price);
			Player->UpdateInventory(Slot, -Amount);
		}
	}
}

// Handle a skill bar change
void _Server::HandleSkillBar(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	// Read skills
	for(int i = 0; i < BATTLE_MAXSKILLS; i++) {
		Player->SetSkillBar(i, Stats->GetSkill(Packet->Read<char>()));
	}

	Player->CalculatePlayerStats();
}

// Handles a skill adjust
void _Server::HandleSkillAdjust(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	// Process packet
	bool Spend = Data.ReadBit();
	int SkillID = Data.Read<char>();
	if(Spend) {
		Player->AdjustSkillLevel(SkillID, 1);
	}
	else {
		Player->AdjustSkillLevel(SkillID, -1);
	}

	Player->CalculateSkillPoints();
	Player->CalculatePlayerStats();
}

// Handles a player's request to not start a battle with other players
void _Server::HandlePlayerBusy(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	bool Value = Data.ReadBit();
	Player->SetBusy(Value);

	//printf("HandlePlayerBusy: Value=%d\n", Value);
}

// Handles a player's request to attack another player
void _Server::HandleAttackPlayer(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player || !Player->CanAttackPlayer())
		return;

	_Map *Map = Player->Map;
	if(!Map)
		return;

	// Check for a valid pvp tile
	if(Player->GetTile()->PVP) {

		// Reset timer
		Player->ResetAttackPlayerTime();

		// Get a list of players next to the player
		std::list<_Object *> Players;
		Map->GetClosePlayers(Player, 1.5f * 1.5f, Players);

		// Find a suitable player to attack
		for(std::list<_Object *>::iterator Iterator = Players.begin(); Iterator != Players.end(); ++Iterator) {
			_Object *VictimPlayer = *Iterator;
			if(VictimPlayer->State != _Object::STATE_BATTLE) {
				_ServerBattle *Battle = new _ServerBattle();
				Battles.push_back(Battle);

				Battle->AddFighter(Player, 1);
				Battle->AddFighter(VictimPlayer, 0);
				Battle->StartBattle();
				break;
			}
		}
	}
}

// Handle a chat message
void _Server::HandleChatMessage(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	// Get message
	std::string Message = Data.ReadString();
	if(Message.length() > NETWORKING_CHAT_SIZE)
		Message.resize(NETWORKING_CHAT_SIZE);

	// Append name
	Message = Player->Name + ": " + Message;

	// Send message to other players
	_Buffer NewPacket;
	NewPacket.Write<char>(Packet::CHAT_MESSAGE);
	NewPacket.Write<glm::vec4>(COLOR_WHITE);
	NewPacket.WriteString(Message.c_str());

	// Broadcast message
	auto &Objects = ObjectManager->GetObjects();
	for(auto &Object : Objects) {
		OldServerNetwork->SendPacketToPeer(&NewPacket, Object->OldPeer);
	}
}

// Handle a trade request
void _Server::HandleTradeRequest(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	// Get map object
	_Map *Map = Player->Map;
	if(!Map)
		return;

	// Find the nearest player to trade with
	_Object *TradePlayer = Map->FindTradePlayer(Player, 2.0f * 2.0f);
	if(TradePlayer == nullptr) {

		// Set up trade post
		Player->State = _Object::STATE_TRADE;
		Player->TradeGold = 0;
		Player->TradeAccepted = false;
		Player->TradePlayer = nullptr;
	}
	else {

		// Set up trade screen for both players
		SendTradeInformation(Player, TradePlayer);
		SendTradeInformation(TradePlayer, Player);

		Player->TradePlayer = TradePlayer;
		Player->TradeAccepted = false;
		TradePlayer->TradePlayer = Player;
		TradePlayer->TradeAccepted = false;

		Player->State = _Object::STATE_TRADE;
		TradePlayer->State = _Object::STATE_TRADE;
	}
}

// Handles a trade cancel
void _Server::HandleTradeCancel(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	// Notify trading player
	_Object *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {
		TradePlayer->State = _Object::STATE_TRADE;
		TradePlayer->TradePlayer = nullptr;
		TradePlayer->TradeAccepted = false;

		_Buffer NewPacket;
		NewPacket.Write<char>(Packet::TRADE_CANCEL);
		OldServerNetwork->SendPacketToPeer(&NewPacket, TradePlayer->OldPeer);
	}

	// Set state back to normal
	Player->State = _Object::STATE_WALK;
}

// Handle a trade gold update
void _Server::HandleTradeGold(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	// Set gold amount
	int Gold = Data.Read<int32_t>();
	if(Gold < 0)
		Gold = 0;
	else if(Gold > Player->Gold)
		Gold = Player->Gold;
	Player->TradeGold = Gold;
	Player->TradeAccepted = false;

	// Notify player
	_Object *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {
		TradePlayer->TradeAccepted = false;

		_Buffer NewPacket;
		NewPacket.Write<char>(Packet::TRADE_GOLD);
		NewPacket.Write<int32_t>(Gold);
		OldServerNetwork->SendPacketToPeer(&NewPacket, TradePlayer->OldPeer);
	}
}

// Handles a trade accept from a player
void _Server::HandleTradeAccept(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	// Get trading player
	_Object *TradePlayer = Player->TradePlayer;
	if(TradePlayer) {

		// Set the player's state
		bool Accepted = !!Packet->Read<char>();
		Player->TradeAccepted = Accepted;

		// Check if both player's agree
		if(Accepted && TradePlayer->TradeAccepted) {

			// Exchange items
			_InventorySlot TempItems[PLAYER_TRADEITEMS];
			for(int i = 0; i < PLAYER_TRADEITEMS; i++) {
				int InventorySlot = i + _Object::INVENTORY_TRADE;
				TempItems[i] = Player->Inventory[InventorySlot];

				Player->SetInventory(InventorySlot, &TradePlayer->Inventory[InventorySlot]);
				TradePlayer->SetInventory(InventorySlot, &TempItems[i]);
			}

			// Exchange gold
			Player->UpdateGold(TradePlayer->TradeGold - Player->TradeGold);
			TradePlayer->UpdateGold(Player->TradeGold - TradePlayer->TradeGold);

			// Send packet to players
			{
				_Buffer NewPacket;
				NewPacket.Write<char>(Packet::TRADE_EXCHANGE);
				BuildTradeItemsPacket(Player, &NewPacket, Player->Gold);
				OldServerNetwork->SendPacketToPeer(&NewPacket, Player->OldPeer);
			}
			{
				_Buffer NewPacket;
				NewPacket.Write<char>(Packet::TRADE_EXCHANGE);
				BuildTradeItemsPacket(TradePlayer, &NewPacket, TradePlayer->Gold);
				OldServerNetwork->SendPacketToPeer(&NewPacket, TradePlayer->OldPeer);
			}

			Player->State = _Object::STATE_WALK;
			Player->TradePlayer = nullptr;
			Player->TradeGold = 0;
			Player->MoveTradeToInventory();
			TradePlayer->State = _Object::STATE_WALK;
			TradePlayer->TradePlayer = nullptr;
			TradePlayer->TradeGold = 0;
			TradePlayer->MoveTradeToInventory();
		}
		else {

			// Notify trading player
			_Buffer NewPacket;
			NewPacket.Write<char>(Packet::TRADE_ACCEPT);
			NewPacket.Write<char>(Accepted);
			OldServerNetwork->SendPacketToPeer(&NewPacket, TradePlayer->OldPeer);
		}
	}
}

// Handles a teleport request
void _Server::HandleTeleport(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	Player->StartTeleport();
}

// Handles a trader accept
void _Server::HandleTraderAccept(_Buffer &Data, _Peer *Peer) {
	_Object *Player = (_Object *)Peer->data;
	if(!Player)
		return;

	const _Trader *Trader = Player->Trader;
	if(!Trader)
		return;

	// Get trader information
	int RequiredItemSlots[8];
	int RewardSlot = Player->GetRequiredItemSlots(RequiredItemSlots);
	if(RewardSlot == -1)
		return;

	// Exchange items
	Player->AcceptTrader(RequiredItemSlots, RewardSlot);
	Player->Trader = nullptr;
	Player->State = _Object::STATE_WALK;
	Player->CalculatePlayerStats();
}

// Send a message to the player
void _OldServerState::SendMessage(_Object *Player, const std::string &Message, const glm::vec4 &Color) {
	if(!Player)
		return;

	// Build message
	_Buffer Packet;
	Packet.Write<char>(Packet::CHAT_MESSAGE);
	Packet.Write<glm::vec4>(Color);
	Packet.WriteString(Message.c_str());

	// Send
	Network->SendPacket(Packet, Peer);
}

// Updates the player's HUD
void _OldServerState::SendHUD(_Object *Player) {

	_Buffer Packet;
	Packet.Write<char>(Packet::WORLD_HUD);
	Packet.Write<int32_t>(Player->Experience);
	Packet.Write<int32_t>(Player->Gold);
	Packet.Write<int32_t>(Player->Health);
	Packet.Write<int32_t>(Player->Mana);
	Packet.Write<float>(Player->HealthAccumulator);
	Packet.Write<float>(Player->ManaAccumulator);

	Network->SendPacket(Packet, Peer);
}

// Sends a player an event message
void _OldServerState::SendEvent(_Object *Player, int Type, int Data) {

	// Create packet
	_Buffer Packet;
	Packet.Write<char>(Packet::EVENT_START);
	Packet.Write<char>(Type);
	Packet.Write<int32_t>(Data);
	Packet.Write<char>(Player->Position.x);
	Packet.Write<char>(Player->Position.y);

	Network->SendPacket(Packet, Peer);
}

// Sends information to another player about items they're trading
void _OldServerState::SendTradeInformation(_Object *Sender, _Object *Receiver) {

	// Send items to trader player
	_Buffer Packet;
	Packet.Write<char>(Packet::TRADE_REQUEST);
	Packet.Write<char>(Sender->NetworkID);
	BuildTradeItemsPacket(Sender, &Packet, Sender->TradeGold);
	OldServerNetwork->SendPacketToPeer(&Packet, Receiver->OldPeer);
}

// Adds trade item information to a packet
void _OldServerState::BuildTradeItemsPacket(_Object *Player, _Buffer *Packet, int Gold) {
	Packet->Write<int32_t>(Gold);
	for(int i = _Object::INVENTORY_TRADE; i < _Object::INVENTORY_COUNT; i++) {
		if(Player->Inventory[i].Item) {
			Packet->Write<int32_t>(Player->Inventory[i].Item->ID);
			Packet->Write<char>(Player->Inventory[i].Count);
		}
		else
			Packet->Write<int32_t>(0);
	}
}

// Removes a player from a battle and deletes the battle if necessary
void _OldServerState::RemovePlayerFromBattle(_Object *Player) {
	_ServerBattle *Battle = (_ServerBattle *)Player->Battle;
	if(!Battle)
		return;

	// Delete instance
	if(Battle->RemoveFighter(Player) == 0) {

		// Loop through loaded battles
		for(auto BattleIterator = Battles.begin(); BattleIterator != Battles.end(); ++BattleIterator) {
			if(*BattleIterator == Battle) {
				Battles.erase(BattleIterator);
				delete Battle;
				return;
			}
		}
	}
}

// Deletes an object on the server and broadcasts it to the clients
void _OldServerState::DeleteObject(_Object *Object) {

	// Remove the object from their current map
	_Map *Map = Object->Map;
	if(Map)
		Map->RemoveObject(Object);
}

// Called when object gets deleted
void ObjectDeleted(_Object *Object) {

	OldServerState.DeleteObject(Object);
}

// Teleports a player back to town
void _OldServerState::PlayerTeleport(_Object *Player) {
	Player->RestoreHealthMana();
	SpawnPlayer(Player, Player->SpawnMapID, _Map::EVENT_SPAWN, Player->SpawnPoint);
	SendHUD(Player);
	Player->Save();
}

*/