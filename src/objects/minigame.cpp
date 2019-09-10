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
#include <objects/minigame.h>
#include <objects/sprite.h>
#include <objects/grid.h>
#include <ae/manager.h>
#include <ae/physics.h>
#include <ae/assets.h>
#include <ae/camera.h>
#include <ae/input.h>
#include <ae/ui.h>
#include <ae/random.h>
#include <ae/font.h>
#include <ae/graphics.h>
#include <ae/program.h>
#include <ae/audio.h>
#include <stats.h>
#include <constants.h>
#include <SDL_mouse.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <algorithm>
#include <iostream>

// Constructor
_Minigame::_Minigame(const _MinigameType *Minigame) :
	IsServer(false),
	Minigame(Minigame),
	State(StateType::NEEDSEED),
	Time(0),
	DropX(0),
	Bucket((size_t)-1),
	Debug(0),
	Bounces(0) {

	Camera = new ae::_Camera(glm::vec3(0.0f, 0.0f, CAMERA_DISTANCE), CAMERA_DIVISOR, CAMERA_FOVY, CAMERA_NEAR, CAMERA_FAR);
	Camera->CalculateFrustum(ae::Graphics.AspectRatio);

	Boundary.Start = glm::vec2(-8, -6.5);
	Boundary.End = glm::vec2(8, 6);

	glm::ivec2 GridSize(std::ceil(Boundary.End.x - Boundary.Start.x), std::ceil(Boundary.End.y - Boundary.Start.y));
	Grid = new _Grid(GridSize, -Boundary.Start);

	Sprites = new ae::_Manager<_Sprite>();
	Ball = Sprites->Create();
	Ball->Scale = glm::vec2(0.7f);
	Ball->Shape.HalfWidth[0] = 0.5f * Ball->Scale.x;
	Ball->RigidBody.Acceleration.y = 10;
	Ball->RigidBody.SetMass(0);
	Ball->RigidBody.Restitution = 0.75f;
	Ball->RigidBody.CollisionGroup = 0;
	Ball->RigidBody.CollisionMask = 0;
	Ball->RigidBody.CollisionResponse = false;
	Ball->RigidBody.Position.y = Boundary.Start.y + Ball->Shape.HalfWidth[0] * 2;
	Ball->Texture = ae::Assets.Textures["textures/minigames/ball.png"];

	float SpacingX = 2.0f;
	float SpacingY = 2.0f;
	float OffsetY = 3;
	int Odd = 0;
	int Columns = 9;
	int Rows = 5;
	for(float i = 0; i < Rows; i++) {
		for(float j = 0; j < Columns; j++) {
			float X = j * SpacingX + Odd * SpacingX / 2.0f + Boundary.Start.x;
			float Y = i * SpacingY + OffsetY + Boundary.Start.y;
			if(X > Boundary.End.x)
				continue;

			_Sprite *Sprite = Sprites->Create();
			Sprite->RigidBody.Acceleration.y = 0;
			Sprite->RigidBody.SetMass(0);
			Sprite->RigidBody.Restitution = 1;
			Sprite->RigidBody.CollisionMask = 1;
			Sprite->RigidBody.CollisionGroup = 2;
			Sprite->RigidBody.ForcePosition(glm::vec2(X, Y));

			if(i == Rows-1) {
				Sprite->Texture = ae::Assets.Textures["textures/minigames/bar.png"];
				Sprite->Scale = glm::vec2(0.5f, 2.5f);
				Sprite->Shape.HalfWidth = glm::vec2(0.5f, 0.5f) * glm::vec2(0.5f, 2.5f);
				Sprite->RigidBody.ForcePosition(glm::vec2(X, Y + 0.75f));

				_Sprite *Tip = Sprites->Create();
				Tip->RigidBody.Acceleration.y = 0;
				Tip->RigidBody.SetMass(0);
				Tip->RigidBody.Restitution = 1;
				Tip->RigidBody.CollisionMask = 1;
				Tip->RigidBody.CollisionGroup = 2;
				Tip->RigidBody.ForcePosition(glm::vec2(X, Y-0.5f));
				Tip->Texture = ae::Assets.Textures["textures/minigames/halfpeg.png"];
				Tip->Scale = glm::vec2(0.5f, 0.5f);
				Tip->Shape.HalfWidth = glm::vec2(0.5f, 0.0f) * Sprite->Scale;
				Grid->AddObject(Tip, Tip->RigidBody.Position, Tip->Shape.HalfWidth);
			}
			else {
				Sprite->Texture = ae::Assets.Textures["textures/minigames/peg.png"];
				Sprite->Scale = glm::vec2(0.5f, 0.5f);
				Sprite->Shape.HalfWidth = glm::vec2(0.5f, 0.0f) * Sprite->Scale;
			}

			Grid->AddObject(Sprite, Sprite->RigidBody.Position, Sprite->Shape.HalfWidth);
		}

		Odd = !Odd;
	}

	Update(0);
}

