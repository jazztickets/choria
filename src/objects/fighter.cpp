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
#include <constants.h>
#include <graphics.h>
#include <random.h>
#include <stats.h>
#include <assets.h>
#include <font.h>
#include <program.h>
#include <ui/element.h>
#include <ui/image.h>
#include <instances/battle.h>
#include <objects/skill.h>
#include <sstream>

// Constructor
_Fighter::_Fighter(int Type)
:	_Object(Type),
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

	SkillBar[0] = OldStats.GetSkill(0);

	Offset.x = Offset.y = 0;
}

// Destructor
_Fighter::~_Fighter() {

}

// Renders the fighter during a battle
void _Fighter::RenderBattle(bool ShowResults, float TimerPercent, _FighterResult *Result, bool IsTarget) {

	// Get slot ui element depending on side
	_Element *Slot;
	if(GetSide() == 0)
		Slot = Assets.Elements["element_side_left"];
	else
		Slot = Assets.Elements["element_side_right"];

	// Draw slot
	Slot->Offset = Offset;
	Slot->CalculateBounds();
	Slot->SetVisible(true);
	Slot->Render();
	Slot->SetVisible(false);

	// Get slot center
	glm::ivec2 SlotPosition = (Slot->Bounds.Start + Slot->Bounds.End) / 2;

	// Name
	Assets.Fonts["hud_medium"]->DrawText(Name.c_str(), Slot->Bounds.Start + glm::ivec2(32, -10), COLOR_WHITE, CENTER_BASELINE);

	// Portrait
	if(Portrait) {
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(SlotPosition, Portrait);
	}

	// Health/mana bars
	glm::ivec2 BarSize(90, 22);
	glm::ivec2 BarOffset(Portrait->Size.x/2 + 10, -15);
	if(MaxMana == 0)
		BarOffset.y = 0;

	// Get health percent
	float HealthPercent = MaxHealth > 0 ? Health / (float)MaxHealth : 0;

	// Get ui size
	_Bounds BarBounds;
	BarBounds.Start = SlotPosition + glm::ivec2(0, -BarSize.y/2) + BarOffset;
	BarBounds.End = SlotPosition + glm::ivec2(BarSize.x, BarSize.y/2) + BarOffset;
	glm::ivec2 BarCenter = (BarBounds.Start + BarBounds.End) / 2;
	glm::ivec2 HealthBarCenter = BarCenter;
	int BarEndX = BarBounds.End.x;

	// Draw empty bar
	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	Graphics.SetVBO(VBO_NONE);
	Graphics.DrawImage(BarBounds, Assets.Images["image_hud_health_bar_empty"]->Texture, true);

	// Draw full bar
	BarBounds.End = SlotPosition + glm::ivec2(BarSize.x * HealthPercent, BarSize.y/2) + BarOffset;
	Graphics.DrawImage(BarBounds, Assets.Images["image_hud_health_bar_full"]->Texture, true);

	// Draw health text
	std::stringstream Buffer;
	Buffer << Health << " / " << MaxHealth;
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), BarCenter + glm::ivec2(0, 5), COLOR_WHITE, CENTER_BASELINE);
	Buffer.str("");

	// Draw mana
	if(MaxMana > 0) {
		float ManaPercent = MaxMana > 0 ? Mana / (float)MaxMana : 0;

		// Get ui size
		BarOffset.y = -BarOffset.y;
		BarBounds.Start = SlotPosition + glm::ivec2(0, -BarSize.y/2) + BarOffset;
		BarBounds.End = SlotPosition + glm::ivec2(BarSize.x, BarSize.y/2) + BarOffset;
		BarCenter = (BarBounds.Start + BarBounds.End) / 2;

		// Draw empty bar
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.SetVBO(VBO_NONE);
		Graphics.DrawImage(BarBounds, Assets.Images["image_hud_mana_bar_empty"]->Texture, true);

		// Draw full bar
		BarBounds.End = SlotPosition + glm::ivec2(BarSize.x * ManaPercent, BarSize.y/2) + BarOffset;
		Graphics.DrawImage(BarBounds, Assets.Images["image_hud_mana_bar_full"]->Texture, true);

		// Draw mana text
		Buffer << Mana << " / " << MaxMana;
		Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), BarCenter + glm::ivec2(0, 5), COLOR_WHITE, CENTER_BASELINE);
		Buffer.str("");
	}

	// Show results of last turn
	if(ShowResults) {

		float AlphaPercent;
		if(TimerPercent > 0.75f)
			AlphaPercent = 1;
		else
			AlphaPercent = TimerPercent / 0.75f;

		char Sign = ' ';
		glm::vec4 Color = COLOR_GRAY;
		if(Result->HealthChange > 0) {
			Sign = '+';
			Color = COLOR_GREEN;
		}
		else if(Result->HealthChange < 0) {
			Color = COLOR_RED;
		}
		Color.a = AlphaPercent;

		Buffer << Sign << Result->HealthChange;
		Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), HealthBarCenter + glm::ivec2(0, -20), Color, CENTER_BASELINE);

		const _Texture *SkillTexture;
		if(SkillUsed)
			SkillTexture = SkillUsed->Image;
		else
			SkillTexture = Assets.Textures["skills/attack.png"];

		// Draw skill icon
		glm::vec4 WhiteAlpha = glm::vec4(1.0f, 1.0f, 1.0f, AlphaPercent);
		glm::ivec2 SkillUsedPosition = SlotPosition - glm::ivec2(Portrait->Size.x/2 + SkillTexture->Size.x/2 + 10, 0);
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(SkillUsedPosition, SkillTexture, WhiteAlpha);

		// Draw damage dealt
		Assets.Fonts["hud_medium"]->DrawText(std::to_string(Result->DamageDealt).c_str(), SkillUsedPosition, WhiteAlpha, CENTER_MIDDLE);
	}
	// Draw the skill used
	if(SkillUsing) {
		glm::ivec2 SkillUsingPosition = SlotPosition - glm::ivec2(Portrait->Size.x/2 + SkillUsing->Image->Size.x/2 + 10, 0);
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(SkillUsingPosition, SkillUsing->Image);
	}

	// Draw target
	if(IsTarget) {
		const _Texture *Texture = Assets.Textures["battle/target.png"];
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(glm::ivec2(BarEndX + Texture->Size.x/2 + 10, SlotPosition.y), Texture);
	}
}

