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
#include <objects/skill.h>
#include <stats.h>
#include <random.h>
#include <buffer.h>
#include <font.h>
#include <graphics.h>
#include <input.h>
#include <assets.h>
#include <packet.h>
#include <instances/battle.h>
#include <objects/object.h>
#include <ui/label.h>
#include <ui/element.h>
#include <algorithm>

// Draw tooltip
void _Skill::DrawTooltip(const _Object *Player, const _Cursor &Tooltip, bool DrawNextLevel) const {
	_Element *TooltipElement = Assets.Elements["element_skills_tooltip"];
	_Label *TooltipName = Assets.Labels["label_skills_tooltip_name"];
	TooltipElement->SetVisible(true);

	// Set label values
	TooltipName->Text = Name;

	// Get window width
	glm::ivec2 Size = TooltipElement->Size;

	// Position window
	glm::ivec2 WindowOffset = Input.GetMouse();
	WindowOffset.x += INVENTORY_TOOLTIP_OFFSET;
	WindowOffset.y += -Size.y / 2;

	// Reposition window if out of bounds
	if(WindowOffset.y < Graphics.Element->Bounds.Start.x + INVENTORY_TOOLTIP_PADDING)
		WindowOffset.y = Graphics.Element->Bounds.Start.x + INVENTORY_TOOLTIP_PADDING;
	if(WindowOffset.x + Size.x > Graphics.Element->Bounds.End.x - INVENTORY_TOOLTIP_PADDING)
		WindowOffset.x -= Size.x + INVENTORY_TOOLTIP_OFFSET + INVENTORY_TOOLTIP_PADDING;
	if(WindowOffset.y + Size.y > Graphics.Element->Bounds.End.y - INVENTORY_TOOLTIP_PADDING)
		WindowOffset.y -= Size.y + INVENTORY_TOOLTIP_OFFSET - (TooltipElement->Bounds.End.y - TooltipElement->Bounds.Start.y) / 2;

	TooltipElement->SetOffset(WindowOffset);

	// Render tooltip
	TooltipElement->Render();
	TooltipElement->SetVisible(false);

	// Set draw position to center of window
	glm::ivec2 DrawPosition(WindowOffset.x + 20, TooltipName->Bounds.End.y);
	DrawPosition.y += 30;

	// Get current skill level
	int32_t SkillLevel = 0;
	auto SkillLevelIterator = Player->SkillLevels.find(ID);
	if(SkillLevelIterator != Player->SkillLevels.end())
		SkillLevel = SkillLevelIterator->second;

	// Get current level description
	Assets.Fonts["hud_small"]->DrawText("Level " + std::to_string(std::max(1, SkillLevel)), DrawPosition, COLOR_WHITE, LEFT_BASELINE);
	DrawPosition.y += 25;
	DrawDescription(SkillLevel, DrawPosition, Size.x);

	// Get next level description
	if(DrawNextLevel && SkillLevel > 0) {
		DrawPosition.y += 25;
		Assets.Fonts["hud_small"]->DrawText("Level " + std::to_string(SkillLevel+1), DrawPosition, COLOR_WHITE, LEFT_BASELINE);
		DrawPosition.y += 25;
		DrawDescription(SkillLevel+1, DrawPosition, Size.x);
	}

	// Additional information
	switch(Type) {
		case _Skill::TYPE_PASSIVE:
			DrawPosition.y += 25;
			Assets.Fonts["hud_small"]->DrawText("Passive skills must be equipped to the actionbar", DrawPosition, COLOR_GRAY, LEFT_BASELINE);
		break;
	}
}

