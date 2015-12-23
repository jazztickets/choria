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
#include <objects/item.h>
#include <objects/object.h>
#include <ui/label.h>
#include <ui/element.h>
#include <scripting.h>
#include <stats.h>
#include <constants.h>
#include <font.h>
#include <stats.h>
#include <hud.h>
#include <scripting.h>
#include <graphics.h>
#include <input.h>
#include <assets.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>

// Draw tooltip
void _Item::DrawTooltip(_Scripting *Scripting, const _Object *Player, const _Cursor &Tooltip, bool DrawNextLevel) const {
	_Element *TooltipElement = Assets.Elements["element_item_tooltip"];
	_Label *TooltipName = Assets.Labels["label_item_tooltip_name"];
	_Label *TooltipType = Assets.Labels["label_item_tooltip_type"];
	TooltipElement->SetVisible(true);

	// Set label values
	TooltipName->Text = Name;
	TooltipType->Text = Player->Stats->ItemTypes[(uint32_t)Type];

	// Set window width
	_TextBounds TextBounds;
	Assets.Fonts["hud_medium"]->GetStringDimensions(TooltipName->Text, TextBounds);
	int Width = 250;
	int SidePadding = 20;
	Width = std::max(Width, TextBounds.Width) + SidePadding * 2;

	// Position window
	glm::vec2 WindowOffset = Input.GetMouse();
	WindowOffset.x += INVENTORY_TOOLTIP_OFFSET;
	WindowOffset.y += -(TooltipElement->Bounds.End.y - TooltipElement->Bounds.Start.y) / 2;

	// Reposition window if out of bounds
	if(WindowOffset.x + Width > Graphics.Element->Bounds.End.x - INVENTORY_TOOLTIP_PADDING)
		WindowOffset.x -= Width + INVENTORY_TOOLTIP_OFFSET + INVENTORY_TOOLTIP_PADDING;

	TooltipElement->SetOffset(WindowOffset);
	TooltipElement->SetWidth(Width);

	// Render tooltip
	TooltipElement->Render();
	TooltipElement->SetVisible(false);

	// Set draw position to center of window
	glm::vec2 DrawPosition(TooltipElement->Size.x / 2 + WindowOffset.x, TooltipType->Bounds.End.y);
	DrawPosition.y += 40;

	glm::vec2 Spacing(10, 0);
	int SpacingY = 25;

	// Render damage
	int Min, Max;
	GetDamageRange(Min, Max);
	if(Min != 0 || Max != 0) {
		std::stringstream Buffer;
		if(Min != Max)
			Buffer << Min << " - " << Max;
		else
			Buffer << Min;

		Assets.Fonts["hud_medium"]->DrawText("Damage", DrawPosition + -Spacing, glm::vec4(1.0f), RIGHT_BASELINE);
		Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, glm::vec4(1.0f), LEFT_BASELINE);
		DrawPosition.y += SpacingY;
	}

	// Render defense
	GetDefenseRange(Min, Max);
	if(Min != 0 || Max != 0) {
		std::stringstream Buffer;
		if(Min != Max)
			Buffer << Min << " - " << Max;
		else
			Buffer << Min;

		Assets.Fonts["hud_medium"]->DrawText("Defense", DrawPosition + -Spacing, glm::vec4(1.0f), RIGHT_BASELINE);
		Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, glm::vec4(1.0f), LEFT_BASELINE);
		DrawPosition.y += SpacingY;
	}

	switch(Type) {
		case ItemType::ONEHANDED_WEAPON:
		case ItemType::TWOHANDED_WEAPON:
		case ItemType::HELMET:
		case ItemType::ARMOR:
		case ItemType::BOOTS:
		case ItemType::SHIELD:
		break;
		case ItemType::CONSUMABLE:
			if(Tooltip.Window == _HUD::WINDOW_INVENTORY) {
				DrawPosition.y -= 20;
				Assets.Fonts["hud_small"]->DrawText("Right-click to use", DrawPosition, COLOR_GRAY, CENTER_BASELINE);
				DrawPosition.y += 40;
			}
		break;
		default:
		break;
	}

	// Get item description
	std::string Info = "";
	if(Scripting->StartMethodCall(Script, "GetInfo")) {
		Scripting->PushInt(Level);
		Scripting->MethodCall(1, 1);
		Info = Scripting->GetString(1);
		Scripting->FinishMethodCall();

		std::stringstream Buffer(Info);
		std::string Token;

		// Draw description
		float TextSpacingY = 18;
		while(std::getline(Buffer, Token, '\n')) {
			std::list<std::string> Strings;
			Assets.Fonts["hud_small"]->BreakupString(Token, Width - SidePadding * 2, Strings, true);
			for(const auto &LineToken : Strings) {
				Assets.Fonts["hud_small"]->DrawTextFormatted(LineToken, DrawPosition, CENTER_BASELINE);
				DrawPosition.y += TextSpacingY;
			}
		}

		DrawPosition.y += SpacingY;
	}

