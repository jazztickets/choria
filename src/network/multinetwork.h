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
#pragma once

// Libraries
#include <network/network.h>

class _MultiNetwork : public _Network {

	public:

		void Init(bool TServer, uint16_t Port);
		void Close() override;

		// Updates
		void Update() override;

		// Connections
		int Connect(const char *TIPAddress, uint16_t Port) override;
		void Disconnect(ENetPeer *TPeer=0) override;
		void WaitForDisconnect();

		// Packets
		void SendPacketToHost(_Buffer *Buffer, SendType Type=RELIABLE, uint8_t Channel=0) override;
		void SendPacketToPeer(_Buffer *Buffer, ENetPeer *TPeer, SendType Type=RELIABLE, uint8_t Channel=0) override;

		// Info
		uint16_t GetPort() override;
		enet_uint32 GetRTT() override;

	private:

		bool Active;

		ENetHost *Connection;
		ENetPeer *Peer;
};
