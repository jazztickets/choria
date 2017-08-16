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
#include <ae/manager.h>
#include <objects/object.h>
#include <objects/map.h>
#include <objects/battle.h>
#include <limits>
#include <stdexcept>

// Constructor
template <class T>
_Manager<T>::_Manager() :
	NextID(0) {

}

// Destructor
template <class T>
_Manager<T>::~_Manager() {

	for(auto &Object : Objects)
		delete Object;
}

// Update
template <class T>
void _Manager<T>::Update(double FrameTime) {

	// Update objects
	for(auto Iterator = Objects.begin(); Iterator != Objects.end(); ) {
		T *Object = *Iterator;

		// Update the object
		Object->Update(FrameTime);

		// Move deleted objects to deleted list
		if(Object->Deleted) {
			DeleteList.push_back(Object);
			Iterator = Objects.erase(Iterator);
		}
		else {
			++Iterator;
		}
	}

	// Delete objects
	for(auto &Object : DeleteList) {
		IDMap[Object->NetworkID] = nullptr;
		delete Object;
	}

	DeleteList.clear();
}

// Generate object with new network id
template <class T>
T *_Manager<T>::Create() {

	// Search for an empty slot
	for(NetworkIDType i = 0; i <= std::numeric_limits<NetworkIDType>::max(); i++) {
		if(!IDMap[NextID]) {
			T *Object = new T;
			Object->NetworkID = NextID;

			Objects.push_back(Object);
			IDMap[NextID] = Object;

			return Object;
		}

		NextID++;
	}

	throw std::runtime_error("Ran out of object ids");
}

// Create object with existing id
template <class T>
T *_Manager<T>::CreateWithID(NetworkIDType ID) {
	T *Object = new T;
	Object->NetworkID = ID;

	Objects.push_back(Object);
	IDMap[ID] = Object;

	return Object;
}

// Get object from manager
template <class T>
T *_Manager<T>::GetObject(NetworkIDType ID) {

	auto Iterator = IDMap.find(ID);
	if(Iterator == IDMap.end())
		return nullptr;

	return Iterator->second;
}

// Delete all objects and reset
template <class T>
void _Manager<T>::Clear() {

	for(auto Object : Objects)
		delete Object;

	IDMap.clear();
	Objects.clear();
	NextID = 0;
}

template class _Manager<_Object>;
template class _Manager<_Map>;
template class _Manager<_Battle>;