/*	// Get current skill level
	int32_t SkillLevel = 0;
	auto SkillLevelIterator = Player->Skills.find(ID);
	if(SkillLevelIterator != Player->Skills.end())
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
*/

	// Boosts
	if(MaxHealth > 0) {
		std::stringstream Buffer;
		Buffer << "+" << MaxHealth;
		Assets.Fonts["hud_medium"]->DrawText("HP", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
		Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
		DrawPosition.y += SpacingY;
	}
	if(MaxMana > 0) {
		std::stringstream Buffer;
		Buffer << "+" << MaxMana;
		Assets.Fonts["hud_medium"]->DrawText("MP", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
		Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
		DrawPosition.y += SpacingY;
	}
	if(HealthRegen > 0) {
		std::stringstream Buffer;
		Buffer << "+" << std::setprecision(2) << HealthRegen << "%";
		Assets.Fonts["hud_medium"]->DrawText("HP Regen", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
		Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
		DrawPosition.y += SpacingY;
	}
	if(ManaRegen > 0) {
		std::stringstream Buffer;
		Buffer << "+" << std::setprecision(2) << ManaRegen << "%";
		Assets.Fonts["hud_medium"]->DrawText("MP Regen", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
		Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
		DrawPosition.y += SpacingY;
	}

	// Vendors
	if(Player->Vendor) {
		DrawPosition.y += SpacingY;
		if(Tooltip.Window == _HUD::WINDOW_VENDOR) {
			std::stringstream Buffer;
			Buffer << "Buy " << Tooltip.Count << "x for " << Tooltip.Cost << " gold";
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition, COLOR_GOLD, CENTER_BASELINE);
			DrawPosition.y += SpacingY;
			Assets.Fonts["hud_small"]->DrawText("Right-click to buy", DrawPosition, COLOR_GRAY, CENTER_BASELINE);
			DrawPosition.y += SpacingY;
		}
		else if(Tooltip.Window == _HUD::WINDOW_INVENTORY) {
			std::stringstream Buffer;
			Buffer << "Sell for " << Tooltip.Cost << " gold";
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition, COLOR_GOLD, CENTER_BASELINE);
			DrawPosition.y += SpacingY;
			Assets.Fonts["hud_small"]->DrawText("Shift+Right-click to sell", DrawPosition, COLOR_GRAY, CENTER_BASELINE);
			DrawPosition.y += SpacingY;
		}
	}

	if(Tooltip.Window == _HUD::WINDOW_INVENTORY && Tooltip.Count > 1) {
		Assets.Fonts["hud_small"]->DrawText("Ctrl+click to split", DrawPosition, COLOR_GRAY, CENTER_BASELINE);
		DrawPosition.y += SpacingY;
	}
}

// Draw skill description
void _Item::DrawDescription(_Scripting *Scripting, int SkillLevel, glm::vec2 &DrawPosition, float Width) const {
	/*if(!Script.length())
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
	}*/
}

// Returns the range of damage
void _Item::GetDamageRange(int &Min, int &Max) const {

	Min = (int)(Damage - DamageRange);
	Max = (int)(Damage + DamageRange);
}

// Returns the range of defense
void _Item::GetDefenseRange(int &Min, int &Max) const {

	Min = (int)(Defense - DefenseRange);
	Max = (int)(Defense + DefenseRange);
}

// Returns the item's price to/from a vendor
int _Item::GetPrice(const _Vendor *Vendor, int QueryCount, bool Buy) const {
	if(!Vendor)
		return 0;

	// Calculate
	float Percent;
	if(Buy)
		Percent = Vendor->BuyPercent;
	else
		Percent = Vendor->SellPercent;

	int Price = (int)(Cost * Percent) * QueryCount;

	// Cap
	if(Price < 0)
		Price = 0;
	else if(Price > STATS_MAXGOLD)
		Price = STATS_MAXGOLD;

	return Price;
}

// Return true if the item can be used
bool _Item::CanUse(_Scripting *Scripting, _ActionResult &ActionResult) const {
	_Object *Object = ActionResult.Source.Object;
	if(!Object)
		return false;

	// Unlocking skill for the first time
	if(IsSkill() && ActionResult.ActionUsed.FromInventory) {
		return !Object->HasLearned(this);
	}

	// Check scope
	if(Scope == ScopeType::NONE || (Scope != ScopeType::ALL && Scope != ActionResult.Scope))
		return false;

	// Check script's function
	if(Scripting->StartMethodCall(Script, "CanUse")) {
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->MethodCall(2, 1);
		int Value = Scripting->GetInt(1);
		Scripting->FinishMethodCall();

		return Value;
	}

	return true;
}

// Apply the cost
void _Item::ApplyCost(_Scripting *Scripting, _ActionResult &ActionResult) const {
	if(Scripting->StartMethodCall(Script, "ApplyCost")) {
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushActionResult(&ActionResult);
		Scripting->MethodCall(2, 1);
		Scripting->GetActionResult(1, ActionResult);
		Scripting->FinishMethodCall();
	}
}

// Use an item
void _Item::Use(_Scripting *Scripting, _ActionResult &ActionResult) const {
	if(Scripting->StartMethodCall(ActionResult.ActionUsed.Item->Script, "Use")) {
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->PushObject(ActionResult.Target.Object);
		Scripting->PushActionResult(&ActionResult);
		Scripting->MethodCall(4, 1);
		Scripting->GetActionResult(1, ActionResult);
		Scripting->FinishMethodCall();
	}
}
