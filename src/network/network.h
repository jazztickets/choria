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

		virtual void Init(bool Server) { }
		virtual void Close() { }

		// Updates
		virtual void Update() { }

		// Connections
		virtual int Connect(const char *IPAddress, uint16_t Port) { return 0; }
		virtual void Disconnect(ENetPeer *Peer=0) { }

		virtual enet_uint32 GetRTT() { return 0; }
		virtual uint16_t GetPort() { return 0; }

		// Packets
		virtual void SendPacketToHost(_Buffer *Packet, SendType Type=RELIABLE, uint8_t Channel=0) { }
		virtual void SendPacketToPeer(_Buffer *Packet, ENetPeer *Peer, SendType Type=RELIABLE, uint8_t Channel=0) { }

		// Static functions
		static void InitializeSystem();
		static void CloseSystem();

	protected:

};
