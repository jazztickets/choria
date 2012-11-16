/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef SERVERBATTLE_H
#define SERVERBATTLE_H

// Libraries
#include "battle.h"

// Forward Declarations
class ActionClass;

// Classes
class ServerBattleClass : public BattleClass {

	public:

		enum StateType {
			STATE_BATTLE,
			STATE_END,
		};

		ServerBattleClass();
		~ServerBattleClass();

		// Setup
		void StartBattle();

		// Objects
		int RemovePlayer(PlayerClass *Player);

		// Input
		void HandleInput(PlayerClass *Player, int ActionBarIndex, int Target);

		// Updates
		void Update(u32 FrameTime);

		// Resolve
		void ResolveAction(FighterClass *Fighter);
		void CheckEnd();

	private:

		void SendPacketToPlayers(PacketClass *Packet);
		void SendActionToPlayers(FighterClass *Fighter);
		void SendUpdateToPlayers(BattleUpdateStruct &Update);

		u32 RoundTime;
};

#endif
