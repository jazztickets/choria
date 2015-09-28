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
#pragma once

// Libraries
#include <irrlicht.h>
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
		virtual bool HandleKeyPress(EKEY_CODE TKey) { return false; }
		virtual bool HandleKeyRelease(EKEY_CODE TKey) { return false; }
		virtual bool HandleMousePress(int TButton, int TMouseX, int TMouseY) { return false; }
		virtual void HandleMouseRelease(int TButton, int TMouseX, int TMouseY) { }
		virtual void HandleMouseWheel(float TDirection) { }
		virtual void HandleMouseMotion(int TMouseX, int TMouseY) { }
		virtual void HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement) { }

		virtual void HandleConnect(ENetEvent *TEvent) { }
		virtual void HandleDisconnect(ENetEvent *TEvent) { }
		virtual void HandlePacket(ENetEvent *TEvent) { }

		// Update
		virtual void Update(u32 TDeltaTime) { }
		virtual void Draw() { };

	private:

};
