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
#include <objects/fighter.h>
#include <graphics.h>
#include <random.h>
#include <stats.h>
#include <instances/battle.h>
#include <objects/skill.h>

// Constructor
_Fighter::_Fighter(int TType)
:	Type(TType),
	Name(""),
	Level(0),
	Health(0),
	MaxHealth(0),
	Mana(0),
	MaxMana(0),
	MinDamage(0),
	MaxDamage(0),
	MinDefense(0),
	MaxDefense(0),
	HealthRegen(0.0f),
	ManaRegen(0.0f),
	HealthAccumulator(0.0f),
	ManaAccumulator(0.0f),
	Battle(nullptr),
	Command(-1),
	Target(0),
	SkillUsing(nullptr),
	SkillUsed(nullptr),
	Portrait(nullptr) {

	for(int i = 0; i < FIGHTER_MAXSKILLS; i++)
		SkillBar[i] = nullptr;

	SkillBar[0] = Stats.GetSkill(0);

	Offset.x = Offset.y = 0;
}

// Destructor
_Fighter::~_Fighter() {

}

// Renders the fighter during a battle
void _Fighter::RenderBattle(bool TShowResults, float TTimerPercent, _FighterResult *TResult, bool TTarget) {

	/*
	char String[256];
	uint32_t AlphaPercent = (uint32_t)(255 * TTimerPercent);
	if(TTimerPercent > 0.75f)
		AlphaPercent = 255;
	else
		AlphaPercent = (uint32_t)(255 * TTimerPercent / 0.75f);

	// Sides
	_Graphics::ImageType SlotImage;
	if(GetSide() == 0) {
		SlotImage = _Graphics::IMAGE_BATTLESLOTLEFT;
	}
	else {
		SlotImage = _Graphics::IMAGE_BATTLESLOTRIGHT;
	}

	// Name
	Graphics.SetFont(_Graphics::FONT_10);
	//Graphics.RenderText(Name.c_str(), Offset.x + 32, Offset.y - 3, _Graphics::ALIGN_CENTER);

	// Portrait
	Graphics.DrawImage(SlotImage, Offset.x + 32, Offset.y + 50);
	if(Portrait)
		Graphics.DrawCenteredImage(Portrait, Offset.x + 32, Offset.y + 50);

	// Health
	int BarWidth = 80, BarHeight = 16, BarX = Offset.x + 70, BarY = Offset.y + 27;

	float HealthPercent = MaxHealth > 0 ? Health / (float)MaxHealth : 0;
	sprintf(String, "%d / %d", Health, MaxHealth);
	Graphics.SetFont(_Graphics::FONT_8);
	Graphics.DrawBar(_Graphics::IMAGE_HEALTH, BarX, BarY, HealthPercent, BarWidth, BarHeight);
	//Graphics.RenderText(String, BarX + BarWidth / 2, BarY + 1, _Graphics::ALIGN_CENTER);

	// Mana
	BarY += 30;
	float ManaPercent = MaxMana > 0 ? Mana / (float)MaxMana : 0;
	sprintf(String, "%d / %d", Mana, MaxMana);
	Graphics.DrawBar(_Graphics::IMAGE_MANA, BarX, BarY, ManaPercent, BarWidth, BarHeight);
	//Graphics.RenderText(String, BarX + BarWidth / 2, BarY + 1, _Graphics::ALIGN_CENTER);

	// Show results of last turn
	if(TShowResults) {

		char Sign = ' ';
		video::SColor Color(AlphaPercent, 150, 150, 150);
		if(TResult->HealthChange > 0) {
			Sign = '+';
			Color.set(AlphaPercent, 0, 255, 0);
		}
		else if(TResult->HealthChange < 0) {
			Color.set(AlphaPercent, 255, 0, 0);
		}

		sprintf(String, "%c%d", Sign, TResult->HealthChange);
		Graphics.SetFont(_Graphics::FONT_14);
		//Graphics.RenderText(String, BarX + BarWidth / 2, Offset.y + 2, _Graphics::ALIGN_CENTER, Color);
		Graphics.SetFont(_Graphics::FONT_10);

		// Draw the skill used
		if(SkillUsed) {
			Graphics.DrawCenteredImage(SkillUsed->GetImage(), Offset.x + 180, Offset.y + 50, video::SColor(AlphaPercent, 255, 255, 255));
		}

		// Draw damage dealt
		if(TResult->DamageDealt) {
			sprintf(String, "%d", TResult->DamageDealt);
			Graphics.SetFont(_Graphics::FONT_14);
			//Graphics.RenderText(String, Offset.x + 178, Offset.y + 38, _Graphics::ALIGN_CENTER, video::SColor(AlphaPercent, 255, 255, 255));
			Graphics.SetFont(_Graphics::FONT_10);
		}
	}

	// Draw the skill used
	if(SkillUsing) {
		Graphics.DrawCenteredImage(SkillUsing->GetImage(), Offset.x + 180, Offset.y + 50, video::SColor(255, 255, 255, 255));
	}

	// Draw target
	if(TTarget)
		Graphics.DrawImage(_Graphics::IMAGE_BATTLETARGET, Offset.x - 20, Offset.y + 50);
		*/
}

