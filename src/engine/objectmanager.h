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
#ifndef OBJECTMANAGER_H
#define OBJECTMANAGER_H

// Libraries
#include <irrlicht/irrlicht.h>

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

		void Update(u32 FrameTime);
		void Render();
		void SetObjectDeletedCallback(void (* Callback)(ObjectClass *Object)) { ObjectDeletedCallback = Callback; }

		void ClearObjects();
		void DeletesObjectsExcept(ObjectClass *DeleteObject);
		ObjectClass *CreateObjectFromTemplate(int TemplateID);
		ObjectClass *AddObject(ObjectClass *Object);
		ObjectClass *AddObjectWithNetworkID(ObjectClass *Object, int NetworkID);
		void DeleteObject(ObjectClass *Object);

		int GetObjectCount() const { return Objects.getSize(); }
		const list<ObjectClass *> &GetObjects() const { return Objects; }
		ObjectClass *GetObjectFromNetworkID(int ID);

	private:

		int GetNextNetworkID();
		list<ObjectClass *> Objects;

		// Networking
		ObjectClass **ObjectArray;
		void (* ObjectDeletedCallback)(ObjectClass *Object);

		int NextNetworkID;
};

#endif
