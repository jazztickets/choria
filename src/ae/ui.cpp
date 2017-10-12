/******************************************************************************
* Copyright (c) 2017 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*******************************************************************************/
#include <ae/ui.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <ae/input.h>
#include <ae/assets.h>
#include <ae/font.h>
#include <ae/texture.h>
#include <ae/atlas.h>
#include <constants.h>
#include <SDL_keycode.h>
#include <tinyxml2.h>
#include <algorithm>
#include <iostream>

_Element *FocusedElement = nullptr;
const glm::vec4 DebugColors[] = {
	{ 0.0f, 1.0f, 1.0f, 1.0f },
	{ 1.0f, 1.0f, 0.0f, 1.0f },
	{ 1.0f, 0.0f, 0.0f, 1.0f },
	{ 0.0f, 1.0f, 0.0f, 1.0f },
	{ 0.0f, 0.0f, 1.0f, 1.0f }
};
const int DebugColorCount = sizeof(DebugColors) / sizeof(glm::vec4);

// Constructor
_Element::_Element() :
	Parent(nullptr),
	Index(-1),
	UserData(nullptr),
	Active(false),
	Enabled(true),
	Checked(false),
	Clickable(true),
	MaskOutside(false),
	Stretch(false),
	Debug(0),
	Color(1.0f, 1.0f, 1.0f, 1.0f),
	Style(nullptr),
	HoverStyle(nullptr),
	DisabledStyle(nullptr),
	Texture(nullptr),
	Atlas(nullptr),
	TextureIndex(0),
	Fade(1.0f),
	HitElement(nullptr),
	PressedElement(nullptr),
	ReleasedElement(nullptr),
	Font(nullptr),
	MaxLength(0),
	CursorPosition(0),
	CursorTimer(0),
	Password(false),
	ChildrenOffset(0, 0) {
}

// Constructor for loading from xml
_Element::_Element(tinyxml2::XMLElement *Node, _Element *Parent) :
	_Element() {

	this->Parent = Parent;
	std::string TextureName;
	std::string StyleName;
	std::string HoverStyleName;
	std::string DisabledStyleName;
	std::string FontName;
	AssignAttributeString(Node, "id", Name);
	AssignAttributeString(Node, "texture", TextureName);
	AssignAttributeString(Node, "style", StyleName);
	AssignAttributeString(Node, "hover_style", HoverStyleName);
	AssignAttributeString(Node, "disabled_style", DisabledStyleName);
	AssignAttributeString(Node, "color", ColorName);
	AssignAttributeString(Node, "font", FontName);
	AssignAttributeString(Node, "text", Text);
	Node->QueryUnsignedAttribute("maxlength", (uint32_t *)&MaxLength);
	Node->QueryFloatAttribute("offset_x", &Offset.x);
	Node->QueryFloatAttribute("offset_y", &Offset.y);
	Node->QueryFloatAttribute("size_x", &Size.x);
	Node->QueryFloatAttribute("size_y", &Size.y);
	Node->QueryIntAttribute("alignment_x", &Alignment.Horizontal);
	Node->QueryIntAttribute("alignment_y", &Alignment.Vertical);
	Node->QueryBoolAttribute("clickable", &Clickable);
	Node->QueryBoolAttribute("stretch", &Stretch);
	Node->QueryIntAttribute("index", &Index);
	Node->QueryIntAttribute("debug", &Debug);

	// Check ids
	if(Assets.Elements.find(Name) != Assets.Elements.end())
		throw std::runtime_error("Duplicate element id: " + Name);
	if(TextureName != "" && Assets.Textures.find(TextureName) == Assets.Textures.end())
		throw std::runtime_error("Unable to find texture: " + TextureName + " for image: " + Name);
	if(StyleName != "" && Assets.Styles.find(StyleName) == Assets.Styles.end())
		throw std::runtime_error("Unable to find style: " + StyleName + " for element: " + Name);
	if(HoverStyleName != "" && Assets.Styles.find(HoverStyleName) == Assets.Styles.end())
		throw std::runtime_error("Unable to find hover_style: " + HoverStyleName + " for element: " + Name);
	if(DisabledStyleName != "" && Assets.Styles.find(DisabledStyleName) == Assets.Styles.end())
		throw std::runtime_error("Unable to find disabled_style: " + DisabledStyleName + " for element: " + Name);
	if(ColorName != "" && Assets.Colors.find(ColorName) == Assets.Colors.end())
		throw std::runtime_error("Unable to find color: " + ColorName + " for element: " + Name);
	if(FontName != "" && Assets.Fonts.find(FontName) == Assets.Fonts.end())
		throw std::runtime_error("Unable to find font: " + FontName + " for element: " + Name);

	// Assign pointers
	Texture = Assets.Textures[TextureName];
	Style = Assets.Styles[StyleName];
	HoverStyle = Assets.Styles[HoverStyleName];
	DisabledStyle = Assets.Styles[DisabledStyleName];
	Color = Assets.Colors[ColorName];
	Font = Assets.Fonts[FontName];

	// Assign to list
	if(Name != "")
		Assets.Elements[Name] = this;

	// Load children
	for(tinyxml2::XMLElement *ChildNode = Node->FirstChildElement(); ChildNode != nullptr; ChildNode = ChildNode->NextSiblingElement()) {
		_Element *ChildElement = new _Element(ChildNode, this);
		Children.push_back(ChildElement);
	}

	// Set debug for children
	SetDebug(Debug);
}

