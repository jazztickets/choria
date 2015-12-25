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
void _Item::DrawTooltip(_Scripting *Scripting, const _Object *Player, const _Cursor &Tooltip) const {
	if(!Player)
		return;

	_Element *TooltipElement = Assets.Elements["element_item_tooltip"];
	_Label *TooltipName = Assets.Labels["label_item_tooltip_name"];
	_Label *TooltipType = Assets.Labels["label_item_tooltip_type"];
	TooltipElement->SetVisible(true);

	// Get window dimensions
	glm::vec2 Size = TooltipElement->Size;

	// Set label values
	TooltipName->Text = Name;
	TooltipType->Text = Player->Stats->ItemTypes[(uint32_t)Type];

	// Set window width
	_TextBounds TextBounds;
	Assets.Fonts["hud_medium"]->GetStringDimensions(TooltipName->Text, TextBounds);
	Size.x = 250;
	float SidePadding = 15;
	float SpacingY = 25;
	Size.x = std::max(Size.x, (float)TextBounds.Width) + SidePadding * 2;

	// Position window
	glm::vec2 WindowOffset = Input.GetMouse();
	WindowOffset.x += INVENTORY_TOOLTIP_OFFSET;
	WindowOffset.y += -(TooltipElement->Bounds.End.y - TooltipElement->Bounds.Start.y) / 2;

	// Reposition window if out of bounds
	if(WindowOffset.x + Size.x > Graphics.Element->Bounds.End.x - INVENTORY_TOOLTIP_PADDING)
		WindowOffset.x -= Size.x + INVENTORY_TOOLTIP_OFFSET + INVENTORY_TOOLTIP_PADDING;
	if(WindowOffset.y + Size.y > Graphics.Element->Bounds.End.y - INVENTORY_TOOLTIP_PADDING)
		WindowOffset.y -= Size.y + INVENTORY_TOOLTIP_OFFSET - (TooltipElement->Bounds.End.y - TooltipElement->Bounds.Start.y) / 2;

	TooltipElement->SetOffset(WindowOffset);
	TooltipElement->SetWidth(Size.x);

	// Render tooltip
	TooltipElement->Render();
	TooltipElement->SetVisible(false);

	// Set draw position to center of window
	glm::vec2 DrawPosition(TooltipElement->Size.x / 2 + WindowOffset.x, TooltipType->Bounds.End.y);
	DrawPosition.y += 40;

	// Draw target text
	if(TargetID != TargetType::NONE) {
		DrawPosition.y -= 20;
		std::string InfoText = "Target " + Player->Stats->TargetTypes[(uint32_t)TargetID];
		Assets.Fonts["hud_small"]->DrawText(InfoText, DrawPosition, COLOR_WHITE, CENTER_BASELINE);
		DrawPosition.y += 40;
	}

	// Get level of item or skill
	int32_t DrawLevel = Level;
	bool IsLocked = false;

	// Get player skill level
	auto SkillIterator = Player->Skills.find(ID);
	if(SkillIterator != Player->Skills.end())
		DrawLevel = SkillIterator->second;
	else
		IsLocked = true;

	// For skills set minimum of level 1
	if(IsSkill())
		DrawLevel = std::max(DrawLevel, 1);

	// Determine whether to show level
	bool ShowLevel = false;
	if(IsSkill() && (Tooltip.Window == _HUD::WINDOW_SKILLS || Tooltip.Window == _HUD::WINDOW_ACTIONBAR))
		ShowLevel = true;

	// Draw description
	DrawDescription(Scripting, DrawPosition, DrawLevel, ShowLevel, Size.x - SidePadding * 2, SpacingY);

	// Draw next level description
	if(IsSkill() && Tooltip.Window == _HUD::WINDOW_SKILLS) {
		DrawDescription(Scripting, DrawPosition, DrawLevel+1, true, Size.x - SidePadding * 2, SpacingY);
	}

	glm::vec2 Spacing(10, 0);

	// Render damage
	bool StatDrawn = false;
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
		StatDrawn = true;
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
		StatDrawn = true;
	}

	// Boosts
	if(MaxHealth > 0) {
		std::stringstream Buffer;
		Buffer << "+" << MaxHealth;
		Assets.Fonts["hud_medium"]->DrawText("HP", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
		Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}
	if(MaxMana > 0) {
		std::stringstream Buffer;
		Buffer << "+" << MaxMana;
		Assets.Fonts["hud_medium"]->DrawText("MP", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
		Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}
	if(HealthRegen > 0) {
		std::stringstream Buffer;
		Buffer << "+" << std::setprecision(2) << HealthRegen << "%";
		Assets.Fonts["hud_medium"]->DrawText("HP Regen", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
		Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}
	if(ManaRegen > 0) {
		std::stringstream Buffer;
		Buffer << "+" << std::setprecision(2) << ManaRegen << "%";
		Assets.Fonts["hud_medium"]->DrawText("MP Regen", DrawPosition + -Spacing, COLOR_WHITE, RIGHT_BASELINE);
		Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + Spacing, COLOR_WHITE, LEFT_BASELINE);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	if(StatDrawn)
		DrawPosition.y += SpacingY;

	// Vendors
	if(Player->Vendor) {
		std::stringstream Buffer;
		if(Tooltip.Window == _HUD::WINDOW_VENDOR) {
			Buffer << "Buy " << Tooltip.Count << "x for " << Tooltip.Cost << " gold";
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition, COLOR_GOLD, CENTER_BASELINE);
			DrawPosition.y += SpacingY;
			Assets.Fonts["hud_small"]->DrawText("Right-click to buy", DrawPosition, COLOR_GRAY, CENTER_BASELINE);
			DrawPosition.y += SpacingY;
		}
		else if(Tooltip.Window == _HUD::WINDOW_INVENTORY) {
			Buffer << "Sell for " << Tooltip.Cost << " gold";
			Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition, COLOR_GOLD, CENTER_BASELINE);
			DrawPosition.y += SpacingY;
			Assets.Fonts["hud_small"]->DrawText("Shift+Right-click to sell", DrawPosition, COLOR_GRAY, CENTER_BASELINE);
			DrawPosition.y += SpacingY;
		}
	}

	// Draw help text
	std::string InfoText;
	switch(Type) {
		case ItemType::ONEHANDED_WEAPON:
		case ItemType::TWOHANDED_WEAPON:
		case ItemType::HELMET:
		case ItemType::ARMOR:
		case ItemType::BOOTS:
		case ItemType::SHIELD:
		break;
		case ItemType::CONSUMABLE:
			if(Tooltip.Window == _HUD::WINDOW_INVENTORY)
				InfoText = "Right-click to use";
			else if(Tooltip.Window == _HUD::WINDOW_ACTIONBAR)
				InfoText = "Left-click to use";
		break;
		case ItemType::SKILL:
			if(Tooltip.Window == _HUD::WINDOW_INVENTORY) {
				if(IsLocked)
					InfoText = "Right-click to learn";
				else
					InfoText = "Already learned";
			}
			else if(Tooltip.Window == _HUD::WINDOW_ACTIONBAR && CheckScope(ScopeType::WORLD) && TargetID != TargetType::NONE)
				InfoText = "Left-click to use";
		break;
		default:
		break;
	}

	if(InfoText.length()) {
		Assets.Fonts["hud_small"]->DrawText(InfoText, DrawPosition, COLOR_GRAY, CENTER_BASELINE);
		DrawPosition.y += 20;
	}

	if(Tooltip.Window == _HUD::WINDOW_INVENTORY && Tooltip.Count > 1) {
		Assets.Fonts["hud_small"]->DrawText("Ctrl+click to split", DrawPosition, COLOR_GRAY, CENTER_BASELINE);
		DrawPosition.y += SpacingY;
	}
}

