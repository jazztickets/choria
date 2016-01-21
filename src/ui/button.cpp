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
#include <ui/button.h>
#include <ui/label.h>
#include <ui/style.h>
#include <assets.h>
#include <texture.h>
#include <atlas.h>
#include <graphics.h>

// Constructor
_Button::_Button() :
	HoverStyle(nullptr),
	Checked(false),
	TextureIndex(0) {

}

// Destructor
_Button::~_Button() {
}

// Render the element
void _Button::Render(bool IgnoreVisible) const {
	if(!Visible && !IgnoreVisible)
		return;

	if(Enabled) {
		if(Style) {

			if(Style->Texture) {
				Graphics.SetProgram(Style->Program);
				Graphics.SetVBO(VBO_NONE);
				Graphics.SetColor(Style->TextureColor);
				Graphics.DrawImage(Bounds, Style->Texture, Style->Stretch);
			}
			else if(Style->Atlas) {
				Graphics.SetProgram(Style->Program);
				Graphics.SetVBO(VBO_NONE);
				Graphics.SetColor(Style->TextureColor);
				Graphics.DrawAtlas(Bounds, Style->Atlas->Texture, Style->Atlas->GetTextureCoords(TextureIndex));
			}
			else {
				Graphics.SetProgram(Assets.Programs["ortho_pos"]);
				Graphics.SetVBO(VBO_NONE);
				Graphics.SetColor(Style->BackgroundColor);
				Graphics.DrawRectangle(Bounds, true);
				Graphics.SetColor(Style->BorderColor);
				Graphics.DrawRectangle(Bounds, false);
			}
		}

		// Draw hover texture
		if(HoverStyle && (Checked || HitElement)) {

			if(HoverStyle->Texture) {
				Graphics.SetProgram(HoverStyle->Program);
				Graphics.SetVBO(VBO_NONE);
				Graphics.SetColor(HoverStyle->TextureColor);
				Graphics.DrawImage(Bounds, HoverStyle->Texture, Style->Stretch);
			}
			else {
				Graphics.SetProgram(Assets.Programs["ortho_pos"]);
				Graphics.SetVBO(VBO_NONE);
				if(HoverStyle->HasBackgroundColor) {
					Graphics.SetColor(HoverStyle->BackgroundColor);
					Graphics.DrawRectangle(Bounds, true);
				}

				if(HoverStyle->HasBorderColor) {
					Graphics.SetColor(HoverStyle->BorderColor);
					Graphics.DrawRectangle(Bounds, false);
				}
			}
		}
	}
	else {
		if(DisabledStyle) {

			if(DisabledStyle->Texture) {
				Graphics.SetProgram(DisabledStyle->Program);
				Graphics.SetVBO(VBO_NONE);
				Graphics.SetColor(DisabledStyle->TextureColor);
				Graphics.DrawImage(Bounds, DisabledStyle->Texture, DisabledStyle->Stretch);
			}
			else if(DisabledStyle->Atlas) {
				Graphics.SetProgram(DisabledStyle->Program);
				Graphics.SetVBO(VBO_NONE);
				Graphics.SetColor(DisabledStyle->TextureColor);
				Graphics.DrawAtlas(Bounds, DisabledStyle->Atlas->Texture, DisabledStyle->Atlas->GetTextureCoords(TextureIndex));
			}
			else {
				Graphics.SetProgram(Assets.Programs["ortho_pos"]);
				Graphics.SetVBO(VBO_NONE);
				Graphics.SetColor(DisabledStyle->BackgroundColor);
				Graphics.DrawRectangle(Bounds, true);
				Graphics.SetColor(DisabledStyle->BorderColor);
				Graphics.DrawRectangle(Bounds, false);
			}
		}
	}

	// Render all children
	for(auto &Child : Children) {
		Child->Render();
	}
}
