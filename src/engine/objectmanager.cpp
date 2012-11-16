/*************************************************************************************
*	godmode - http://godmode.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#include "objectmanager.h"
#include "../objects/object.h"
#include "../objects/player.h"

static const int MAX_OBJECTS = 200;

// Constructor
ObjectManagerClass::ObjectManagerClass() {

	ObjectDeletedCallback = NULL;
	ObjectArray = new ObjectClass *[MAX_OBJECTS];
	for(int i = 0; i < MAX_OBJECTS; i++)
		ObjectArray[i] = NULL;

	NextNetworkID = 0;
}

// Destructor
ObjectManagerClass::~ObjectManagerClass() {

	ClearObjects();

	delete[] ObjectArray;
}

// Adds an object to the manager
ObjectClass *ObjectManagerClass::AddObject(ObjectClass *Object) {

	if(Object != NULL) {
	
		// Assign the object a network ID
		int NetworkID = GetNextNetworkID();
		if(NetworkID == -1) {
			printf("No available network ID\n");
			return NULL;
		}

		Object->NetworkID = NetworkID;
		Objects.push_back(Object);
		ObjectArray[NetworkID] = Object;
	}

	return Object;
}

// Adds an object with an assigned network ID
ObjectClass *ObjectManagerClass::AddObjectWithNetworkID(ObjectClass *Object, int NetworkID) {

	if(Object != NULL && NetworkID < MAX_OBJECTS) {
		Object->NetworkID = NetworkID;
		Objects.push_back(Object);
		ObjectArray[NetworkID] = Object;
	}

	return Object;
}

// Deletes an object
void ObjectManagerClass::DeleteObject(ObjectClass *Object) {

	Object->Deleted = true;
}

// Deletes all of the objects
void ObjectManagerClass::ClearObjects() {

	// Delete objects
	for(list<ObjectClass *>::Iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		delete (*Iterator);
	}

	Objects.clear();
}

// Deletes all the objects except for one
void ObjectManagerClass::DeletesObjectsExcept(ObjectClass *DeleteObject) {
	for(list<ObjectClass *>::Iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		ObjectClass *Object = *Iterator;
		if(Object != DeleteObject)
			Object->Deleted = true;
	}
}

// Updates all objects in the scene
void ObjectManagerClass::Update(u32 FrameTime) {

	// Update objects
	for(list<ObjectClass *>::Iterator Iterator = Objects.begin(); Iterator != Objects.end(); ) {
		ObjectClass *Object = *Iterator;

		// Update the object
		Object->Update(FrameTime);

		// Delete old objects
		if(Object->Deleted) {

			if(ObjectDeletedCallback != NULL) {
				ObjectDeletedCallback(Object);
			}

			ObjectArray[Object->NetworkID] = NULL;

			delete Object;			
			Iterator = Objects.erase(Iterator);
		}
		else {
			
			++Iterator;
		}
	}

}

// Renders all of the objects
void ObjectManagerClass::Render() {
	for(list<ObjectClass *>::Iterator Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		ObjectClass *Object = *Iterator;
		Object->RenderWorld();
	}
}

// Creates an object from a template ID
ObjectClass *ObjectManagerClass::CreateObjectFromTemplate(int TemplateID) {

	ObjectClass *NewObject = NULL;
	NewObject = new PlayerClass();

	return NewObject;
}

// Returns the first available network id
int ObjectManagerClass::GetNextNetworkID() {

	for(int i = 0; i < MAX_OBJECTS; i++) {
		if(ObjectArray[NextNetworkID] == NULL)
			return NextNetworkID;

		NextNetworkID++;
		if(NextNetworkID >= MAX_OBJECTS)
			NextNetworkID = 0;
	}

	return -1;
}

// Returns an object given a network ID
ObjectClass *ObjectManagerClass::GetObjectFromNetworkID(int ID) {

	if(ID < 0 || ID >= MAX_OBJECTS)
		return NULL;

	return ObjectArray[ID];
}
