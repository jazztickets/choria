/******************************************************************************
* Copyright (c) 2017 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*******************************************************************************/
#include <ae/clientnetwork.h>
#include <ae/peer.h>
#include <ae/buffer.h>
#include <enet/enet.h>
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
void _ClientNetwork::Connect(const std::string &HostAddress, uint16_t Port) {
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
	if(enet_peer_send(Peer->ENetPeer, Channel, EPacket) != 0)
		enet_packet_destroy(EPacket);

	//enet_host_flush(Connection);
}

// Get round trip time
uint32_t _ClientNetwork::GetRTT() {
	if(!Peer)
		return 0;

	return Peer->ENetPeer->roundTripTime;
}
