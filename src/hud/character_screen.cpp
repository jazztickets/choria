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
#include <stats.h>
#include <glm/vec2.hpp>
#include <sstream>

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

	// Experience
	if(HUD->Player->Character->ExperienceMultiplier != 1.0f) {
		Buffer << (int)(HUD->Player->Character->ExperienceMultiplier * 100) << "%";
		Font->DrawText("Experience Bonus", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Gold
	if(HUD->Player->Character->GoldMultiplier != 1.0f) {
		Buffer << (int)(HUD->Player->Character->GoldMultiplier * 100) << "%";
		Font->DrawText("Gold Bonus", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Max Health
	if(HUD->Player->Character->MaxHealthMultiplier != 1.0f) {
		Buffer << (int)(HUD->Player->Character->MaxHealthMultiplier * 100) << "%";
		Font->DrawText("Max Health", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Max Mana
	if(HUD->Player->Character->MaxManaMultiplier != 1.0f) {
		Buffer << (int)(HUD->Player->Character->MaxManaMultiplier * 100) << "%";
		Font->DrawText("Max Mana", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
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

	// Battle speed
	Buffer << HUD->Player->Character->BattleSpeed << "%";
	Font->DrawText("Battle Speed", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Hit chance
	if(HUD->Player->Character->HitChance != 100) {
		Buffer << HUD->Player->Character->HitChance << "%";
		Font->DrawText("Hit Chance", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Evasion
	if(HUD->Player->Character->Evasion != 0) {
		Buffer << HUD->Player->Character->Evasion << "%";
		Font->DrawText("Evasion", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Consume chance
	if(HUD->Player->Character->ConsumeChance != 100) {
		Buffer << HUD->Player->Character->ConsumeChance << "%";
		Font->DrawText("Consume Chance", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Cooldown reduction
	if(HUD->Player->Character->CooldownMultiplier != 1.0f) {
		Buffer << (int)(HUD->Player->Character->CooldownMultiplier * 100) << "%";
		Font->DrawText("Cooldowns", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Spell Damage
	if(HUD->Player->Character->SpellDamage != 100) {
		Buffer << (int)(HUD->Player->Character->SpellDamage) << "%";
		Font->DrawText("Spell Damage", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Attack Power
	if(HUD->Player->Character->AttackPower != 1.0f) {
		Buffer << (int)(HUD->Player->Character->AttackPower * 100) << "%";
		Font->DrawText("Attack Power", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Heal Power
	if(HUD->Player->Character->HealPower != 1.0f) {
		Buffer << (int)(HUD->Player->Character->HealPower * 100) << "%";
		Font->DrawText("Heal Power", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Mana Power
	if(HUD->Player->Character->ManaPower != 1.0f) {
		Buffer << (int)(HUD->Player->Character->ManaPower * 100) << "%";
		Font->DrawText("Mana Power", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Physical Power
	if(HUD->Player->Character->PhysicalPower != 1.0f) {
		Buffer << (int)(HUD->Player->Character->PhysicalPower * 100) << "%";
		Font->DrawText("Physical Power", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Fire Power
	if(HUD->Player->Character->FirePower != 1.0f) {
		Buffer << (int)(HUD->Player->Character->FirePower * 100) << "%";
		Font->DrawText("Fire Power", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Cold Power
	if(HUD->Player->Character->ColdPower != 1.0f) {
		Buffer << (int)(HUD->Player->Character->ColdPower * 100) << "%";
		Font->DrawText("Cold Power", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Lightning Power
	if(HUD->Player->Character->LightningPower != 1.0f) {
		Buffer << (int)(HUD->Player->Character->LightningPower * 100) << "%";
		Font->DrawText("Lightning Power", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Bleed Power
	if(HUD->Player->Character->BleedPower != 1.0f) {
		Buffer << (int)(HUD->Player->Character->BleedPower * 100) << "%";
		Font->DrawText("Bleed Power", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Poison Power
	if(HUD->Player->Character->PoisonPower != 1.0f) {
		Buffer << (int)(HUD->Player->Character->PoisonPower * 100) << "%";
		Font->DrawText("Poison Power", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Pet Power
	if(HUD->Player->Character->PetPower != 1.0f) {
		Buffer << (int)(HUD->Player->Character->PetPower * 100) << "%";
		Font->DrawText("Pet Power", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Difficulty
	if(HUD->Player->Character->Difficulty != 0) {
		Buffer << HUD->Player->Character->Difficulty << "%";
		Font->DrawText("Difficulty", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Resistances
	bool HasResist = false;
	for(auto &Resistance : HUD->Player->Character->Resistances) {
		if(Resistance.first == 0 || Resistance.second == 0)
			continue;

		Buffer << Resistance.second << "%";
		Font->DrawText(HUD->Player->Stats->DamageTypes.at(Resistance.first).Name + " Resist", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;

		HasResist = true;
	}

	// Rebirth Wealth
	if(HUD->Player->Character->RebirthWealth > 0) {
		Buffer << HUD->Player->Character->RebirthWealth << "%";
		Font->DrawText("Rebirth Wealth", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Rebirth Wisdom
	if(HUD->Player->Character->RebirthWisdom > 0) {
		Buffer << HUD->Player->Character->RebirthWisdom;
		Font->DrawText("Rebirth Wisdom", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Rebirth Knowledge
	if(HUD->Player->Character->RebirthKnowledge > 0) {
		Buffer << HUD->Player->Character->RebirthKnowledge;
		Font->DrawText("Rebirth Knowledge", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Rebirth Power
	if(HUD->Player->Character->RebirthPower > 0) {
		Buffer << HUD->Player->Character->RebirthPower;
		Font->DrawText("Rebirth Power", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Rebirth Girth
	if(HUD->Player->Character->RebirthGirth > 0) {
		Buffer << HUD->Player->Character->RebirthGirth;
		Font->DrawText("Rebirth Girth", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Rebirth Proficiency
	if(HUD->Player->Character->RebirthProficiency > 0) {
		Buffer << HUD->Player->Character->RebirthProficiency;
		Font->DrawText("Rebirth Proficiency", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Rebirth Insight
	if(HUD->Player->Character->RebirthInsight > 0) {
		Buffer << HUD->Player->Character->RebirthInsight;
		Font->DrawText("Rebirth Insight", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Rebirth Passage
	if(HUD->Player->Character->RebirthPassage > 0) {
		Buffer << HUD->Player->Character->RebirthPassage;
		Font->DrawText("Rebirth Passage", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}

	// Separator
	if(HasResist)
		DrawPosition.y += SpacingY;

	// Play time
	_HUD::FormatTime(Buffer, (int64_t)HUD->Player->Character->PlayTime);
	Font->DrawText("Play Time", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
	Font->DrawText(Buffer.str(), DrawPosition + Spacing);
	Buffer.str("");
	DrawPosition.y += SpacingY;

	// Rebirth time
	if(HUD->Player->Character->Rebirths > 0) {
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

	// Rebirths
	if(HUD->Player->Character->Rebirths > 0) {
		Buffer << HUD->Player->Character->Rebirths;
		Font->DrawText("Rebirths", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		Font->DrawText(Buffer.str(), DrawPosition + Spacing);
		Buffer.str("");
		DrawPosition.y += SpacingY;
	}
}
