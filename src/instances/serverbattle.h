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
#include <instances/battle.h>

// Forward Declarations
class _Packet;

// Classes
class _ServerBattle : public _Battle {

	public:

		enum StateType {
			STATE_INPUT,
			STATE_RESOLVETURN,
			STATE_END,
		};

		_ServerBattle();
		~_ServerBattle();

		// Setup
		void StartBattle();

		// Objects
		int RemovePlayer(_Player *TPlayer);

		// Input
		void HandleInput(_Player *TPlayer, int TCommand, int TTarget);

		// Updates
		void Update(uint32_t TDeltaTime);

		// Resolve
		void ResolveTurn();
		void CheckEnd();

	private:

		void SendPacketToPlayers(_Packet *TPacket);
		void SendSkillToPlayers(_Player *TPlayer);

		uint32_t RoundTime;
};
