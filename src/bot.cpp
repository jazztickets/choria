/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020 Alan Witkowski
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
#include <ae/peer.h>
#include <ae/clientnetwork.h>
#include <ae/actions.h>
#include <ae/random.h>
#include <ae/buffer.h>
#include <ae/manager.h>
#include <objects/object.h>
#include <objects/battle.h>
#include <objects/map.h>
#include <objects/components/character.h>
#include <objects/components/inventory.h>
#include <objects/components/fighter.h>
#include <objects/components/controller.h>
#include <packet.h>
#include <stats.h>
#include <constants.h>
#include <actiontype.h>
#include <scripting.h>
#include <iostream>
#include <iomanip>

// Constructor
_Bot::_Bot(const _Stats *Stats, const std::string &Username, const std::string &Password, const std::string &HostAddress, uint16_t Port) :
	Network(new ae::_ClientNetwork()),
	Map(nullptr),
	Battle(nullptr),
	Player(nullptr),
	Stats(Stats),
	Username(Username),
	Password(Password) {

	ObjectManager = new ae::_Manager<_Object>();

	Scripting = new _Scripting();
	Scripting->Setup(Stats, SCRIPTS_GAME);
	Script = "Bot_Client";

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
	ae::_NetworkEvent NetworkEvent;
	while(Network->GetNetworkEvent(NetworkEvent)) {

		switch(NetworkEvent.Type) {
			case ae::_NetworkEvent::CONNECT: {
				//std::cout << Username << " connected" << std::endl;

				ae::_Buffer Packet;
				Packet.Write<PacketType>(PacketType::ACCOUNT_LOGININFO);
				Packet.WriteBit(0);
				Packet.WriteString(Username.c_str());
				Packet.WriteString(Password.c_str());
				Packet.Write<uint64_t>(0);
				Network->SendPacket(Packet);
			} break;
			case ae::_NetworkEvent::DISCONNECT:
				//std::cout << Username << " disconnected" << std::endl;
				ObjectManager->Clear();
				AssignPlayer(nullptr);

				delete Battle;
				delete Map;
				Battle = nullptr;
				Map = nullptr;
			break;
			case ae::_NetworkEvent::PACKET:
				HandlePacket(*NetworkEvent.Data);
				delete NetworkEvent.Data;
			break;
		}
	}

	if(!Player || !Map)
		return;

	// Respawn
	if(!Player->Controller->WaitForServer && !Player->Character->IsAlive()) {
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_RESPAWN);
		Network->SendPacket(Packet);

		Player->Controller->WaitForServer = true;
		return;
	}

	// Call ai script
	if(Scripting->StartMethodCall(Script, "Update")) {
		Scripting->PushReal(FrameTime);
		Scripting->PushObject(Player);
		Scripting->MethodCall(2, 0);
		Scripting->FinishMethodCall();
	}

	// Set input
	if(Player->Character->AcceptingMoveInput()) {
		int InputState = 0;

		// Call ai input script
		if(Scripting->StartMethodCall(Script, "GetInputState")) {
			Scripting->PushObject(Player);
			Scripting->MethodCall(1, 1);
			InputState = Scripting->GetInt(1);
			Scripting->FinishMethodCall();
		}

		Player->Controller->InputStates.clear();
		if(InputState)
			Player->Controller->InputStates.push_back(InputState);
	}

	// Update objects
	ObjectManager->Update(FrameTime);

	// Update map
	Map->Update(FrameTime);

	// Send input to server
	if(Player->Controller->DirectionMoved) {
		if(Player->Character->Path.size())
			Player->Character->Path.erase(Player->Character->Path.begin());

		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::WORLD_MOVECOMMAND);
		Packet.Write<char>((char)Player->Controller->DirectionMoved);
		Network->SendPacket(Packet);
	}

	// Update battle system
	if(Battle) {
		if(!Player->Fighter->PotentialAction.IsSet()) {
			Battle->ClientHandleInput(Action::GAME_SKILL1);
			Battle->ClientHandleInput(Action::GAME_SKILL1);
		}

		if(!Player->Character->Battle) {
			delete Battle;
			Battle = nullptr;
		} else
			Battle->Update(FrameTime);
	}
}

