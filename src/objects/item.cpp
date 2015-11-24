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
#include <stats.h>
#include <constants.h>
#include <font.h>
#include <hud.h>
#include <objects/player.h>
#include <ui/label.h>
#include <ui/element.h>
#include <graphics.h>
#include <input.h>
#include <assets.h>
#include <sstream>
#include <iomanip>

// Draw tooltip
void _Item::DrawTooltip(const _Player *Player, const _Cursor &Tooltip) const {
	_Element *TooltipElement = Assets.Elements["element_item_tooltip"];
	_Label *TooltipName = Assets.Labels["label_item_tooltip_name"];
	_Label *TooltipType = Assets.Labels["label_item_tooltip_type"];
	TooltipElement->SetVisible(true);

	// Set label values
	TooltipName->Text = Name;
	GetType(TooltipType->Text);

	// Set window width
	_TextBounds TextBounds;
	Assets.Fonts["hud_medium"]->GetStringDimensions(TooltipName->Text, TextBounds);
	int Width = 250;
	Width = std::max(Width, TextBounds.Width) + 20;

	// Position window
	glm::ivec2 WindowOffset = Input.GetMouse();
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
	glm::ivec2 DrawPosition(TooltipElement->Size.x / 2 + WindowOffset.x, TooltipType->Bounds.End.y);
	DrawPosition.y += 40;

	glm::ivec2 Spacing(10, 0);
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
		case _Item::TYPE_WEAPON1HAND:
		case _Item::TYPE_WEAPON2HAND:
		case _Item::TYPE_HEAD:
		case _Item::TYPE_BODY:
		case _Item::TYPE_LEGS:
		case _Item::TYPE_SHIELD:
		break;
		case _Item::TYPE_POTION:
			if(Tooltip.Window == _HUD::WINDOW_INVENTORY) {
				DrawPosition.y -= 20;
				Assets.Fonts["hud_small"]->DrawText("Right-click to use", DrawPosition, COLOR_GRAY, CENTER_BASELINE);
				DrawPosition.y += 40;
			}
			if(HealthRestore > 0) {
				std::stringstream Buffer;
				Buffer << "+" << HealthRestore << " HP";
				Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition, COLOR_GREEN, CENTER_BASELINE);
				DrawPosition.y += SpacingY;
			}
			if(ManaRestore > 0) {
				std::stringstream Buffer;
				Buffer << "+" << ManaRestore << " MP";
				Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition, COLOR_BLUE, CENTER_BASELINE);
				DrawPosition.y += SpacingY;
			}
			if(InvisPower > 0) {
				std::stringstream Buffer;
				Buffer << "+" << InvisPower << " Invisibility Time";
				Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition, COLOR_TWHITE, CENTER_BASELINE);
				DrawPosition.y += SpacingY;
			}
		break;
	}

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
			Buffer << "Buy for " << Tooltip.Cost << " gold";
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

// Returns a string of the item type
void _Item::GetType(std::string &String) const {

	switch(Type) {
		case TYPE_HEAD:
			String = "Helmet";
		break;
		case TYPE_BODY:
			String = "Armor";
		break;
		case TYPE_LEGS:
			String = "Boots";
		break;
		case TYPE_WEAPON1HAND:
			String = "Weapon";
		break;
		case TYPE_WEAPON2HAND:
			String = "Weapon";
		break;
		case TYPE_SHIELD:
			String = "Shield";
		break;
		case TYPE_RING:
			String = "Ring";
		break;
		case TYPE_POTION:
			String = "Potion";
		break;
		case TYPE_TRADE:
			String = "Tradable";
		break;
		default:
		break;
	}
}

// Returns the item's price to/from a vendor
int _Item::GetPrice(const _Vendor *Vendor, int QueryCount, bool Buy) const {
	if(!Vendor)
		return 0;

	// Calculate
	int Price;
	if(Buy)
		Price = (int)(Cost * Vendor->BuyPercent) * QueryCount;
	else
		Price = (int)(Cost * Vendor->SellPercent) * QueryCount;

	// Cap
	if(Price < 0)
		Price = 0;
	else if(Price > STATS_MAXGOLD)
		Price = STATS_MAXGOLD;

	return Price;
}
