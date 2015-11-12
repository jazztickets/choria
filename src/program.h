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

#include <opengl.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <string>

class _Shader;

// Program
class _Program {

	public:

		_Program(const _Shader *VertexShader, const _Shader *FragmentShader, int Attribs);
		~_Program();

		void Use() const;

		GLuint ID;
		GLuint ViewProjectionTransformID;
		GLuint ModelTransformID;
		GLuint LightPositionID;
		GLuint LightAttenuationID;
		GLuint AmbientLightID;
		int Attribs;

		glm::vec3 LightPosition;
		glm::vec3 LightAttenuation;
		glm::vec4 AmbientLight;

	private:

		GLuint SamplerIDs[4];

};

// Shader
class _Shader {

	public:

		_Shader(const std::string &Path, GLenum ProgramType);
		~_Shader();

		GLuint ID;

	private:

};
