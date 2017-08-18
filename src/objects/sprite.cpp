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
#include <objects/sprite.h>
#include <ae/graphics.h>

// Constructor
_Sprite::_Sprite() :
	Texture(nullptr) {

}

// Update
void _Sprite::Update(double FrameTime) {

}

// Render
void _Sprite::Render(double BlendFactor) {

	glm::vec3 RenderPosition(
		RigidBody.Position.x * BlendFactor + RigidBody.LastPosition.x * (1.0f - BlendFactor) + 0.5f,
		RigidBody.Position.y * BlendFactor + RigidBody.LastPosition.y * (1.0f - BlendFactor) + 0.5f,
		0.0f
	);

	if(Texture) {
		Graphics.SetVBO(VBO_QUAD);
		Graphics.DrawSprite(RenderPosition, Texture);
	}
}
