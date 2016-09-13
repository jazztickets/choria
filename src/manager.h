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
#include <packet.h>
#include <unordered_map>
#include <list>

// Classes
template<class T> class _Manager {

	public:

		_Manager();
		~_Manager();

		// Updates
		void Update(double FrameTime);

		// Object management
		T *Create();
		T *CreateWithID(NetworkIDType ID);
		T *GetObject(NetworkIDType ID);
		void Clear();

		// Storage
		std::list<T *> Objects;
		std::list<T *> DeleteList;

	private:

		// IDs
		std::unordered_map<NetworkIDType, T *> IDMap;
		NetworkIDType NextID;

};
