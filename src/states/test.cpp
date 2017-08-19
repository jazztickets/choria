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
#include <ae/physics.h>
#include <ae/camera.h>
#include <ae/program.h>
#include <ae/random.h>
#include <objects/sprite.h>
#include <framework.h>
#include <constants.h>
#include <SDL_scancode.h>
#include <glm/gtc/type_ptr.hpp>

_TestState TestState;

// Constructor
_TestState::_TestState() :
	Camera(nullptr) {
}

// Initialize
void _TestState::Init() {
	Camera = new _Camera(glm::vec3(0.0f, 0.0f, CAMERA_DISTANCE), CAMERA_DIVISOR, CAMERA_FOVY, CAMERA_NEAR, CAMERA_FAR);
	Camera->CalculateFrustum(Graphics.AspectRatio);

	Boundary = glm::vec4(-8, -8, 8, 8);
	Sprites = new _Manager<_Sprite>();
	{
		Ball = Sprites->Create();
		Ball->RigidBody.Acceleration.y = 10;
		Ball->RigidBody.SetMass(0);
		Ball->RigidBody.Restitution = 0.75f;
		Ball->RigidBody.CollisionGroup = 0;
		Ball->RigidBody.CollisionMask = 0;
		Ball->RigidBody.CollisionResponse = false;
		Ball->Scale = glm::vec2(0.7f);
		Ball->Shape.HalfWidth[0] = 0.5f * Ball->Scale.x;
		Ball->Texture = Assets.Textures["textures/minigames/ball.png"];
	}

	float SpacingX = 2.0f;
	float SpacingY = 2.0f;
	float OffsetY = SpacingY * 2;
	int Odd = 1;
	for(float i = Boundary[1]; i < Boundary[3]-OffsetY; i += SpacingY) {
		for(float j = Boundary[0]; j <= Boundary[2]; j += SpacingX) {
			float X = j + Odd * SpacingX / 2.0f;
			float Y = i + OffsetY;
			if(X > Boundary[2])
				continue;

			_Sprite *Sprite = Sprites->Create();
			Sprite->RigidBody.Acceleration.y = 0;
			Sprite->RigidBody.SetMass(0);
			Sprite->RigidBody.Restitution = 1;
			Sprite->RigidBody.CollisionMask = 1;
			Sprite->RigidBody.CollisionGroup = 2;
			Sprite->RigidBody.ForcePosition(glm::vec2(X, Y));

			if(Y > Boundary[3]-OffsetY) {
				Sprite->Texture = Assets.Textures["textures/minigames/bar.png"];
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
				Tip->Texture = Assets.Textures["textures/minigames/bounce.png"];
				Tip->Scale = glm::vec2(0.5f, 0.5f);
				Tip->Shape.HalfWidth = glm::vec2(0.5f, 0.0f) * Sprite->Scale;
			}
			else {
				Sprite->Texture = Assets.Textures["textures/minigames/bounce.png"];
				Sprite->Scale = glm::vec2(0.5f, 0.5f);
				Sprite->Shape.HalfWidth = glm::vec2(0.5f, 0.0f) * Sprite->Scale;
			}
		}

		Odd = !Odd;
	}
}

// Close
void _TestState::Close() {
	delete Sprites;
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

	if(MouseEvent.Pressed) {
		if(MouseEvent.Button == SDL_BUTTON_LEFT) {
			//State = GameState::DROP;
			//Ball->RigidBody.SetMass(1.0f);

			if(0){
				_Sprite *Sprite = Sprites->Create();
				Sprite->Name = "drop";
				Sprite->RigidBody = Ball->RigidBody;
				Sprite->Shape = Ball->Shape;
				Sprite->RigidBody.SetMass(1);
				Sprite->RigidBody.CollisionMask = 2;
				Sprite->RigidBody.CollisionGroup = 1;
				Sprite->RigidBody.CollisionResponse = true;
				Sprite->Scale = Ball->Scale;
				Sprite->Texture = Assets.Textures["textures/minigames/ball.png"];
			}
		}
	}
}

// Mouse movement handler
void _TestState::HandleMouseMove(const glm::ivec2 &Position) {
}

