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
	Focused(false),
	Font(nullptr),
	MaxLength(0),
	DrawCursor(true),
	CursorTimer(0) {
}

// Destructor
_TextBox::~_TextBox() {
}

// Update cursor
void _TextBox::Update(double FrameTime, const glm::ivec2 &Mouse) {
	_Element::Update(FrameTime, Mouse);

	if(Focused) {
		CursorTimer += FrameTime;
		if(CursorTimer > 0.5) {
			CursorTimer = 0;
			DrawCursor = !DrawCursor;
		}
	}
}

// Handle pressed event
void _TextBox::HandleInput(bool Pressed) {
	_Element::HandleInput(Pressed);

	if(HitElement) {
		Focused = true;
		ResetCursor();
	}
	else {
		Focused = false;
	}
}

// Handle key event
void _TextBox::HandleKeyEvent(const _KeyEvent &KeyEvent) {

	if(Focused && KeyEvent.Pressed) {
		if(KeyEvent.Key == SDL_SCANCODE_BACKSPACE && Text.length() > 0) {
			Text.erase(Text.length() - 1, 1);
			ResetCursor();
		}
	}
}

// Handle text event
void _TextBox::HandleTextEvent(const char *Text) {
	if(Focused && this->Text.length() < MaxLength && Text[0] >= 32 && Text[0] <= 126) {
		this->Text += Text[0];
		ResetCursor();
	}
}

// Render the element
void _TextBox::Render() const {
	std::string RenderText;
	if(DrawCursor && Focused)
		RenderText = Text + "|";
	else
		RenderText = Text;

	_Element::Render();

	Font->DrawText(RenderText, glm::vec2(Bounds.Start) + glm::vec2(5.0f, 22.0f), glm::vec4(1.0f));
}
