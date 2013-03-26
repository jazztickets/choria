/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2010  Alan Witkowski
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
**************************************************************************************/
#include "packetstream.h"

// Constructor for a new packet
PacketClass::PacketClass(char TPacketType, ENetPacketFlag TFlag, enet_uint8 TChannel)
:	CurrentByte(0),
	CurrentBit(0),
	Channel(TChannel) {

	// Create the enet packet
	Packet = enet_packet_create(NULL, 32, TFlag);

	// Write the packet header
	WriteChar(TPacketType);
};

// Constructor for existing packets
PacketClass::PacketClass(ENetPacket *TPacket)
:	CurrentByte(0),
	CurrentBit(0),
	Packet(TPacket) {

}

// Shrinks the enet packet to the current used size
void PacketClass::Shrink() {

	int Size = CurrentByte;
	if(CurrentBit)
		Size++;

	enet_packet_resize(Packet, Size);
}

// Aligns the stream to the next byte
void PacketClass::AlignBitIndex() {

	// Check to see if some bits were written before this
	if(CurrentBit) {
		CurrentBit = 0;
		CurrentByte++;
	}
}

// Aligns the stream to the next byte and checks for a valid size
void PacketClass::AlignAndExpand(int TNewWriteSize) {
	AlignBitIndex();

	// Resize the packet if needed
	size_t NewPacketSize = CurrentByte + TNewWriteSize;
	if(NewPacketSize > Packet->dataLength)
		enet_packet_resize(Packet, NewPacketSize << 1);
}

// Writes a single bit to the stream
void PacketClass::WriteBit(bool TData) {

	// If it's the first bit in the byte, clear the byte
	if(CurrentBit == 0) {
		AlignAndExpand(1);
		Packet->data[CurrentByte] = 0;
	}

	// Write the bit
	Packet->data[CurrentByte] |= TData << CurrentBit;

	// Increment the bit index
	CurrentBit++;
	if(CurrentBit == 8) {
		CurrentBit = 0;
		CurrentByte++;
	}
}

// Write a single char to the stream
void PacketClass::WriteChar(char TData) {
	AlignAndExpand(1);

	Packet->data[CurrentByte] = TData;
	CurrentByte++;
}

// Write a single int to the stream
void PacketClass::WriteInt(int TData) {
	AlignAndExpand(sizeof(int));

	*((int *)&Packet->data[CurrentByte]) = TData;
	CurrentByte += sizeof(int);
}

// Write a single float to the stream
void PacketClass::WriteFloat(float TData) {
	AlignAndExpand(sizeof(float));

	*((float *)&Packet->data[CurrentByte]) = TData;
	CurrentByte += sizeof(float);
}

// Write a string to the stream
void PacketClass::WriteString(const char *TData) {
	int StringLength = strlen(TData);
	AlignAndExpand(StringLength + 1);

	// Copy string to stream
	strcpy((char *)&Packet->data[CurrentByte], TData);
	CurrentByte += StringLength;

	// Write end of string
	Packet->data[CurrentByte] = 0;
	CurrentByte++;
}

// Reads a bit from the stream
bool PacketClass::ReadBit() {

	bool Bit = !!(Packet->data[CurrentByte] & (1 << CurrentBit));

	// Increment bit index
	CurrentBit++;
	if(CurrentBit == 8) {
		CurrentBit = 0;
		CurrentByte++;
	}

	return Bit;
}

// Reads a char from the stream
char PacketClass::ReadChar() {
	AlignBitIndex();

	char Char = Packet->data[CurrentByte];
	CurrentByte++;

	return Char;
}

// Reads an int from the stream
int PacketClass::ReadInt() {
	AlignBitIndex();

	int Int = *(int *)(&Packet->data[CurrentByte]);
	CurrentByte += sizeof(int);

	return Int;
}

// Reads a float from the stream
float PacketClass::ReadFloat() {
	AlignBitIndex();

	float Float = *(float *)(&Packet->data[CurrentByte]);
	CurrentByte += sizeof(float);

	return Float;
}

// Reads a string from the stream
const char *PacketClass::ReadString() {
	AlignBitIndex();

	const char *String = (const char *)(&Packet->data[CurrentByte]);
	CurrentByte += strlen(String) + 1;

	return String;
}
