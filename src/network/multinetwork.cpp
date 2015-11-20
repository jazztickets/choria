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
#include <network/multinetwork.h>
#include <buffer.h>
#include <framework.h>
#include <state.h>
#include <constants.h>
#include <config.h>
#include <stdexcept>

// Initializes the network system
void _MultiNetwork::Init(bool Server, uint16_t Port) {
	ClientPeer = nullptr;
	Connection = nullptr;
	Active = false;

	if(Server) {

		// Set up port
		ENetAddress Address;
		Address.host = ENET_HOST_ANY;
		Address.port = Port;

		// Create listener connection
		Connection = enet_host_create(&Address, 250, 0, 0, 0);
		if(Connection == nullptr) {
			throw std::runtime_error("enet_host_create failed");
		}

		Active = true;
	}
	else {

		// Create connection
		Connection = enet_host_create(nullptr, 1, 0, 0, 0);
		if(Connection == nullptr) {
			throw std::runtime_error("enet_host_create failed");
		}
	}
}

// Closes the network system
void _MultiNetwork::Close() {

	if(Connection)
		enet_host_destroy(Connection);
}

// Connect to a host
int _MultiNetwork::Connect(const char *IPAddress, uint16_t Port) {

	// Get server address
	ENetAddress Address;
	enet_address_set_host(&Address, IPAddress);
	Address.port = Port;

	// Connect to server
	ClientPeer = enet_host_connect(Connection, &Address, 2, 0);
	if(ClientPeer == nullptr) {
		return 0;
	}

	Active = true;

	return 1;
}

// Disconnect from the host or disconnect a client
void _MultiNetwork::Disconnect(ENetPeer *Peer) {
	if(Peer)
		enet_peer_disconnect(Peer, 0);
	else if(ClientPeer)
		enet_peer_disconnect(ClientPeer, 0);
}

// Waits for a disconnect
void _MultiNetwork::WaitForDisconnect() {

	if(ClientPeer) {
		ENetEvent Event;
		while(enet_host_service(Connection, &Event, 1000) > 0) {
			if(Event.type == ENET_EVENT_TYPE_DISCONNECT) {
				ClientPeer = nullptr;
				return;
			}
		}
	}
	else if(Active) {
		ENetEvent Event;
		while(enet_host_service(Connection, &Event, 1000) > 0) {
			if(Event.type == ENET_EVENT_TYPE_DISCONNECT) {
				ClientPeer = nullptr;
				return;
			}
		}
	}
}

// Get round trip time
enet_uint32 _MultiNetwork::GetRTT() {
	if(ClientPeer)
		return ClientPeer->roundTripTime;

	return 0;
}

// Update enet
void _MultiNetwork::Update() {
	if(!Active)
		return;

	_State *State = Framework.GetState();

	ENetEvent Event;
	while(enet_host_service(Connection, &Event, 0) > 0) {
		switch(Event.type) {
			case ENET_EVENT_TYPE_CONNECT:
				State->HandleConnect(&Event);
			break;
			case ENET_EVENT_TYPE_DISCONNECT:
				State->HandleDisconnect(&Event);
				if(ClientPeer)
					Active = false;
				ClientPeer = nullptr;
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
void _MultiNetwork::SendPacketToHost(_Buffer *Buffer, SendType Type, uint8_t Channel) {
	if(!ClientPeer)
		return;

	// Create enet packet
	ENetPacket *EPacket = enet_packet_create(Buffer->GetData(), Buffer->GetCurrentSize(), Type);

	// Send packet
	enet_peer_send(ClientPeer, Channel, EPacket);
	enet_host_flush(Connection);
}

// Server: Sends a packet to a single peer
void _MultiNetwork::SendPacketToPeer(_Buffer *Buffer, ENetPeer *Peer, SendType Type, uint8_t Channel) {
	if(!Peer)
		return;

	// Create enet packet
	ENetPacket *Packet = enet_packet_create(Buffer->GetData(), Buffer->GetCurrentSize(), Type);

	// Send packet
	enet_peer_send(Peer, Channel, Packet);
	enet_host_flush(Connection);
}

// Get port used
uint16_t _MultiNetwork::GetPort() {
	if(Connection)
		return Connection->address.port;

	return 0;
}
