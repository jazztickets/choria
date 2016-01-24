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
#include <list>
#include <thread>

// Forward Declarations
class _Stats;
class _ClientNetwork;
class _Buffer;
class _Bot;
class _Stats;

// Bot manager class
class _BotsState : public _State {

	public:

		_BotsState();

		// Setup
		void Init() override;
		void Close() override;

		// Update
		void Update(double FrameTime) override;

		// Commands
		void Add();
		void DisconnectAll();
		void Quit();

	protected:

		bool Done;
		std::thread *Thread;

		std::string HostAddress;
		uint16_t Port;

		_Stats *Stats;
		std::list<_Bot *> Bots;
};

extern _BotsState BotState;
