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
#include <ui/textbox.h>
#include <font.h>
#include <input.h>
#include <graphics.h>
#include <assets.h>
#include <SDL_keycode.h>
#include <tinyxml2.h>
#include <iostream>

// Constructor
_TextBox::_TextBox() :
	Font(nullptr),
	MaxLength(0),
	CursorPosition(0),
	CursorTimer(0),
	Password(false) {

	//TODO fix
	ParentOffset = glm::vec2(5.0f, 22.0f);
}

// Destructor
_TextBox::~_TextBox() {
}

// Update cursor
void _TextBox::Update(double FrameTime, const glm::vec2 &Mouse) {
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
bool _TextBox::HandleKey(const _KeyEvent &KeyEvent) {

	if(FocusedElement == this && Visible && KeyEvent.Pressed) {
		if(Text.length() < MaxLength && KeyEvent.Text[0] >= 32 && KeyEvent.Text[0] <= 126) {
			if(CursorPosition > Text.length())
				CursorPosition = Text.length();

			Text.insert(CursorPosition, 1, KeyEvent.Text[0]);
			CursorPosition++;
		}
		else if(KeyEvent.Scancode == SDL_SCANCODE_BACKSPACE && Text.length() > 0 && CursorPosition > 0) {
			Text.erase(CursorPosition - 1, 1);
			if(CursorPosition > 0)
				CursorPosition--;
		}
		else if(KeyEvent.Scancode == SDL_SCANCODE_RETURN) {
			return false;
		}
		else if(KeyEvent.Scancode == SDL_SCANCODE_DELETE) {
			Text.erase(CursorPosition, 1);
			if(CursorPosition >= Text.length())
				CursorPosition = Text.length();
		}
		else if(KeyEvent.Scancode == SDL_SCANCODE_LEFT) {
			if(Input.ModKeyDown(KMOD_ALT))
				CursorPosition = 0;
			else if(CursorPosition > 0)
				CursorPosition--;
		}
		else if(KeyEvent.Scancode == SDL_SCANCODE_RIGHT) {
			if(Input.ModKeyDown(KMOD_ALT))
				CursorPosition = Text.length();
			else if(CursorPosition < Text.length())
				CursorPosition++;
		}
		else if(KeyEvent.Scancode == SDL_SCANCODE_HOME) {
			CursorPosition = 0;
		}
		else if(KeyEvent.Scancode == SDL_SCANCODE_END) {
			CursorPosition = Text.length();
		}
		else {
			return false;
		}

		ResetCursor();
		return true;
	}

	return false;
}

// Render the element
void _TextBox::Render(bool IgnoreVisible) const {
	if(!Visible && !IgnoreVisible)
		return;

	// Get text to render
	std::string RenderText;
	if(Password)
		RenderText = std::string(Text.length(), '*');
	else
		RenderText = Text;

	// Render base element
	_Element::Render();

	// Mask outside of bounds
	Graphics.SetProgram(Assets.Programs["ortho_pos"]);
	Graphics.EnableStencilTest();
	Graphics.DrawMask(Bounds);

	// Get width at cursor position
	_TextBounds TextBounds;
	Font->GetStringDimensions(RenderText.substr(0, CursorPosition), TextBounds);

	// Draw text
	glm::vec2 StartPosition = glm::vec2(Bounds.Start) + ParentOffset;
	Font->DrawText(RenderText, StartPosition, glm::vec4(1.0f));

	// Draw cursor
	if(CursorTimer < 0.5 && FocusedElement == this) {
		Graphics.SetProgram(Assets.Programs["ortho_pos"]);
		Graphics.DrawRectangle(glm::vec2(StartPosition.x + TextBounds.Width+1, StartPosition.y - Font->MaxAbove), glm::vec2(StartPosition.x + TextBounds.Width+2, StartPosition.y + Font->MaxBelow));
	}

	// Disable mask
	Graphics.DisableStencilTest();
}

// Serialize attributes
void _TextBox::SerializeAttributes(tinyxml2::XMLElement *Node) {
	if(FontName.size())
		Node->SetAttribute("font", FontName.c_str());
	if(MaxLength)
		Node->SetAttribute("maxlength", (uint32_t)MaxLength);

	_Element::SerializeAttributes(Node);
}
