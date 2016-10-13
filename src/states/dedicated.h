/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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

// Libraries
#include <state.h>
#include <thread>

// Forward Declarations
class _Server;
class _Stats;

// Dedicated server state
class _DedicatedState : public _State {

	public:

		_DedicatedState();

		// Setup
		void Init() override;
		void Close() override;

		// Update
		void Update(double FrameTime) override;

		// State parameters
		void SetNetworkPort(uint16_t NetworkPort) { this->NetworkPort = NetworkPort; }
		void SetHardcore(bool Hardcore) { this->Hardcore = Hardcore; }

		// Commands
		void ShowCommands();
		void ShowPlayers();
		void ShowBattles();

	protected:

		_Server *Server;
		_Stats *Stats;

		std::thread *Thread;

		// Parameters
		uint16_t NetworkPort;
		bool Hardcore;

};

extern _DedicatedState DedicatedState;
