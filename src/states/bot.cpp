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
#include <states/bot.h>
#include <network/peer.h>
#include <network/clientnetwork.h>
#include <objects/object.h>
#include <objects/battle.h>
#include <framework.h>
#include <buffer.h>
#include <stats.h>
#include <iomanip>

_BotState BotState;

// Command loop
void RunCommandThread() {
	std::cout << "start" << std::endl;

	bool Done = false;
	while(!Done) {
		std::string Input;
		std::getline(std::cin, Input);
		if(Input == "stop" || std::cin.eof() == 1) {
			BotState.Quit();
			Done = true;
		}
		else if(Input == "d" || Input == "disconnect")
			BotState.Network->Disconnect();
		else
			std::cout << "Command not recognized" << std::endl;
	}
}

// Constructor
_BotState::_BotState() :
	Done(false),
	Thread(nullptr) {

}

// Init
void _BotState::Init() {

	try {
		Thread = new std::thread(RunCommandThread);
	}
	catch(std::exception &Error) {
		std::cerr << Error.what() << std::endl;
		Framework.Done = true;
	}

	Network = new _ClientNetwork();
	Network->Connect("127.0.0.1", 31234);
}

// Close
void _BotState::Close() {
	if(Thread) {
		Thread->join();
		delete Thread;
	}

	if(Network)
		Network->Disconnect();

	delete Network;
}

// Exit
void _BotState::Quit() {
	Done = true;

	if(Network->IsConnected())
		Network->Disconnect();
}

// Update
void _BotState::Update(double FrameTime) {

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
				Packet.WriteString("a");
				Packet.WriteString("a");
				Packet.Write<uint64_t>(0);
				Network->SendPacket(Packet);
			} break;
			case _NetworkEvent::DISCONNECT:
				if(Done)
					Framework.Done = true;
			break;
			case _NetworkEvent::PACKET:
				HandlePacket(*NetworkEvent.Data);
				delete NetworkEvent.Data;
			break;
		}
	}

}

// Handle packet
void _BotState::HandlePacket(_Buffer &Data) {
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
			//HandleObjectStats(Data);
		break;
		case PacketType::WORLD_CHANGEMAPS:
			//HandleChangeMaps(Data);
		break;
		case PacketType::WORLD_OBJECTLIST:
			//HandleObjectList(Data);
		break;
		case PacketType::WORLD_CREATEOBJECT:
			//HandleObjectCreate(Data);
		break;
		case PacketType::WORLD_DELETEOBJECT:
			//HandleObjectDelete(Data);
		break;
		case PacketType::WORLD_OBJECTUPDATES:
			//HandleObjectUpdates(Data);
		break;
		case PacketType::WORLD_POSITION:
			//HandlePlayerPosition(Data);
		break;
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
