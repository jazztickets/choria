/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef FILESTREAM_H
#define FILESTREAM_H

// Libraries
#include <fstream>

// Classes
class FileClass {

	public:

		FileClass();
		~FileClass();

		int OpenForWrite(const char *Filename);
		int OpenForRead(const char *Filename);
		void Close();

		void WriteChar(char Data);
		void WriteInt(int Data);
		void WriteStruct(void *Data, int Size);
		void WriteString(const char *Data);

		char ReadChar();
		int ReadInt();
		void ReadStruct(void *Data, int Size);
		void ReadString(char *Data);
		
	private:

		std::fstream File;

};

#endif
