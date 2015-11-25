/******************************************************************************
* choria - https://github.com/jazztickets/choria
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
#pragma once

#include <state.h>
#include <log.h>

// Forward Declarations
class _Font;
class _HUD;
class _Map;
class _Object;
class _Item;
class _Camera;
class _ClientNetwork;
class _Server;
class _Buffer;
class _Stats;

// Play state
class _ClientState : public _State {

	public:

		// Setup
		_ClientState();
		void Init() override;
		void Close() override;

		// Network
		void Connect(bool IsLocal);

		// Input
		bool HandleAction(int InputType, int Action, int Value) override;
		void KeyEvent(const _KeyEvent &KeyEvent) override;
		void MouseEvent(const _MouseEvent &MouseEvent) override;
		void WindowEvent(uint8_t Event) override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		// Parameters
		std::string Level;
		std::string SaveFilename;
		bool IsTesting;
		bool FromEditor;

		// Game
		_Stats *Stats;
		_LogFile Log;

		// Map
		_Map *Map;

		// Entities
		_Object *Player;

		// HUD
		_HUD *HUD;

		// Camera
		_Camera *Camera;

		// Network
		_ClientNetwork *Network;
		_Server *Server;
		std::string HostAddress;
		uint16_t TimeSteps;
		uint16_t LastServerTimeSteps;
		uint16_t ConnectPort;

	protected:

		void StartLocalServer();

		void HandlePacket(_Buffer &Buffer);
		void HandleConnect();
		void HandleMapInfo(_Buffer &Buffer);
		void HandleObjectList(_Buffer &Buffer);
		void HandleObjectUpdates(_Buffer &Buffer);
		void HandleObjectCreate(_Buffer &Buffer);
		void HandleObjectDelete(_Buffer &Buffer);

		bool IsPaused();

};

extern _ClientState ClientState;
