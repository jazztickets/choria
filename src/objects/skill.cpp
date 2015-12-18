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
#include <objects/skill.h>
#include <objects/action.h>
#include <ui/element.h>
#include <ui/label.h>
#include <objects/object.h>
#include <constants.h>
#include <scripting.h>
#include <font.h>
#include <graphics.h>
#include <input.h>
#include <assets.h>
#include <algorithm>
#include <sstream>
#include <iostream>

// Draw tooltip
void _Skill::DrawTooltip(_Scripting *Scripting, const _Object *Player, bool DrawNextLevel) const {
	_Element *TooltipElement = Assets.Elements["element_skills_tooltip"];
	_Label *TooltipName = Assets.Labels["label_skills_tooltip_name"];
	TooltipElement->SetVisible(true);

	// Set label values
	TooltipName->Text = Name;

	// Get window width
	glm::vec2 Size = TooltipElement->Size;

	// Position window
	glm::vec2 WindowOffset = Input.GetMouse();
	WindowOffset.x += INVENTORY_TOOLTIP_OFFSET;
	WindowOffset.y += -Size.y / 2;

	// Reposition window if out of bounds
	if(WindowOffset.y < Graphics.Element->Bounds.Start.x + INVENTORY_TOOLTIP_PADDING)
		WindowOffset.y = Graphics.Element->Bounds.Start.x + INVENTORY_TOOLTIP_PADDING;
	if(WindowOffset.x + Size.x > Graphics.Element->Bounds.End.x - INVENTORY_TOOLTIP_PADDING)
		WindowOffset.x -= Size.x + INVENTORY_TOOLTIP_OFFSET + INVENTORY_TOOLTIP_PADDING;
	if(WindowOffset.y + Size.y > Graphics.Element->Bounds.End.y - INVENTORY_TOOLTIP_PADDING)
		WindowOffset.y -= Size.y + INVENTORY_TOOLTIP_OFFSET - (TooltipElement->Bounds.End.y - TooltipElement->Bounds.Start.y) / 2;

	TooltipElement->SetOffset(WindowOffset);

	// Render tooltip
	TooltipElement->Render();
	TooltipElement->SetVisible(false);

	// Set draw position to center of window
	glm::vec2 DrawPosition(WindowOffset.x + 20, TooltipName->Bounds.End.y);
	DrawPosition.y += 30;

	// Get current skill level
	int32_t SkillLevel = 0;
	auto SkillLevelIterator = Player->SkillLevels.find(ID);
	if(SkillLevelIterator != Player->SkillLevels.end())
		SkillLevel = SkillLevelIterator->second;

	// Get current level description
	Assets.Fonts["hud_small"]->DrawText("Level " + std::to_string(std::max(1, SkillLevel)), DrawPosition, COLOR_WHITE, LEFT_BASELINE);
	DrawPosition.y += 25;
	DrawDescription(Scripting, SkillLevel, DrawPosition, Size.x);

	// Get next level description
	if(DrawNextLevel && SkillLevel > 0) {
		DrawPosition.y += 25;
		Assets.Fonts["hud_small"]->DrawText("Level " + std::to_string(SkillLevel+1), DrawPosition, COLOR_WHITE, LEFT_BASELINE);
		DrawPosition.y += 25;
		DrawDescription(Scripting, SkillLevel+1, DrawPosition, Size.x);
	}

	// Additional information
	/*switch(Type) {
		case _Skill::TYPE_PASSIVE:
			DrawPosition.y += 25;
			Assets.Fonts["hud_small"]->DrawText("Passive skills must be equipped to the actionbar", DrawPosition, COLOR_GRAY, LEFT_BASELINE);
		break;
	}*/
}

// Draw skill description
void _Skill::DrawDescription(_Scripting *Scripting, int SkillLevel, glm::vec2 &DrawPosition, float Width) const {
	if(!Script.length())
		return;

	// Show unskilled levels as level 1
	if(SkillLevel == 0)
		SkillLevel = 1;

	// Get skill description
	std::string Info = "";
	if(Scripting->StartMethodCall(Script, "GetInfo")) {
		Scripting->PushInt(SkillLevel);
		Scripting->MethodCall(1, 1);
		Info = Scripting->GetString(1);
		Scripting->FinishMethodCall();
	}

	float SpacingY = 18;

	std::stringstream Buffer(Info);
	std::string Token;

	// Draw description
	while(std::getline(Buffer, Token, '\n')) {
		std::list<std::string> Strings;
		Assets.Fonts["hud_small"]->BreakupString(Token, Width, Strings);
		for(const auto &LineToken : Strings) {
			Assets.Fonts["hud_small"]->DrawText(LineToken, DrawPosition, COLOR_GRAY, LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		}
	}
}

// Return true if the skill can be used
bool _Skill::CanUse(_Scripting *Scripting, _ActionResult &ActionResult) const {
	if(Scope == ScopeType::NONE || (Scope != ScopeType::ALL && Scope != ActionResult.Scope))
		return false;

	if(Scripting->StartMethodCall(Script, "CanUse")) {
		Scripting->PushInt(ActionResult.Source.Object->SkillLevels[ID]);
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->MethodCall(2, 1);
		int Value = Scripting->GetInt(1);
		Scripting->FinishMethodCall();

		return Value;
	}

	return true;
}

// Apply the cost
void _Skill::ApplyCost(_Scripting *Scripting, _ActionResult &ActionResult) const {
	if(Scripting->StartMethodCall(Script, "ApplyCost")) {
		Scripting->PushInt(ActionResult.Source.Object->SkillLevels[ID]);
		Scripting->PushActionResult(&ActionResult);
		Scripting->MethodCall(2, 1);
		Scripting->GetActionResult(1, ActionResult);
		Scripting->FinishMethodCall();
	}
}

// Use a skill
void _Skill::Use(_Scripting *Scripting, _ActionResult &ActionResult) const {
	if(Scripting->StartMethodCall(ActionResult.SkillUsed->Script, "Use")) {
		Scripting->PushInt(ActionResult.Source.Object->SkillLevels[ActionResult.SkillUsed->ID]);
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->PushObject(ActionResult.Target.Object);
		Scripting->PushActionResult(&ActionResult);
		Scripting->MethodCall(4, 1);
		Scripting->GetActionResult(1, ActionResult);
		Scripting->FinishMethodCall();
	}
}
