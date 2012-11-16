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
#include "multinetwork.h"
#include "packetstream.h"
#include "../engine/game.h"
#include "../engine/state.h"

// Initializes the network system
int MultiNetworkClass::Init(bool Server) {
	Peer = NULL;
	Connection = NULL;
	Active = false;

	if(Server) {

		// Set up port
		ENetAddress Address;
		Address.host = ENET_HOST_ANY;
		Address.port = NETWORKING_PORT;

		// Create listener connection
		Connection = enet_host_create(&Address, 64, 0, 0, 0);
		if(Connection == NULL) {
			return 0;
		}

		Active = true;
	}
	else {

		// Create connection
		Connection = enet_host_create(NULL, 1, 0, 0, 0);
		if(Connection == NULL) {
			return 0;
		}
	}

	return 1;
}

// Closes the network system
int MultiNetworkClass::Close() {

	if(Connection)
		enet_host_destroy(Connection);

	return 1;
}

// Connect to a host
int MultiNetworkClass::Connect(const char *IPAddress) {

	// Get server address
    ENetAddress Address;
    enet_address_set_host(&Address, IPAddress);
    Address.port = NETWORKING_PORT;

    // Connect to server
    Peer = enet_host_connect(Connection, &Address, 2, 0);
    if(Peer == NULL) {
		return 0;
    }

	Active = true;

	return 1;
}

// Disconnect from the host
void MultiNetworkClass::Disconnect() {
	if(Peer)
		enet_peer_disconnect(Peer, 0);
}

// Waits for a disconnect
void MultiNetworkClass::WaitForDisconnect() {

	if(Peer) {
		ENetEvent Event;
		while(enet_host_service(Connection, &Event, 1000) > 0) {
			if(Event.type == ENET_EVENT_TYPE_DISCONNECT) {
				Peer = NULL;
				return;
			}
		}
	}
}

// Update enet
void MultiNetworkClass::Update() {
	if(!Active)
		return;

	StateClass *State = Game.GetState();

	ENetEvent Event;
	while(enet_host_service(Connection, &Event, 0) > 0) {
		switch(Event.type) {
			case ENET_EVENT_TYPE_CONNECT:
				State->HandleConnect(&Event);
			break;
			case ENET_EVENT_TYPE_DISCONNECT:
				State->HandleDisconnect(&Event);
				if(Peer)
					Active = false;
				Peer = NULL;
			break;
			case ENET_EVENT_TYPE_RECEIVE:
				State->HandlePacket(&Event);
				enet_packet_destroy(Event.packet);
			break;
			default:
			break;
		}
	}

}

// Client: Sends a packet to the host
void MultiNetworkClass::SendPacketToHost(PacketClass *Packet) {
	if(!Peer)
		return;

	// Resize
	Packet->Shrink();

    // Send packet
	enet_peer_send(Peer, Packet->GetChannel(), Packet->GetENetPacket());
}

// Server: Sends a packet to a single peer
void MultiNetworkClass::SendPacketToPeer(PacketClass *Packet, ENetPeer *Peer) {
	if(!Peer)
		return;

	// Resize
	Packet->Shrink();

    // Send packet
    enet_peer_send(Peer, Packet->GetChannel(), Packet->GetENetPacket());
}
