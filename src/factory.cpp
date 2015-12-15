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
#include <factory.h>
#include <objects/object.h>
#include <instances/map.h>
#include <limits>

// Constructor
template <class T>
_Factory<T>::_Factory() :
	NextID(0) {

}

// Destructor
template <class T>
_Factory<T>::~_Factory() {

}

// Generate object with new network id
template <class T>
T *_Factory<T>::Create() {

	// Search for an empty slot
	for(NetworkIDType i = 0; i <= std::numeric_limits<NetworkIDType>::max(); i++) {
		if(!Objects[NextID]) {
			T *Object = new T;
			Object->NetworkID = NextID;
			Objects[NextID] = Object;

			return Object;
		}

		NextID++;
	}

	throw std::runtime_error("Ran out of object ids");
}

// Create object with existing id
template <class T>
T *_Factory<T>::CreateWithID(NetworkIDType ID) {
	T *Object = new T;
	Object->NetworkID = ID;

	Objects[ID] = Object;

	return Object;
}

template class _Factory<_Object>;
template class _Factory<_Map>;