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
#include <texture.h>

// Forward Declarations
class _Object;
struct _Vendor;
struct _ActionResult;
struct _Cursor;

// Classes
class _Skill {

	public:

		enum SkillType {
			TYPE_ATTACK,
			TYPE_SPELL,
			TYPE_PASSIVE,
			TYPE_USEPOTION,
		};

		void DrawTooltip(const _Object *Player, const _Cursor &Tooltip, bool DrawNextLevel) const;
		void DrawDescription(int SkillLevel, glm::ivec2 &DrawPosition, int Width) const;

		int GetManaCost(int Level) const;
		int GetPower(int Level) const;
		void GetPowerRange(int Level, int &Min, int &Max) const;
		void GetPowerRangeRound(int Level, int &Min, int &Max) const;
		void GetPowerRange(int Level, float &Min, float &Max) const;

		void ResolveSkill(_ActionResult *Result, _ActionResult *TargetResult) const;
		bool CanUse(_Object *Fighter) const;

		uint32_t ID;
		int Type;
		std::string Name;
		std::string Info;
		const _Texture *Image;
		int SkillCost;

		float ManaCostBase;
		float ManaCost;
		float PowerBase;
		float PowerRangeBase;
		float Power;
		float PowerRange;

	private:

};