// Destructor
_Element::~_Element() {
	for(auto &Child : Children) {
		if(Graphics.Element->HitElement == Child)
			Graphics.Element->HitElement = nullptr;
		delete Child;
	}
}

// Serialize element and children to xml node
void _Element::SerializeElement(tinyxml2::XMLDocument &Document, tinyxml2::XMLElement *ParentNode) {

	// Create xml node
	tinyxml2::XMLElement *Node = Document.NewElement("element");

	// Set attributes
	if(ParentNode) {
		Node->SetAttribute("id", Name.c_str());
		if(Texture)
			Node->SetAttribute("texture", Texture->Name.c_str());
		if(Style)
			Node->SetAttribute("style", Style->Name.c_str());
		if(HoverStyle)
			Node->SetAttribute("hover_style", HoverStyle->Name.c_str());
		if(DisabledStyle)
			Node->SetAttribute("disabled_style", DisabledStyle->Name.c_str());
		if(ColorName.size())
			Node->SetAttribute("color", ColorName.c_str());
		if(Font)
			Node->SetAttribute("font", Font->ID.c_str());
		if(Text.size())
			Node->SetAttribute("text", Text.c_str());
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
		if(MaxLength)
			Node->SetAttribute("maxlength", (uint32_t)MaxLength);
		if(Clickable != 1)
			Node->SetAttribute("clickable", Clickable);
		if(Stretch)
			Node->SetAttribute("stretch", Stretch);
		if(Index != -1)
			Node->SetAttribute("index", Index);

		ParentNode->InsertEndChild(Node);
	}
	else
		Document.InsertEndChild(Node);

	// Add children
	for(const auto &Child : Children)
		Child->SerializeElement(Document, Node);
}

