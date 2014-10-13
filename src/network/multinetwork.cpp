/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2010  Alan Witkowski
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
**************************************************************************************/
#include "multinetwork.h"
#include "packetstream.h"
#include "../engine/game.h"
#include "../engine/state.h"

// Initializes the network system
int MultiNetworkClass::Init(bool TServer) {
	Peer = NULL;
	Connection = NULL;
	Active = false;

	if(TServer) {

		// Set up port
		ENetAddress Address;
		Address.host = ENET_HOST_ANY;
		Address.port = NETWORKING_PORT;

		// Create listener connection
		Connection = enet_host_create(&Address, 250, 0, 0, 0);
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
int MultiNetworkClass::Connect(const char *TIPAddress) {

	// Get server address
    ENetAddress Address;
    enet_address_set_host(&Address, TIPAddress);
    Address.port = NETWORKING_PORT;

    // Connect to server
    Peer = enet_host_connect(Connection, &Address, 2, 0);
    if(Peer == NULL) {
		return 0;
    }

	Active = true;

	return 1;
}

// Disconnect from the host or disconnect a client
void MultiNetworkClass::Disconnect(ENetPeer *TPeer) {
	if(TPeer)
		enet_peer_disconnect(TPeer, 0);
	else if(Peer)
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
	else if(Active) {
		ENetEvent Event;
		while(enet_host_service(Connection, &Event, 1000) > 0) {
			if(Event.type == ENET_EVENT_TYPE_DISCONNECT) {
				Peer = NULL;
				return;
			}
		}
	}
}

// Get round trip time
enet_uint32 MultiNetworkClass::GetRTT() {
	if(Peer)
		return Peer->roundTripTime;

	return 0;
}

// Update enet
void MultiNetworkClass::Update() {
	if(!Active)
		return;

	StateClass *State = Game::Instance().GetState();

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
void MultiNetworkClass::SendPacketToHost(PacketClass *TPacket) {
	if(!Peer)
		return;

	// Resize
	TPacket->Shrink();

    // Send packet
	enet_peer_send(Peer, TPacket->GetChannel(), TPacket->GetENetPacket());
}

// Server: Sends a packet to a single peer
void MultiNetworkClass::SendPacketToPeer(PacketClass *TPacket, ENetPeer *TPeer) {
	if(!TPeer)
		return;

	// Resize
	TPacket->Shrink();

    // Send packet
    enet_peer_send(TPeer, TPacket->GetChannel(), TPacket->GetENetPacket());
}
