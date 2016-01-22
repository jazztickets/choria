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
class _Stats;
class _ClientNetwork;
class _Buffer;

// Dedicated server state
class _BotState : public _State {

	public:

		_BotState();

		// Setup
		void Init() override;
		void Close() override;
		void Quit();

		// Update
		void Update(double FrameTime) override;

		// State parameters
		void SetNetworkPort(uint16_t NetworkPort) { this->NetworkPort = NetworkPort; }

		_ClientNetwork *Network;
		bool Done;

	protected:

		void HandlePacket(_Buffer &Data);

		std::string HostAddress;
		uint16_t NetworkPort;

		std::thread *Thread;
};

extern _BotState BotState;