// Draw item description
void _Item::DrawDescription(_Scripting *Scripting, glm::vec2 &DrawPosition, int DrawLevel, bool ShowLevel, float Width, float SpacingY) const {

	// Check for scripting function
	std::string Info = "";
	if(Scripting->StartMethodCall(Script, "GetInfo")) {

		// Draw level text
		if(ShowLevel) {
			Assets.Fonts["hud_small"]->DrawText("Level " + std::to_string(DrawLevel), DrawPosition, COLOR_GRAY, CENTER_BASELINE);
			DrawPosition.y += SpacingY;
		}

		// Get description from script
		Scripting->PushInt(DrawLevel);
		Scripting->MethodCall(1, 1);
		Info = Scripting->GetString(1);
		Scripting->FinishMethodCall();

		std::stringstream Buffer(Info);
		std::string Token;

		// Draw description
		float TextSpacingY = 18;
		while(std::getline(Buffer, Token, '\n')) {
			std::list<std::string> Strings;
			Assets.Fonts["hud_small"]->BreakupString(Token, Width, Strings, true);
			for(const auto &LineToken : Strings) {
				Assets.Fonts["hud_small"]->DrawTextFormatted(LineToken, DrawPosition, CENTER_BASELINE);
				DrawPosition.y += TextSpacingY;
			}
		}

		DrawPosition.y += SpacingY;
	}
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
	if(!CheckScope(ActionResult.Scope))
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

// Check if the item can be used in the given scope
bool _Item::CheckScope(ScopeType CheckScope) const {

	// Check scope
	if(Scope == ScopeType::NONE || (Scope != ScopeType::ALL && Scope != CheckScope))
		return false;

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
	if(Scripting->StartMethodCall(Script, "Use")) {
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->PushObject(ActionResult.Target.Object);
		Scripting->PushActionResult(&ActionResult);
		Scripting->MethodCall(4, 1);
		Scripting->GetActionResult(1, ActionResult);
		Scripting->FinishMethodCall();
	}
}

// Get passive stats
void _Item::Stats(_Scripting *Scripting, _ActionResult &ActionResult) const {
	if(Scripting->StartMethodCall(Script, "Stats")) {
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->PushStatChange(&ActionResult.Source);
		Scripting->MethodCall(3, 1);
		Scripting->GetStatChange(1, ActionResult.Source);
		Scripting->FinishMethodCall();
	}
}
