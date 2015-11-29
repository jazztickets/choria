/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2015  Alan Witkowski
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
#include <ui/textbox.h>
#include <font.h>
#include <input.h>
#include <SDL_keycode.h>

// Constructor
_TextBox::_TextBox() :
	Password(false),
	Font(nullptr),
	MaxLength(0),
	CursorTimer(0) {

	//TODO fix
	TextOffset = glm::vec2(5.0f, 22.0f);
}

// Destructor
_TextBox::~_TextBox() {
}

// Update cursor
void _TextBox::Update(double FrameTime, const glm::ivec2 &Mouse) {
	_Element::Update(FrameTime, Mouse);

	if(FocusedElement == this) {
		CursorTimer += FrameTime;
		if(CursorTimer >= 1) {
			CursorTimer = 0;
		}
	}
}

// Handle pressed event
void _TextBox::HandleInput(bool Pressed) {
	if(HitElement) {
		ResetCursor();
		FocusedElement = this;
	}
}

// Handle key event
bool _TextBox::HandleKeyEvent(const _KeyEvent &KeyEvent) {

	if(FocusedElement == this && Visible) {
		if(Text.length() < MaxLength && KeyEvent.Text[0] >= 32 && KeyEvent.Text[0] <= 126) {
			Text += KeyEvent.Text[0];
			ResetCursor();

			return true;
		}
		else if(KeyEvent.Pressed && KeyEvent.Scancode == SDL_SCANCODE_BACKSPACE && Text.length() > 0) {
			Text.erase(Text.length() - 1, 1);
			ResetCursor();

			return true;
		}
		else if(KeyEvent.Pressed && KeyEvent.Scancode == SDL_SCANCODE_RETURN) {
		}
	}

	return false;
}

// Render the element
void _TextBox::Render() const {
	if(!Visible)
		return;

	// Get text to render
	std::string RenderText;
	if(Password)
		RenderText = std::string(Text.length(), '*');
	else
		RenderText = Text;

	// Add cursor
	if(CursorTimer < 0.5 && FocusedElement == this)
		RenderText += "|";

	_Element::Render();

	Font->DrawText(RenderText, glm::vec2(Bounds.Start) + TextOffset, glm::vec4(1.0f));
}
