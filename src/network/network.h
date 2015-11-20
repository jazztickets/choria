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
#pragma once

// Libraries
#include <enet/enet.h>
#include <cstdint>

// Forward Declarations
class _Buffer;
class _Peer;

// Network Event
struct _NetworkEvent {

	// Types
	enum EventType {
		CONNECT,
		DISCONNECT,
		PACKET,
	};

	_NetworkEvent() : Data(nullptr), Peer(nullptr) { }

	EventType Type;
	float Time;
	_Buffer *Data;
	_Peer *Peer;
};

// Classes
class _Network {

	public:

		enum PacketType {
			NETWORK_SYNCHRONIZETIME,
			VERSION,
			CHAT_MESSAGE,
			ACCOUNT_LOGININFO,
			ACCOUNT_EXISTS,
			ACCOUNT_NOTFOUND,
			ACCOUNT_ALREADYLOGGEDIN,
			ACCOUNT_SUCCESS,
			CHARACTERS_REQUEST,
			CHARACTERS_LIST,
			CHARACTERS_PLAY,
			CHARACTERS_DELETE,
			CREATECHARACTER_INFO,
			CREATECHARACTER_SUCCESS,
			CREATECHARACTER_INUSE,
			WORLD_MOVECOMMAND,
			WORLD_YOURCHARACTERINFO,
			WORLD_CHANGEMAPS,
			WORLD_CREATEOBJECT,
			WORLD_DELETEOBJECT,
			WORLD_OBJECTUPDATES,
			WORLD_HUD,
			WORLD_STARTBATTLE,
			WORLD_BUSY,
			WORLD_ATTACKPLAYER,
			WORLD_TELEPORT,
			WORLD_POSITION,
			BATTLE_COMMAND,
			BATTLE_TURNRESULTS,
			BATTLE_CLIENTDONE,
			BATTLE_END,
			EVENT_START,
			EVENT_END,
			INVENTORY_MOVE,
			INVENTORY_USE,
			INVENTORY_SPLIT,
			VENDOR_EXCHANGE,
			TRADER_ACCEPT,
			SKILLS_SKILLBAR,
			SKILLS_SKILLADJUST,
			TRADE_REQUEST,
			TRADE_ITEM,
			TRADE_GOLD,
			TRADE_ACCEPT,
			TRADE_CANCEL,
			TRADE_EXCHANGE,
		};

		// Different ways to send data
		enum SendType {
			RELIABLE = 1,
			UNSEQUENCED = 2,
		};

		// Different states for connection
		enum ConnectionStateType {
			DISCONNECTED,
			CONNECTING,
			CONNECTED,
			DISCONNECTING,
		};

		_Network();
		virtual ~_Network();

		virtual void Init(bool TServer) { }
		virtual void Close() { }

		// Updates
		virtual void Update() { }

		// Connections
		virtual int Connect(const char *TIPAddress, uint16_t Port) { return 0; }
		virtual void Disconnect(ENetPeer *TPeer=0) { }

		virtual enet_uint32 GetRTT() { return 0; }

		virtual uint16_t GetPort() { return 0; }

		// Packets
		virtual void SendPacketToHost(_Buffer *Packet, SendType Type=RELIABLE, uint8_t Channel=0) { }
		virtual void SendPacketToPeer(_Buffer *Packet, ENetPeer *TPeer, SendType Type=RELIABLE, uint8_t Channel=0) { }

		// Static functions
		static void InitializeSystem();
		static void CloseSystem();

	protected:

};
