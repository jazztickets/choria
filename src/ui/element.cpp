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
#include <ui/element.h>
#include <ui/style.h>
#include <graphics.h>
#include <input.h>
#include <assets.h>
#include <constants.h>
#include <algorithm>
#include <tinyxml2.h>

const glm::vec4 DebugColors[] = { COLOR_CYAN, COLOR_YELLOW, COLOR_RED, COLOR_GREEN, COLOR_BLUE };
const int DebugColorCount = sizeof(DebugColors) / sizeof(glm::vec4);

// Constructor
_Element::_Element() :
	GlobalID(0),
	Parent(nullptr),
	UserData((void *)-1),
	UserDataAlt(nullptr),
	Visible(false),
	Enabled(true),
	Checked(false),
	Clickable(true),
	MaskOutside(false),
	Stretch(false),
	UserCreated(false),
	Debug(0),
	Color(1.0f, 1.0f, 1.0f, 1.0f),
	Style(nullptr),
	HoverStyle(nullptr),
	DisabledStyle(nullptr),
	TextureIndex(0),
	Fade(1.0f),
	HitElement(nullptr),
	PressedElement(nullptr),
	ReleasedElement(nullptr),
	MaxLength(0) {

}

// Destructor
_Element::~_Element() {
}

// Handle key event
bool _Element::HandleKey(const _KeyEvent &KeyEvent) {

	// Pass event to children
	for(auto &Child : Children) {
		if(Child->HandleKey(KeyEvent))
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

// Serialize element and children to xml node
void _Element::SerializeElement(tinyxml2::XMLDocument &Document, tinyxml2::XMLElement *ParentNode) {

	// Create xml node
	tinyxml2::XMLElement *Node = Document.NewElement(GetTypeName());

	// Set base attributes
	Node->SetAttribute("identifier", Identifier.c_str());
	if(Style)
		Node->SetAttribute("style", Style->Identifier.c_str());
	if(HoverStyle)
		Node->SetAttribute("hover_style", HoverStyle->Identifier.c_str());
	if(DisabledStyle)
		Node->SetAttribute("disabled_style", DisabledStyle->Identifier.c_str());
	if(ColorName.size())
		Node->SetAttribute("color", ColorName.c_str());
	if(FontName.size())
		Node->SetAttribute("font", FontName.c_str());

	// Set attributes
	SerializeAttributes(Node);

	// Append
	ParentNode->InsertEndChild(Node);

	// Add children
	for(const auto &Child : Children)
		Child->SerializeElement(Document, Node);
}

// Serialize attributes
void _Element::SerializeAttributes(tinyxml2::XMLElement *Node) {

	if(Offset.x != 0.0f)
		Node->SetAttribute("offset_x", Offset.x);
	if(Offset.y != 0.0f)
		Node->SetAttribute("offset_y", Offset.y);
	if(Size.x != 0.0f)
		Node->SetAttribute("size_x", Size.x);
	if(Size.y != 0.0f)
		Node->SetAttribute("size_y", Size.y);
	if(Alignment.Horizontal != _Alignment::CENTER)
		Node->SetAttribute("alignment_x", Alignment.Horizontal);
	if(Alignment.Vertical != _Alignment::MIDDLE)
		Node->SetAttribute("alignment_y", Alignment.Vertical);
	if(Clickable != 1)
		Node->SetAttribute("clickable", Clickable);
	if(Stretch)
		Node->SetAttribute("stretch", Stretch);
	if(MaxLength)
		Node->SetAttribute("maxlength", (uint32_t)MaxLength);
	if((intptr_t)UserData != -1)
		Node->SetAttribute("userdata", (intptr_t)UserData);
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

// Set fade of element and children
void _Element::SetFade(float Fade) {
	this->Fade = Fade;

	for(auto &Child : Children)
		Child->SetFade(Fade);
}

// Set enabled state of element
void _Element::SetEnabled(bool Enabled) {
	this->Enabled = Enabled;

	for(auto &Child : Children)
		Child->SetEnabled(Enabled);
}
