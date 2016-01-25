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
#include <stats.h>
#include <constants.h>
#include <scripting.h>
#include <buffer.h>
#include <iostream>
#include <iomanip>

// Constructor
_Bot::_Bot(_Stats *Stats, const std::string &HostAddress, uint16_t Port) :
	Network(new _ClientNetwork()),
	Map(nullptr),
	Player(nullptr),
	Stats(Stats),
	Pather(nullptr) {

	ObjectManager = new _Manager<_Object>();

	Scripting = new _Scripting();
	Scripting->Setup(Stats, SCRIPTS_PATH + SCRIPTS_GAME);

	Username = "a";
	Password = "a";
	Network->Connect(HostAddress, Port);
}

// Destructor
_Bot::~_Bot() {

	delete ObjectManager;
	//DeleteBattle();
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
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::ACCOUNT_LOGININFO);
				Packet.WriteBit(0);
				Packet.WriteString(Username.c_str());
				Packet.WriteString(Password.c_str());
				Packet.Write<uint64_t>(0);
				Network->SendPacket(Packet);
			} break;
			case _NetworkEvent::DISCONNECT:
			break;
			case _NetworkEvent::PACKET:
				HandlePacket(*NetworkEvent.Data);
				delete NetworkEvent.Data;
			break;
		}
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
		case PacketType::CHARACTERS_LIST: {

			// Get count
			uint8_t CharacterCount = Data.Read<uint8_t>();
			std::cout << "character count: " << (int)CharacterCount << std::endl;

			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::CHARACTERS_PLAY);
			Packet.Write<uint8_t>(0);
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
					delete Pather;
					delete Map;
					Pather = nullptr;
					Map = nullptr;
				}

				Map = new _Map();
				Map->Clock = Clock;
				Map->NetworkID = MapID;
				Map->Load(Stats->GetMap(MapID)->File);
				Player = nullptr;

				//Pather = new micropather::MicroPather(Map, (unsigned)(Map->Size.x * Map->Size.y), 4);
				//float TotalCost;
				//int Result = Pather->Solve(Map->PositionToNode(glm::ivec2(76, 89)), Map->PositionToNode(glm::ivec2(25, 14)), &PathFound, &TotalCost);
			}
		} break;
		case PacketType::WORLD_OBJECTLIST: {

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
			_Object *Object = ObjectManager->IDMap[NetworkID];
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
			//HandleTeleportStart(Data);
		break;
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
		case PacketType::BATTLE_START:
			//HandleBattleStart(Data);
		break;
		case PacketType::BATTLE_ACTION:
			//HandleBattleAction(Data);
		break;
		case PacketType::BATTLE_LEAVE:
			//HandleBattleLeave(Data);
		break;
		case PacketType::BATTLE_END:
			//HandleBattleEnd(Data);
		break;
		case PacketType::ACTION_RESULTS:
			//HandleActionResults(Data);
		break;
		case PacketType::STAT_CHANGE: {
			//_StatChange StatChange;
			//HandleStatChange(Data, StatChange);
		} break;
		case PacketType::WORLD_HUD:
			//HandleHUD(Data);
		break;
		default:
		break;
	}
}

// Assigns the client player pointer
void _Bot::AssignPlayer(_Object *Object) {
	Player = Object;
	if(Player)
		Player->CalcLevelStats = true;

	//if(Battle)
	//	Battle->ClientPlayer = Player;
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