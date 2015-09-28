/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
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
*******************************************************************************/
#ifndef PACKETSTREAM_H
#define PACKETSTREAM_H

// Libraries
#include <enet/enet.h>
#include <cstring>

// Classes
class PacketClass {

	public:

		PacketClass(char TPacketType, ENetPacketFlag TFlag=ENET_PACKET_FLAG_RELIABLE, enet_uint8 TChannel=0);
		PacketClass(ENetPacket *TPacket);

		ENetPacket *GetENetPacket() { return Packet; }
		enet_uint8 GetChannel() { return Channel; }
		void Shrink();

		void WriteBit(bool TData);
		void WriteChar(char TData);
		void WriteInt(int TData);
		void WriteFloat(float TData);
		void WriteString(const char *TData);

		bool ReadBit();
		char ReadChar();
		int ReadInt();
		float ReadFloat();
		const char *ReadString();

	private:

		void AlignBitIndex();
		void AlignAndExpand(int TNewWriteSize);

		// Packet information
		ENetPacket *Packet;
		enet_uint8 Channel;

		// Data
		enet_uint32 CurrentByte, CurrentBit;
};

#endif