// Update health
void _Fighter::UpdateHealth(int Value) {
	Health += Value;

	if(Health < 0)
		Health = 0;
	else if(Health > MaxHealth)
		Health = MaxHealth;
}

// Update mana
void _Fighter::UpdateMana(int Value) {
	Mana += Value;

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
void _Fighter::UpdateRegen(int &HealthUpdate, int &ManaUpdate) {

	HealthUpdate = 0;
	ManaUpdate = 0;
	HealthAccumulator += HealthRegen * 0.01f * MaxHealth;
	ManaAccumulator += ManaRegen * 0.01f * MaxMana;

	if(HealthAccumulator >= 1.0f) {
		int IntegerAccumulator = (int)HealthAccumulator;

		HealthUpdate = IntegerAccumulator;
		HealthAccumulator -= IntegerAccumulator;
	}
	else if(HealthAccumulator < 0.0f) {
		HealthAccumulator = 0;
	}

	if(ManaAccumulator >= 1.0f) {
		int IntegerAccumulator = (int)ManaAccumulator;

		ManaUpdate = IntegerAccumulator;
		ManaAccumulator -= IntegerAccumulator;
	}
	else if(ManaAccumulator < 0.0f) {
		ManaAccumulator = 0;
	}
}

// Command input
int _Fighter::GetCommand() {
	 if(Type == MONSTER)
		 return 0;
	 else
		 return Command;
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
int _Fighter::GetSkillBarID(int Slot) {
	if(SkillBar[Slot] == nullptr)
		return -1;

	return SkillBar[Slot]->ID;
}

// Get a skill from the skill bar
const _Skill *_Fighter::GetSkillBar(int Slot) {
	if(Slot < 0 || Slot >= 8)
		return nullptr;

	return SkillBar[Slot];
}

// Updates the monster's target based on AI
void _Fighter::UpdateTarget(const std::vector<_Fighter *> &Fighters) {

	// Get count of fighters
	int Count = Fighters.size();

	// Get a random index
	std::uniform_int_distribution<int> Distribution(0, Count-1);
	int RandomIndex = Distribution(RandomGenerator);

	Target = Fighters[RandomIndex]->BattleSlot;
}