// Destructor
_Minigame::~_Minigame() {
	delete Camera;
	delete Sprites;
	delete Grid;
}

// Update
void _Minigame::Update(double FrameTime) {
	if(Camera) {
		Camera->Set2DPosition(glm::vec2(0.0f, 0.0f));
		Camera->Update(FrameTime);
	}

	if(Debug == -1 || (Camera && State == StateType::CANDROP)) {
		glm::vec2 WorldPosition;
		Camera->ConvertScreenToWorld(ae::Input.GetMouse(), WorldPosition);
		WorldPosition.x = glm::clamp(WorldPosition.x, Boundary.Start.x + Ball->Shape.HalfWidth[0], Boundary.End.x - Ball->Shape.HalfWidth[0]);
		WorldPosition.y = Ball->RigidBody.Position.y;
		Ball->RigidBody.ForcePosition(WorldPosition);
		Ball->Visible = true;
		if(Debug == -1 && ae::Input.MouseDown(SDL_BUTTON_RIGHT)) {
			StartGame(0);
			Drop(WorldPosition.x);
		}
	}
	else
		Ball->Visible = false;

	// Update objects
	Sprites->Update(FrameTime);

	// Check collision
	std::list<ae::_Manifold> Manifolds;
	bool PlayedSound = false;
	for(auto &Sprite : Sprites->Objects) {
		Sprite->Touching = false;
		if(Sprite->RigidBody.InverseMass <= 0.0f)
			continue;

		// Get list of objects from grid
		std::list<const void *> PotentialObjects;
		Grid->GetObjectList(Sprite->RigidBody.Position, Sprite->Shape.HalfWidth, PotentialObjects);
		for(auto &TestObject : PotentialObjects) {
			_Sprite *TestSprite = (_Sprite *)TestObject;
			if(!(Sprite->RigidBody.CollisionGroup & TestSprite->RigidBody.CollisionMask))
				continue;

			if(Sprite == TestSprite)
				continue;

			if(Sprite->Shape.IsAABB()) {

			}
			else {
				ae::_Manifold Manifold;
				bool AxisAlignedPush = false;
				if(TestSprite->CheckCircle(Sprite->RigidBody.Position, Sprite->Shape.HalfWidth[0], Manifold.Normal, Manifold.Penetration, AxisAlignedPush)) {
					if(Sprite->RigidBody.InverseMass > 0.0f && TestSprite->RigidBody.CollisionResponse) {
						Manifold.ObjectA = Sprite;
						Manifold.ObjectB = TestSprite;
						Sprite->Touching = true;

						Manifolds.push_back(Manifold);
					}
				}
			}
		}

		if(Sprite->RigidBody.InverseMass > 0.0f) {
			if(Debug > 0) {
				std::cout.precision(17);
				std::cout << "x=" << Sprite->RigidBody.Position.x << std::endl;
			}

			glm::vec4 AABB = Sprite->Shape.GetAABB(Sprite->RigidBody.Position);
			if(AABB[0] < Boundary.Start.x) {
				ae::_Manifold Manifold;
				Manifold.ObjectA = Sprite;
				Manifold.Penetration = std::abs(AABB[0] - Boundary.Start.x);
				Manifold.Normal = glm::vec2(1.0, 0);
				Manifolds.push_back(Manifold);
				Sprite->Touching = true;
			}
			else if(AABB[2] > Boundary.End.x) {
				ae::_Manifold Manifold;
				Manifold.ObjectA = Sprite;
				Manifold.Penetration = std::abs(AABB[2] - Boundary.End.x);
				Manifold.Normal = glm::vec2(-1.0, 0);
				Manifolds.push_back(Manifold);
				Sprite->Touching = true;
			}
			else if(AABB[3] > Boundary.End.y) {
				ae::_Manifold Manifold;
				Manifold.ObjectA = Sprite;
				Manifold.Penetration = std::abs(AABB[3] - Boundary.End.y);
				Manifold.Normal = glm::vec2(0.0, -1.0f);

				float Width = Boundary.End.x - Boundary.Start.x;
				Bucket = (size_t)((Sprite->RigidBody.Position.x - Boundary.Start.x) / Width * 8.0f);
				Sprite->Deleted = true;
				State = StateType::DONE;
				if(Debug) {
					std::cout << "bucket=" << Bucket << " time=" << Time << std::endl;
				}
			}
		}

		// Play sound
		if(Sprite->Touching && Sprite->Touching != Sprite->LastTouching) {
			if(!Debug && !IsServer && !PlayedSound) {
				ae::Audio.PlaySound(ae::Assets.Sounds["bounce0.ogg"], 0.45f);
				PlayedSound = true;
			}

			Bounces++;
		}

		Sprite->LastTouching = Sprite->Touching;
	}

	// Resolve penetration
	for(auto &Manifold : Manifolds) {
		_Sprite *SpriteA = (_Sprite *)Manifold.ObjectA;
		_Sprite *SpriteB = (_Sprite *)Manifold.ObjectB;
		glm::vec2 Normal = -Manifold.Normal;

		// Check for response
		if(!SpriteA->RigidBody.CollisionResponse)
			continue;

		// Separate objects
		if(SpriteA->RigidBody.InverseMass > 0.0f)
			SpriteA->RigidBody.Position += Manifold.Normal * Manifold.Penetration;

		// Get relative velocity
		glm::vec2 RelativeVelocity = -SpriteA->RigidBody.Velocity;
		if(SpriteB)
			RelativeVelocity += SpriteB->RigidBody.Velocity;

		// Check if objects are moving towards each other
		float VelocityDotNormal = glm::dot(RelativeVelocity, Normal);
		if(VelocityDotNormal > 0)
			continue;

		// Get restitution
		float MinimumRestitution = SpriteA->RigidBody.Restitution;
		if(SpriteB)
			MinimumRestitution = std::min(MinimumRestitution, SpriteB->RigidBody.Restitution);

		// Calculate impulse magnitude
		float ImpulseScalar = -(1 + MinimumRestitution) * VelocityDotNormal;
		float ImpulseDenominator = SpriteA->RigidBody.InverseMass;
		if(SpriteB)
			ImpulseDenominator += SpriteB->RigidBody.InverseMass;

		ImpulseScalar /= ImpulseDenominator;

		// Apply impulse
		glm::vec2 ImpulseVector = ImpulseScalar * Normal;
		SpriteA->RigidBody.Velocity -= SpriteA->RigidBody.InverseMass * ImpulseVector;
		if(SpriteB)
			SpriteB->RigidBody.Velocity += SpriteB->RigidBody.InverseMass * ImpulseVector;
	}

	Time += FrameTime;
}