// Handle key event, return true if handled
bool _Element::HandleKey(const _KeyEvent &KeyEvent) {
	if(!Active)
		return false;

	// Handle editable text fields
	if(MaxLength) {
		if(FocusedElement == this && Active && KeyEvent.Pressed) {
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

	// Pass event to children
	for(auto &Child : Children) {
		if(Child->HandleKey(KeyEvent))
			return true;
	}

	return false;
}

// Handle a press event
void _Element::HandleMouseButton(bool Pressed) {
	if(!Active)
		return;

	if(MaxLength && Enabled) {
	   if(HitElement || (Parent && Parent->HitElement)) {
		   ResetCursor();
		   FocusedElement = this;
	   }

	   return;
	}

	// Pass event to children
	for(auto &Child : Children)
		Child->HandleMouseButton(Pressed);

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
	if(Bounds.Inside(Mouse) && Active && Clickable && Enabled) {
		HitElement = this;
	}
	else if(MaskOutside) {
		HitElement = nullptr;
		return;
	}

	// Test children
	if(Active) {
		for(auto &Child : Children) {
			Child->Update(FrameTime, Mouse);
			if(Child->HitElement)
				HitElement = Child->HitElement;
		}
	}

	// Handle edit boxes
	if(MaxLength) {
		if(FocusedElement == this || FocusedElement == Parent) {
			CursorTimer += FrameTime;
			if(CursorTimer >= 1) {
				CursorTimer = 0;
			}
		}
	}
}

// Render the element
void _Element::Render() const {
	if(!Active)
		return;

	// Mask outside bounds of element
	if(MaskOutside) {
		Graphics.SetProgram(Assets.Programs["ortho_pos"]);
		Graphics.EnableStencilTest();
		Graphics.DrawMask(Bounds);
	}

	if(Enabled) {
		if(Style) {
			DrawStyle(Style);
		}
		else if(Atlas) {
			Graphics.SetColor(Color);
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.SetVBO(VBO_NONE);
			Graphics.DrawAtlas(Bounds, Atlas->Texture, Atlas->GetTextureCoords(TextureIndex));
		}
		else if(Texture) {
			Graphics.SetColor(Color);
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.SetVBO(VBO_NONE);
			Graphics.DrawImage(Bounds, Texture, Stretch);
		}

		// Draw hover texture
		if(HoverStyle && (Checked || HitElement)) {
			DrawStyle(HoverStyle);
		}
	}
	else if(DisabledStyle) {
		DrawStyle(DisabledStyle);
	}

	// Set color
	if(Texts.size() || Text != "" || MaxLength) {
		glm::vec4 RenderColor(Color.r, Color.g, Color.b, Color.a*Fade);
		if(!Enabled)
			RenderColor.a *= 0.5f;

		Graphics.SetProgram(Assets.Programs["pos_uv"]);
		Graphics.SetVBO(VBO_NONE);
		if(Texts.size()) {

			// Center box
			float LineHeight = Font->MaxHeight + 2;
			float Y = Bounds.Start.y - (int)((LineHeight * Texts.size() - LineHeight) / 2);
			for(const auto &Token : Texts) {
				Font->DrawText(Token, glm::vec2(Bounds.Start.x, Y), Alignment, RenderColor);

				Y += LineHeight;
			}
		}
		else {
			std::string RenderText = Password ? std::string(Text.length(), '*') : Text;

			if(MaxLength) {

				// Get width at cursor position
				_TextBounds TextBounds;
				Font->GetStringDimensions(RenderText.substr(0, CursorPosition), TextBounds);

				// Draw text
				glm::vec2 StartPosition = glm::vec2(Bounds.Start);
				Font->DrawText(RenderText, StartPosition, Alignment, RenderColor);

				// Draw cursor
				if(CursorTimer < 0.5 && (FocusedElement == this || FocusedElement == Parent)) {
					Graphics.SetProgram(Assets.Programs["ortho_pos"]);
					Graphics.DrawRectangle(glm::vec2(StartPosition.x + TextBounds.Width+1, StartPosition.y - Font->MaxAbove), glm::vec2(StartPosition.x + TextBounds.Width+2, StartPosition.y + Font->MaxBelow));
				}
			}
			else
				Font->DrawText(RenderText, Bounds.Start, Alignment, RenderColor);
		}
	}

	// Render all children
	for(auto &Child : Children) {
		Child->Render();
	}

	// Disable mask
	if(MaskOutside)
		Graphics.DisableStencilTest();

	// Draw debug info
	if(Debug && Debug-1 < DebugColorCount) {
		Graphics.SetProgram(Assets.Programs["ortho_pos"]);
		Graphics.SetVBO(VBO_NONE);
		Graphics.SetColor(DebugColors[Debug-1]);
		Graphics.DrawRectangle(Bounds.Start, Bounds.End);
	}
}

// Draw an element using a style
void _Element::DrawStyle(const _Style *DrawStyle) const {
	Graphics.SetProgram(DrawStyle->Program);
	Graphics.SetVBO(VBO_NONE);
	if(DrawStyle->Texture) {
		Graphics.SetColor(DrawStyle->TextureColor);
		Graphics.DrawImage(Bounds, DrawStyle->Texture, DrawStyle->Stretch);
	}
	else {
		if(DrawStyle->HasBackgroundColor) {
			glm::vec4 RenderColor(DrawStyle->BackgroundColor);
			RenderColor.a *= Fade;
			Graphics.SetColor(RenderColor);
			Graphics.DrawRectangle(Bounds, true);
		}
		if(DrawStyle->HasBorderColor) {
			glm::vec4 RenderColor(DrawStyle->BorderColor);
			RenderColor.a *= Fade;
			Graphics.SetColor(RenderColor);
			Graphics.DrawRectangle(Bounds, false);
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

// Set the debug flag, and increment for children
void _Element::SetDebug(int Debug) {
	this->Debug = Debug;

	for(auto &Child : Children) {
		if(Debug)
			Child->SetDebug(Debug + 1);
	}
}

// Set clickable/hoverable flag of element and children. Depth=-1 is full recursion
void _Element::SetClickable(bool Clickable, int Depth) {
	if(Depth == 0)
		return;

	this->Clickable = Clickable;

	if(Depth != -1)
		Depth--;

	for(auto &Child : Children)
		Child->SetClickable(Clickable, Depth);
}

// Set active state of element and children
void _Element::SetActive(bool Visible) {
	this->Active = Visible;

	for(auto &Child : Children)
		Child->SetActive(Visible);
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

// Break up text into multiple strings
void _Element::SetWrap(float Width) {

	Texts.clear();
	Font->BreakupString(Text, Width, Texts);
}

// Assign a string from xml attribute
void _Element::AssignAttributeString(tinyxml2::XMLElement *Node, const char *Attribute, std::string &String) {
	const char *Value = Node->Attribute(Attribute);
	if(Value)
		String = Value;
}
