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
#include <globals.h>
#include <instances/battle.h>
#include <network/network.h>
#include <buffer.h>
#include <objects/fighter.h>
#include <objects/player.h>

// Gets the mana cost of a skill
int _Skill::GetManaCost(int TLevel) const {
	if(TLevel < 1)
		TLevel = 1;

	return (int)(ManaCostBase + ManaCost * (TLevel - 1));
}

// Gets a random number between min and max power
int _Skill::GetPower(int TLevel) const {
	if(TLevel < 1)
		TLevel = 1;

	// Get range
	int Min, Max;
	GetPowerRangeRound(TLevel, Min, Max);
	std::uniform_int_distribution<int> Distribution(Min, Max);

	return Distribution(RandomGenerator);
}

// Returns the range of power
void _Skill::GetPowerRange(int TLevel, int &Min, int &Max) const {
	if(TLevel < 1)
		TLevel = 1;

	int FinalPower = (int)(PowerBase + Power * (TLevel - 1));
	int FinalPowerRange = (int)(PowerRangeBase + PowerRange * (TLevel - 1));

	Min = FinalPower - FinalPowerRange;
	Max = FinalPower + FinalPowerRange;
}

// Returns the range of power rounded
void _Skill::GetPowerRangeRound(int TLevel, int &Min, int &Max) const {
	if(TLevel < 1)
		TLevel = 1;

	int FinalPower = (int)(std::roundf(PowerBase + Power * (TLevel - 1)));
	int FinalPowerRange = (int)(std::roundf(PowerRangeBase + PowerRange * (TLevel - 1)));

	Min = FinalPower - FinalPowerRange;
	Max = FinalPower + FinalPowerRange;
}

// Returns the range of power
void _Skill::GetPowerRange(int TLevel, float &Min, float &Max) const {
	if(TLevel < 1)
		TLevel = 1;

	float FinalPower = PowerBase + Power * (TLevel - 1);
	float FinalPowerRange = PowerRangeBase + PowerRange * (TLevel - 1);

	Min = FinalPower - FinalPowerRange;
	Max = FinalPower + FinalPowerRange;
}

// Resolves the use of a skill in battle.
void _Skill::ResolveSkill(_FighterResult *Result, _FighterResult *TTargetResult) const {
	_Fighter *Fighter = Result->Fighter;
	_Fighter *TargetFighter = TTargetResult->Fighter;
	int SkillLevel = Fighter->GetSkillLevel(ID);

	int Damage = 0, Healing = 0, ManaRestore = 0, ManaCost = 0;
	switch(Type) {
		case TYPE_ATTACK:
			Damage = Fighter->GenerateDamage();
		break;
		case TYPE_SPELL:
			switch(ID) {
				// Heal
				case 3:
					Healing += GetPower(SkillLevel);
				break;
				// Flame, Lightning bolt
				case 6:
				case 11:
					Damage = GetPower(SkillLevel);
				break;
			}
			ManaCost = GetManaCost(SkillLevel);
		break;
		case TYPE_USEPOTION:
			if(Fighter->Type == _Fighter::TYPE_PLAYER) {
				_Player *Player = static_cast<_Player *>(Fighter);

				// Use the potion
				int Type = (ID == 2);
				int Slot = Player->GetPotionBattle(Type);
				if(Slot != -1) {
					int HealthChange = 0, ManaChange = 0;
					Player->UsePotionBattle(Slot, Type, HealthChange, ManaChange);
					Healing += HealthChange;
					ManaRestore += ManaChange;

					// Write packet
					_Buffer Packet;
					Packet.Write<char>(_Network::INVENTORY_USE);
					Packet.Write<char>(Slot);
					ServerNetwork->SendPacketToPeer(&Packet, Player->GetPeer());
				}
			}
		break;
	}

	// Generate defense
	Damage -= TargetFighter->GenerateDefense();
	if(Damage < 0)
		Damage = 0;
	Result->DamageDealt = Damage;

	// Update results
	TTargetResult->HealthChange += -Damage;
	Result->HealthChange += Healing;
	Result->ManaChange += ManaRestore - ManaCost;
}

// Determines if a skill can be used
bool _Skill::CanUse(_Fighter *TFighter) const {
	int Level = TFighter->GetSkillLevel(ID);

	// Check for bad types
	if(Type == TYPE_PASSIVE)
		return false;

	// Spell cost
	if(TFighter->Mana < GetManaCost(Level))
		return false;

	// Potions
	if(Type == TYPE_USEPOTION) {
		if(TFighter->Type == _Fighter::TYPE_MONSTER)
			return false;

		_Player *Player = static_cast<_Player *>(TFighter);
		return Player->GetPotionBattle(ID == 2) != -1;
	}

	return true;
}
