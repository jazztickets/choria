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
#include <instances/battle.h>
#include <glm/vec2.hpp>
#include <string>

// Forward Declarations
class _Object;
class _Texture;
class _Scripting;
struct _ActionResult;
struct _Cursor;

// Classes
class _Buff {

	public:

		void DrawTooltip(_Scripting *Scripting, const _Object *Player, const _Cursor &Tooltip, bool DrawNextLevel) const;
		void DrawDescription(_Scripting *Scripting, int SkillLevel, glm::vec2 &DrawPosition, int Width) const;

		//bool CanUse(_Scripting *Scripting, _ActionResult &ActionResult) const;
		//void ApplyCost(_Scripting *Scripting, _ActionResult &ActionResult) const;
		//void Use(_Scripting *Scripting, _ActionResult &ActionResult) const;

		uint32_t ID;
		std::string Name;
		std::string Script;
		const _Texture *Texture;

	private:

};
