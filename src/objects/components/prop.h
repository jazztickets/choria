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
#include <glm/vec4.hpp>
#include <cstdint>

// Forward Declarations
class _Object;
namespace ae {
	class _Texture;
}

// Classes
class _Prop {

	public:

		_Prop() : _Prop(nullptr) { }
		_Prop(_Object *Object);

		// Base
		_Object *Object;

		// Attributes
		const ae::_Texture *Texture;
		glm::vec4 Color;

	private:

};
