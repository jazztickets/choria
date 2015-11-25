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
#include <network/network.h>

// Forward Declarations
class _Buffer;
class _Peer;

class _ServerNetwork : public _Network {

	public:

		_ServerNetwork(int NetworkPort);
		~_ServerNetwork();

		// Connections
		void DisconnectAll();

		// Packets
		void SendPacket(const _Buffer &Buffer, const _Peer *Peer, SendType Type=RELIABLE, uint8_t Channel=0);
		void BroadcastPacket(const _Buffer &Buffer, _Peer *ExceptionPeer, SendType Type=RELIABLE, uint8_t Channel=0);

		// Peers
		const std::list<_Peer *> &GetPeers() const { return Peers; }
		void DeletePeer(_Peer *Peer);

	private:

		void CreateEvent(_NetworkEvent &Event, double Time, ENetEvent &EEvent) override;
		void HandleEvent(_NetworkEvent &Event, ENetEvent &EEvent) override;

		// Delete peers and empty list
		void ClearPeers();

		// Peers
		std::list<_Peer *> Peers;
};
