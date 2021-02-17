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
#include <hud/character_screen.h>
#include <hud/hud.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <ae/ui.h>
#include <ae/assets.h>
#include <ae/font.h>
#include <ae/input.h>
#include <ae/util.h>
#include <stats.h>
#include <glm/vec2.hpp>
#include <sstream>
#include <SDL_keycode.h>

// Constructor
_CharacterScreen::_CharacterScreen(_HUD *HUD, ae::_Element *Element) :
	_Screen(HUD, Element) {
}

// Render
void _CharacterScreen::Render(double BlendFactor) {
	if(!Element->Active)
		return;

	// Render background
	Element->Render();

	// Get font
	ae::_Font *Font = ae::Assets.Fonts["hud_char"];

	// Set up UI
	int SpacingY = Font->MaxAbove + Font->MaxBelow;
	glm::vec2 Spacing((int)(SpacingY * 0.5f), 0);
	glm::vec2 DrawPosition = Element->Bounds.Start;
	DrawPosition.x += (int)(Element->Size.x/2 + 25 * ae::_Element::GetUIScale());
	DrawPosition.y += (int)(SpacingY * 2.0f);
	std::stringstream Buffer;

	// Damage
	if(ae::Input.ModKeyDown(KMOD_ALT))
		Buffer << ae::Round((HUD->Player->Character->Attributes["MinDamage"].Int + HUD->Player->Character->Attributes["MaxDamage"].Int) * 0.5f);
	else
		Buffer << HUD->Player->Character->Attributes["MinDamage"].Int << " - " << HUD->Player->Character->Attributes["MaxDamage"].Int;
	Font->DrawText("Weapon Damage", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Display attributes
	int LastCategory = 1;
	for(const auto &AttributeName : HUD->Player->Stats->AttributeRank) {
		const _Attribute &Attribute = HUD->Player->Stats->Attributes.at(AttributeName);
		if(!Attribute.Show)
			continue;

		// Get category
		int Category = std::abs(Attribute.Show);
		bool AlwaysShow = Attribute.Show < 0;

		_Value &AttributeStorage = HUD->Player->Character->Attributes[AttributeName];
		if(Attribute.UpdateType == StatUpdateType::MULTIPLICATIVE && AttributeStorage.Int == 0)
			continue;

		if(!AlwaysShow && AttributeStorage.Int == Attribute.Default.Int)
			continue;

		// Separator
		if(LastCategory != Category)
			DrawPosition.y += SpacingY;

		// Build buffer
		switch(Attribute.Type) {
			case StatValueType::INTEGER:
				Buffer << AttributeStorage.Int;
			break;
			case StatValueType::INTEGER64:
				Buffer << AttributeStorage.Int64;
			break;
			case StatValueType::PERCENT:
				Buffer << AttributeStorage.Int << "%";
			break;
			case StatValueType::TIME:
				_HUD::FormatTime(Buffer, (int64_t)AttributeStorage.Double);
			break;
			default:
			break;
		}

		// Draw values
		Font->DrawText(Attribute.Label, DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;

		LastCategory = Category;
	}
}
