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

const float TOOLTIP_WIDTH = 350;
const float TOOLTIP_HEIGHT = 480;

// Constructor
_Skill::_Skill() :
	Experience(0) {
}

// Draw skill tooltip
void _Skill::DrawTooltip(const glm::vec2 &Position, const _Object *Player, const _Cursor &Tooltip, const _Slot &CompareSlot) const {
	if(!Player)
		return;

	ae::_Element *TooltipElement = ae::Assets.Elements["element_item_tooltip"];
	ae::_Element *TooltipName = ae::Assets.Elements["label_item_tooltip_name"];
	ae::_Element *TooltipType = ae::Assets.Elements["label_item_tooltip_type"];
	TooltipElement->SetActive(true);

	// Set label values
	TooltipName->Text = Name;
	TooltipType->Text = "Skill";

	// Set up window size
	glm::vec2 Size;
	Size.x = TOOLTIP_WIDTH * ae::_Element::GetUIScale();
	float SidePadding = 36 * ae::_Element::GetUIScale();
	float SpacingY = 36 * ae::_Element::GetUIScale();
	float LargeSpacingY = 56 * ae::_Element::GetUIScale();

	// Set window width
	ae::_TextBounds TextBounds;
	ae::Assets.Fonts["hud_medium"]->GetStringDimensions(TooltipName->Text, TextBounds);
	Size.x = std::max(Size.x, (float)TextBounds.Width / ae::_Element::GetUIScale()) + SidePadding * 2;
	Size.x += 42 * ae::_Element::GetUIScale();

	// Set window height
	Size.y = TOOLTIP_HEIGHT * ae::_Element::GetUIScale();

	// Position window
	glm::vec2 WindowOffset = Position;

	// Center vertically
	if(Position.y < 0) {
		WindowOffset.y = (ae::Graphics.CurrentSize.y - Size.y) / 2;
	}
	else {
		WindowOffset.x += INVENTORY_TOOLTIP_OFFSET * ae::_Element::GetUIScale();
		WindowOffset.y += -(TooltipElement->Bounds.End.y - TooltipElement->Bounds.Start.y) / 2;
	}

	// Reposition window if out of bounds
	if(WindowOffset.x + Size.x > ae::Graphics.Element->Bounds.End.x - INVENTORY_TOOLTIP_PADDING)
		WindowOffset.x -= Size.x + INVENTORY_TOOLTIP_OFFSET + INVENTORY_TOOLTIP_PADDING;
	if(WindowOffset.y + Size.y > ae::Graphics.Element->Bounds.End.y - INVENTORY_TOOLTIP_PADDING)
		WindowOffset.y -= Size.y + INVENTORY_TOOLTIP_OFFSET - (TooltipElement->Bounds.End.y - TooltipElement->Bounds.Start.y) / 2;

	TooltipElement->Offset = WindowOffset;
	TooltipElement->Size = Size;
	TooltipElement->CalculateBounds(false);

	// Render tooltip
	TooltipElement->Render();
	TooltipElement->SetActive(false);

	// Set draw position to center of window
	glm::vec2 DrawPosition((int)(TooltipElement->Size.x / 2 + WindowOffset.x), (int)TooltipType->Bounds.End.y);
	DrawPosition.y += LargeSpacingY;

	// Draw target text
	if(Target != TargetType::NONE) {
		DrawPosition.y -= 28 * ae::_Element::GetUIScale();
		std::string InfoText = "Target " + Player->Stats->TargetTypes.at(Target).second;
		ae::Assets.Fonts["hud_small"]->DrawText(InfoText, DrawPosition, ae::CENTER_BASELINE, glm::vec4(1.0f));
		DrawPosition.y += LargeSpacingY;
	}

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
	else {
		if(!IsLocked) {
			InfoText = "Already learned";
			InfoColor = ae::Assets.Colors["red"];
		}
	}

	if(InfoText.length()) {
		ae::Assets.Fonts["hud_small"]->DrawText(InfoText, DrawPosition, ae::CENTER_BASELINE, InfoColor);
	}
}
