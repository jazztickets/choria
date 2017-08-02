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
#include <string>
#include <list>

// Forward Declarations
struct _Style;
struct _KeyEvent;
namespace tinyxml2 {
class XMLDocument;
	class XMLElement;
}

// Classes
class _Element {

	public:

		_Element();
		virtual ~_Element();

		virtual const char *GetTypeName() const { return "element"; }
		virtual void Update(double FrameTime, const glm::vec2 &Mouse);
		virtual void Render(bool IgnoreVisible=false) const;
		virtual bool HandleKey(const _KeyEvent &KeyEvent);
		virtual void HandleInput(bool Pressed);
		virtual void CalculateBounds();
		_Element *GetClickedElement();

		void RemoveChild(_Element *Element);
		void UpdateChildrenOffset(const glm::vec2 &Update) { ChildrenOffset += Update; CalculateChildrenBounds(); }
		void CalculateChildrenBounds();
		void SerializeElement(tinyxml2::XMLDocument &Document, tinyxml2::XMLElement *ParentNode);

		void SetDebug(int Debug);
		void SetClickable(bool Clickable, int Depth=-1);
		void SetVisible(bool Visible);
		void SetFade(float Fade);
		void SetEnabled(bool Enabled);
		void SetOffset(const glm::vec2 &Offset) { this->Offset = Offset; CalculateBounds(); }
		void SetWidth(float Width) { Size.x = Width; CalculateBounds(); }
		void SetHeight(float Height) { Size.y = Height; CalculateBounds(); }

		// Attributes
		size_t GlobalID;
		std::string Identifier;
		std::string ParentIdentifier;
		_Element *Parent;
		void *UserData;
		void *UserDataAlt;

		bool Visible : 1;
		bool Enabled : 1;
		bool Clickable : 1;
		bool MaskOutside : 1;
		bool UserCreated : 1;
		int Debug;

		// Graphics
		std::string ColorName;
		std::string FontName;
		const _Style *Style;
		const _Style *DisabledStyle;
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

		// Children
		std::list<_Element *> Children;
		glm::vec2 ChildrenOffset;

	protected:

		virtual void SerializeAttributes(tinyxml2::XMLElement *Node);

};
