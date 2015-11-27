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
#include <network/clientnetwork.h>
#include <network/peer.h>
#include <enet/enet.h>
#include <buffer.h>
#include <stdexcept>

// Constructor
_ClientNetwork::_ClientNetwork()
:	ConnectionState(State::DISCONNECTED),
	Peer(nullptr) {

	// Create client connection
	Connection = enet_host_create(nullptr, 1, 0, 0, 0);
	if(!Connection)
		throw std::runtime_error("enet_host_create failed");

	Peer = new _Peer(nullptr);
}

// Destructor
_ClientNetwork::~_ClientNetwork() {
	delete Peer;
}

// Connect to a host
void _ClientNetwork::Connect(const std::string &HostAddress, int Port) {
	if(!CanConnect())
		return;

	// Get server address
	ENetAddress Address;
	enet_address_set_host(&Address, HostAddress.c_str());
	Address.port = Port;

	// Connect to server
	_ENetPeer *ENetPeer = enet_host_connect(Connection, &Address, 2, 0);
	if(ENetPeer == nullptr)
		throw std::runtime_error("enet_host_connect returned nullptr");

	Peer->ENetPeer = ENetPeer;
	ConnectionState = State::CONNECTING;
}

// Disconnect from the host
void _ClientNetwork::Disconnect(bool Force) {

	// Soft disconnect
	if(CanDisconnect() || Force) {

		// Disconnect from host
		if(Peer->ENetPeer)
			enet_peer_disconnect(Peer->ENetPeer, 0);

		// Force disconnection state
		if(Force)
			ConnectionState = State::DISCONNECTED;
		else
			ConnectionState = State::DISCONNECTING;
	}
}

// Create a _NetworkEvent from an enet event
void _ClientNetwork::CreateEvent(_NetworkEvent &Event, double Time, ENetEvent &EEvent) {

	// Create network event
	Event.Time = Time;
	Event.Type = _NetworkEvent::EventType(EEvent.type-1);
}

// Handle the event internally
void _ClientNetwork::HandleEvent(_NetworkEvent &Event, ENetEvent &EEvent) {

	// Add peer
	switch(Event.Type) {
		case _NetworkEvent::CONNECT: {
			ConnectionState = State::CONNECTED;
		} break;
		case _NetworkEvent::DISCONNECT:
			ConnectionState = State::DISCONNECTED;
		break;
		case _NetworkEvent::PACKET: {
			Event.Data = new _Buffer((char *)EEvent.packet->data, EEvent.packet->dataLength);
			enet_packet_destroy(EEvent.packet);
		} break;
	}
}

// Send a packet
void _ClientNetwork::SendPacket(_Buffer &Buffer, SendType Type, uint8_t Channel) {

	// Create enet packet
	ENetPacket *EPacket = enet_packet_create(Buffer.GetData(), Buffer.GetCurrentSize(), Type);

	// Send packet
	enet_peer_send(Peer->ENetPeer, Channel, EPacket);
	enet_host_flush(Connection);
}

// Get round trip time
uint32_t _ClientNetwork::GetRTT() {
	if(!Peer)
		return 0;

	return Peer->ENetPeer->roundTripTime;
}
