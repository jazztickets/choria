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
#include <buffer.h>
#include <cstring>

// Constructor for a new buffer
_Buffer::_Buffer(size_t InitialSize) :
	CurrentByte(0),
	CurrentBit(0) {

	AllocatedSize = InitialSize;
	Data = new char[AllocatedSize];
}

// Constructor for an existing buffer
_Buffer::_Buffer(const char *ExistingBuffer, size_t Length) :
	CurrentByte(0),
	CurrentBit(0) {

	AllocatedSize = Length;
	Data = new char[AllocatedSize];

	memcpy(Data, ExistingBuffer, AllocatedSize);
}

// Destructor
_Buffer::~_Buffer() {

	delete[] Data;
}

// Resize the buffer
void _Buffer::Resize(size_t NewSize) {
	if(NewSize == AllocatedSize)
		return;

	// Make new buffer
	char *NewData = new char[NewSize];
	if(NewSize < AllocatedSize)
		memcpy(NewData, Data, NewSize);
	else
		memcpy(NewData, Data, AllocatedSize);

	// Delete old buffer and set size
	delete[] Data;
	Data = NewData;
	AllocatedSize = NewSize;
}

// Shrinks the buffer to the current used size
void _Buffer::Shrink() {
	size_t NewSize = CurrentByte;
	if(CurrentBit)
		NewSize++;

	Resize(NewSize);
}

// Aligns the buffer to the next byte
void _Buffer::AlignBitIndex() {

	// Check to see if some bits were written before this
	if(CurrentBit) {
		CurrentBit = 0;
		CurrentByte++;
	}
}

// Aligns the buffer to the next byte and checks for a valid size
void _Buffer::AlignAndExpand(size_t NewWriteSize) {
	AlignBitIndex();

	// Resize the buffer if needed
	size_t NewSize = CurrentByte + NewWriteSize;
	if(NewSize > AllocatedSize)
		Resize(NewSize << 1);
}

// Writes a bit to the buffer
void _Buffer::WriteBit(bool Value) {

	// If it's the first bit in the byte, clear the byte
	if(CurrentBit == 0) {
		AlignAndExpand(1);
		Data[CurrentByte] = 0;
	}

	// Write the bit
	Data[CurrentByte] |= Value << CurrentBit;

	// Increment the bit index
	CurrentBit++;
	if(CurrentBit == 8) {
		CurrentBit = 0;
		CurrentByte++;
	}
}

// Write a string to the buffer
void _Buffer::WriteString(const char *Value) {
	size_t StringLength = strlen(Value);
	AlignAndExpand(StringLength + 1);

	// Copy string to buffer
	strcpy((char *)&Data[CurrentByte], Value);
	CurrentByte += StringLength;

	// Write end of string
	Data[CurrentByte] = 0;
	CurrentByte++;
}

// Reads a bit from the buffer
bool _Buffer::ReadBit() {
	bool Bit = !!(Data[CurrentByte] & (1 << CurrentBit));

	// Increment bit index
	CurrentBit++;
	if(CurrentBit == 8) {
		CurrentBit = 0;
		CurrentByte++;
	}

	return Bit;
}

// Reads a string from the buffer
const char *_Buffer::ReadString() {
	AlignBitIndex();

	const char *String = (const char *)(&Data[CurrentByte]);
	CurrentByte += strlen(String) + 1;

	return String;
}
