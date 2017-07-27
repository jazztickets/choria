/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
#include <bot.h>
#include <network/peer.h>
#include <network/clientnetwork.h>
#include <objects/object.h>
#include <objects/battle.h>
#include <objects/map.h>
#include <objects/inventory.h>
#include <stats.h>
#include <constants.h>
#include <actions.h>
#include <actiontype.h>
#include <random.h>
#include <scripting.h>
#include <buffer.h>
#include <iostream>
#include <iomanip>

// Constructor
_Bot::_Bot(_Stats *Stats, const std::string &Username, const std::string &Password, const std::string &HostAddress, uint16_t Port) :
	Network(new _ClientNetwork()),
	Map(nullptr),
	Battle(nullptr),
	Player(nullptr),
	Stats(Stats),
	Username(Username),
	Password(Password) {

	ObjectManager = new _Manager<_Object>();

	Scripting = new _Scripting();
	Scripting->Setup(Stats, SCRIPTS_PATH + SCRIPTS_GAME);
	Script = "Bot_Basic";

	Network->Connect(HostAddress, Port);
}

// Destructor
_Bot::~_Bot() {

	delete ObjectManager;
	delete Battle;
	delete Map;
	delete Scripting;
}

// Update
void _Bot::Update(double FrameTime) {

	// Update network
	Network->Update(FrameTime);

	// Get events
	_NetworkEvent NetworkEvent;
	while(Network->GetNetworkEvent(NetworkEvent)) {

		switch(NetworkEvent.Type) {
			case _NetworkEvent::CONNECT: {
				//std::cout << Username << " connected" << std::endl;

				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::ACCOUNT_LOGININFO);
				Packet.WriteBit(0);
				Packet.WriteString(Username.c_str());
				Packet.WriteString(Password.c_str());
				Packet.Write<uint64_t>(0);
				Network->SendPacket(Packet);
			} break;
			case _NetworkEvent::DISCONNECT:
				//std::cout << Username << " disconnected" << std::endl;
				ObjectManager->Clear();
				AssignPlayer(nullptr);

				delete Battle;
				delete Map;
				Battle = nullptr;
				Map = nullptr;
			break;
			case _NetworkEvent::PACKET:
				HandlePacket(*NetworkEvent.Data);
				delete NetworkEvent.Data;
			break;
		}
	}

	if(!Player || !Map)
		return;

	// Respawn
	if(!Player->WaitForServer && !Player->IsAlive()) {
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_RESPAWN);
		Network->SendPacket(Packet);

		Player->WaitForServer = true;
		return;
	}

	// Call ai script
	if(Scripting->StartMethodCall(Script, "Update")) {
		Scripting->PushObject(Player);
		Scripting->MethodCall(1, 0);
		Scripting->FinishMethodCall();
	}

	// Set input
	if(Player->AcceptingMoveInput()) {
		int InputState = 0;

		// Call ai input script
		if(Scripting->StartMethodCall(Script, "GetInputState")) {
			Scripting->PushObject(Player);
			Scripting->MethodCall(1, 1);
			InputState = Scripting->GetInt(1);
			Scripting->FinishMethodCall();
		}

		Player->InputStates.clear();
		if(InputState)
			Player->InputStates.push_back(InputState);
	}

	// Update objects
	ObjectManager->Update(FrameTime);

	// Update map
	Map->Update(FrameTime);

	// Send input to server
	if(Player->Moved) {
		if(Player->Path.size())
			Player->Path.erase(Player->Path.begin());

		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_MOVECOMMAND);
		Packet.Write<char>((char)Player->Moved);
		Network->SendPacket(Packet);
	}

	// Update battle system
	if(Battle) {
		if(!Player->PotentialAction.IsSet()) {
			Battle->ClientHandleInput(Action::SKILL1);
			Battle->ClientHandleInput(Action::SKILL1);
		}

		if(!Player->Battle) {
			delete Battle;
			Battle = nullptr;
		} else
			Battle->Update(FrameTime);
	}
}