// Draw skill description
void _Skill::DrawDescription(int SkillLevel, glm::ivec2 &DrawPosition, int Width) const {

	// Get power range
	int PowerMin, PowerMax;
	GetPowerRange(SkillLevel, PowerMin, PowerMax);

	// Get power range rounded
	int PowerMinRound, PowerMaxRound;
	GetPowerRangeRound(SkillLevel, PowerMinRound, PowerMaxRound);

	// Get floating point range
	float PowerMinFloat, PowerMaxFloat;
	GetPowerRange(SkillLevel, PowerMinFloat, PowerMaxFloat);

	// Get percent
	int PowerPercent = (int)std::roundf(PowerMaxFloat * 100);

	// Draw description
	int SpacingY = 25;
	char Buffer[512];
	Buffer[0] = 0;
	switch(Type) {
		case _Skill::TYPE_ATTACK:
			sprintf(Buffer, Info.c_str(), PowerPercent);
			Assets.Fonts["hud_small"]->DrawText(Buffer, DrawPosition, COLOR_GRAY, LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		break;
		case _Skill::TYPE_SPELL:
			switch(ID) {
				case 4:
					sprintf(Buffer, Info.c_str(), PowerMaxRound);
				break;
				case 7:
				case 12:
					sprintf(Buffer, Info.c_str(), PowerMinRound, PowerMaxRound);
				break;
			}
			Assets.Fonts["hud_small"]->DrawText(Buffer, DrawPosition, COLOR_GRAY, LEFT_BASELINE);
			DrawPosition.y += SpacingY;

			sprintf(Buffer, "%d Mana", GetManaCost(SkillLevel));
			Assets.Fonts["hud_small"]->DrawText(Buffer, DrawPosition, COLOR_BLUE, LEFT_BASELINE);
			DrawPosition.y += 15;
		break;
		case _Skill::TYPE_USEPOTION:
			sprintf(Buffer, Info.c_str(), PowerMin);
			Assets.Fonts["hud_small"]->DrawText(Buffer, DrawPosition, COLOR_GRAY, LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		break;
		case _Skill::TYPE_PASSIVE:
			switch(ID) {
				case 5:
				case 6:
					sprintf(Buffer, Info.c_str(), PowerMaxRound);
				break;
				case 8:
				case 9:
					sprintf(Buffer, Info.c_str(), PowerMaxFloat);
				break;
				case 10:
				case 11:
					sprintf(Buffer, Info.c_str(), PowerMax);
				break;
			}
			Assets.Fonts["hud_small"]->DrawText(Buffer, DrawPosition, COLOR_GRAY, LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		break;
		default:
			Assets.Fonts["hud_small"]->DrawText(Info.c_str(), DrawPosition, COLOR_GRAY, LEFT_BASELINE);
			DrawPosition.y += SpacingY;
		break;
	}
}

// Gets the mana cost of a skill
int _Skill::GetManaCost(int Level) const {
	if(Level < 1)
		Level = 1;

	return (int)(ManaCostBase + ManaCost * (Level - 1));
}

// Gets a random number between min and max power
int _Skill::GetPower(int Level) const {
	if(Level < 1)
		Level = 1;

	// Get range
	int Min, Max;
	GetPowerRangeRound(Level, Min, Max);
	std::uniform_int_distribution<int> Distribution(Min, Max);

	return Distribution(RandomGenerator);
}

// Returns the range of power
void _Skill::GetPowerRange(int Level, int &Min, int &Max) const {
	if(Level < 1)
		Level = 1;

	int FinalPower = (int)(PowerBase + Power * (Level - 1));
	int FinalPowerRange = (int)(PowerRangeBase + PowerRange * (Level - 1));

	Min = FinalPower - FinalPowerRange;
	Max = FinalPower + FinalPowerRange;
}

// Returns the range of power rounded
void _Skill::GetPowerRangeRound(int Level, int &Min, int &Max) const {
	if(Level < 1)
		Level = 1;

	int FinalPower = (int)(std::roundf(PowerBase + Power * (Level - 1)));
	int FinalPowerRange = (int)(std::roundf(PowerRangeBase + PowerRange * (Level - 1)));

	Min = FinalPower - FinalPowerRange;
	Max = FinalPower + FinalPowerRange;
}

// Returns the range of power
void _Skill::GetPowerRange(int Level, float &Min, float &Max) const {
	if(Level < 1)
		Level = 1;

	float FinalPower = PowerBase + Power * (Level - 1);
	float FinalPowerRange = PowerRangeBase + PowerRange * (Level - 1);

	Min = FinalPower - FinalPowerRange;
	Max = FinalPower + FinalPowerRange;
}

// Resolves the use of a skill in battle.
void _Skill::ResolveSkill(_ActionResult *Result, _ActionResult *TargetResult) const {
	_Object *Fighter = Result->Fighter;
	_Object *TargetFighter = TargetResult->Fighter;
	int SkillLevel = Fighter->SkillLevels[ID];

	int Damage = 0, Healing = 0, ManaRestore = 0, ManaCost = 0;
	switch(Type) {
		case TYPE_ATTACK:
			Damage = Fighter->GenerateDamage();
		break;
		case TYPE_SPELL:
			switch(ID) {
				// Heal
				case 4:
					Healing += GetPower(SkillLevel);
				break;
				// Flame, Lightning bolt
				case 7:
				case 12:
					Damage = GetPower(SkillLevel);
				break;
			}
			ManaCost = GetManaCost(SkillLevel);
		break;
		case TYPE_USEPOTION:
			if(Fighter->Type == _Object::PLAYER) {
				_Object *Player = (_Object *)Fighter;

				// Use the potion
				int Type = (ID == 3);
				int Slot = Player->GetPotionBattle(Type);
				if(Slot != -1) {
					int HealthChange = 0, ManaChange = 0;
					Player->UsePotionBattle(Slot, Type, HealthChange, ManaChange);
					Healing += HealthChange;
					ManaRestore += ManaChange;

					// Write packet
					_Buffer Packet;
					Packet.Write<char>(Packet::INVENTORY_USE);
					Packet.Write<char>(Slot);
					//OldServerNetwork->SendPacketToPeer(&Packet, Player->Peer);
				}
			}
		break;
	}

	// Generate defense
	Damage -= TargetFighter->GenerateDefense();
	if(Damage < 0)
		Damage = 0;

	// Update results
	Result->DamageDealt = Damage;
	TargetResult->HealthChange += -Damage;
	Result->HealthChange += Healing;
	Result->ManaChange += ManaRestore - ManaCost;
}

// Determines if a skill can be used
bool _Skill::CanUse(_Object *Fighter) const {
	int Level = Fighter->SkillLevels[ID];

	// Check for bad types
	if(Type == TYPE_PASSIVE)
		return false;

	// Spell cost
	if(Fighter->Mana < GetManaCost(Level))
		return false;

	// Potions
	if(Type == TYPE_USEPOTION) {
		if(Fighter->Type == _Object::MONSTER)
			return false;

		_Object *Player = (_Object *)Fighter;
		return Player->GetPotionBattle(ID == 3) != -1;
	}

	return true;
}
