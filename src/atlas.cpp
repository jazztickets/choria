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
#include <atlas.h>
#include <texture.h>

// Constructor
_Atlas::_Atlas(const _Texture *Texture, const glm::vec2 &Size, float Padding) :
	Texture(Texture),
	Size(Size),
	TexelSize(1.0f / (glm::vec2)Texture->Size),
	TextureSizeInTexels(Size / (glm::vec2)Texture->Size),
	Padding(Padding) {

	Columns = (uint32_t)(Texture->Size.x / Size.x);
}

// Destructor
_Atlas::~_Atlas() {
}

// Returns coords given a texture index
glm::vec4 _Atlas::GetTextureCoords(uint32_t Index) const {
	float X = Index % Columns;
	float Y = Index / Columns;

	float TexelOffsetX = TexelSize.x + X * (Size.x + Padding * 2.0f) * TexelSize.x;
	float TexelOffsetY = TexelSize.y + Y * (Size.y + Padding * 2.0f) * TexelSize.y;

	return glm::vec4(TexelOffsetX, TexelOffsetY, TexelOffsetX + TextureSizeInTexels.x, TexelOffsetY + TextureSizeInTexels.y);
}
