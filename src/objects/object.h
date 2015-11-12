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
#include <cstdint>
#include <position2d.h>

// Forward Declarations
class _Map;

// Classes
class _Object {

	public:

		enum Type {
			NONE,
			FIGHTER,
			PLAYER,
			MONSTER,
		};

		virtual void RenderWorld(const _Map *TMap, const _Object *TClientPlayer=nullptr) { }

		_Object(int TType);
		virtual ~_Object();

		virtual void Update(double FrameTime) { }

		int GetType() const { return Type; }

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
		int Type;

		// State
		bool Deleted;
		irr::core::position2di Position;

		// Networking
		char NetworkID;

};
