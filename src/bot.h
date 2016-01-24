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
#include <string>
#include <memory>

// Forward Declarations
class _Stats;
class _ClientNetwork;
class _Buffer;
class _Map;
class _Stats;

// Bot class
class _Bot {

	public:

		_Bot(_Stats *Stats, const std::string &HostAddress, uint16_t Port);
		~_Bot();

		// Update
		void Update(double FrameTime);
		void HandlePacket(_Buffer &Data);

		std::unique_ptr<_ClientNetwork> Network;

		_Map *Map;
		_Stats *Stats;

		std::string Username;
		std::string Password;
};