// Handle packet
void _Bot::HandlePacket(_Buffer &Data) {
	PacketType Type = Data.Read<PacketType>();

	//std::cout << (int)Type << std::endl;

	switch(Type) {
		case PacketType::ACCOUNT_SUCCESS: {

			// Request character list
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::CHARACTERS_REQUEST);
			Network->SendPacket(Packet);
		} break;
		case PacketType::ACCOUNT_NOTFOUND: {
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::ACCOUNT_LOGININFO);
			Packet.WriteBit(1);
			Packet.WriteString(Username.c_str());
			Packet.WriteString(Password.c_str());
			Packet.Write<uint64_t>(0);
			Network->SendPacket(Packet);
		} break;
		case PacketType::CHARACTERS_LIST: {

			// Get count
			Data.Read<uint8_t>();
			uint8_t CharacterCount = Data.Read<uint8_t>();
			//std::cout << "character count: " << (int)CharacterCount << std::endl;

			// Get characters
			int FirstSlot = -1;
			for(size_t i = 0; i < CharacterCount; i++) {
				size_t Slot = Data.Read<uint8_t>();
				Data.Read<uint8_t>();
				Data.ReadString();
				Data.Read<uint32_t>();
				Data.Read<int>();
				Data.Read<int>();

				if(FirstSlot == -1)
					FirstSlot = (int)Slot;
			}

			// Create character
			if(FirstSlot == -1) {
				std::string Name = Username + "_" + std::to_string(GetRandomInt(0, 1000));
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::CREATECHARACTER_INFO);
				Packet.WriteBit(0);
				Packet.WriteString(Name.c_str());
				Packet.Write<uint32_t>(1);
				Packet.Write<uint32_t>(1);
				Packet.Write<uint8_t>(0);
				Network->SendPacket(Packet);

				FirstSlot = 0;
			}

			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::CHARACTERS_PLAY);
			Packet.Write<uint8_t>((uint8_t)FirstSlot);
			Network->SendPacket(Packet);
		} break;
		case PacketType::OBJECT_STATS:
			if(Player)
				Player->UnserializeStats(Data);
		break;
		case PacketType::WORLD_CHANGEMAPS: {

			// Load map
			NetworkIDType MapID = (NetworkIDType)Data.Read<uint32_t>();
			double Clock = Data.Read<double>();

			// Delete old map and create new
			if(!Map || Map->NetworkID != MapID) {
				if(Map) {
					delete Map;
					Map = nullptr;
				}

				Map = new _Map();
				Map->Stats = Stats;
				Map->Clock = Clock;
				Map->NetworkID = MapID;
				Map->Load(Stats->GetMap(MapID));
				AssignPlayer(nullptr);
			}
		} break;
		case PacketType::WORLD_OBJECTLIST: {
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
			}
			else {
				// Error
			}
		} break;
		case PacketType::WORLD_CREATEOBJECT: {
			if(!Map || !Player)
				break;

			// Read packet
			NetworkIDType NetworkID = Data.Read<NetworkIDType>();

			// Check id
			if(NetworkID != Player->NetworkID) {

				// Create object
				CreateObject(Data, NetworkID);
			}
		} break;
		case PacketType::WORLD_DELETEOBJECT: {
			NetworkIDType NetworkID = Data.Read<NetworkIDType>();

			// Get object
			_Object *Object = ObjectManager->GetObject(NetworkID);
			if(Object && Object != Player) {
				Object->Deleted = true;
			}
		} break;
		case PacketType::WORLD_OBJECTUPDATES:
			//HandleObjectUpdates(Data);
		break;
		case PacketType::WORLD_POSITION: {
			if(!Player)
				break;

			Player->Position = Data.Read<glm::ivec2>();
			Player->WaitForServer = false;
			Player->TeleportTime = -1;
		} break;
		case PacketType::WORLD_TELEPORTSTART:
			Player->TeleportTime = Data.Read<double>();
		break;
		case PacketType::ACTION_CLEAR: {
			NetworkIDType NetworkID = Data.Read<NetworkIDType>();
			_Object *Object = ObjectManager->GetObject(NetworkID);
			if(!Object)
				return;

			Object->Action.Unset();
			Object->Targets.clear();
		} break;
		case PacketType::EVENT_START:
			//HandleEventStart(Data);
		break;
		case PacketType::CHAT_MESSAGE:
			//HandleChatMessage(Data);
		break;
		case PacketType::INVENTORY:
			//HandleInventory(Data);
		break;
		case PacketType::INVENTORY_USE:
			//HandleInventoryUse(Data);
		break;
		case PacketType::INVENTORY_SWAP:
			//HandleInventorySwap(Data);
		break;
		case PacketType::INVENTORY_UPDATE:
			//HandleInventoryUpdate(Data);
		break;
		case PacketType::INVENTORY_GOLD:
			//HandleInventoryGold(Data);
		break;
		case PacketType::TRADE_REQUEST:
			//HandleTradeRequest(Data);
		break;
		case PacketType::TRADE_CANCEL:
			//HandleTradeCancel(Data);
		break;
		case PacketType::TRADE_ITEM:
			//HandleTradeItem(Data);
		break;
		case PacketType::TRADE_GOLD:
			//HandleTradeGold(Data);
		break;
		case PacketType::TRADE_ACCEPT:
			//HandleTradeAccept(Data);
		break;
		case PacketType::TRADE_EXCHANGE:
			//HandleTradeExchange(Data);
		break;
		case PacketType::BATTLE_START: {
			//std::cout << "BATTLE_START" << std::endl;

			// Already in a battle
			if(Battle)
				break;

			// Allow player to hit menu buttons
			Player->WaitForServer = false;

			// Create a new battle instance
			Battle = new _Battle();
			Battle->Manager = ObjectManager;
			Battle->Stats = Stats;
			Battle->Scripting = Scripting;
			Battle->ClientPlayer = Player;
			Battle->ClientNetwork = Network.get();

			Battle->Unserialize(Data, nullptr);
		} break;
		case PacketType::BATTLE_ACTION:
			//HandleBattleAction(Data);
		break;
		case PacketType::BATTLE_JOIN: {
			NetworkIDType NetworkID = Data.Read<NetworkIDType>();
			Data.Read<uint32_t>();

			// Get object
			_Object *Object = ObjectManager->GetObject(NetworkID);
			if(Object) {
				Object->UnserializeBattle(Data);
				Battle->AddFighter(Object, Object->BattleSide, true);
			}
		} break;
		case PacketType::BATTLE_LEAVE: {
			NetworkIDType NetworkID = Data.Read<NetworkIDType>();
			_Object *Object = ObjectManager->GetObject(NetworkID);
			if(Object)
				Battle->RemoveFighter(Object);
		} break;
		case PacketType::BATTLE_END: {
			if(!Player || !Battle)
				return;

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

				uint32_t ItemID = Data.Read<uint32_t>();
				const _Item *Item = Stats->Items[ItemID];
				int Upgrades = (int)Data.Read<uint8_t>();
				int Count = (int)Data.Read<uint8_t>();

				// Add items
				Player->Inventory->AddItem(Item, Upgrades, Count);
			}

			Player->Battle = nullptr;
			delete Battle;
			Battle = nullptr;

			// Call ai script
			if(Scripting->StartMethodCall(Script, "DetermineNextGoal")) {
				Scripting->PushObject(Player);
				Scripting->MethodCall(1, 0);
				Scripting->FinishMethodCall();
			}
		} break;
		case PacketType::ACTION_RESULTS: {

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

					// No damage dealt
					if((ActionResult.ActionUsed.GetTargetType() == TargetType::ENEMY || ActionResult.ActionUsed.GetTargetType() == TargetType::ENEMY_ALL)
					   && ((ActionResult.Target.HasStat(StatType::HEALTH) && ActionResult.Target.Values[StatType::HEALTH].Integer == 0) || ActionResult.Target.HasStat(StatType::MISS))) {
						ActionResult.Timeout = HUD_ACTIONRESULT_TIMEOUT_SHORT;
						ActionResult.Speed = HUD_ACTIONRESULT_SPEED_SHORT;
					}
					else {
						ActionResult.Timeout = HUD_ACTIONRESULT_TIMEOUT;
						ActionResult.Speed = HUD_ACTIONRESULT_SPEED;
					}

					Battle->ActionResults.push_back(ActionResult);
				}
				else if(ActionResult.Target.Object == Player) {
				}
			}
		} break;
		case PacketType::STAT_CHANGE: {
			_StatChange StatChange;
			HandleStatChange(Data, StatChange);
		} break;
		case PacketType::WORLD_HUD: {
			Player->Health = Data.Read<int>();
			Player->Mana = Data.Read<int>();
			Player->MaxHealth = Data.Read<int>();
			Player->MaxMana = Data.Read<int>();
			Player->Experience = Data.Read<int>();
			Player->Gold = Data.Read<int>();
			double Clock = Data.Read<double>();

			Player->CalculateStats();

			if(Map)
				Map->Clock = Clock;
		} break;
		default:
		break;
	}
}

// Handles a stat change
void _Bot::HandleStatChange(_Buffer &Data, _StatChange &StatChange) {
	if(!Player)
		return;

	// Get stats
	StatChange.Unserialize(Data, ObjectManager);
	if(StatChange.Object) {

		// Update object
		StatChange.Object->UpdateStats(StatChange);
	}
}

// Assigns the client player pointer
void _Bot::AssignPlayer(_Object *Object) {
	Player = Object;
	if(Player) {
		Player->CalcLevelStats = true;
		Player->Path.clear();
	}

	if(Battle)
		Battle->ClientPlayer = Player;
}

// Creates an object from a buffer
_Object *_Bot::CreateObject(_Buffer &Data, NetworkIDType NetworkID) {

	// Create object
	_Object *Object = ObjectManager->CreateWithID(NetworkID);
	Object->Scripting = Scripting;
	Object->Stats = Stats;
	Object->Map = Map;
	Object->CalcLevelStats = false;
	Object->UnserializeCreate(Data);

	// Add to map
	Map->AddObject(Object);

	return Object;
}
