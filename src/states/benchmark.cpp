/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2018  Alan Witkowski
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
#include <states/benchmark.h>
#include <ae/graphics.h>
#include <ae/ui.h>
#include <ae/assets.h>
#include <ae/camera.h>
#include <ae/random.h>
#include <ae/atlas.h>
#include <ae/texture.h>
#include <ae/program.h>
#include <framework.h>
#include <constants.h>
#include <SDL_scancode.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

const int CALLS = 1000;

_BenchmarkState BenchmarkState;

// Constructor
_BenchmarkState::_BenchmarkState() :
	Camera(nullptr),
	Atlas(nullptr),
	Stage(STAGE_COLOR_STATIC),
	//Stage(STAGE_ATLAS_ZEROBUFFER),
	Frames(0) {
}

// Initialize
void _BenchmarkState::Init() {
	Camera = new ae::_Camera(glm::vec3(0.0f, 0.0f, CAMERA_DISTANCE), CAMERA_DIVISOR, CAMERA_FOVY, CAMERA_NEAR, CAMERA_FAR);
	Camera->CalculateFrustum(ae::Graphics.AspectRatio);

	ae::RandomGenerator.seed(0);

	// Create buffers
	{
		float Vertices[] = {
			0.0f, 1.0f,
			1.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 0.0f,
		};

		glGenBuffers(1, &VertexBuffer[VBO_STATIC]);
		glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[VBO_STATIC]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
	}

	{
		float Vertices[CALLS * 8];
		glGenBuffers(1, &VertexBuffer[VBO_DYNAMIC]);
		glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[VBO_DYNAMIC]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_DYNAMIC_DRAW);
	}

	{
		float Vertices[] = {
			1.0f, 0.0f,
			0.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f,
			1.0f, 0.0f,
			0.0f, 0.0f,
			1.0f, 1.0f,
			0.0f, 1.0f,
		};

		glGenBuffers(1, &VertexBuffer[VBO_ATLAS]);
		glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[VBO_ATLAS]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
	}

	{
		float Vertices[CALLS * 16];
		glGenBuffers(1, &VertexBuffer[VBO_ATLAS_DYNAMIC]);
		glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[VBO_ATLAS_DYNAMIC]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_DYNAMIC_DRAW);
	}

	Atlas = new ae::_Atlas(ae::Assets.Textures["textures/map/atlas0.png"], glm::ivec2(MAP_TILE_WIDTH, MAP_TILE_HEIGHT), 1);
}

// Close
void _BenchmarkState::Close() {
	delete Camera;
	delete Atlas;
	glDeleteBuffers(VBO_COUNT, &VertexBuffer[0]);
}

