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
#include <IGUIElement.h>
#include <enet/enet.h>
#include <cstdint>

// Classes
class _State {

	public:

		// Setup functions
		virtual int Init() { return 1; }
		virtual int Close() { return 1; }

		virtual ~_State() { }

		// Events
		virtual bool HandleKeyPress(irr::EKEY_CODE TKey) { return false; }
		virtual bool HandleKeyRelease(irr::EKEY_CODE TKey) { return false; }
		virtual bool HandleMousePress(int TButton, int TMouseX, int TMouseY) { return false; }
		virtual void HandleMouseRelease(int TButton, int TMouseX, int TMouseY) { }
		virtual void HandleMouseWheel(float TDirection) { }
		virtual void HandleMouseMotion(int TMouseX, int TMouseY) { }
		virtual void HandleGUI(irr::gui::EGUI_EVENT_TYPE TEventType, irr::gui::IGUIElement *TElement) { }

		virtual void HandleConnect(ENetEvent *TEvent) { }
		virtual void HandleDisconnect(ENetEvent *TEvent) { }
		virtual void HandlePacket(ENetEvent *TEvent) { }

		// Update
		virtual void Update(uint32_t TDeltaTime) { }
		virtual void Draw() { };

	private:

};
