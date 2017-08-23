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
#include <ae/program.h>
#include <ae/graphics.h>
#include <ae/util.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

// Load a program from two shaders
_Program::_Program(const std::string &Name, const _Shader *VertexShader, const _Shader *FragmentShader, int Attribs) :
	Name(Name),
	ViewProjectionTransformID(-1),
	ModelTransformID(-1),
	LightPositionID(-1),
	LightAttenuationID(-1),
	AmbientLightID(-1),
	Attribs(Attribs),
	LightAttenuation(1.0f, 0.0f, 0.0f),
	AmbientLight(1.0f) {

	// Create program
	ID = glCreateProgram();
	glAttachShader(ID, VertexShader->ID);
	glAttachShader(ID, FragmentShader->ID);
	glLinkProgram(ID);

	// Check the program
	GLint Result = GL_FALSE;
	glGetProgramiv(ID, GL_LINK_STATUS, &Result);
	if(!Result) {

		// Get error message length
		GLint ResultLength;
		glGetProgramiv(ID, GL_INFO_LOG_LENGTH, &ResultLength);

		// Get message
		std::string ErrorMessage((size_t)ResultLength, 0);
		glGetProgramInfoLog(ID, ResultLength, NULL, (GLchar *)ErrorMessage.data());

		throw std::runtime_error(ErrorMessage);
	}

	// Get uniforms
	SamplerIDs[0] = glGetUniformLocation(ID, "sampler0");
	glBindAttribLocation(ID, 0, "vertex_pos");
	glBindAttribLocation(ID, 1, "vertex_uv");
	glBindAttribLocation(ID, 2, "vertex_norm");

	ViewProjectionTransformID = glGetUniformLocation(ID, "view_projection_transform");
	ModelTransformID = glGetUniformLocation(ID, "model_transform");
	LightPositionID = glGetUniformLocation(ID, "light_position");
	LightAttenuationID = glGetUniformLocation(ID, "light_attenuation");
	AmbientLightID = glGetUniformLocation(ID, "ambient_light");
}

// Destructor
_Program::~_Program() {
	glDeleteProgram(ID);
}

// Enable the program
void _Program::Use() const {
	glUseProgram(ID);

	// Set uniforms
	if(SamplerIDs[0] != -1)
		glUniform1i(SamplerIDs[0], 0);

	if(LightPositionID != -1)
		glUniform3fv(LightPositionID, 1, &LightPosition[0]);

	if(LightAttenuationID != -1)
		glUniform3fv(LightAttenuationID, 1, &LightAttenuation[0]);

	if(AmbientLightID != -1)
		glUniform4fv(AmbientLightID, 1, &AmbientLight[0]);
}

// Loads a shader
_Shader::_Shader(const std::string &Path, GLenum ProgramType) {

	// Load program from file
	const char *ShaderSource = LoadFileIntoMemory(Path.c_str());
	if(!ShaderSource)
		throw std::runtime_error("Failed to load shader file: " + Path);

	// Create the shader
	ID = glCreateShader(ProgramType);

	// Compile shader
	glShaderSource(ID, 1, &ShaderSource, NULL);
	delete[] ShaderSource;

	glCompileShader(ID);

	// Check for errors
	GLint Result = GL_FALSE;
	glGetShaderiv(ID, GL_COMPILE_STATUS, &Result);
	if(!Result) {

		// Get error message length
		GLint ResultLength;
		glGetShaderiv(ID, GL_INFO_LOG_LENGTH, &ResultLength);

		// Get message
		std::string ErrorMessage((size_t)ResultLength, 0);
		glGetShaderInfoLog(ID, ResultLength, NULL, (GLchar *)ErrorMessage.data());

		throw std::runtime_error("Error in " + Path + '\n' + ErrorMessage);
	}
}

// Destructor
_Shader::~_Shader() {
	glDeleteShader(ID);
}