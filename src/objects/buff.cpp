/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#include <objects/buff.h>
#include <ui.h>
#include <objects/object.h>
#include <objects/statchange.h>
#include <constants.h>
#include <scripting.h>
#include <font.h>
#include <graphics.h>
#include <input.h>
#include <assets.h>
#include <sstream>
#include <iostream>

// Draw tooltip
void _Buff::DrawTooltip(_Scripting *Scripting, int Level) const {
	_Element *TooltipElement = Assets.Elements["element_buffs_tooltip"];
	_Element *TooltipName = Assets.Elements["label_buffs_tooltip_name"];
	TooltipElement->SetActive(true);

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
	TooltipElement->SetActive(false);

	// Set draw position to center of window
	glm::vec2 DrawPosition(TooltipElement->Size.x / 2 + WindowOffset.x, (int)(TooltipName->Bounds.End.y + 40));

	if(!Script.length())
		return;

	// Get description
	std::string Info = "";
	if(Scripting->StartMethodCall(Script, "GetInfo")) {
		Scripting->PushInt(Level);
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
		Assets.Fonts["hud_small"]->BreakupString(Token, Size.x, Strings, true);
		for(const auto &LineToken : Strings) {
			Assets.Fonts["hud_small"]->DrawTextFormatted(LineToken, DrawPosition, CENTER_BASELINE);
			DrawPosition.y += SpacingY;
		}
	}
}

// Call scripting function by name
void _Buff::ExecuteScript(_Scripting *Scripting, const std::string &Function, int Level, _StatChange &StatChange) const {
	if(Scripting->StartMethodCall(Script, Function)) {
		Scripting->PushInt(Level);
		Scripting->PushObject(StatChange.Object);
		Scripting->PushStatChange(&StatChange);
		Scripting->MethodCall(3, 1);
		Scripting->GetStatChange(1, StatChange);
		Scripting->FinishMethodCall();
	}
}