// Handle packet
void _Bot::HandlePacket(ae::_Buffer &Data) {
	PacketType Type = Data.Read<PacketType>();

	//std::cout << (int)Type << std::endl;

	switch(Type) {
		case PacketType::ACCOUNT_SUCCESS: {

			// Request character list
			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::CHARACTERS_REQUEST);
			Network->SendPacket(Packet);
		} break;
		case PacketType::ACCOUNT_NOTFOUND: {
			ae::_Buffer Packet;
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
			for(std::size_t i = 0; i < CharacterCount; i++) {
				std::size_t Slot = Data.Read<uint8_t>();
				Data.Read<uint8_t>();
				Data.ReadString();
				Data.Read<uint8_t>();
				Data.Read<int>();
				Data.Read<int64_t>();
				Data.Read<int16_t>();
				Data.Read<int16_t>();

				if(FirstSlot == -1)
					FirstSlot = (int)Slot;
			}

			// Create character
			if(FirstSlot == -1) {
				std::string Name = Username + "_" + std::to_string(ae::GetRandomInt(0, 1000));
				ae::_Buffer Packet;
				Packet.Write<PacketType>(PacketType::CREATECHARACTER_INFO);
				Packet.WriteBit(0);
				Packet.WriteString(Name.c_str());
				Packet.Write<uint32_t>(1);
				Packet.Write<uint32_t>(1);
				Packet.Write<uint8_t>(0);
				Network->SendPacket(Packet);

				FirstSlot = 0;
			}

			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::CHARACTERS_PLAY);
			Packet.WriteBit(false);
			Packet.Write<uint8_t>((uint8_t)FirstSlot);
			Network->SendPacket(Packet);
		} break;
		case PacketType::OBJECT_STATS:
			if(Player)
				Player->UnserializeStats(Data);
		break;
		case PacketType::WORLD_CHANGEMAPS: {

			// Load map
			ae::NetworkIDType MapID = (ae::NetworkIDType)Data.Read<uint32_t>();
			double Clock = Data.Read<double>();
			Data.ReadBit();

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
				Map->Headless = true;
				Map->Load(&Stats->Maps.at(MapID));
				AssignPlayer(nullptr);
			}
		} break;
		case PacketType::WORLD_OBJECTLIST: {
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
			}
			else {
				// Error
			}
		} break;
		case PacketType::WORLD_CREATEOBJECT: {
			if(!Map || !Player)
				break;

			// Read packet
			ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();

			// Check id
			if(NetworkID != Player->NetworkID) {

				// Create object
				CreateObject(Data, NetworkID);
			}
		} break;
		case PacketType::WORLD_DELETEOBJECT: {
			ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();

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
			Player->Controller->WaitForServer = false;
			Player->Character->TeleportTime = -1;
		} break;
		case PacketType::WORLD_TELEPORTSTART:
			Player->Character->TeleportTime = Data.Read<double>();
		break;
		case PacketType::ACTION_CLEAR: {
			ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();
			_Object *Object = ObjectManager->GetObject(NetworkID);
			if(!Object)
				return;

			Object->Character->Action.Unset();
			Object->Character->Targets.clear();
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
			Player->Controller->WaitForServer = false;

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
			ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();
			Data.Read<uint32_t>();

			// Get object
			_Object *Object = ObjectManager->GetObject(NetworkID);
			if(Object) {
				Object->UnserializeBattle(Data, Object == Player);
				Battle->AddObject(Object, Object->Fighter->BattleSide, true);
			}
		} break;
		case PacketType::BATTLE_LEAVE: {
			ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();
			_Object *Object = ObjectManager->GetObject(NetworkID);
			if(Object)
				Battle->RemoveObject(Object);
		} break;
		case PacketType::BATTLE_END: {
			if(!Player || !Battle)
				return;

			Player->Controller->WaitForServer = false;

			_StatChange StatChange;
			StatChange.Object = Player;

			// Get ending stats
			Data.Read<float>();
			Player->Character->Attributes["PlayerKills"].Int = Data.Read<int>();
			Player->Character->Attributes["MonsterKills"].Int = Data.Read<int>();
			Player->Character->Attributes["GoldLost"].Int64 = Data.Read<int64_t>();
			Player->Character->Attributes["Bounty"].Int64 = Data.Read<int64_t>();
			StatChange.Values["Experience"].Int64 = Data.Read<int64_t>();
			StatChange.Values["Gold"].Int64 = Data.Read<int64_t>();
			uint8_t ItemCount = Data.Read<uint8_t>();
			for(uint8_t i = 0; i < ItemCount; i++) {

				uint32_t ItemID = Data.Read<uint32_t>();
				const _Item *Item = Stats->Items.at(ItemID);
				int Upgrades = (int)Data.Read<uint8_t>();
				int Count = (int)Data.Read<uint8_t>();

				// Add items
				Player->Inventory->AddItem(Item, Upgrades, Count);
			}

			Player->Character->Battle = nullptr;
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
							std::size_t Index;
							if(Player->Inventory->FindItem(ActionResult.ActionUsed.Item, Index, (std::size_t)InventorySlot)) {
								Player->Inventory->UpdateItemCount(_Slot(BagType::INVENTORY, Index), -1);
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
						&& ((ActionResult.Target.HasStat("Health") && ActionResult.Target.Values["Health"].Int == 0) || ActionResult.Target.HasStat("Miss"))) {
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
			Player->Character->Attributes["Health"].Int = Data.Read<int>();
			Player->Character->Attributes["Mana"].Int = Data.Read<int>();
			Player->Character->Attributes["MaxHealth"].Int = Data.Read<int>();
			Player->Character->Attributes["MaxMana"].Int = Data.Read<int>();
			Player->Character->Attributes["Experience"].Int64 = Data.Read<int64_t>();
			Player->Character->Attributes["Gold"].Int64 = Data.Read<int64_t>();
			Player->Character->Attributes["Bounty"].Int64 = Data.Read<int64_t>();
			double Clock = Data.Read<double>();

			Player->Character->CalculateStats();

			if(Map)
				Map->Clock = Clock;
		} break;
		default:
		break;
	}
}

// Handles a stat change
void _Bot::HandleStatChange(ae::_Buffer &Data, _StatChange &StatChange) {
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
		Player->Character->CalcLevelStats = true;
		Player->Character->Path.clear();
	}

	if(Battle)
		Battle->ClientPlayer = Player;
}

// Creates an object from a buffer
_Object *_Bot::CreateObject(ae::_Buffer &Data, ae::NetworkIDType NetworkID) {

	// Create object
	_Object *Object = ObjectManager->CreateWithID(NetworkID);
	Object->CreateComponents();
	Object->Scripting = Scripting;
	Object->Stats = Stats;
	Object->Map = Map;
	Object->Character->CalcLevelStats = false;
	Object->Character->Init();
	Object->UnserializeCreate(Data);

	// Add to map
	Map->AddObject(Object);

	return Object;
}
