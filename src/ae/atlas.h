/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

// Forward Declarations
class _Texture;

// Classes
class _Atlas {

	public:

		_Atlas(const _Texture *Texture, const glm::vec2 &Size, float Padding);
		~_Atlas();

		glm::vec4 GetTextureCoords(uint32_t Index) const;

		// Attributes
		const _Texture *Texture;

		// Dimension for a single texture in the atlas
		glm::vec2 Size;

	private:

		// Internal attributes
		glm::vec2 TexelSize;
		glm::vec2 TextureSizeInTexels;
		float Padding;
		uint32_t Columns;

};
