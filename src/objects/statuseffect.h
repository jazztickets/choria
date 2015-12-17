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
#include <glm/vec2.hpp>

// Forward Declarations
class _Buff;
class _Element;
class _Buffer;
class _Stats;

// Classes
class _StatusEffect {

	public:

		_StatusEffect();
		~_StatusEffect();

		void Serialize(_Buffer &Data);
		void Unserialize(_Buffer &Data, _Stats *Stats);

		_Element *CreateUIElement(_Element *Parent);

		void Render(_Element *Element);

		const _Buff *Buff;
		_Element *BattleElement;
		_Element *HUDElement;
		double Time;
		int Level;
		int Count;
};