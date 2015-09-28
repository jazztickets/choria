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

// Classes
class ServerBattleClass : public BattleClass {

	public:

		enum StateType {
			STATE_INPUT,
			STATE_RESOLVETURN,
			STATE_END,
		};

		ServerBattleClass();
		~ServerBattleClass();

		// Setup
		void StartBattle();

		// Objects
		int RemovePlayer(PlayerClass *TPlayer);

		// Input
		void HandleInput(PlayerClass *TPlayer, int TCommand, int TTarget);

		// Updates
		void Update(u32 TDeltaTime);

		// Resolve
		void ResolveTurn();
		void CheckEnd();

	private:

		void SendPacketToPlayers(PacketClass *TPacket);
		void SendSkillToPlayers(PlayerClass *TPlayer);

		u32 RoundTime;
};
