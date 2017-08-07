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
#include <ui.h>
#include <graphics.h>
#include <assets.h>
#include <input.h>
#include <assets.h>
#include <constants.h>
#include <font.h>
#include <texture.h>
#include <atlas.h>
#include <algorithm>
#include <iostream>
#include <SDL_keycode.h>
#include <tinyxml2.h>

_Element *FocusedElement = nullptr;
const glm::vec4 DebugColors[] = { COLOR_CYAN, COLOR_YELLOW, COLOR_RED, COLOR_GREEN, COLOR_BLUE };
const int DebugColorCount = sizeof(DebugColors) / sizeof(glm::vec4);

// Constructor
_Element::_Element() :
	Type(NONE),
	Parent(nullptr),
	Index(-1),
	UserData(nullptr),
	Visible(false),
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

	//TODO cleanup
	ParentOffset = glm::vec2(5.0f, 22.0f);
}

// Constructor for loading from xml
_Element::_Element(tinyxml2::XMLElement *Node, _Element *Parent) :
	_Element() {

	this->Parent = Parent;
	std::string TypeName = Node->Value();
	if(TypeName == "element")
		Type = ELEMENT;
	else if(TypeName == "image")
		Type = IMAGE;
	else if(TypeName == "button")
		Type = BUTTON;
	else if(TypeName == "label")
		Type = LABEL;
	else if(TypeName == "textbox")
		Type = TEXTBOX;

	std::string TextureName;
	std::string StyleName;
	std::string HoverStyleName;
	std::string DisabledStyleName;
	AssignAttributeString(Node, "id", ID);
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

	// Check identifiers
	if(Assets.Elements.find(ID) != Assets.Elements.end())
		throw std::runtime_error("Duplicate element identifier: " + ID);
	if(TextureName != "" && Assets.Textures.find(TextureName) == Assets.Textures.end())
		throw std::runtime_error("Unable to find texture: " + TextureName + " for image: " + ID);
	if(StyleName != "" && Assets.Styles.find(StyleName) == Assets.Styles.end())
		throw std::runtime_error("Unable to find style: " + StyleName + " for element: " + ID);
	if(HoverStyleName != "" && Assets.Styles.find(HoverStyleName) == Assets.Styles.end())
		throw std::runtime_error("Unable to find hover_style: " + HoverStyleName + " for element: " + ID);
	if(DisabledStyleName != "" && Assets.Styles.find(DisabledStyleName) == Assets.Styles.end())
		throw std::runtime_error("Unable to find disabled_style: " + DisabledStyleName + " for element: " + ID);
	if(ColorName != "" && Assets.Colors.find(ColorName) == Assets.Colors.end())
		throw std::runtime_error("Unable to find color: " + ColorName + " for element: " + ID);
	if(FontName != "" && Assets.Fonts.find(FontName) == Assets.Fonts.end())
		throw std::runtime_error("Unable to find font: " + FontName + " for element: " + ID);

	// Assign pointers
	Texture = Assets.Textures[TextureName];
	Style = Assets.Styles[StyleName];
	HoverStyle = Assets.Styles[HoverStyleName];
	DisabledStyle = Assets.Styles[DisabledStyleName];
	Color = Assets.Colors[ColorName];
	Font = Assets.Fonts[FontName];

	// Assign to list
	if(ID != "")
		Assets.Elements[ID] = this;

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
	for(auto &Child : Children)
		delete Child;
}

// Serialize element and children to xml node
void _Element::SerializeElement(tinyxml2::XMLDocument &Document, tinyxml2::XMLElement *ParentNode) {

	// Create xml node
	tinyxml2::XMLElement *Node = Document.NewElement(GetTypeName());

	// Set attributes
	if(ParentNode) {
		Node->SetAttribute("id", ID.c_str());
		if(Texture)
			Node->SetAttribute("texture", Texture->Identifier.c_str());
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

// Get type name
const char *_Element::GetTypeName() const {

	switch(Type) {
		case NONE:
			return "";
		case ELEMENT:
			return "element";
		case BUTTON:
			return "button";
		case IMAGE:
			return "image";
		case LABEL:
			return "label";
		case TEXTBOX:
			return "textbox";
	}

	return "";
}

// Handle key event
bool _Element::HandleKey(const _KeyEvent &KeyEvent) {

	if(Type == TEXTBOX) {
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

	if(Type == TEXTBOX) {
	   if(HitElement) {
		   ResetCursor();
		   FocusedElement = this;
	   }
	   return;
	}

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

	if(Type == TEXTBOX) {
		if(FocusedElement == this) {
			CursorTimer += FrameTime;
			if(CursorTimer >= 1) {
				CursorTimer = 0;
			}
		}
	}
}

// Render the element
void _Element::Render() const {
	if(Type == NONE)
		throw std::runtime_error("Bad _Element type!");

	if(!Visible)
		return;

	switch(Type) {
		case BUTTON:
			if(Enabled) {
				if(Style) {
					if(Style->Texture) {
						Graphics.SetProgram(Style->Program);
						Graphics.SetVBO(VBO_NONE);
						Graphics.SetColor(Style->TextureColor);
						Graphics.DrawImage(Bounds, Style->Texture, Style->Stretch);
					}
					else if(Atlas) {
						Graphics.SetProgram(Style->Program);
						Graphics.SetVBO(VBO_NONE);
						Graphics.SetColor(Style->TextureColor);
						Graphics.DrawAtlas(Bounds, Atlas->Texture, Atlas->GetTextureCoords(TextureIndex));
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
					else if(Atlas) {
						Graphics.SetProgram(DisabledStyle->Program);
						Graphics.SetVBO(VBO_NONE);
						Graphics.SetColor(DisabledStyle->TextureColor);
						Graphics.DrawAtlas(Bounds, Atlas->Texture, Atlas->GetTextureCoords(TextureIndex));
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

			return;
		break;
		case IMAGE: {
			Graphics.SetColor(Color);
			if(Texture) {
				Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
				Graphics.SetVBO(VBO_NONE);
				Graphics.DrawImage(Bounds, Texture, Stretch);
			}
		} break;
		case LABEL: {

			// Set color
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
					Font->DrawText(Token, glm::vec2(Bounds.Start.x, Y), RenderColor, Alignment);

					Y += LineHeight;
				}
			}
			else {
				Font->DrawText(Text, Bounds.Start, RenderColor, Alignment);
			}
		} break;
		default:
		break;
	}

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
		Graphics.SetColor(DebugColors[Debug-1]);
		Graphics.DrawRectangle(Bounds.Start, Bounds.End);
	}

	if(Type == TEXTBOX) {

		// Get text to render
		std::string RenderText;
		if(Password)
			RenderText = std::string(Text.length(), '*');
		else
			RenderText = Text;

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

// Set the debug, and increment for children
void _Element::SetDebug(int Debug) {
	this->Debug = Debug;

	for(auto &Child : Children) {
		if(Debug)
			Child->SetDebug(Debug + 1);
	}
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
