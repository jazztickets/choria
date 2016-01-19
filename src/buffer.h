/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
#include <cstddef>

// Classes
class _Buffer {

	public:

		_Buffer(size_t InitialSize=32);
		_Buffer(const char *ExistingBuffer, size_t Length);
		~_Buffer();

		// Write data
		template<typename T> T *Write(const T &Value) {
			AlignAndExpand(sizeof(T));

			T *Address = (T *)&Data[CurrentByte];
			*Address = Value;

			CurrentByte += sizeof(T);
			return Address;
		}

		// Read data
		template<typename T> T Read() {
			AlignBitIndex();

			T Value = *(T *)(&Data[CurrentByte]);
			CurrentByte += sizeof(T);

			return Value;
		}

		void WriteBit(bool Value);
		void WriteString(const char *Value);

		bool ReadBit();
		const char *ReadString();

		const char *GetData() const { return Data; }
		char &operator[](size_t Index) { return Data[Index]; }
		char operator[](size_t Index) const { return Data[Index]; }

		void Shrink();
		size_t GetAllocatedSize() const { return AllocatedSize; }
		size_t GetCurrentSize() const { return CurrentByte + (CurrentBit != 0); }
		bool End() const { return CurrentByte == AllocatedSize; }

		void StartRead() { CurrentByte = 0; }

	private:

		void Resize(size_t NewSize);
		void AlignBitIndex();
		void AlignAndExpand(size_t NewWriteSize);

		char *Data;
		size_t AllocatedSize, CurrentByte;
		unsigned char CurrentBit;
};
