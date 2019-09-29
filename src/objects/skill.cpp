/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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
#include <objects/skill.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <ae/ui.h>
#include <ae/assets.h>
#include <ae/font.h>
#include <ae/graphics.h>
#include <stats.h>
#include <hud/hud.h>
#include <constants.h>
#include <glm/vec4.hpp>

// Constructor
_Skill::_Skill() :
	Experience(0) {
}

// Draw skill tooltip
void _Skill::DrawTooltip(const glm::vec2 &Position, const _Object *Player, const _Cursor &Tooltip, const _Slot &CompareSlot) const {
	if(!Player)
		return;

	float SpacingY = TOOLTIP_SPACING * ae::_Element::GetUIScale();

	// Draw tooltip window
	glm::vec2 DrawPosition;
	glm::vec2 Size(INVENTORY_TOOLTIP_WIDTH, INVENTORY_TOOLTIP_HEIGHT);
	float SidePadding = TOOLTIP_SIDE_PADDING * ae::_Element::GetUIScale();
	DrawTooltipBase(Position, Player, "Skill", DrawPosition, Size);

	// Get level skill
	int DrawLevel = Level;
	bool IsLocked = false;
	bool ShowLevel = false;

	// Get skill level
	auto SkillIterator = Player->Character->Skills.find(ID);
	if(SkillIterator != Player->Character->Skills.end())
		DrawLevel = SkillIterator->second;
	else
		IsLocked = true;

	// For skills set minimum of level 1
	DrawLevel = std::max(DrawLevel, 1);

	// Determine whether to show level
	if(Tooltip.Window == _HUD::WINDOW_SKILLS || Tooltip.Window == _HUD::WINDOW_ACTIONBAR)
		ShowLevel = true;

	// Draw description
	DrawDescription(Player->Scripting, DrawPosition, DrawLevel, ShowLevel, Size.x - SidePadding * 2, SpacingY);

	// Draw next level description
	if(Tooltip.Window == _HUD::WINDOW_SKILLS && DrawLevel < MaxLevel)
		DrawDescription(Player->Scripting, DrawPosition, DrawLevel+1, true, Size.x - SidePadding * 2, SpacingY);

	// Draw help text
	std::string InfoText;
	glm::vec4 InfoColor = ae::Assets.Colors["gray"];
	if(Tooltip.Window == _HUD::WINDOW_ACTIONBAR) {
		if(CheckScope(ScopeType::WORLD) && Target != TargetType::NONE)
			InfoText = "Left-click to use";
	}
	else if(Tooltip.Window == _HUD::WINDOW_SKILLS) {
		if(Scope == ScopeType::NONE)
			InfoText = "Passive skills must be equipped";
	}
	else if(!IsLocked) {
		InfoText = "Already learned";
		InfoColor = ae::Assets.Colors["red"];
	}

	if(InfoText.length())
		ae::Assets.Fonts["hud_small"]->DrawText(InfoText, DrawPosition, ae::CENTER_BASELINE, InfoColor);
}

// Apply cost and return non zero flags to continue with action result
bool _Skill::ApplyCost(_ActionResult &ActionResult, ActionResultFlag &ResultFlags) const {
	CallApplyCost(ActionResult.Source.Object->Scripting, ActionResult);

	return true;
}
