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
#pragma once

// Libraries
#include <ui/ui.h>
#include <string>
#include <vector>

// Forward Declarations
struct _Style;
struct _KeyEvent;

// Classes
class _Element {

	public:

		_Element();
		virtual ~_Element();

		virtual void Update(double FrameTime, const glm::ivec2 &Mouse);

		virtual void CalculateBounds();
		virtual void Render() const;
		virtual void HandleKeyEvent(const _KeyEvent &KeyEvent);
		virtual void HandleTextEvent(const char *Text);
		virtual void HandleInput(bool Pressed);
		_Element *GetClickedElement();

		void UpdateChildrenOffset(const glm::ivec2 &Update) { ChildrenOffset += Update; CalculateChildrenBounds(); }
		void CalculateChildrenBounds();

		void SetDebug(int Debug);
		void SetOffset(const glm::ivec2 &Offset) { this->Offset = Offset; CalculateBounds(); }
		void SetWidth(int Width) { Size.x = Width; CalculateBounds(); }
		void SetHeight(int Height) { Size.y = Height; CalculateBounds(); }

		// Attributes
		size_t GlobalID;
		std::string Identifier;
		std::string ParentIdentifier;
		_Element *Parent;
		void *UserData;

		int MaskOutside : 1;
		int Enabled : 1;
		int Debug : 4;

		// Graphics
		const _Style *Style;
		const _Style *DisabledStyle;
		float Fade;

		// Layout
		_Bounds Bounds;
		_Alignment Alignment;
		glm::ivec2 Size;
		glm::ivec2 Offset;

		// Input
		_Element *HitElement;
		_Element *PressedElement;
		_Element *ReleasedElement;

		// Children
		std::vector<_Element *> Children;
		glm::ivec2 ChildrenOffset;

	protected:

};
