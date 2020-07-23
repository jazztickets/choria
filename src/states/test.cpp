/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020 Alan Witkowski
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
#include <ae/texture_array.h>
#include <objects/minigame.h>
#include <stats.h>
#include <framework.h>
#include <constants.h>
#include <SDL_scancode.h>
#include <SDL_mouse.h>
#include <SDL_timer.h>
#include <glm/gtc/type_ptr.hpp>

_TestState TestState;

// Constructor
_TestState::_TestState() :
	Camera(nullptr),
	Stats(nullptr),
	Minigame(nullptr) {
}

// Initialize
void _TestState::Init() {
	Camera = new ae::_Camera(glm::vec3(0.0f, 0.0f, CAMERA_DISTANCE), CAMERA_DIVISOR, CAMERA_FOVY, CAMERA_NEAR, CAMERA_FAR);
	Camera->CalculateFrustum(ae::Graphics.AspectRatio);

	Stats = new _Stats();

	double MaxTime = 0;
	double MinTime = 30;
	int MaxBounces = 0;
	int MinBounces = 99999;

	// gawk 'BEGIN{IFS=OFS="\t";print "runs", "time", "sold", "profit", "gold/sec"} { time+=$1; total+=int($2/2) } END{ prof=(total-NR*10); print NR, time, total, prof, prof/time }'
	int Simulations = 0;
	for(int i = 0; i < Simulations; i++) {
		//double StartTime = SDL_GetPerformanceCounter();
		uint32_t Seed = ae::GetRandomInt((uint32_t)1, std::numeric_limits<uint32_t>::max());

		Minigame = new _Minigame(&Stats->Minigames.at("ball_drop"));
		Minigame->IsServer = true;
		Minigame->Debug = 0;
		Minigame->StartGame(Seed);

		// Drop ball over highest value prize
		float X = 0;
		if((0)) {
			int HighestPrizeIndex = 0;
			int HighestPrizeValue = 0;
			int Index = 0;
			for(const auto &Prize : Minigame->Prizes) {
				if(Prize && Prize->Item->Cost > HighestPrizeValue) {
					HighestPrizeValue = Prize->Item->Cost;
					HighestPrizeIndex = Index;
				}
				Index++;
			}

			X = Minigame->Boundary.Start.x + 1 + HighestPrizeIndex * 2;
		}
		X = (float)ae::GetRandomReal(-7.65, 7.65);
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
		if(Minigame->Bounces < MinBounces) {
			//std::cout << "minbounces=" << Minigame->Bounces << " seed=" << Seed << " x=" << X << std::endl;
			MinBounces = Minigame->Bounces;
		}
		if(Minigame->Bounces > MaxBounces) {
			//std::cout << "maxbounces=" << Minigame->Bounces << " seed=" << Seed << " x=" << X << std::endl;
			MaxBounces = Minigame->Bounces;
		}

		std::cout << Minigame->Bounces << std::endl;

		if(Minigame->Bucket < Minigame->Prizes.size()) {
			const _MinigameItem *MinigameItem = Minigame->Prizes[Minigame->Bucket];
			if(MinigameItem && MinigameItem->Item) {
				//std::cout << Time << "\t" << MinigameItem->Item->Cost * MinigameItem->Count << std::endl;
			}
			else {
				//std::cout << Time << "\t" << "0" << std::endl;
			}
		}

		delete Minigame;

		//std::cout << (SDL_GetPerformanceCounter() - StartTime) / (double)SDL_GetPerformanceFrequency() << std::endl;
	}

	Minigame = nullptr;

	//Framework.Done = true;

	if((0)){
		Minigame = new _Minigame(&Stats->Minigames.at("ball_drop"));
		Minigame->Debug = -1;
		Minigame->StartGame(1049117602);
		Minigame->Drop(4.3513402938842773);
		Minigame->StartGame(1290408577);
		Minigame->Drop(-7.1556816101074219);
		Minigame->StartGame(2109853616);
		Minigame->Drop(-4.879331111907959);
	}

	float Vertices[] = {
		0, 1,
		1, 1,
		0, 0,
		1, 0,
		0, 1,
		1, 1,
		0, 0,
		1, 0,
	};

	glGenBuffers(1, &VertexBuffer[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
}

// Close
void _TestState::Close() {
	delete Stats;
	delete Minigame;
	delete Camera;

	glDeleteBuffers(1, &VertexBuffer[0]);
}

// Key handler
bool _TestState::HandleKey(const ae::_KeyEvent &KeyEvent) {
	ae::Graphics.Element->HandleKey(KeyEvent);

	if(KeyEvent.Pressed) {
		if(KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
			Framework.Done = true;
	}

	return true;
}

// Mouse handler
void _TestState::HandleMouseButton(const ae::_MouseEvent &MouseEvent) {
	ae::FocusedElement = nullptr;
	if(MouseEvent.Button == SDL_BUTTON_LEFT)
		ae::Graphics.Element->HandleMouseButton(MouseEvent.Pressed);

	if(Minigame)
		Minigame->HandleMouseButton(MouseEvent);
}

// Mouse movement handler
void _TestState::HandleMouseMove(const glm::ivec2 &Position) {
}

// Handle window updates
void _TestState::HandleWindow(uint8_t Event) {
	if(Event == SDL_WINDOWEVENT_SIZE_CHANGED) {
		if(Camera)
			Camera->CalculateFrustum(ae::Graphics.AspectRatio);
		if(Minigame && Minigame->Camera)
			Minigame->Camera->CalculateFrustum(ae::Graphics.AspectRatio);

	}
}

// Handle quit events
void _TestState::HandleQuit() {
	Framework.Done = true;
}

// Update
void _TestState::Update(double FrameTime) {

	// Update UI
	ae::Graphics.Element->Update(FrameTime, ae::Input.GetMouse());

	// Update camera
	if(Camera) {
		Camera->Set2DPosition(glm::vec2(0.0f, 0.0f));
		Camera->Update(FrameTime);
	}

	if(Minigame)
		Minigame->Update(FrameTime);

	Time += FrameTime;
}

// Render the state
void _TestState::Render(double BlendFactor) {
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
		ae::Graphics.SetProgram(ae::Assets.Programs["test"]);
		glUniformMatrix4fv(ae::Assets.Programs["test"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	}

	ae::Assets.Programs["pos_uv_static"]->AmbientLight = glm::vec4(1.0f);

	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos_uv"]->TextureTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	ae::Graphics.SetColor(glm::vec4(1.0f));

	if(Minigame)
		Minigame->Render(BlendFactor);

	RenderMapTest();

	ae::Graphics.Setup2D();
	ae::Graphics.SetStaticUniforms();

	Render2DTest();
}

void _TestState::Render2DTest() {
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
	ae::Graphics.SetColor(glm::vec4(1, 0, 0, 1));
	ae::Graphics.DrawRectangle(glm::vec2(1, 1), glm::vec2(4, 4), true);
	ae::Graphics.SetColor(glm::vec4(0, 1, 0, 1));
	ae::Graphics.DrawRectangle(glm::vec2(5, 1), glm::vec2(8, 4), false);

	ae::Graphics.EnableScissorTest();
	ae::_Bounds ScissorRegion(glm::vec2(9, 1), glm::vec2(12, 4));
	ae::Graphics.SetScissor(ScissorRegion);
	ae::Graphics.SetColor(glm::vec4(0, 1, 1, 1));
	ae::Graphics.DrawRectangle(glm::vec2(8, 0), glm::vec2(13, 5), true);
	ae::Graphics.DisableScissorTest();

	ae::_Bounds MaskRegion(glm::vec2(13, 1), glm::vec2(16, 4));
	ae::Graphics.EnableStencilTest();
	ae::Graphics.DrawMask(MaskRegion);
	ae::Graphics.SetColor(glm::vec4(1, 1, 0, 1));
	ae::Graphics.DrawRectangle(glm::vec2(12, 0), glm::vec2(17, 5), true);
	ae::Graphics.DisableStencilTest();
}

void _TestState::RenderMapTest() {
	ae::Graphics.SetTextureID(ae::Assets.TextureArrays["default"]->ID, GL_TEXTURE_2D_ARRAY);
	ae::Graphics.ResetState();
	ae::_Program *Program = ae::Assets.Programs["test"];
	ae::Graphics.SetProgram(Program);
	ae::Graphics.SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	Program->SetUniformVec2("tile_count", glm::vec2(5, 3));
	Program->SetUniformFloat("texture_scale", 64.0f / 66.0f);
	Program->SetUniformFloat("texture_offset", 1.0f / 66.0f);

	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[0]);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (GLvoid *)(sizeof(float) * 8));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	ae::Graphics.ResetState();
}
