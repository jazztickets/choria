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

// Libraries
#include <state.h>
#include <thread>
#include <list>
#include <glm/vec4.hpp>

// Forward Declarations
class _Database;
class _ObjectManager;
class _Map;
class _Battle;
class _Buffer;
class _Object;
class _Player;

class _ServerNetwork;

// Classes
class _OldServerState : public _State {

	public:

		void Init();
		void Close();

		void Update(double FrameTime);
		void DeleteObject(_Object *Object);

		void PlayerTeleport(_Player *Player);
		double GetServerTime() const { return ServerTime; }

		void StartCommandThread();
		void StopServer() { StopRequested = true; }
		void SendMessage(_Player *Player, const std::string &Message, const glm::vec4 &Color=glm::vec4(1.0f));

		_Map *GetMap(int MapID);

	private:

		void CreateDefaultDatabase();

		void SendPlayerPosition(_Player *Player);
		void SpawnPlayer(_Player *Player, int NewMapID, int EventType, int EventData);
		void SendHUD(_Player *Player);
		void SendCharacterList(_Player *Player);
		void SendEvent(_Player *Player, int Type, int Data);
		void SendTradeInformation(_Player *Sender, _Player *Receiver);

		void BuildTradeItemsPacket(_Player *Player, _Buffer *Packet, int Gold);

		void RemovePlayerFromBattle(_Player *Player);

		_Database *Database;
		_ObjectManager *ObjectManager;
		std::list<_Map *> Maps;
		std::list<_Battle *> Battles;

		bool StopRequested;

		std::thread *CommandThread;

		double ServerTime;
		_ServerNetwork *Network;
};

extern _OldServerState OldServerState;
