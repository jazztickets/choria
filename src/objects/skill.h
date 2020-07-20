/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020  Alan Witkowski
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
#include <objects/usable.h>

struct _WeaponType;

// Classes
class _BaseSkill : public _Usable {

	public:

		_BaseSkill();

		bool IsSkill() const override { return true; }
		void DrawTooltip(const glm::vec2 &Position, const _Object *Player, const _Cursor &Tooltip, const _Slot &CompareSlot) const override;
		bool ApplyCost(_ActionResult &ActionResult, ActionResultFlag &ResultFlags) const override;
		bool CanEquip(_Scripting *Scripting, _Object *Object) const override;

		const _WeaponType *WeaponTypeRequired;

	private:

};

inline bool CompareSkills(const _BaseSkill *First, const _BaseSkill *Second) {
	return First->Name < Second->Name;
}