// Handle window updates
void _TestState::HandleWindow(uint8_t Event) {
	if(Camera && Event == SDL_WINDOWEVENT_SIZE_CHANGED)
		Camera->CalculateFrustum(Graphics.AspectRatio);
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

	switch(State) {
		case GameState::PLACEMENT: {

			if(Input.MouseDown(SDL_BUTTON_LEFT)) {
				_Sprite *Sprite = Sprites->Create();
				Sprite->Name = "drop";
				Sprite->RigidBody = Ball->RigidBody;
				Sprite->Shape = Ball->Shape;
				Sprite->RigidBody.SetMass(1);
				Sprite->RigidBody.CollisionMask = 2;
				Sprite->RigidBody.CollisionGroup = 1;
				Sprite->RigidBody.CollisionResponse = true;
				Sprite->Scale = Ball->Scale;
				Sprite->Texture = Assets.Textures["textures/minigames/ball.png"];
			}
			glm::vec2 WorldPosition;
			Camera->ConvertScreenToWorld(Input.GetMouse(), WorldPosition);

			WorldPosition.x = glm::clamp(WorldPosition.x, Boundary[0] + Ball->Shape.HalfWidth[0], Boundary[2] - Ball->Shape.HalfWidth[0]);
			WorldPosition.y = Boundary[1] + Ball->Shape.HalfWidth[0] * 2;
			//WorldPosition.x = 0;
			//WorldPosition.y = Boundary[3] - 2;
			Ball->RigidBody.ForcePosition(WorldPosition);
		} break;
		case GameState::DROP:
		break;
	}

	// Update objects
	Sprites->Update(FrameTime);

	// Check collision
	bool AxisAlignedPush = false;
	std::list<_Manifold> Manifolds;
	for(auto &Sprite : Sprites->Objects) {
		if(Sprite->RigidBody.InverseMass > 0 && Sprite->RigidBody.Velocity.x == 0) {
			Sprite->RigidBody.Velocity.x = (float)GetRandomReal(0.05, 0.1);
			if(GetRandomInt(0, 1))
				Sprite->RigidBody.Velocity.x = -Sprite->RigidBody.Velocity.x;
		}

		for(auto &TestSprite : Sprites->Objects) {
			if(!(Sprite->RigidBody.CollisionGroup & TestSprite->RigidBody.CollisionMask))
				continue;

			if(Sprite == TestSprite)
				continue;

			if(Sprite->Shape.IsAABB()) {

			}
			else {
				_Manifold Manifold;
				if(TestSprite->CheckCircle(Sprite->RigidBody.Position, Sprite->Shape.HalfWidth[0], Manifold.Normal, Manifold.Penetration, AxisAlignedPush)) {
					if(Sprite->RigidBody.InverseMass > 0.0f && TestSprite->RigidBody.CollisionResponse) {
						Manifold.ObjectA = Sprite;
						Manifold.ObjectB = TestSprite;
						Manifolds.push_back(Manifold);
					}
				}
			}
		}

		if(Sprite->RigidBody.InverseMass > 0.0f) {
			glm::vec4 AABB = Sprite->Shape.GetAABB(Sprite->RigidBody.Position);
			if(AABB[0] < Boundary[0]) {
				_Manifold Manifold;
				Manifold.ObjectA = Sprite;
				Manifold.Penetration = std::abs(AABB[0] - Boundary[0]);
				Manifold.Normal = glm::vec2(1.0, 0);
				Manifolds.push_back(Manifold);
			}
			else if(AABB[2] > Boundary[2]) {
				_Manifold Manifold;
				Manifold.ObjectA = Sprite;
				Manifold.Penetration = std::abs(AABB[2] - Boundary[2]);
				Manifold.Normal = glm::vec2(-1.0, 0);
				Manifolds.push_back(Manifold);
			}
			else if(AABB[3] > Boundary[3]) {
				_Manifold Manifold;
				Manifold.ObjectA = Sprite;
				Manifold.Penetration = std::abs(AABB[3] - Boundary[3]);
				Manifold.Normal = glm::vec2(0.0, -1.0f);
				Manifolds.push_back(Manifold);

				float Width = Boundary[2] - Boundary[0];
				std::cout << (int)((Sprite->RigidBody.Position.x - Boundary[0]) / Width * 8.0f) << std::endl;
				Sprite->Deleted = true;
			}
		}
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
		if(SpriteA->RigidBody.InverseMass > 0.0f && !(AxisAlignedPush && Manifold.IsDiagonal()))
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

	Graphics.SetColor(Assets.Colors["white"]);
	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	for(auto &Sprite : Sprites->Objects) {
		Sprite->Render(BlendFactor);
	}

	Graphics.SetProgram(Assets.Programs["pos"]);
	Graphics.SetVBO(VBO_NONE);
	Graphics.SetColor(Assets.Colors["cyan"]);
	Graphics.DrawRectangle(glm::vec2(Boundary[0] - 0.5f, Boundary[1] - 0.5f), glm::vec2(Boundary[2] - 0.5f, Boundary[3] - 0.5f));

	Graphics.Setup2D();
	Graphics.SetStaticUniforms();
}
