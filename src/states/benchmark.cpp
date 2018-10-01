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
#include <ae/program.h>
#include <framework.h>
#include <constants.h>
#include <SDL_scancode.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

_BenchmarkState BenchmarkState;

// Constructor
_BenchmarkState::_BenchmarkState() :
	Camera(nullptr),
	Stage(0),
	Frames(0) {
}

// Initialize
void _BenchmarkState::Init() {
	Camera = new ae::_Camera(glm::vec3(0.0f, 0.0f, CAMERA_DISTANCE), CAMERA_DIVISOR, CAMERA_FOVY, CAMERA_NEAR, CAMERA_FAR);
	Camera->CalculateFrustum(ae::Graphics.AspectRatio);

	ae::RandomGenerator.seed(0);

	float Vertices[] = {
		0.0f, 1.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
	};

	// Create buffers
	glGenBuffers(1, &VertexBuffer[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &VertexBuffer[1]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_DYNAMIC_DRAW);
}

// Close
void _BenchmarkState::Close() {
	delete Camera;
	glDeleteBuffers(1, &VertexBuffer[0]);
	glDeleteBuffers(1, &VertexBuffer[1]);
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

// Mouse handler
void _BenchmarkState::HandleMouseButton(const ae::_MouseEvent &MouseEvent) {
	ae::FocusedElement = nullptr;
	if(MouseEvent.Button == SDL_BUTTON_LEFT)
		ae::Graphics.Element->HandleMouseButton(MouseEvent.Pressed);
}

// Mouse movement handler
void _BenchmarkState::HandleMouseMove(const glm::ivec2 &Position) {
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

	if(Stage > 2) {
		Framework.Done = true;
	}
}

// Render the state
void _BenchmarkState::Render(double BlendFactor) {

	ae::_Program *Program = ae::Assets.Programs["ortho_pos"];
	ae::Graphics.Setup2D();
	ae::Graphics.SetStaticUniforms();
	ae::Graphics.SetProgram(Program);

	for(int i = 0; i < 1000; i++ ) {

		// Normal VBO with transform
		if(Stage == 0) {
			glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[0]);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
			glColor4f(1.0f, 0.0f, 0.0f, 1.0f);

			glm::vec2 Start(ae::GetRandomInt(0, ae::Graphics.CurrentSize.x), ae::GetRandomInt(0, ae::Graphics.CurrentSize.y));
			glm::vec2 End(ae::GetRandomInt(0, ae::Graphics.CurrentSize.x), ae::GetRandomInt(0, ae::Graphics.CurrentSize.y));
			glm::mat4 Transform(1.0f);
			Transform = glm::translate(Transform, glm::vec3(Start, 0.0f));
			Transform = glm::scale(Transform, glm::vec3(End - Start, 0.0f));

			glUniformMatrix4fv(Program->ModelTransformID, 1, GL_FALSE, glm::value_ptr(Transform));
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
		// Deprecated zero-buffer VBO
		else if(Stage == 1) {
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glEnableVertexAttribArray(0);
			glColor4f(0.0f, 1.0f, 0.0f, 1.0f);

			glm::vec2 Start(ae::GetRandomInt(0, ae::Graphics.CurrentSize.x), ae::GetRandomInt(0, ae::Graphics.CurrentSize.y));
			glm::vec2 End(ae::GetRandomInt(0, ae::Graphics.CurrentSize.x), ae::GetRandomInt(0, ae::Graphics.CurrentSize.y));
			glm::mat4 Transform(1.0f);

			float Vertices[] = {
				Start.x, End.y,
				End.x,   End.y,
				Start.x, Start.y,
				End.x,   Start.y,
			};

			glUniformMatrix4fv(Program->ModelTransformID, 1, GL_FALSE, glm::value_ptr(Transform));
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, Vertices);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
		// glBufferSubData
		else if(Stage == 2) {
			glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[1]);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
			glColor4f(0.0f, 0.0f, 1.0f, 1.0f);

			glm::vec2 Start(ae::GetRandomInt(0, ae::Graphics.CurrentSize.x), ae::GetRandomInt(0, ae::Graphics.CurrentSize.y));
			glm::vec2 End(ae::GetRandomInt(0, ae::Graphics.CurrentSize.x), ae::GetRandomInt(0, ae::Graphics.CurrentSize.y));
			glm::mat4 Transform(1.0f);

			float Vertices[] = {
				Start.x, End.y,
				End.x,   End.y,
				Start.x, Start.y,
				End.x,   Start.y,
			};

			glUniformMatrix4fv(Program->ModelTransformID, 1, GL_FALSE, glm::value_ptr(Transform));
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
	}

	Frames++;
}
