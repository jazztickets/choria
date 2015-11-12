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
#include <states/playserver.h>
#include <game.h>

// Initializes the network system
int _SingleNetwork::Init(bool TServer) {

	Server = TServer;
	Connected = false;
	Peer.address.host = 0;
	Peer.address.port = 0;

	return 1;
}

// Closes the network system
int _SingleNetwork::Close() {

	return 1;
}

// Connect to a host
int _SingleNetwork::Connect(const char *TIPAddress) {

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
void _SingleNetwork::Disconnect(ENetPeer *TPeer) {

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
void _SingleNetwork::SendPacketToHost(_Buffer *Buffer, SendType Type, uint8_t Channel) {

	if(Connected) {

		// Simulate packet event
		ENetEvent Event;
		Event.peer = &Peer;
		Event.packet = enet_packet_create(Buffer->GetData(), Buffer->GetCurrentSize(), Type);
		PlayServerState.HandlePacket(&Event);
		enet_packet_destroy(Event.packet);
	}
}

// Server: Sends a packet to a single peer
void _SingleNetwork::SendPacketToPeer(_Buffer *Buffer, ENetPeer *TPeer, SendType Type, uint8_t Channel) {

	// Simulate packet event
	ENetEvent Event;
	Event.peer = &Peer;
	Event.packet = enet_packet_create(Buffer->GetData(), Buffer->GetCurrentSize(), Type);
	Game.GetState()->HandlePacket(&Event);
	enet_packet_destroy(Event.packet);
}
