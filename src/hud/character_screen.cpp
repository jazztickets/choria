/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020  Alan Witkowski
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
#include <stats.h>
#include <glm/vec2.hpp>
#include <sstream>

// Constructor
_CharacterScreen::_CharacterScreen(_HUD *MainHUD, ae::_Element *MainElement) :
	_Screen(MainHUD, MainElement) {
}

// Render
void _CharacterScreen::Render(double BlendFactor) {
	if(!Element->Active)
		return;

	// Render background
	Element->Render();

	// Get font
	ae::_Font *Font = ae::Assets.Fonts["hud_small"];

	// Set up UI
	int SpacingY = Font->MaxAbove + Font->MaxBelow + 1;
	glm::vec2 Spacing((int)(SpacingY * 0.5f), 0);
	glm::vec2 DrawPosition = glm::ivec2(Element->Bounds.Start);
	DrawPosition.x += (int)(Element->Size.x/2 + SpacingY * 0.8f);
	DrawPosition.y += SpacingY * 2;
	std::stringstream Buffer;

	// Damage
	Buffer << HUD->Player->Character->MinDamage << " - " << HUD->Player->Character->MaxDamage;
	Font->DrawText("Damage", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Armor
	Buffer << HUD->Player->Character->Armor;
	Font->DrawText("Armor", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Damage Block
	Buffer << HUD->Player->Character->DamageBlock;
	Font->DrawText("Damage Block", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Pierce
	if(HUD->Player->Character->Pierce != 0) {
		Buffer << HUD->Player->Character->Pierce;
		Font->DrawText("Pierce", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Health Regen
	if(HUD->Player->Character->HealthRegen != 0) {
		Buffer << HUD->Player->Character->HealthRegen;
		Font->DrawText("Health Regen", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Mana Regen
	if(HUD->Player->Character->ManaRegen != 0) {
		Buffer << HUD->Player->Character->ManaRegen;
		Font->DrawText("Mana Regen", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Move speed
	Buffer << HUD->Player->Character->MoveSpeed << "%";
	Font->DrawText("Move Speed", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Hit chance
	Buffer << HUD->Player->Character->HitChance << "%";
	Font->DrawText("Hit Chance", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Evasion
	Buffer << HUD->Player->Character->Evasion << "%";
	Font->DrawText("Evasion", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Drop rate
	if(HUD->Player->Character->DropRate != 0) {
		Buffer << HUD->Player->Character->DropRate;
		Font->DrawText("Drop Rate", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Separator
	DrawPosition.y += SpacingY;

	// Resistances
	bool HasResist = false;
	for(auto &Resistance : HUD->Player->Character->Resistances) {
		if(Resistance.first == 0)
			continue;

		Buffer << Resistance.second << "%";
		Font->DrawText(HUD->Player->Stats->DamageTypes.at((DamageType)Resistance.first).second + " Resist", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;

		HasResist = true;
	}

	// Separator
	if(HasResist)
		DrawPosition.y += SpacingY;

	// Play time
	int64_t PlayTime = (int64_t)HUD->Player->Character->PlayTime;
	if(PlayTime < 60)
		Buffer << PlayTime << "s";
	else if(PlayTime < 3600)
		Buffer << PlayTime / 60 << "m";
	else
		Buffer << PlayTime / 3600 << "h" << (PlayTime / 60 % 60) << "m";

	Font->DrawText("Play Time", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Battle time
	int64_t BattleTime = (int64_t)HUD->Player->Character->BattleTime;
	if(BattleTime < 60)
		Buffer << BattleTime << "s";
	else if(BattleTime < 3600)
		Buffer << BattleTime / 60 << "m";
	else
		Buffer << BattleTime / 3600 << "h" << (BattleTime / 60 % 60) << "m";

	Font->DrawText("Battle Time", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Monster kills
	if(HUD->Player->Character->MonsterKills > 0) {
		Buffer << HUD->Player->Character->MonsterKills;
		Font->DrawText("Monster Kills", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Player kills
	if(HUD->Player->Character->PlayerKills > 0) {
		Buffer << HUD->Player->Character->PlayerKills;
		Font->DrawText("Player Kills", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Deaths
	if(HUD->Player->Character->Deaths > 0) {
		Buffer << HUD->Player->Character->Deaths;
		Font->DrawText("Deaths", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Bounty
	if(HUD->Player->Character->Bounty > 0) {
		Buffer << HUD->Player->Character->Bounty;
		Font->DrawText("Bounty", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Gold lost
	if(HUD->Player->Character->GoldLost > 0) {
		Buffer << HUD->Player->Character->GoldLost;
		Font->DrawText("Gold Lost", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Games played
	if(HUD->Player->Character->GamesPlayed > 0) {
		Buffer << HUD->Player->Character->GamesPlayed;
		Font->DrawText("Games Played", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}
}
