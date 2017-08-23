/******************************************************************************
* Copyright (c) 2017 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*******************************************************************************/
#pragma once

// Libraries
#include <ae/network.h>

// Forward Declarations
class _Buffer;
class _Peer;

// Classes
class _ClientNetwork : public _Network {

	public:

		// Different states for connection
		enum class State {
			DISCONNECTED,
			CONNECTING,
			CONNECTED,
			DISCONNECTING,
		};

		_ClientNetwork();
		~_ClientNetwork();

		// Connections
		void Connect(const std::string &HostAddress, uint16_t Port);
		void Disconnect(bool Force=false);

		// Stats
		uint32_t GetRTT();

		// Packets
		void SendPacket(_Buffer &Buffer, SendType Type=RELIABLE, uint8_t Channel=0);

		// State
		bool IsDisconnected() { return ConnectionState == State::DISCONNECTED; }
		bool IsConnected() { return ConnectionState == State::CONNECTED; }
		bool CanConnect() { return IsDisconnected(); }
		bool CanDisconnect() { return ConnectionState == State::CONNECTED; }
		State GetConnectionState() { return ConnectionState; }

	private:

		void CreateEvent(_NetworkEvent &Event, double Time, ENetEvent &EEvent) override;
		void HandleEvent(_NetworkEvent &Event, ENetEvent &EEvent) override;

		// State
		State ConnectionState;

		// Peers
		_Peer *Peer;
};