// Render
void _Minigame::Render(double BlendFactor) {
	if(!Camera)
		return;

	// Setup 3D
	ae::Graphics.Setup3D();
	Camera->Set3DProjection(BlendFactor);
	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv_static"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos_uv_static"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["text"]);
	glUniformMatrix4fv(ae::Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

	ae::Assets.Programs["pos_uv_static"]->AmbientLight = glm::vec4(1.0f);

	// Set up gl scissor
	ae::Graphics.EnableScissorTest();
	ae::_Bounds ScissorRegion;
	GetUIBoundary(ScissorRegion);
	ae::Graphics.SetScissor(ScissorRegion);

	// Draw sprites
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv_static"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos_uv_static"]->TextureTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	ae::Graphics.SetColor(glm::vec4(1.0f));
	for(auto &Sprite : Sprites->Objects) {
		Sprite->Render(BlendFactor);
	}

	// Draw prizes
	glm::vec3 Position(Boundary.Start.x+1, Boundary.End.y - 0.7f, 0);
	for(auto &Item : Prizes) {
		if(Item) {
			ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv_static"]);
			ae::Graphics.DrawSprite(Position, Item->Item->Texture, (float)Time * 50);
			if(Item->Count > 1)
				ae::Assets.Fonts["hud_medium"]->DrawText(std::to_string(Item->Count), glm::vec2(Position) + glm::vec2(0.30f, 0.5f), ae::LEFT_BASELINE, glm::vec4(1), 1/64.0f);
		}

		Position.x += 2;
	}

	//Grid->Render();
	ae::Graphics.DisableScissorTest();

	// Setup 2D
	ae::Graphics.Setup2D();
	ae::Graphics.SetStaticUniforms();
}

