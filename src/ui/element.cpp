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
#include <ui/element.h>
#include <ui/style.h>
#include <graphics.h>
#include <input.h>
#include <assets.h>
#include <constants.h>
#include <algorithm>

const glm::vec4 DebugColors[] = { COLOR_CYAN, COLOR_YELLOW, COLOR_RED, COLOR_GREEN, COLOR_BLUE };
const int DebugColorCount = sizeof(DebugColors) / sizeof(glm::vec4);

// Constructor
_Element::_Element() :
	GlobalID(0),
	Parent(nullptr),
	UserData((void *)-1),
	Visible(false),
	Enabled(true),
	Clickable(true),
	MaskOutside(false),
	UserCreated(false),
	Debug(0),
	Style(nullptr),
	DisabledStyle(nullptr),
	Fade(1.0f),
	HitElement(nullptr),
	PressedElement(nullptr),
	ReleasedElement(nullptr) {

}

// Destructor
_Element::~_Element() {
}

// Handle key event
bool _Element::HandleKeyEvent(const _KeyEvent &KeyEvent) {

	// Pass event to children
	for(auto &Child : Children) {
		if(Child->HandleKeyEvent(KeyEvent))
			return true;
	}

	return false;
}

// Handle a press event
void _Element::HandleInput(bool Pressed) {
	if(!Visible)
		return;

	// Pass event to children
	for(auto &Child : Children)
		Child->HandleInput(Pressed);

	// Set pressed element
	if(Pressed)
		PressedElement = HitElement;

	// Get released element
	if(!Pressed && PressedElement && HitElement) {
		ReleasedElement = PressedElement;
		PressedElement = nullptr;
	}
}

// Get the element that was clicked and released
_Element *_Element::GetClickedElement() {
	if(HitElement == ReleasedElement)
		return HitElement;

	return nullptr;
}

// Remove a child element
void _Element::RemoveChild(_Element *Element) {
	auto Iterator = std::find(Children.begin(), Children.end(), Element);
	if(Iterator != Children.end()) {
		if(Graphics.Element->HitElement == Element)
			Graphics.Element->HitElement = nullptr;
		Children.erase(Iterator);
	}
}

// Handle mouse movement
void _Element::Update(double FrameTime, const glm::vec2 &Mouse) {
	HitElement = nullptr;
	ReleasedElement = nullptr;

	// Test element first
	if(Mouse.x >= Bounds.Start.x && Mouse.y >= Bounds.Start.y && Mouse.x < Bounds.End.x && Mouse.y < Bounds.End.y && Visible && Clickable && Enabled) {
		HitElement = this;
	}
	else if(MaskOutside) {
		HitElement = nullptr;
		return;
	}

	// Test children
	if(Visible) {
		for(auto &Child : Children) {
			Child->Update(FrameTime, Mouse);
			if(Child->HitElement)
				HitElement = Child->HitElement;
		}
	}
}

// Calculate the screen space bounds for the element
void _Element::CalculateBounds() {
	Bounds.Start = Offset;

	// Handle horizontal alignment
	switch(Alignment.Horizontal) {
		case _Alignment::CENTER:
			if(Parent)
				Bounds.Start.x += Parent->Size.x / 2;
			Bounds.Start.x -= Size.x / 2;
		break;
		case _Alignment::RIGHT:
			if(Parent)
				Bounds.Start.x += Parent->Size.x;
			Bounds.Start.x -= Size.x;
		break;
	}

	// Handle vertical alignment
	switch(Alignment.Vertical) {
		case _Alignment::MIDDLE:
			if(Parent)
				Bounds.Start.y += Parent->Size.y / 2;
			Bounds.Start.y -= Size.y / 2;
		break;
		case _Alignment::BOTTOM:
			if(Parent)
				Bounds.Start.y += Parent->Size.y;
			Bounds.Start.y -= Size.y;
		break;
	}

	// Offset from parent
	if(Parent)
		Bounds.Start += Parent->Bounds.Start + Parent->ChildrenOffset;

	// Set end
	Bounds.End = Bounds.Start + Size;

	// Update children
	CalculateChildrenBounds();
}

// Update children bounds
void _Element::CalculateChildrenBounds() {

	// Update children
	for(auto &Child : Children)
		Child->CalculateBounds();
}

// Render the element
void _Element::Render() const {
	if(!Visible)
		return;

	if(MaskOutside) {
		Graphics.SetProgram(Assets.Programs["ortho_pos"]);
		Graphics.EnableStencilTest();
		Graphics.DrawMask(Bounds);
	}

	if(Style) {
		Graphics.SetProgram(Style->Program);
		Graphics.SetVBO(VBO_NONE);
		if(Style->HasBackgroundColor) {
			glm::vec4 RenderColor(Style->BackgroundColor);
			RenderColor.a *= Fade;
			Graphics.SetColor(RenderColor);
			Graphics.DrawRectangle(Bounds, true);
		}

		if(Style->HasBorderColor) {
			glm::vec4 RenderColor(Style->BorderColor);
			RenderColor.a *= Fade;
			Graphics.SetColor(RenderColor);
			Graphics.DrawRectangle(Bounds, false);
		}
	}

	// Render all children
	for(auto &Child : Children) {
		Child->Render();
	}

	if(MaskOutside)
		Graphics.DisableStencilTest();

	if(Debug && Debug-1 < DebugColorCount) {
		Graphics.SetProgram(Assets.Programs["ortho_pos"]);
		Graphics.SetVBO(VBO_NONE);
		Graphics.SetColor(DebugColors[1]);
		Graphics.DrawRectangle(Bounds.Start, Bounds.End);
	}

}

// Set the debug, and increment for children
void _Element::SetDebug(int Debug) {
	this->Debug = Debug;

	for(auto &Child : Children)
		Child->SetDebug(Debug + 1);
}

// Set clickable flag of element and children. Depth=-1 is full recursion
void _Element::SetClickable(bool Clickable, int Depth) {
	if(Depth == 0)
		return;

	this->Clickable = Clickable;

	if(Depth != -1)
		Depth--;

	for(auto &Child : Children)
		Child->SetClickable(Clickable, Depth);
}

// Set visibility of element and children
void _Element::SetVisible(bool Visible) {
	this->Visible = Visible;

	for(auto &Child : Children)
		Child->SetVisible(Visible);
}
