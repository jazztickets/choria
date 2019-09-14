/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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
#include <ae/state.h>
#include <list>
#include <thread>
#include <mutex>

// Forward Declarations
class _Stats;
class _Bot;
class _Stats;

namespace ae {
	class _ClientNetwork;
	class _Buffer;
}

// Bot manager class
class _BotsState : public ae::_State {

	public:

		_BotsState();

		// Setup
		void Init() override;
		void Close() override;

		// Update
		void Update(double FrameTime) override;

		// Commands
		void ShowCommands();
		void Add();
		void DisconnectAll();
		void HandleQuit() override;

		std::string HostAddress;
		uint16_t Port;

	protected:

		bool Done;
		std::thread *Thread;
		std::mutex Mutex;

		const _Stats *Stats;
		std::list<_Bot *> Bots;

		int NextBotNumber;
};

extern _BotsState BotState;
