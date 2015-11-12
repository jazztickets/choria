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
_Object *_ObjectManager::AddObject(_Object *TObject) {

	if(TObject != nullptr) {

		// Assign the object a network ID
		int NetworkID = GetNextNetworkID();
		if(NetworkID == -1) {
			printf("No available network ID\n");
			return nullptr;
		}

		TObject->SetNetworkID(NetworkID);
		Objects.push_back(TObject);
		ObjectArray[NetworkID] = TObject;
	}

	return TObject;
}

// Adds an object with an assigned network ID
_Object *_ObjectManager::AddObjectWithNetworkID(_Object *TObject, int TNetworkID) {

	if(TObject != nullptr && TNetworkID < MAX_OBJECTS) {
		TObject->SetNetworkID(TNetworkID);
		Objects.push_back(TObject);
		ObjectArray[TNetworkID] = TObject;
	}

	return TObject;
}

// Deletes an object
void _ObjectManager::DeleteObject(_Object *TObject) {

	TObject->SetDeleted(true);
}

// Deletes all of the objects
void _ObjectManager::ClearObjects() {

	// Delete objects
	for(std::list<_Object *>::iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		delete (*Iterator);
	}

	Objects.clear();
}

// Deletes all the objects except for one
void _ObjectManager::DeletesObjectsExcept(_Object *TObject) {
	for(std::list<_Object *>::iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		_Object *Object = *Iterator;
		if(Object != TObject)
			Object->SetDeleted(true);
	}
}

// Updates all objects in the scene
void _ObjectManager::Update(double FrameTime) {

	// Update objects
	for(std::list<_Object *>::iterator Iterator = Objects.begin(); Iterator != Objects.end(); ) {
		_Object *Object = *Iterator;

		// Update the object
		Object->Update(FrameTime);

		// Delete old objects
		if(Object->GetDeleted()) {

			if(ObjectDeletedCallback != nullptr) {
				ObjectDeletedCallback(Object);
			}

			ObjectArray[(int)Object->GetNetworkID()] = nullptr;

			delete Object;
			Iterator = Objects.erase(Iterator);
		}
		else {

			++Iterator;
		}
	}

}

// Renders all of the objects
void _ObjectManager::Render(const _Map *TMap, _Object *TClientPlayer) {
	for(std::list<_Object *>::iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		_Object *Object = *Iterator;
		Object->RenderWorld(TMap, TClientPlayer);
	}
}

// Creates an object from a template ID
_Object *_ObjectManager::CreateObjectFromTemplate(int TTemplateID) {

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
_Object *_ObjectManager::GetObjectFromNetworkID(int TID) {

	if(TID < 0 || TID >= MAX_OBJECTS)
		return nullptr;

	return ObjectArray[TID];
}
