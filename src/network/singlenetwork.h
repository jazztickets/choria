/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef SINGLENETWORK_H
#define SINGLENETWORK_H

// Libraries
#include "network.h"

class SingleNetworkClass : public NetworkClass {

	public:

		int Init(bool Server);
		int Close();

		// Connections
		int Connect(const char *IPAddress);
		void Disconnect();

		// Packets
		void SendPacketToHost(PacketClass *Packet);
		void SendPacketToPeer(PacketClass *Packet, ENetPeer *Peer);	

	private:

		ENetPeer Peer;
		bool Server, Connected;
};

#endif
