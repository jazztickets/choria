/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020 Alan Witkowski
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
#include <ae/type.h>
#include <string>
#include <memory>
#include <list>
#include <glm/vec2.hpp>

// Forward Declarations
class _Object;
class _Battle;
class _Map;
class _Stats;
class _Scripting;
struct _Event;
class _StatChange;

namespace ae {
	template<class T> class _Manager;
	class _Stats;
	class _ClientNetwork;
	class _Buffer;
}

// Bot class
class _Bot {

	public:

		_Bot(const std::string &Username, const std::string &Password, const std::string &HostAddress, uint16_t Port);
		~_Bot();

		// Update
		void Update(double FrameTime);

		// Network
		void HandlePacket(ae::_Buffer &Data);
		void AssignPlayer(_Object *Object);
		void HandleStatChange(ae::_Buffer &Data, _StatChange &StatChange);
		_Object *CreateObject(ae::_Buffer &Data, ae::NetworkIDType NetworkID);

		std::unique_ptr<ae::_ClientNetwork> Network;

		ae::_Manager<_Object> *ObjectManager;

		_Scripting *Scripting;
		_Map *Map;
		_Stats *Stats;
		_Battle *Battle;
		_Object *Player;

		std::string Script;

		std::string Username;
		std::string Password;

};
