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
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

// Forward Declarations
class _Buff;
class _Stats;
class _Object;

namespace ae {
	class _Element;
	class _Buffer;
}

// Classes
class _StatusEffect {

	public:

		_StatusEffect();
		~_StatusEffect();

		void Serialize(ae::_Buffer &Data);
		void Unserialize(ae::_Buffer &Data, const _Stats *Stats);

		ae::_Element *CreateUIElement(ae::_Element *Parent);

		void Render(ae::_Element *Element, const glm::vec4 &Color);

		const _Buff *Buff;
		ae::_Element *BattleElement;
		ae::_Element *HUDElement;
		_Object *Source;
		double Time;
		double Duration;
		double MaxDuration;
		int Level;
		bool Infinite;
		bool Deleted;

};
