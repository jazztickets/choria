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
#include "filestream.h"
#include <cstring>

using namespace std;

// Constructor
FileClass::FileClass() {

}

// Destructor
FileClass::~FileClass() {

}

// Open for writing
int FileClass::OpenForWrite(const char *TFilename) {

	File.open(TFilename, ios::out | ios::binary);
	if(!File.is_open())
		return 0;

	return 1;
}

// Open for reading
int FileClass::OpenForRead(const char *TFilename) {

	File.open(TFilename, ios::in | ios::binary);
	if(!File.is_open())
		return 0;

	return 1;
}

// Closes the file
void FileClass::Close() {
	
	File.close();
}

// Write a char
void FileClass::WriteChar(char TData) {

	File.put(TData);
}

// Write an integer
void FileClass::WriteInt(int TData) {

	File.write(reinterpret_cast<char *>(&TData), sizeof(TData));
}

// Write a general struct
void FileClass::WriteStruct(void *TData, int TSize) {

	File.write(reinterpret_cast<char *>(TData), TSize);
}

// Writes a string
void FileClass::WriteString(const char *TData) {

	File.write(TData, strlen(TData));
	File.put(0);
}

// Reads a char
char FileClass::ReadChar() {

	return File.get();
}

// Reads an integer
int FileClass::ReadInt() {

	int Data;
	File.read(reinterpret_cast<char *>(&Data), sizeof(Data));

	return Data;
}

// Read a general struct
void FileClass::ReadStruct(void *TData, int TSize) {

	File.read(reinterpret_cast<char *>(TData), TSize);
}

// Reads a string
void FileClass::ReadString(char *TData) {

	File.get(TData, 2147483647, 0);
	File.get();
}
