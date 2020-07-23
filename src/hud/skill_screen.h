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
#include <hud/screen.h>
#include <cstdint>

// Forward Declarations
class _BaseSkill;

// Classes
class _SkillScreen : public _Screen {

	public:

		_SkillScreen(_HUD *MainHUD, ae::_Element *MainElement);

		void Init() override;
		bool Close(bool SendNotify=true) override;

		void Toggle() override;
		void Render(double BlendFactor) override;

		void ClearSkills();
		void EquipSkill(const _BaseSkill *Skill);

	private:

};
