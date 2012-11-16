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
#ifndef STATE_H
#define STATE_H

// Libraries
#include <irrlicht/irrlicht.h>
#include <enet/enet.h>

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Classes
class StateClass {

	public:

		// Setup functions
		virtual int Init() { return 1; }
		virtual int Close() { return 1; }

		virtual ~StateClass() { }

		// Events
		virtual bool HandleKeyPress(EKEY_CODE Key) { return false; }
		virtual bool HandleKeyRelease(EKEY_CODE Key) { return false; }
		virtual bool HandleMousePress(int Button, int MouseX, int MouseY) { return false; }
		virtual void HandleMouseRelease(int Button, int MouseX, int MouseY) { }
		virtual void HandleMouseWheel(float Direction) { }
		virtual void HandleMouseMotion(int MouseX, int MouseY) { }
		virtual void HandleGUI(EGUI_EVENT_TYPE EventType, IGUIElement *Element) { }

		virtual void HandleConnect(ENetEvent *Event) { }
		virtual void HandleDisconnect(ENetEvent *Event) { }
		virtual void HandlePacket(ENetEvent *Event) { }

		// Update
		virtual void Update(u32 FrameTime) { }
		virtual void Draw() { };

	private:

};

#endif