// Returns the fighter's current battle
_Battle *_Fighter::GetBattle() {

	return Battle;
}

// Update health
void _Fighter::UpdateHealth(int TValue) {
	Health += TValue;

	if(Health < 0)
		Health = 0;
	else if(Health > MaxHealth)
		Health = MaxHealth;
}

// Update mana
void _Fighter::UpdateMana(int TValue) {
	Mana += TValue;

	if(Mana < 0)
		Mana = 0;
	else if(Mana > MaxMana)
		Mana = MaxMana;
}

// Set health and mana to max
void _Fighter::RestoreHealthMana() {
	Health = MaxHealth;
	Mana = MaxMana;
}

// Updates the fighter's regen
void _Fighter::UpdateRegen(int &THealthUpdate, int &TManaUpdate) {

	THealthUpdate = 0;
	TManaUpdate = 0;
	HealthAccumulator += HealthRegen * 0.01f * MaxHealth;
	ManaAccumulator += ManaRegen * 0.01f * MaxMana;

	if(HealthAccumulator >= 1.0f) {
		int IntegerAccumulator = (int)HealthAccumulator;

		THealthUpdate = IntegerAccumulator;
		HealthAccumulator -= IntegerAccumulator;
	}
	else if(HealthAccumulator < 0.0f) {
		HealthAccumulator = 0;
	}

	if(ManaAccumulator >= 1.0f) {
		int IntegerAccumulator = (int)ManaAccumulator;

		TManaUpdate = IntegerAccumulator;
		ManaAccumulator -= IntegerAccumulator;
	}
	else if(ManaAccumulator < 0.0f) {
		ManaAccumulator = 0;
	}
}

// Sets the fighter's accumulators
void _Fighter::SetRegenAccumulators(float THealthAccumulator, float TManaAccumulator) {
	HealthAccumulator = THealthAccumulator;
	ManaAccumulator = TManaAccumulator;
}

// Generate damage
int _Fighter::GenerateDamage() {
	std::uniform_int_distribution<int> Distribution(MinDamage, MaxDamage);
	return Distribution(RandomGenerator);
}

// Generate defense
int _Fighter::GenerateDefense() {
	std::uniform_int_distribution<int> Distribution(MinDefense, MaxDefense);
	return Distribution(RandomGenerator);
}

// Gets a skill id from the skill bar
int _Fighter::GetSkillBarID(int TSlot) {
	if(SkillBar[TSlot] == nullptr)
		return -1;

	return SkillBar[TSlot]->GetID();
}

// Get a skill from the skill bar
const _Skill *_Fighter::GetSkillBar(int TSlot) {
	if(TSlot < 0 || TSlot >= 8)
		return nullptr;

	return SkillBar[TSlot];
}
