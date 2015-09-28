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
#include <network/singlenetwork.h>
#include <network/packetstream.h>
#include <game.h>
#include <game.h>
#include <states/playserver.h>

// Initializes the network system
int SingleNetworkClass::Init(bool TServer) {

	Server = TServer;
	Connected = false;
	Peer.address.host = 0;
	Peer.address.port = 0;

	return 1;
}

// Closes the network system
int SingleNetworkClass::Close() {

	return 1;
}

// Connect to a host
int SingleNetworkClass::Connect(const char *TIPAddress) {

	if(Connected)
		return 0;

	Connected = true;

	// Simulate the connect
	ENetEvent Event;
	Event.peer = &Peer;
	PlayServerState.HandleConnect(&Event);
	Game.GetState()->HandleConnect(&Event);

	return 1;
}

// Disconnect from the host
void SingleNetworkClass::Disconnect(ENetPeer *TPeer) {

	if(Connected) {
		Connected = false;

		// Simulate the disconnect
		ENetEvent Event;
		Event.peer = &Peer;
		PlayServerState.HandleDisconnect(&Event);
		Game.GetState()->HandleDisconnect(&Event);
	}
}

// Client: Sends a packet to the host
void SingleNetworkClass::SendPacketToHost(PacketClass *TPacket) {

	if(Connected) {

		// Simulate packet event
		ENetEvent Event;
		Event.peer = &Peer;
		Event.packet = TPacket->GetENetPacket();
		PlayServerState.HandlePacket(&Event);
		enet_packet_destroy(Event.packet);
	}
}

// Server: Sends a packet to a single peer
void SingleNetworkClass::SendPacketToPeer(PacketClass *TPacket, ENetPeer *TPeer) {

	// Simulate packet event
	ENetEvent Event;
	Event.peer = &Peer;
	Event.packet = TPacket->GetENetPacket();
	Game.GetState()->HandlePacket(&Event);
	enet_packet_destroy(Event.packet);
}
