/*************************************************************************************
*	Choria - http://choria.googlecode.com/
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
#ifndef OBJECT_H
#define OBJECT_H

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
class MapClass;

// Classes
class ObjectClass {

	public:

		enum ObjectType {
			NONE,
			PLAYER,
		};

		virtual void RenderWorld() { }

		ObjectClass(int Type);
		virtual ~ObjectClass();

		virtual void Update(u32 FrameTime) { }

		// Properties
		int Type;

		// State
		bool Deleted;
		position2di Position;

		// Networking
		int NetworkID;

		// Instances
		MapClass *Map;	

		int GetMapID() const;

	protected:	

};

#endif
