/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#include "fighter.h"
#include "../engine/graphics.h"
#include "../engine/random.h"
#include "../engine/stats.h"
#include "../instances/battle.h"
#include "skill.h"

// Constructor
FighterClass::FighterClass(int Type)
:	Type(Type),
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
	TurnTimer(0),
	TurnTimerMax(3000),
	Battle(NULL),
	Portrait(NULL),
	BattleAction(NULL),
	Name(""),
	Target(0) {

	for(int i = 0; i < FIGHTER_MAXSKILLS; i++)
		ActionBar[i] = NULL;

	Offset.X = Offset.Y = 0;
}

// Destructor
FighterClass::~FighterClass() {

}

// Renders the fighter during a battle
void FighterClass::RenderBattle(bool Target) {
	char String[256];

	// Sides
	GraphicsClass::ImageType SlotImage;
	if(GetSide() == 0) {
		SlotImage = GraphicsClass::IMAGE_BATTLESLOTLEFT;
	}
	else {
		SlotImage = GraphicsClass::IMAGE_BATTLESLOTRIGHT;
	}

	// Name
	Graphics.SetFont(GraphicsClass::FONT_10);
	Graphics.RenderText(Name.c_str(), Offset.X + 32, Offset.Y - 3, GraphicsClass::ALIGN_CENTER);

	// Portrait
	Graphics.DrawImage(SlotImage, Offset.X + 32, Offset.Y + 50);
	if(Portrait)
		Graphics.DrawCenteredImage(Portrait, Offset.X + 32, Offset.Y + 50);

	// Health
	int BarWidth = 80, BarHeight = 16, BarX = Offset.X + 70, BarY = Offset.Y + 18;

	float HealthPercent = MaxHealth > 0 ? Health / (float)MaxHealth : 0;
	sprintf(String, "%d / %d", Health, MaxHealth);
	Graphics.SetFont(GraphicsClass::FONT_8);
	Graphics.DrawBar(GraphicsClass::IMAGE_HEALTH, BarX, BarY, HealthPercent, BarWidth, BarHeight);
	Graphics.RenderText(String, BarX + BarWidth / 2, BarY + 1, GraphicsClass::ALIGN_CENTER);

	// Mana
	BarY += 24;
	float ManaPercent = MaxMana > 0 ? Mana / (float)MaxMana : 0;
	sprintf(String, "%d / %d", Mana, MaxMana);
	Graphics.DrawBar(GraphicsClass::IMAGE_MANA, BarX, BarY, ManaPercent, BarWidth, BarHeight);
	Graphics.RenderText(String, BarX + BarWidth / 2, BarY + 1, GraphicsClass::ALIGN_CENTER);

	// Turn timer
	BarY += 24;
	float TurnTimerPercent = TurnTimerMax > 0 ? TurnTimer / (float)TurnTimerMax : 0;
	Graphics.DrawBar(GraphicsClass::IMAGE_TIMER, BarX, BarY, TurnTimerPercent, BarWidth, BarHeight);
	
	// Draw the action used
	if(BattleAction) {
		Graphics.DrawCenteredImage(BattleAction->GetImage(), Offset.X + 180, Offset.Y + 50);
	}

	// Draw target
	if(Target)
		Graphics.DrawImage(GraphicsClass::IMAGE_BATTLETARGET, Offset.X - 20, Offset.Y + 50);
}

// Sets the fighter's battle
void FighterClass::SetBattle(BattleClass *Battle) {

	this->Battle = Battle;
}

// Returns the fighter's current battle
BattleClass *FighterClass::GetBattle() {

	return Battle;
}

// Update health
void FighterClass::UpdateHealth(int Value) {
	Health += Value;

	if(Health < 0)
		Health = 0;
	else if(Health > MaxHealth)
		Health = MaxHealth;
}

// Update mana
void FighterClass::UpdateMana(int Value) {
	Mana += Value;

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
void FighterClass::UpdateRegen(int &HealthUpdate, int &ManaUpdate) {

	HealthUpdate = 0;
	ManaUpdate = 0;
	HealthAccumulator += HealthRegen * 0.01f * MaxHealth;
	ManaAccumulator += ManaRegen * 0.01f * MaxMana;

	if(HealthAccumulator >= 1.0f) {
		int IntegerAccumulator = (int)HealthAccumulator;

		HealthUpdate = IntegerAccumulator;
		HealthAccumulator -= IntegerAccumulator;
	}
	if(ManaAccumulator >= 1.0f) {
		int IntegerAccumulator = (int)ManaAccumulator;

		ManaUpdate = IntegerAccumulator;
		ManaAccumulator -= IntegerAccumulator;
	}
}

// Sets the fighter's accumulators
void FighterClass::SetRegenAccumulators(float HealthAccumulator, float ManaAccumulator) {
	HealthAccumulator = HealthAccumulator;
	ManaAccumulator = ManaAccumulator;
}

// Generate damage
int FighterClass::GenerateDamage() const {

	return Random.GenerateRange(MinDamage, MaxDamage);
}

// Generate defense
int FighterClass::GenerateDefense() const {

	return Random.GenerateRange(MinDefense, MaxDefense);
}

// Get an action from the action bar
const ActionClass *FighterClass::GetActionBar(int Index) {
	if(Index < 0 || Index >= FIGHTER_MAXACTIONS)
		return NULL;

	return ActionBar[Index];
}

// Update the battle timer
void FighterClass::UpdateTurnTimer(u32 UpdateTime) {
	TurnTimer += UpdateTime;
	if(TurnTimer > TurnTimerMax)
		TurnTimer = TurnTimerMax;
}
