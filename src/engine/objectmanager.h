/******************************************************************************
*	godmode - http://godmode.googlecode.com/
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
#pragma once

// Libraries
#include <irrlicht.h>

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Forward Declarations
class ObjectClass;
class MapClass;

// Classes
class ObjectManagerClass {

	public:

		ObjectManagerClass();
		~ObjectManagerClass();

		void Update(u32 TDeltaTime);
		void Render(const MapClass *TMap, ObjectClass *TClientPlayer=NULL);
		void SetObjectDeletedCallback(void (* Callback)(ObjectClass *TObject)) { ObjectDeletedCallback = Callback; }

		void ClearObjects();
		void DeletesObjectsExcept(ObjectClass *TObject);
		ObjectClass *CreateObjectFromTemplate(int TTemplateID);
		ObjectClass *AddObject(ObjectClass *TObject);
		ObjectClass *AddObjectWithNetworkID(ObjectClass *TObject, int TNetworkID);
		void DeleteObject(ObjectClass *TObject);

		int GetObjectCount() const { return Objects.getSize(); }
		const list<ObjectClass *> &GetObjects() const { return Objects; }
		ObjectClass *GetObjectFromNetworkID(int TID);

	private:

		int GetNextNetworkID();
		list<ObjectClass *> Objects;

		// Networking
		ObjectClass **ObjectArray;
		void (* ObjectDeletedCallback)(ObjectClass *TObject);

		int NextNetworkID;
};
