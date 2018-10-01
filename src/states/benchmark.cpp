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

_BenchmarkState BenchmarkState;

// Constructor
_BenchmarkState::_BenchmarkState() :
	Camera(nullptr) {
}

// Initialize
void _BenchmarkState::Init() {
	Camera = new ae::_Camera(glm::vec3(0.0f, 0.0f, CAMERA_DISTANCE), CAMERA_DIVISOR, CAMERA_FOVY, CAMERA_NEAR, CAMERA_FAR);
	Camera->CalculateFrustum(ae::Graphics.AspectRatio);
}

// Close
void _BenchmarkState::Close() {
	delete Camera;
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
}

// Render the state
void _BenchmarkState::Render(double BlendFactor) {
	ae::Graphics.Setup3D();
	if(Camera) {
		Camera->Set3DProjection(BlendFactor);
		ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
		glUniformMatrix4fv(ae::Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
		ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
		glUniformMatrix4fv(ae::Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
		ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv_static"]);
		glUniformMatrix4fv(ae::Assets.Programs["pos_uv_static"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
		ae::Graphics.SetProgram(ae::Assets.Programs["text"]);
		glUniformMatrix4fv(ae::Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	}

	ae::Graphics.Setup2D();
	ae::Graphics.SetStaticUniforms();
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
	ae::Graphics.DrawRectangle(glm::vec2(1, 1), glm::vec2(4, 4), true);
	ae::Graphics.SetColor(glm::vec4(1,0,0,1));
	ae::Graphics.DrawRectangle(glm::vec2(5, 1), glm::vec2(8, 4), false);

	ae::Graphics.EnableScissorTest();
	ae::_Bounds ScissorRegion(glm::vec2(9, 1), glm::vec2(12, 4));
	ae::Graphics.SetScissor(ScissorRegion);
	ae::Graphics.SetColor(glm::vec4(0,1,0,1));
	ae::Graphics.DrawRectangle(glm::vec2(8, 0), glm::vec2(13, 5), true);
	ae::Graphics.DisableScissorTest();
}
