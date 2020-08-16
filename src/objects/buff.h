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

// Libraries
#include <string>
#include <cstdint>

// Forward Declarations
class _Object;
class _Scripting;
class _StatChange;

namespace ae {
	class _Texture;
}

// Classes
class _Buff {

	public:

		void DrawTooltip(_Scripting *Scripting, int Level, bool Infinite, double Duration, int DismissLevel) const;
		void ExecuteScript(_Scripting *Scripting, const std::string &Function, int Level, _StatChange &StatChange) const;

		uint32_t ID;
		std::string Name;
		std::string Script;
		const ae::_Texture *Texture;
		bool PauseDuringBattle;
		bool ShowLevel;
		int Dismiss;
		double WarningTime;

	private:

};
