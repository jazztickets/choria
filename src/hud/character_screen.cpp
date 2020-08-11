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
	DrawPosition.x += (int)(Element->Size.x/2 + 10 * ae::_Element::GetUIScale());
	DrawPosition.y += (int)(SpacingY * 2.0f);
	std::stringstream Buffer;

	// Damage
	if(ae::Input.ModKeyDown(KMOD_ALT))
		Buffer << ae::Round((HUD->Player->Character->Attributes["MinDamage"].Integer + HUD->Player->Character->Attributes["MaxDamage"].Integer) * 0.5f);
	else
		Buffer << HUD->Player->Character->Attributes["MinDamage"].Integer << " - " << HUD->Player->Character->Attributes["MaxDamage"].Integer;
	Font->DrawText("Weapon Damage", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Display attributes
	int LastCategory = 1;
	bool Drawn = false;
	for(const auto &AttributeName : HUD->Player->Stats->AttributeRank) {
		const _Attribute &Attribute = HUD->Player->Stats->Attributes.at(AttributeName);
		if(!Attribute.Show)
			continue;

		_AttributeStorage &AttributeStorage = HUD->Player->Character->Attributes[AttributeName];
		if(Attribute.UpdateType == StatUpdateType::MULTIPLICATIVE && AttributeStorage.Integer == 0)
			continue;

		if(AttributeStorage.Integer == Attribute.Default.Integer)
			continue;

		Buffer << AttributeStorage.Integer;
		if(Attribute.Type == StatValueType::PERCENT)
			Buffer << "%";

		// Separator
		if(LastCategory != Attribute.Show && Drawn)
			DrawPosition.y += SpacingY;

		// Draw values
		Font->DrawText(Attribute.Label, DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;

		LastCategory = Attribute.Show;
		Drawn = true;
	}

	// Resistances
	bool First = true;
	for(auto &Resistance : HUD->Player->Character->Resistances) {
		if(Resistance.first == 0 || Resistance.second == 0)
			continue;

		// Separator
		if(First)
			DrawPosition.y += SpacingY;

		// Draw
		Buffer << Resistance.second << "%";
		Font->DrawText(HUD->Player->Stats->DamageTypes.at(Resistance.first).Name + " Resist", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;

		First = false;
	}

	// Separator
	DrawPosition.y += SpacingY;

	// Play time
	_HUD::FormatTime(Buffer, (int64_t)HUD->Player->Character->PlayTime);
	Font->DrawText("Play Time", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Rebirth time
	if(HUD->Player->Character->Attributes["Rebirths"].Integer > 0) {
		_HUD::FormatTime(Buffer, (int64_t)HUD->Player->Character->RebirthTime);
		Font->DrawText("Rebirth Time", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Battle time
	_HUD::FormatTime(Buffer, (int64_t)HUD->Player->Character->BattleTime);
	Font->DrawText("Battle Time", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;
}
