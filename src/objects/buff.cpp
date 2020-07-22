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
void _Buff::DrawTooltip(_Scripting *Scripting, int Level, double Duration, int DismissLevel) const {
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
	Size.y = 180 * ae::_Element::GetUIScale();
	TooltipDuration->BaseOffset.y = 165;
	if(DismissLevel) {
		Size.y += 25 * ae::_Element::GetUIScale();
		TooltipDuration->BaseOffset.y = 185;
	}

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

	TooltipElement->Offset = WindowOffset;
	TooltipElement->Size = Size;
	TooltipElement->CalculateBounds(false);

	// Render tooltip
	TooltipElement->Render();
	TooltipElement->SetActive(false);

	// Get info and line count
	std::string Token;
	std::string Info = "";
	int Lines = 0;
	if(Script.length()) {

		// Get description
		if(Scripting->StartMethodCall(Script, "GetInfo")) {
			Scripting->PushInt(Level);
			Scripting->MethodCall(1, 1);
			Info = Scripting->GetString(1);
			Scripting->FinishMethodCall();
		}

		// Get line count
		Buffer << Info;
		while(std::getline(Buffer, Token, '\n')) {
			std::list<std::string> Strings;
			ae::Assets.Fonts["hud_small"]->BreakupString(Token, Size.x, Strings, true);
			Lines += Strings.size();
		}
		Buffer.clear();
		Buffer.str("");
	}

	// Get offset of description
	int OffsetY = 0;
	switch(Lines) {
		case 1:
			OffsetY = 50;
		break;
		case 2:
			OffsetY = 42;
		break;
		case 3:
			OffsetY = 32;
		break;
	}

	glm::vec2 DrawPosition(TooltipElement->Size.x / 2 + WindowOffset.x, (int)(TooltipName->Bounds.End.y + OffsetY * ae::_Element::GetUIScale()));

	// Draw description
	float SpacingY = 26 * ae::_Element::GetUIScale();
	Buffer << Info;
	while(std::getline(Buffer, Token, '\n')) {
		std::list<std::string> Strings;
		ae::Assets.Fonts["hud_small"]->BreakupString(Token, Size.x, Strings, true);
		for(const auto &LineToken : Strings) {
			ae::Assets.Fonts["hud_small"]->DrawTextFormatted(LineToken, DrawPosition, ae::CENTER_BASELINE);
			DrawPosition.y += SpacingY;
		}
	}
	Buffer.str("");

	if(DismissLevel) {
		DrawPosition.y += 5 * ae::_Element::GetUIScale();

		std::string Text = "Right-click to dismiss";
		if(DismissLevel == 2)
			Text += " one summon";

		ae::Assets.Fonts["hud_small"]->DrawText(Text, DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["gray"]);
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
