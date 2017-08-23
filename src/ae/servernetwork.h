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
#pragma once

// Libraries
#include <ae/network.h>

// Forward Declarations
class _Buffer;
class _Peer;

class _ServerNetwork : public _Network {

	public:

		_ServerNetwork(size_t MaxPeers, uint16_t NetworkPort);
		~_ServerNetwork();

		// Connections
		void DisconnectAll();

		// Packets
		void SendPacket(const _Buffer &Buffer, const _Peer *Peer, SendType Type=RELIABLE, uint8_t Channel=0);
		void BroadcastPacket(const _Buffer &Buffer, _Peer *ExceptionPeer, SendType Type=RELIABLE, uint8_t Channel=0);

		// Peers
		const std::list<_Peer *> &GetPeers() const { return Peers; }
		void DeletePeer(_Peer *Peer);

	private:

		void CreateEvent(_NetworkEvent &Event, double Time, ENetEvent &EEvent) override;
		void HandleEvent(_NetworkEvent &Event, ENetEvent &EEvent) override;

		// Delete peers and empty list
		void ClearPeers();

		// Peers
		std::list<_Peer *> Peers;
};
