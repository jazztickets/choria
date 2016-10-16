/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
#include <ui/image.h>
#include <texture.h>
#include <graphics.h>
#include <assets.h>

// Constructor
_Image::_Image() : Color(1.0f, 1.0f, 1.0f, 1.0f), Stretch(false) {
	Clickable = false;
}

// Destructor
_Image::~_Image() {
}

// Render the element
void _Image::Render(bool IgnoreVisible) const {
	if(!Visible && !IgnoreVisible)
		return;

	Graphics.SetColor(Color);
	if(Texture) {
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.SetVBO(VBO_NONE);
		Graphics.DrawImage(Bounds, Texture, Stretch);
	}

	// Draw children
	_Element::Render();
}
