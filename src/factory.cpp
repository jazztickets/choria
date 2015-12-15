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
#include <limits>

// Constructor
_Factory::_Factory() :
	NextID(0) {

}

// Destructor
_Factory::~_Factory() {

}

// Generate object with new network id
_Object *_Factory::CreateObject() {

	// Search for an empty slot
	for(NetworkIDType i = 0; i <= std::numeric_limits<NetworkIDType>::max(); i++) {
		if(!Objects[NextID]) {
			_Object *Object = new _Object();
			Object->NetworkID = NextID;
			Objects[NextID] = Object;

			return Object;
		}

		NextID++;
	}

	throw std::runtime_error("Ran out of object ids");
}

// Create object with existing id
_Object *_Factory::CreateObjectWithID(NetworkIDType ID) {
	_Object *Object = new _Object();
	Object->NetworkID = ID;

	Objects[ID] = Object;

	return Object;
}
