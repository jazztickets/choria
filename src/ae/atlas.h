/******************************************************************************
* Copyright (c) 2017 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
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
