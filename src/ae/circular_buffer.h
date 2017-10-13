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

// Classes
template<class T> class _CircularBuffer {

	public:

		// Constructor
		_CircularBuffer() : Data(nullptr) { }

		// Constructor with size
		_CircularBuffer(int Size) {
			Init(Size);
		}

		// Destructor
		~_CircularBuffer() {
			delete[] Data;
		}

		// Initialize with size
		void Init(int Size) {
			if(Data)
				delete[] Data;

			Data = new T[Size];
			CurrentSize = 0;
			MaxSize = Size;
			ReadIndex = 0;
			WriteIndex = 0;
		}

		// Free memory
		void Close() {
			delete[] Data;
			Data = nullptr;
		}

		// Clear the buffer
		void Clear() {
			CurrentSize = 0;
			ReadIndex = 0;
			WriteIndex = 0;
		}

		// Check if buffer is empty
		bool IsEmpty() const {
			return CurrentSize == 0;
		}

		// Get size
		int Size() const {
			return CurrentSize;
		}

		// Add to back of queue
		void PushBack(const T &Value) {

			// Update size
			CurrentSize++;
			if(CurrentSize > MaxSize) {
				CurrentSize = MaxSize;
				ReadIndex++;
				if(ReadIndex >= MaxSize)
					ReadIndex = 0;
			}

			// Save data
			Data[WriteIndex] = Value;

			// Update pointers
			WriteIndex++;
			if(WriteIndex >= MaxSize)
				WriteIndex = 0;
		}

		// Pop back of queue
		void PopBack() {

			// Check size
			if(CurrentSize == 0)
				return;

			// Update buffer
			CurrentSize--;
			WriteIndex--;
			if(WriteIndex < 0)
				WriteIndex = MaxSize-1;
		}

		// Remove front of queue
		void Pop() {

			// Check size
			if(CurrentSize == 0)
				return;

			// Update buffer
			CurrentSize--;
			ReadIndex++;
			if(ReadIndex >= MaxSize)
				ReadIndex = 0;
		}

		// Get the front of the queue
		T &Front(int Depth = 0) const {
			int Index = ReadIndex + Depth;
			if(Index >= MaxSize)
				Index -= MaxSize;

			return Data[Index];
		}

		// Get the back of the queue
		T &Back(int Depth = 0) const {
			int Index = WriteIndex - Depth - 1;
			if(Index < 0)
				Index += MaxSize;

			return Data[Index];
		}

	private:

		// Data
		T *Data;

		// Size
		int CurrentSize, MaxSize;

		// Pointers
		int ReadIndex, WriteIndex;
};
