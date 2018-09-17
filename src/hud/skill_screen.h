/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2018  Alan Witkowski
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

// Classes
class _SkillScreen : public _Screen {

	public:

		_SkillScreen(_HUD *HUD, ae::_Element *Element);

		void Init();
		bool Close();

		void Toggle();
		void Render(double BlendFactor);

		void ClearSkills();
		void RefreshSkillButtons();
		void AdjustSkillLevel(uint32_t SkillID, int Amount);
		void EquipSkill(uint32_t SkillID);

	private:

};
