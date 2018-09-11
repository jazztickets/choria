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
#include <states/test.h>
#include <ae/graphics.h>
#include <ae/ui.h>
#include <ae/assets.h>
#include <ae/camera.h>
#include <ae/random.h>
#include <ae/program.h>
#include <objects/minigame.h>
#include <stats.h>
#include <framework.h>
#include <constants.h>
#include <SDL_scancode.h>
#include <SDL_timer.h>
#include <glm/gtc/type_ptr.hpp>

_TestState TestState;

// Constructor
_TestState::_TestState() :
	Camera(nullptr),
	Stats(nullptr) {
}

// Initialize
void _TestState::Init() {
	Camera = new _Camera(glm::vec3(0.0f, 0.0f, CAMERA_DISTANCE), CAMERA_DIVISOR, CAMERA_FOVY, CAMERA_NEAR, CAMERA_FAR);
	Camera->CalculateFrustum(Graphics.AspectRatio);

	Stats = new _Stats();

	double MaxTime = 0;
	double MinTime = 30;

	// gawk '{ total+=int($1/2) } END{ print NR, total, (total - NR*10) }'
	int Simulations = 0;
	for(int i = 0; i < Simulations; i++) {
		//double StartTime = SDL_GetPerformanceCounter();
		uint32_t Seed = GetRandomInt((uint32_t)1, std::numeric_limits<uint32_t>::max());
		float X = (float)GetRandomReal(-7.65, 7.65);

		Minigame = new _Minigame(&Stats->Minigames.at(1));
		Minigame->IsServer = true;
		Minigame->Debug = 0;
		Minigame->StartGame(Seed);
		Minigame->Drop(X);

		Time = 0;
		while(Time < 30) {
			Minigame->Update(DEFAULT_TIMESTEP);
			if(Minigame->State == _Minigame::StateType::DONE) {
				break;
			}
			Time += DEFAULT_TIMESTEP;
		}

		std::cout.precision(17);
		if(Time < MinTime) {
			//std::cout << "mintime=" << Time << " seed=" << Seed << " x=" << X << std::endl;
			MinTime = Time;
		}
		if(Time > MaxTime) {
			//std::cout << "maxtime=" << Time << " seed=" << Seed << " x=" << X << std::endl;
			MaxTime = Time;
		}

		if(Minigame->Bucket < Minigame->Prizes.size()) {
			const _MinigameItem *MinigameItem = Minigame->Prizes[Minigame->Bucket];
			if(MinigameItem && MinigameItem->Item) {
				std::cout << MinigameItem->Item->Cost * MinigameItem->Count << std::endl;
			}
			else
				std::cout << "0" << std::endl;
		}

		delete Minigame;

		//std::cout << (SDL_GetPerformanceCounter() - StartTime) / (double)SDL_GetPerformanceFrequency() << std::endl;
	}

	//Framework.Done = true;

	Minigame = new _Minigame(&Stats->Minigames.at(1));
	Minigame->Debug = -1;
	Minigame->StartGame(1049117602);
	Minigame->Drop(4.3513402938842773);
}

// Close
void _TestState::Close() {
	delete Stats;
	delete Minigame;
	delete Camera;
}

// Key handler
void _TestState::HandleKey(const _KeyEvent &KeyEvent) {
	Graphics.Element->HandleKey(KeyEvent);

	if(KeyEvent.Pressed) {
		if(KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
			Framework.Done = true;
	}
}

// Mouse handler
void _TestState::HandleMouseButton(const _MouseEvent &MouseEvent) {
	FocusedElement = nullptr;
	if(MouseEvent.Button == SDL_BUTTON_LEFT)
		Graphics.Element->HandleMouseButton(MouseEvent.Pressed);

	Minigame->HandleMouseButton(MouseEvent);
}

// Mouse movement handler
void _TestState::HandleMouseMove(const glm::ivec2 &Position) {
}

// Handle window updates
void _TestState::HandleWindow(uint8_t Event) {
	if(Event == SDL_WINDOWEVENT_SIZE_CHANGED) {
		if(Camera)
			Camera->CalculateFrustum(Graphics.AspectRatio);
		if(Minigame->Camera)
			Minigame->Camera->CalculateFrustum(Graphics.AspectRatio);
	}
}

// Handle quit events
void _TestState::HandleQuit() {
	Framework.Done = true;
}

// Update
void _TestState::Update(double FrameTime) {

	// Update UI
	Graphics.Element->Update(FrameTime, Input.GetMouse());

	// Update camera
	Camera->Set2DPosition(glm::vec2(0.0f, 0.0f));
	Camera->Update(FrameTime);

	Minigame->Update(FrameTime);

	Time += FrameTime;
}

// Render the state
void _TestState::Render(double BlendFactor) {
	Graphics.Setup3D();
	Camera->Set3DProjection(BlendFactor);
	Graphics.SetProgram(Assets.Programs["pos"]);
	glUniformMatrix4fv(Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["pos_uv_static"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv_static"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["text"]);
	glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

	Minigame->Render(BlendFactor);
}
