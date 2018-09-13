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
#include <objects/object.h>
#include <objects/statchange.h>
#include <ae/ui.h>
#include <ae/font.h>
#include <ae/graphics.h>
#include <ae/input.h>
#include <ae/assets.h>
#include <ae/util.h>
#include <constants.h>
#include <scripting.h>
#include <sstream>
#include <iostream>
#include <iomanip>

// Draw tooltip
void _Buff::DrawTooltip(_Scripting *Scripting, int Level, double Duration) const {
	std::stringstream Buffer;

	ae::_Element *TooltipElement = ae::Assets.Elements["element_buffs_tooltip"];
	ae::_Element *TooltipName = ae::Assets.Elements["label_buffs_tooltip_name"];
	ae::_Element *TooltipDuration = ae::Assets.Elements["label_buffs_tooltip_duration"];
	TooltipElement->SetActive(true);

	// Set label values
	TooltipName->Text = Name;
	Buffer << std::fixed << std::setprecision(1) << ae::Round((float)Duration) << "s";
	TooltipDuration->Text = Buffer.str();
	Buffer.str("");

	// Get window width
	glm::vec2 Size = TooltipElement->Size;

	// Position window
	glm::vec2 WindowOffset = ae::Input.GetMouse();
	WindowOffset.x += INVENTORY_TOOLTIP_OFFSET;
	WindowOffset.y += -Size.y / 2;

	// Reposition window if out of bounds
	if(WindowOffset.y < ae::Graphics.Element->Bounds.Start.x + INVENTORY_TOOLTIP_PADDING)
		WindowOffset.y = ae::Graphics.Element->Bounds.Start.x + INVENTORY_TOOLTIP_PADDING;
	if(WindowOffset.x + Size.x > ae::Graphics.Element->Bounds.End.x - INVENTORY_TOOLTIP_PADDING)
		WindowOffset.x -= Size.x + INVENTORY_TOOLTIP_OFFSET + INVENTORY_TOOLTIP_PADDING;
	if(WindowOffset.y + Size.y > ae::Graphics.Element->Bounds.End.y - INVENTORY_TOOLTIP_PADDING)
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

	Buffer << Info;
	std::string Token;

	// Draw description
	while(std::getline(Buffer, Token, '\n')) {
		std::list<std::string> Strings;
		ae::Assets.Fonts["hud_small"]->BreakupString(Token, Size.x, Strings, true);
		for(const auto &LineToken : Strings) {
			ae::Assets.Fonts["hud_small"]->DrawTextFormatted(LineToken, DrawPosition, ae::CENTER_BASELINE);
			DrawPosition.y += SpacingY;
		}
	}
	Buffer.str("");
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
