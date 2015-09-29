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
#include <list>
#include <cstdint>

// Forward Declarations
class _Map;
class _Battle;
class _ClientBattle;
class _ServerBattle;

// Classes
class _Instance {

	public:

		_Instance();
		~_Instance();

		void Update(uint32_t TDeltaTime);

		// Maps
		_Map *GetMap(int TMapID);

		// Battles
		_ClientBattle *CreateClientBattle();
		_ServerBattle *CreateServerBattle();
		void DeleteBattle(_Battle *TBattle);

	private:

		std::list<_Map *> Maps;
		std::list<_Map *>::iterator MapIterator;
		std::list<_Battle *> Battles;
		std::list<_Battle *>::iterator BattleIterator;

};
