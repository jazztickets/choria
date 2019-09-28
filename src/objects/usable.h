/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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
#include <enums.h>
#include <string>
#include <cstdint>

// Forward Declarations
namespace ae {
	class _Texture;
}

// An object that goes into an action bar slot
class _Usable {

	public:

		_Usable();

		std::string ID;
		std::string Name;
		std::string Script;
		const ae::_Texture *Texture;
		uint16_t NetworkID;
		int Level;
		int MaxLevel;
		TargetType Target;
		ScopeType Scope;
		bool TargetAlive;

};
