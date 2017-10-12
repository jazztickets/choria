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

#include <ae/opengl.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>
#include <cstring>

// Struct for an interleaved vertex
struct _PackedVertex {

	bool operator<(const _PackedVertex &PackedVertex) const {
		return memcmp((void *)this, (void *)&PackedVertex, sizeof(_PackedVertex)) > 0;
	}

	static const void *GetPositionOffset() { return (void *)&(((_PackedVertex *)0)->Position); }
	static const void *GetUVOffset() { return (void *)&(((_PackedVertex *)0)->UV); }
	static const void *GetNormalOffset() { return (void *)&(((_PackedVertex *)0)->Normal); }

	glm::vec3 Position;
	glm::vec2 UV;
	glm::vec3 Normal;
};

// Triangle Mesh
class _Mesh {

	public:

		enum _Flags {
			HAS_UVS     = (1 << 0),
			HAS_NORMALS = (1 << 1),
		};

		_Mesh(const std::string &Path);
		~_Mesh();

		static void ConvertOBJ(const std::string &Path);

		// Attributes
		std::string Identifier;
		uint32_t IndexCount;
		uint32_t Flags;
		uint8_t Version;

		// VBO
		GLuint VertexBufferID;
		GLuint ElementBufferID;

	private:

};
