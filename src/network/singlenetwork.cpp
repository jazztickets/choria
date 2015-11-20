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
#include <network/singlenetwork.h>
#include <buffer.h>
#include <states/server.h>
#include <framework.h>

// Initializes the network system
void _SingleNetwork::Init(bool Server) {
	this->Server = Server;
	Connected = false;
	DummyPeer.address.host = 0;
	DummyPeer.address.port = 0;
}

// Closes the network system
void _SingleNetwork::Close() {
}

// Connect to a host
int _SingleNetwork::Connect(const char *IPAddress, uint16_t Port) {

	if(Connected)
		return 0;

	Connected = true;

	// Simulate the connect
	ENetEvent Event;
	Event.peer = &DummyPeer;
	ServerState.HandleConnect(&Event);
	Framework.GetState()->HandleConnect(&Event);

	return 1;
}

// Disconnect from the host
void _SingleNetwork::Disconnect(ENetPeer *Peer) {

	if(Connected) {
		Connected = false;

		// Simulate the disconnect
		ENetEvent Event;
		Event.peer = &DummyPeer;
		ServerState.HandleDisconnect(&Event);
		Framework.GetState()->HandleDisconnect(&Event);
	}
}

// Client: Sends a packet to the host
void _SingleNetwork::SendPacketToHost(_Buffer *Buffer, SendType Type, uint8_t Channel) {

	if(Connected) {

		// Simulate packet event
		ENetEvent Event;
		Event.peer = &DummyPeer;
		Event.packet = enet_packet_create(Buffer->GetData(), Buffer->GetCurrentSize(), Type);
		ServerState.HandlePacket(&Event);
		enet_packet_destroy(Event.packet);
	}
}

// Server: Sends a packet to a single peer
void _SingleNetwork::SendPacketToPeer(_Buffer *Buffer, ENetPeer *Peer, SendType Type, uint8_t Channel) {

	// Simulate packet event
	ENetEvent Event;
	Event.peer = &DummyPeer;
	Event.packet = enet_packet_create(Buffer->GetData(), Buffer->GetCurrentSize(), Type);
	Framework.GetState()->HandlePacket(&Event);
	enet_packet_destroy(Event.packet);
}
