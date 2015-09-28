/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#ifndef MULTINETWORK_H
#define MULTINETWORK_H

// Libraries
#include "network.h"

class MultiNetworkClass : public NetworkClass {

	public:

		int Init(bool TServer);
		int Close();

		// Updates
		void Update();

		// Connections
		int Connect(const char *TIPAddress);
		void Disconnect(ENetPeer *TPeer=0);
		void WaitForDisconnect();

		enet_uint32 GetRTT();

		// Packets
		void SendPacketToHost(PacketClass *TPacket);
		void SendPacketToPeer(PacketClass *TPacket, ENetPeer *TPeer);

	private:

		bool Active;

		ENetHost *Connection;
		ENetPeer *Peer;
};

#endif
