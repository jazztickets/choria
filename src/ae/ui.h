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
#pragma once

// Libraries
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <list>

// Forward Declarations
class _Font;
class _Texture;
class _Atlas;
class _Program;
struct _KeyEvent;
namespace tinyxml2 {
	class XMLDocument;
	class XMLElement;
}

// Bounds struct
struct _Bounds {
	_Bounds() { }
	_Bounds(const glm::vec2 &Start, const glm::vec2 &End) : Start(Start), End(End) { }
	_Bounds(const glm::vec4 &Bounds) : Start(Bounds[0], Bounds[1]), End(Bounds[2], Bounds[3]) { }
	bool Inside(const glm::vec2 &Point) { return Point.x >= Start.x && Point.y >= Start.y && Point.x < End.x && Point.y < End.y; }

	glm::vec2 Start;
	glm::vec2 End;
};

// Alignment struct
struct _Alignment {

	enum HorizontalAlignment {
		LEFT,
		CENTER,
		RIGHT,
	};

	enum VerticalAlignment {
		TOP,
		MIDDLE,
		BOTTOM,
		BASELINE,
	};

	_Alignment() { Horizontal = CENTER; Vertical = MIDDLE; }
	_Alignment(int Horizontal, int Vertical) : Horizontal(Horizontal), Vertical(Vertical) { }

	int Horizontal, Vertical;
};

// Style struct
struct _Style {
	_Style() :
		HasBackgroundColor(false),
		HasBorderColor(false),
		Program(nullptr),
		Texture(nullptr),
		Stretch(false) { }

	// Attributes
	std::string Name;

	// Colors
	glm::vec4 TextureColor;
	glm::vec4 BackgroundColor;
	glm::vec4 BorderColor;
	bool HasBackgroundColor;
	bool HasBorderColor;

	// Graphics
	const _Program *Program;
	const _Texture *Texture;

	// Properties
	bool Stretch;
};

// Classes
class _Element {

	public:

		_Element();
		_Element(tinyxml2::XMLElement *Node, _Element *Parent);
		~_Element();

		void SerializeElement(tinyxml2::XMLDocument &Document, tinyxml2::XMLElement *ParentNode);

		void Update(double FrameTime, const glm::vec2 &Mouse);
		void Render() const;
		bool HandleKey(const _KeyEvent &KeyEvent);
		void HandleMouseButton(bool Pressed);
		void CalculateBounds();
		_Element *GetClickedElement();

		void RemoveChild(_Element *Element);
		void UpdateChildrenOffset(const glm::vec2 &Update) { ChildrenOffset += Update; CalculateChildrenBounds(); }
		void CalculateChildrenBounds();

		void Clear() { CursorTimer = 0; Text = ""; CursorPosition = 0; }
		void ResetCursor() { CursorTimer = 0; }

		void SetDebug(int Debug);
		void SetClickable(bool Clickable, int Depth=-1);
		void SetActive(bool Active);
		void SetFade(float Fade);
		void SetEnabled(bool Enabled);
		void SetOffset(const glm::vec2 &Offset) { this->Offset = Offset; CalculateBounds(); }
		void SetWidth(float Width) { Size.x = Width; CalculateBounds(); }
		void SetHeight(float Height) { Size.y = Height; CalculateBounds(); }
		void SetText(const std::string &Text) { this->Text = Text; CursorPosition = Text.length(); }
		void SetWrap(float Width);

		// Attributes
		std::string Name;
		_Element *Parent;
		int Index;
		void *UserData;

		bool Active;
		bool Enabled;
		bool Checked;
		bool Clickable;
		bool MaskOutside;
		bool Stretch;
		int Debug;

		// Graphics
		glm::vec4 Color;
		std::string ColorName;
		const _Style *Style;
		const _Style *HoverStyle;
		const _Style *DisabledStyle;
		const _Texture *Texture;
		const _Atlas *Atlas;
		uint32_t TextureIndex;
		float Fade;

		// Layout
		_Bounds Bounds;
		_Alignment Alignment;
		glm::vec2 Size;
		glm::vec2 Offset;

		// Input
		_Element *HitElement;
		_Element *PressedElement;
		_Element *ReleasedElement;

		// Text
		const _Font *Font;
		std::string Text;
		size_t MaxLength;
		size_t CursorPosition;
		double CursorTimer;
		bool Password;

		// Children
		std::list<_Element *> Children;
		glm::vec2 ChildrenOffset;

	private:

		void DrawStyle(const _Style *DrawStyle) const;
		void AssignAttributeString(tinyxml2::XMLElement *Node, const char *Attribute, std::string &String);

		std::list<std::string> Texts;

};

extern _Element *FocusedElement;

const _Alignment LEFT_TOP         = _Alignment(_Alignment::LEFT,   _Alignment::TOP);
const _Alignment LEFT_BOTTOM      = _Alignment(_Alignment::LEFT,   _Alignment::BOTTOM);
const _Alignment RIGHT_BOTTOM     = _Alignment(_Alignment::RIGHT,  _Alignment::BOTTOM);
const _Alignment CENTER_MIDDLE    = _Alignment(_Alignment::CENTER, _Alignment::MIDDLE);
const _Alignment LEFT_BASELINE    = _Alignment(_Alignment::LEFT,   _Alignment::BASELINE);
const _Alignment RIGHT_BASELINE   = _Alignment(_Alignment::RIGHT,  _Alignment::BASELINE);
const _Alignment CENTER_BASELINE  = _Alignment(_Alignment::CENTER, _Alignment::BASELINE);