// Mouse input
void _Minigame::HandleMouseButton(const ae::_MouseEvent &MouseEvent) {
	if(MouseEvent.Pressed) {
		if(MouseEvent.Button == SDL_BUTTON_LEFT) {
			if(State != StateType::CANDROP)
				return;

			Drop(Ball->RigidBody.Position.x);
		}
	}
}

// Start game and set seed
void _Minigame::StartGame(uint64_t Seed) {
	if(Debug > 0)
		std::cout << "seed=" << Seed << std::endl;

	if(Seed)
		Random.seed(Seed);

	// Vary initial x velocity
	{
		std::uniform_real_distribution<float> Distribution(0.05f, 0.1f);
		Ball->RigidBody.Velocity.x = Distribution(Random);
	}
	{
		std::uniform_int_distribution<int> Distribution(0, 1);
		if(Distribution(Random))
			Ball->RigidBody.Velocity.x = -Ball->RigidBody.Velocity.x;
	}

	// Setup prizes
	RefreshPrizes();

	State = StateType::CANDROP;
}

// Drop the ball
void _Minigame::Drop(float X) {
	_Sprite *Sprite = Sprites->Create();
	Sprite->Name = "drop";
	Sprite->RigidBody = Ball->RigidBody;
	Sprite->Shape = Ball->Shape;
	Sprite->RigidBody.SetMass(1);
	Sprite->RigidBody.CollisionMask = 2;
	Sprite->RigidBody.CollisionGroup = 1;
	Sprite->RigidBody.CollisionResponse = true;
	Sprite->Scale = Ball->Scale;
	Sprite->RigidBody.Position.x = X;
	Sprite->RigidBody.Position.y = Ball->RigidBody.Position.y;
	Sprite->Texture = ae::Assets.Textures["textures/minigames/ball.png"];

	DropX = X;
	State = StateType::DROPPED;
}

// Refresh prizes
void _Minigame::RefreshPrizes() {

	// Add prizes
	Prizes.clear();
	for(const auto &Item : Minigame->Items)
		Prizes.push_back(&Item);

	// Remove two items and shuffle
	ShufflePrizes(Prizes);
	Prizes[0] = nullptr;
	Prizes[1] = nullptr;
	Prizes.resize(8);
	ShufflePrizes(Prizes);

	if(Debug > 0) {
		for(auto &Prize : Prizes) {
			if(Prize && Prize->Item)
				std::cout << Prize->Item->ID << std::endl;
			else
				std::cout << 0 << std::endl;
		}
	}
}

// Shuffle prizes
void _Minigame::ShufflePrizes(std::vector<const _MinigameItem *> &Bag) {
	if(!Bag.size())
		return;

	for(size_t i = Bag.size()-1; i > 0; --i) {
		std::uniform_int_distribution<size_t> Distribution(0, i);
		std::swap(Bag[i], Bag[Distribution(Random)]);
	}
}

// Get UI boundary
void _Minigame::GetUIBoundary(ae::_Bounds &Bounds) {
	if(!Camera)
		return;

	Camera->ConvertWorldToScreen(Boundary.Start, Bounds.Start);
	Camera->ConvertWorldToScreen(Boundary.End, Bounds.End);
}
