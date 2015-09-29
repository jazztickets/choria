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
#include <irrlicht.h>
#include <cstdint>

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Forward Declarations
class _Map;

// Classes
class ObjectClass {

	public:

		enum ObjectType {
			NONE,
			FIGHTER,
			PLAYER,
			MONSTER,
		};

		virtual void RenderWorld(const _Map *TMap, const ObjectClass *TClientPlayer=NULL) { }

		ObjectClass(ObjectType TType);
		virtual ~ObjectClass();

		virtual void Update(uint32_t TDeltaTime) { }

		ObjectType GetType() const { return Type; }

		void SetDeleted(bool TValue) { Deleted = TValue; }
		bool GetDeleted() const { return Deleted; }

		void SetNetworkID(char TValue) { NetworkID = TValue; }
		char GetNetworkID() const { return NetworkID; }

		void SetPosition(const irr::core::position2di &TPosition) { Position = TPosition; }
		const irr::core::position2di &GetPosition() const { return Position; }

		int GetMapID() const;

		// Instances
		void SetMap(_Map *TMap);
		_Map *GetMap();

	protected:

		// Instances
		_Map *Map;

		// Properties
		ObjectType Type;

		// State
		bool Deleted;
		position2di Position;

		// Networking
		char NetworkID;

};
