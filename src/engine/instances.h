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
#ifndef INSTANCE_H
#define INSTANCE_H

// Libraries
#include <irrlicht.h>

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Forward Declarations
class MapClass;
class BattleClass;
class ClientBattleClass;
class ServerBattleClass;

// Classes
class InstanceClass {

	public:

		InstanceClass();
		~InstanceClass();

		void Update(u32 FrameTime);

		// Maps
		MapClass *GetMap(int MapID);
		
		// Battles
		ClientBattleClass *CreateClientBattle();
		ServerBattleClass *CreateServerBattle();
		void DeleteBattle(BattleClass *Battle);

	private:

		list<MapClass *> Maps;
		list<MapClass *>::Iterator MapIterator;
		list<BattleClass *> Battles;
		list<BattleClass *>::Iterator BattleIterator;

};

#endif