// Key handler
bool _BenchmarkState::HandleKey(const ae::_KeyEvent &KeyEvent) {
	ae::Graphics.Element->HandleKey(KeyEvent);

	if(KeyEvent.Pressed) {
		if(KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
			Framework.Done = true;
	}

	return true;
}

// Handle window updates
void _BenchmarkState::HandleWindow(uint8_t Event) {
	if(Event == SDL_WINDOWEVENT_SIZE_CHANGED) {
		if(Camera)
			Camera->CalculateFrustum(ae::Graphics.AspectRatio);
	}
}

// Handle quit events
void _BenchmarkState::HandleQuit() {
	Framework.Done = true;
}

// Update
void _BenchmarkState::Update(double FrameTime) {
	ae::Graphics.Element->Update(FrameTime, ae::Input.GetMouse());

	if(Camera) {
		Camera->Set2DPosition(glm::vec2(0.0f, 0.0f));
		Camera->Update(FrameTime);
	}

	Time += FrameTime;
	if(Time >= 3) {
		std::cout << "Stage " << Stage << " Frames " << Frames << std::endl;
		Stage++;
		Time = 0;
		Frames = 0;
	}

	if(Stage >= STAGE_COUNT) {
		Framework.Done = true;
	}
}

// Render the state
void _BenchmarkState::Render(double BlendFactor) {

	ae::Graphics.Setup2D();
	ae::Graphics.SetStaticUniforms();

	switch(Stage) {

		// Normal VBO with transform
		case STAGE_COLOR_STATIC: {
			ae::_Program *Program = ae::Assets.Programs["ortho_pos"];
			ae::Graphics.SetProgram(Program);
			ae::Graphics.SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

			glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[VBO_STATIC]);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

			for(int i = 0; i < CALLS; i++) {
				glm::vec2 Start(ae::GetRandomInt(0, ae::Graphics.CurrentSize.x), ae::GetRandomInt(0, ae::Graphics.CurrentSize.y));
				glm::vec2 End(Start + glm::vec2(64));
				glm::vec2 Size = End - Start;
				glm::mat4 Transform(1.0f);
				Transform[3][0] = Start.x;
				Transform[3][1] = Start.y;
				Transform[0][0] = Size.x;
				Transform[1][1] = Size.y;

				glUniformMatrix4fv(Program->ModelTransformID, 1, GL_FALSE, glm::value_ptr(Transform));
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}
		} break;

		// glBufferSubData
		case STAGE_COLOR_DYNAMIC: {
			ae::_Program *Program = ae::Assets.Programs["ortho_pos"];
			ae::Graphics.SetProgram(Program);
			ae::Graphics.SetColor(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));

			glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[VBO_DYNAMIC]);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

			glm::mat4 Transform(1.0f);
			glUniformMatrix4fv(Program->ModelTransformID, 1, GL_FALSE, glm::value_ptr(Transform));

			float Vertices[CALLS * 8];
			int Index = 0;
			for(int i = 0; i < CALLS; i++) {
				glm::vec2 Start(ae::GetRandomInt(0, ae::Graphics.CurrentSize.x), ae::GetRandomInt(0, ae::Graphics.CurrentSize.y));
				glm::vec2 End(Start + glm::vec2(64));

				Vertices[Index++] = Start.x;
				Vertices[Index++] = End.y;
				Vertices[Index++] = End.x;
				Vertices[Index++] = End.y;
				Vertices[Index++] = Start.x;
				Vertices[Index++] = Start.y;
				Vertices[Index++] = End.x;
				Vertices[Index++] = Start.y;
			}

			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4 * CALLS);
		} break;

		// Atlas static VBO
		case STAGE_ATLAS_STATIC: {
			ae::_Program *Program = ae::Assets.Programs["test"];
			ae::Graphics.SetProgram(Program);
			ae::Graphics.SetColor(glm::vec4(1.0f));

			glUniformMatrix4fv(Program->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(ae::Graphics.Ortho));
			glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[VBO_ATLAS]);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (GLvoid *)(sizeof(float) * 8));
			glBindTexture(GL_TEXTURE_2D, Atlas->Texture->ID);

			for(int i = 0; i < CALLS; i++) {
				glm::vec2 Start(ae::GetRandomInt(0, ae::Graphics.CurrentSize.x), ae::GetRandomInt(0, ae::Graphics.CurrentSize.y));
				glm::vec2 End(Start + glm::vec2(64));
				glm::vec2 Size = End - Start;

				// Model transform
				glm::mat4 Transform(1.0f);
				Transform[3][0] = Start.x;
				Transform[3][1] = Start.y;
				Transform[0][0] = Size.x;
				Transform[1][1] = Size.y;
				glUniformMatrix4fv(Program->ModelTransformID, 1, GL_FALSE, glm::value_ptr(Transform));

				// Texture transform
				glm::vec4 TextureCoords = Atlas->GetTextureCoords(ae::GetRandomInt(1, 30));
				glm::mat4 TextureTransform(1.0f);
				TextureTransform[3][0] = TextureCoords[0];
				TextureTransform[3][1] = TextureCoords[1];
				TextureTransform[0][0] = TextureCoords[2] - TextureCoords[0];
				TextureTransform[1][1] = TextureCoords[3] - TextureCoords[1];
				glUniformMatrix4fv(Program->TextureTransformID, 1, GL_FALSE, glm::value_ptr(TextureTransform));
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}

			glDisableVertexAttribArray(1);
		} break;

		// Atlas dynamic VBO
		case STAGE_ATLAS_DYNAMIC: {
			ae::_Program *Program = ae::Assets.Programs["test"];
			ae::Graphics.SetProgram(Program);
			ae::Graphics.SetColor(glm::vec4(1.0f));

			glUniformMatrix4fv(Program->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(ae::Graphics.Ortho));
			glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[VBO_ATLAS_DYNAMIC]);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (GLvoid *)(sizeof(float) * 8));
			glBindTexture(GL_TEXTURE_2D, Atlas->Texture->ID);

			glm::mat4 Transform(1.0f);
			glm::mat4 TextureTransform(1.0f);
			glUniformMatrix4fv(Program->ModelTransformID, 1, GL_FALSE, glm::value_ptr(Transform));
			glUniformMatrix4fv(Program->TextureTransformID, 1, GL_FALSE, glm::value_ptr(TextureTransform));

			float Vertices[CALLS * 16];
			int Index = 0;
			for(int i = 0; i < CALLS; i++) {
				glm::vec2 Start(ae::GetRandomInt(0, ae::Graphics.CurrentSize.x), ae::GetRandomInt(0, ae::Graphics.CurrentSize.y));
				glm::vec2 End(Start + glm::vec2(64));
				glm::vec4 TextureCoords = Atlas->GetTextureCoords(ae::GetRandomInt(1, 30));

				Vertices[Index++] = End.x;
				Vertices[Index++] = Start.y;
				Vertices[Index++] = Start.x;
				Vertices[Index++] = Start.y;
				Vertices[Index++] = End.x;
				Vertices[Index++] = End.y;
				Vertices[Index++] = Start.x;
				Vertices[Index++] = End.y;

				Vertices[Index++] = TextureCoords[2];
				Vertices[Index++] = TextureCoords[1];
				Vertices[Index++] = TextureCoords[0];
				Vertices[Index++] = TextureCoords[1];
				Vertices[Index++] = TextureCoords[2];
				Vertices[Index++] = TextureCoords[3];
				Vertices[Index++] = TextureCoords[0];
				Vertices[Index++] = TextureCoords[3];
			}

			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4 * CALLS);
			glDisableVertexAttribArray(1);
		} break;
	}

	Frames++;
}
