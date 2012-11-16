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
#include "singlenetwork.h"
#include "packetstream.h"
#include "../engine/game.h"
#include "../engine/game.h"
#include "../server.h"

// Initializes the network system
int SingleNetworkClass::Init(bool Server) {

	this->Server = Server;
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
int SingleNetworkClass::Connect(const char *IPAddress) {

	if(Connected)
		return 0;
	
	Connected = true;

	// Simulate the connect
	ENetEvent Event;
	Event.peer = &Peer;
	ServerState.HandleConnect(&Event);
	Game.GetState()->HandleConnect(&Event);

	return 1;
}

// Disconnect from the host
void SingleNetworkClass::Disconnect() {

	if(Connected) {
		Connected = false;

		// Simulate the disconnect
		ENetEvent Event;
		Event.peer = &Peer;
		ServerState.HandleDisconnect(&Event);
		Game.GetState()->HandleDisconnect(&Event);
	}
}

// Client: Sends a packet to the host
void SingleNetworkClass::SendPacketToHost(PacketClass *Packet) {

	if(Connected) {
		
		// Simulate packet event
		ENetEvent Event;
		Event.peer = &Peer;
		Event.packet = Packet->GetENetPacket();
		ServerState.HandlePacket(&Event);
		enet_packet_destroy(Event.packet);
	}
}

// Server: Sends a packet to a single peer
void SingleNetworkClass::SendPacketToPeer(PacketClass *Packet, ENetPeer *Peer) {

	// Simulate packet event
	ENetEvent Event;
	Event.peer = &this->Peer;
	Event.packet = Packet->GetENetPacket();
	Game.GetState()->HandlePacket(&Event);
	enet_packet_destroy(Event.packet);
}
