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
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <string>

class _Shader;

// Program
class _Program {

	public:

		_Program(const std::string &Name, const _Shader *VertexShader, const _Shader *FragmentShader, int Attribs);
		~_Program();

		void Use() const;

		std::string Name;

		GLuint ID;
		GLint ViewProjectionTransformID;
		GLint ModelTransformID;
		GLint LightPositionID;
		GLint LightAttenuationID;
		GLint AmbientLightID;
		int Attribs;

		glm::vec3 LightPosition;
		glm::vec3 LightAttenuation;
		glm::vec4 AmbientLight;

	private:

		GLint SamplerIDs[4];

};

// Shader
class _Shader {

	public:

		_Shader(const std::string &Path, GLenum ProgramType);
		~_Shader();

		GLuint ID;

	private:

};
