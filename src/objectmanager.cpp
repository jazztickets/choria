/******************************************************************************
* godmode - http://godmode.googlecode.com/
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
#include <objectmanager.h>
#include <constants.h>
#include <objects/object.h>
#include <objects/player.h>

// Constructor
_ObjectManager::_ObjectManager() {

	ObjectDeletedCallback = nullptr;
	ObjectArray = new _Object *[MAX_OBJECTS];
	for(int i = 0; i < MAX_OBJECTS; i++)
		ObjectArray[i] = nullptr;

	NextNetworkID = 0;
}

// Destructor
_ObjectManager::~_ObjectManager() {

	ClearObjects();

	delete[] ObjectArray;
}

// Adds an object to the manager
_Object *_ObjectManager::AddObject(_Object *Object) {

	if(Object != nullptr) {

		// Assign the object a network ID
		int NetworkID = GetNextNetworkID();
		if(NetworkID == -1) {
			printf("No available network ID\n");
			return nullptr;
		}

		Object->NetworkID = NetworkID;
		Objects.push_back(Object);
		ObjectArray[NetworkID] = Object;
	}

	return Object;
}

// Adds an object with an assigned network ID
_Object *_ObjectManager::AddObjectWithNetworkID(_Object *Object, int NetworkID) {

	if(Object != nullptr && NetworkID < MAX_OBJECTS) {
		Object->NetworkID = NetworkID;
		Objects.push_back(Object);
		ObjectArray[NetworkID] = Object;
	}

	return Object;
}

// Deletes an object
void _ObjectManager::DeleteObject(_Object *Object) {
	Object->Deleted = true;
}

// Deletes all of the objects
void _ObjectManager::ClearObjects() {

	// Delete objects
	for(auto Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		delete (*Iterator);
	}

	Objects.clear();
}

// Deletes all the objects except for one
void _ObjectManager::DeletesObjectsExcept(_Object *ExceptionObject) {
	for(const auto &Object : Objects) {
		if(Object != ExceptionObject)
			Object->Deleted = true;
	}
}

// Updates all objects in the scene
void _ObjectManager::Update(double FrameTime) {

	// Update objects
	for(auto Iterator = Objects.begin(); Iterator != Objects.end(); ) {
		_Object *Object = *Iterator;

		// Update the object
		Object->Update(FrameTime);

		// Delete old objects
		if(Object->Deleted) {

			if(ObjectDeletedCallback != nullptr) {
				ObjectDeletedCallback(Object);
			}

			ObjectArray[(int)Object->NetworkID] = nullptr;

			delete Object;
			Iterator = Objects.erase(Iterator);
		}
		else {

			++Iterator;
		}
	}

}

// Renders all of the objects
void _ObjectManager::Render(_Object *ClientPlayer) {
	for(auto Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		_Object *Object = *Iterator;
		Object->RenderWorld(ClientPlayer);
	}
}

// Creates an object from a template ID
_Object *_ObjectManager::CreateObjectFromTemplate() {

	_Object *NewObject = nullptr;
	NewObject = new _Player();

	return NewObject;
}

// Returns the first available network id
int _ObjectManager::GetNextNetworkID() {

	for(int i = 0; i < MAX_OBJECTS; i++) {
		if(ObjectArray[NextNetworkID] == nullptr)
			return NextNetworkID;

		NextNetworkID++;
		if(NextNetworkID >= MAX_OBJECTS)
			NextNetworkID = 0;
	}

	return -1;
}

// Returns an object given a network ID
_Object *_ObjectManager::GetObjectFromNetworkID(int ID) {
	if(ID < 0 || ID >= MAX_OBJECTS)
		return nullptr;

	return ObjectArray[ID];
}
