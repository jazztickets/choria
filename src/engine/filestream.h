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
#ifndef FILESTREAM_H
#define FILESTREAM_H

// Libraries
#include <fstream>

// Classes
class FileClass {

	public:

		FileClass();
		~FileClass();

		int OpenForWrite(const char *TFilename);
		int OpenForRead(const char *TFilename);
		void Close();

		void WriteChar(char TData);
		void WriteInt(int TData);
		void WriteStruct(void *TData, int TSize);
		void WriteString(const char *TData);

		char ReadChar();
		int ReadInt();
		void ReadStruct(void *TData, int TSize);
		void ReadString(char *TData);

	private:

		std::fstream File;

};

#endif
