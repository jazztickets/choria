/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#include "fighter.h"
#include "../engine/graphics.h"
#include "../engine/random.h"
#include "../engine/stats.h"
#include "../instances/battle.h"
#include "skill.h"

// Constructor
FighterClass::FighterClass(int TType)
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
	Battle(NULL),
	Command(-1),
	Target(0),
	SkillUsing(NULL),
	SkillUsed(NULL),
	Portrait(NULL) {

	for(int i = 0; i < FIGHTER_MAXSKILLS; i++)
		SkillBar[i] = NULL;

	SkillBar[0] = Stats::Instance().GetSkill(0);

	Offset.X = Offset.Y = 0;
}

// Destructor
FighterClass::~FighterClass() {

}

// Renders the fighter during a battle
void FighterClass::RenderBattle(bool TShowResults, float TTimerPercent, FighterResultStruct *TResult, bool TTarget) {
	char String[256];
	u32 AlphaPercent = (u32)(255 * TTimerPercent);
	if(TTimerPercent > 0.75f)
		AlphaPercent = 255;
	else
		AlphaPercent = (u32)(255 * TTimerPercent / 0.75f);

	// Sides
	GraphicsClass::ImageType SlotImage;
	if(GetSide() == 0) {
		SlotImage = GraphicsClass::IMAGE_BATTLESLOTLEFT;
	}
	else {
		SlotImage = GraphicsClass::IMAGE_BATTLESLOTRIGHT;
	}

	// Name
	Graphics::Instance().SetFont(GraphicsClass::FONT_10);
	Graphics::Instance().RenderText(Name.c_str(), Offset.X + 32, Offset.Y - 3, GraphicsClass::ALIGN_CENTER);

	// Portrait
	Graphics::Instance().DrawImage(SlotImage, Offset.X + 32, Offset.Y + 50);
	if(Portrait)
		Graphics::Instance().DrawCenteredImage(Portrait, Offset.X + 32, Offset.Y + 50);

	// Health
	int BarWidth = 80, BarHeight = 16, BarX = Offset.X + 70, BarY = Offset.Y + 27;

	float HealthPercent = MaxHealth > 0 ? Health / (float)MaxHealth : 0;
	sprintf(String, "%d / %d", Health, MaxHealth);
	Graphics::Instance().SetFont(GraphicsClass::FONT_8);
	Graphics::Instance().DrawBar(GraphicsClass::IMAGE_HEALTH, BarX, BarY, HealthPercent, BarWidth, BarHeight);
	Graphics::Instance().RenderText(String, BarX + BarWidth / 2, BarY + 1, GraphicsClass::ALIGN_CENTER);

	// Mana
	BarY += 30;
	float ManaPercent = MaxMana > 0 ? Mana / (float)MaxMana : 0;
	sprintf(String, "%d / %d", Mana, MaxMana);
	Graphics::Instance().DrawBar(GraphicsClass::IMAGE_MANA, BarX, BarY, ManaPercent, BarWidth, BarHeight);
	Graphics::Instance().RenderText(String, BarX + BarWidth / 2, BarY + 1, GraphicsClass::ALIGN_CENTER);

	// Show results of last turn
	if(TShowResults) {

		char Sign = ' ';
		SColor Color(AlphaPercent, 150, 150, 150);
		if(TResult->HealthChange > 0) {
			Sign = '+';
			Color.set(AlphaPercent, 0, 255, 0);
		}
		else if(TResult->HealthChange < 0) {
			Color.set(AlphaPercent, 255, 0, 0);
		}

		sprintf(String, "%c%d", Sign, TResult->HealthChange);
		Graphics::Instance().SetFont(GraphicsClass::FONT_14);
		Graphics::Instance().RenderText(String, BarX + BarWidth / 2, Offset.Y + 2, GraphicsClass::ALIGN_CENTER, Color);
		Graphics::Instance().SetFont(GraphicsClass::FONT_10);

		// Draw the skill used
		if(SkillUsed) {
			Graphics::Instance().DrawCenteredImage(SkillUsed->GetImage(), Offset.X + 180, Offset.Y + 50, SColor(AlphaPercent, 255, 255, 255));
		}

		// Draw damage dealt
		if(TResult->DamageDealt) {
			sprintf(String, "%d", TResult->DamageDealt);
			Graphics::Instance().SetFont(GraphicsClass::FONT_14);
			Graphics::Instance().RenderText(String, Offset.X + 178, Offset.Y + 38, GraphicsClass::ALIGN_CENTER, SColor(AlphaPercent, 255, 255, 255));
			Graphics::Instance().SetFont(GraphicsClass::FONT_10);
		}
	}

	// Draw the skill used
	if(SkillUsing) {
		Graphics::Instance().DrawCenteredImage(SkillUsing->GetImage(), Offset.X + 180, Offset.Y + 50, SColor(255, 255, 255, 255));
	}

	// Draw target
	if(TTarget)
		Graphics::Instance().DrawImage(GraphicsClass::IMAGE_BATTLETARGET, Offset.X - 20, Offset.Y + 50);
}

// Returns the fighter's current battle
BattleClass *FighterClass::GetBattle() {

	return Battle;
}

// Update health
void FighterClass::UpdateHealth(int TValue) {
	Health += TValue;

	if(Health < 0)
		Health = 0;
	else if(Health > MaxHealth)
		Health = MaxHealth;
}

// Update mana
void FighterClass::UpdateMana(int TValue) {
	Mana += TValue;

	if(Mana < 0)
		Mana = 0;
	else if(Mana > MaxMana)
		Mana = MaxMana;
}

// Set health and mana to max
void FighterClass::RestoreHealthMana() {
	Health = MaxHealth;
	Mana = MaxMana;
}

// Updates the fighter's regen
void FighterClass::UpdateRegen(int &THealthUpdate, int &TManaUpdate) {

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
void FighterClass::SetRegenAccumulators(float THealthAccumulator, float TManaAccumulator) {
	HealthAccumulator = THealthAccumulator;
	ManaAccumulator = TManaAccumulator;
}

// Generate damage
int FighterClass::GenerateDamage() {

	return Random::Instance().GenerateRange(MinDamage, MaxDamage);
}

// Generate defense
int FighterClass::GenerateDefense() {

	return Random::Instance().GenerateRange(MinDefense, MaxDefense);
}

// Gets a skill id from the skill bar
int FighterClass::GetSkillBarID(int TSlot) {
	if(SkillBar[TSlot] == NULL)
		return -1;

	return SkillBar[TSlot]->GetID();
}

// Get a skill from the skill bar
const SkillClass *FighterClass::GetSkillBar(int TSlot) {
	if(TSlot < 0 || TSlot >= 8)
		return NULL;

	return SkillBar[TSlot];
}
