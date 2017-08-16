/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#include <ae/peer.h>
#include <enet/enet.h>

// Constructor
_Peer::_Peer(_ENetPeer *ENetPeer) :
	ENetPeer(ENetPeer),
	Object(nullptr),
	AccountID(0),
	LastAck(0) {
}

// Destructor
_Peer::~_Peer() {
	if(ENetPeer)
		enet_peer_reset(ENetPeer);
}
