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
#include <packet.h>
#include <manager.h>
#include <string>
#include <memory>
#include <list>
#include <glm/vec2.hpp>

// Forward Declarations
class _Stats;
class _ClientNetwork;
class _Buffer;
class _Object;
class _Battle;
class _Map;
class _Stats;
class _Scripting;
struct _Event;
struct _StatChange;

namespace micropather {
	class MicroPather;
}

enum class GoalStateType : int {
	NONE,
	FARMING,
	HEALING
};

enum class BotStateType : int {
	IDLE,
	MOVE_PATH,
	MOVE_RANDOM,
	MOVE_HEAL,
};

// Bot class
class _Bot {

	public:

		_Bot(_Stats *Stats, const std::string &Username, const std::string &Password, const std::string &HostAddress, uint16_t Port);
		~_Bot();

		// Update
		void Update(double FrameTime);

		// Network
		void HandlePacket(_Buffer &Data);
		void AssignPlayer(_Object *Object);
		void HandleStatChange(_Buffer &Data, _StatChange &StatChange);
		_Object *CreateObject(_Buffer &Data, NetworkIDType NetworkID);

		// AI
		void EvaluateGoal();
		void DetermineNextGoal();
		bool FindEvent(const _Event &Event, glm::ivec2 &Position);
		void MoveTo(const glm::ivec2 &StartPosition, const glm::ivec2 &EndPosition);
		int GetNextInputState();
		void SetGoal(GoalStateType NewGoal);

		std::unique_ptr<_ClientNetwork> Network;

		_Manager<_Object> *ObjectManager;

		_Scripting *Scripting;
		_Map *Map;
		_Battle *Battle;
		_Object *Player;
		_Stats *Stats;

		micropather::MicroPather *Pather;
		std::list<void *> Path;

		GoalStateType GoalState;
		BotStateType BotState;

		std::string Username;
		std::string Password;

};
