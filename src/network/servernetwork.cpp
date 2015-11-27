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
#include <network/servernetwork.h>
#include <network/peer.h>
#include <enet/enet.h>
#include <buffer.h>
#include <stdexcept>

// Constructor
_ServerNetwork::_ServerNetwork(size_t MaxPeers, uint16_t NetworkPort) {
	ENetAddress Address;
	Address.host = ENET_HOST_ANY;
	Address.port = NetworkPort;

	// Create listener connection
	Connection = enet_host_create(&Address, MaxPeers, 0, 0, 0);
}

// Destructor
_ServerNetwork::~_ServerNetwork() {
	ClearPeers();
}

// Delete a peer and remove from the list
void _ServerNetwork::DeletePeer(_Peer *Peer) {

	// Delete peer
	for(auto Iterator = Peers.begin(); Iterator != Peers.end(); ++Iterator) {
		if((*Iterator) == Peer) {
			Peers.erase(Iterator);
			delete Peer;
			break;
		}
	}
}

// Clear the peer list out
void _ServerNetwork::ClearPeers() {

	// Delete peers
	for(auto &Peer : Peers)
		delete Peer;

	Peers.clear();
}

// Disconnect all peers
void _ServerNetwork::DisconnectAll() {

	// Disconnect all connected peers
	for(auto &Peer : Peers)
		enet_peer_disconnect(Peer->ENetPeer, 0);
}

// Create a _NetworkEvent from an enet event
void _ServerNetwork::CreateEvent(_NetworkEvent &Event, double Time, ENetEvent &EEvent) {
	Event.Time = Time;
	Event.Type = _NetworkEvent::EventType(EEvent.type-1);
	if(EEvent.peer->data)
		Event.Peer = (_Peer *)EEvent.peer->data;
}

// Handle the event internally
void _ServerNetwork::HandleEvent(_NetworkEvent &Event, ENetEvent &EEvent) {

	// Add peer
	switch(Event.Type) {
		case _NetworkEvent::CONNECT: {

			// Create peer
			Event.Peer = new _Peer(EEvent.peer);

			// Set peer in enet's peer
			EEvent.peer->data = Event.Peer;
			Peers.push_back(Event.Peer);
		} break;
		case _NetworkEvent::DISCONNECT:
		break;
		case _NetworkEvent::PACKET: {
			Event.Data = new _Buffer((char *)EEvent.packet->data, EEvent.packet->dataLength);
			enet_packet_destroy(EEvent.packet);
		} break;
	}
}

// Send a packet
void _ServerNetwork::SendPacket(const _Buffer &Buffer, const _Peer *Peer, SendType Type, uint8_t Channel) {

	// Create enet packet
	ENetPacket *EPacket = enet_packet_create(Buffer.GetData(), Buffer.GetCurrentSize(), Type);

	// Send packet
	enet_peer_send(Peer->ENetPeer, Channel, EPacket);
	enet_host_flush(Connection);
}

// Send a packet to all peers
void _ServerNetwork::BroadcastPacket(const _Buffer &Buffer, _Peer *ExceptionPeer, SendType Type, uint8_t Channel) {

	for(auto &Peer : Peers) {
		if(Peer != ExceptionPeer && Peer->Object)
			SendPacket(Buffer, Peer, Type, Channel);
	}
}
