/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#include <network/network.h>

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
