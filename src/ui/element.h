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
#pragma once

// Libraries
#include <ui/ui.h>
#include <glm/vec4.hpp>
#include <string>
#include <list>

// Forward Declarations
class _Font;
class _Texture;
class _Atlas;
struct _Style;
struct _KeyEvent;
namespace tinyxml2 {
	class XMLDocument;
	class XMLElement;
}

// Classes
class _Element {

	public:

		enum ElementType {
			NONE,
			ELEMENT,
			BUTTON,
			IMAGE,
			LABEL,
			TEXTBOX,
		};

		_Element();
		_Element(tinyxml2::XMLElement *Node, _Element *ParentNode);
		~_Element();

		void SerializeElement(tinyxml2::XMLDocument &Document, tinyxml2::XMLElement *ParentNode);

		const char *GetTypeName() const;
		void Update(double FrameTime, const glm::vec2 &Mouse);
		void Render() const;
		bool HandleKey(const _KeyEvent &KeyEvent);
		void HandleInput(bool Pressed);
		void CalculateBounds();
		_Element *GetClickedElement();

		void RemoveChild(_Element *Element);
		void UpdateChildrenOffset(const glm::vec2 &Update) { ChildrenOffset += Update; CalculateChildrenBounds(); }
		void CalculateChildrenBounds();

		void Clear() { CursorTimer = 0; Text = ""; CursorPosition = 0; }
		void ResetCursor() { CursorTimer = 0; }

		void SetDebug(int Debug);
		void SetClickable(bool Clickable, int Depth=-1);
		void SetVisible(bool Visible);
		void SetFade(float Fade);
		void SetEnabled(bool Enabled);
		void SetOffset(const glm::vec2 &Offset) { this->Offset = Offset; CalculateBounds(); }
		void SetWidth(float Width) { Size.x = Width; CalculateBounds(); }
		void SetHeight(float Height) { Size.y = Height; CalculateBounds(); }
		void SetText(const std::string &Text) { this->Text = Text; CursorPosition = Text.length(); }
		void SetWrap(float Width);

		// Attributes
		ElementType Type;
		std::string ID;
		_Element *Parent;
		int Index;
		void *UserData;

		bool Visible;
		bool Enabled;
		bool Checked;
		bool Clickable;
		bool MaskOutside;
		bool Stretch;
		int Debug;

		// Graphics
		glm::vec4 Color;
		std::string ColorName;
		std::string FontName;
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
		glm::vec2 ParentOffset;
		size_t MaxLength;
		size_t CursorPosition;
		double CursorTimer;
		bool Password;

		// Children
		std::list<_Element *> Children;
		glm::vec2 ChildrenOffset;

	private:

		void AssignAttributeString(tinyxml2::XMLElement *Node, const char *Attribute, std::string &String);

		std::list<std::string> Texts;

};
