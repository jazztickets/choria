/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020 Alan Witkowski
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

// Forward Declarations
class _HUD;

namespace  ae {
	class _Element;
}

// Classes
class _Screen {

	public:

		_Screen(_HUD *HUD, ae::_Element *Element);
		virtual ~_Screen() { }

		virtual void Init() { }
		virtual bool Close(bool SendNotify=true) { return false; }

		virtual void Toggle();
		virtual void Render(double BlendFactor) { }

		// UI
		_HUD *HUD;
		ae::_Element *Element;

	protected:


};
