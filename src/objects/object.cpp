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
#include <objects/object.h>
#include <instances/map.h>

// Constructor
_Object::_Object(int TType)
:	Map(nullptr),
	Type(TType),
	Deleted(false),
	Position(-1, -1),
	NetworkID(-1) {

}

// Destructor
_Object::~_Object() {

}

// Changes the object's map
void _Object::SetMap(_Map *TMap) {

	Map = TMap;
}

// Returns the object's map
_Map *_Object::GetMap() {

	return Map;
}

// Returns the object's map id
int _Object::GetMapID() const {
	if(!Map)
		return 0;

	return Map->GetID();
}
