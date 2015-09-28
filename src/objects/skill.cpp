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
#include <objects/skill.h>
#include <stats.h>
#include <random.h>
#include <globals.h>
#include <instances/battle.h>
#include <network/network.h>
#include <network/packetstream.h>
#include <objects/fighter.h>
#include <objects/player.h>

// Gets the mana cost of a skill
int SkillClass::GetManaCost(int TLevel) const {
	if(TLevel < 1)
		TLevel = 1;

	return (int)(ManaCostBase + ManaCost * (TLevel - 1));
}

// Gets a random number between min and max power
int SkillClass::GetPower(int TLevel) const {
	if(TLevel < 1)
		TLevel = 1;

	// Get range
	int Min, Max;
	GetPowerRangeRound(TLevel, Min, Max);
	std::uniform_int_distribution<int> Distribution(Min, Max);
	Distribution(RandomGenerator);

	return Distribution(RandomGenerator);
}

// Returns the range of power
void SkillClass::GetPowerRange(int TLevel, int &TMin, int &TMax) const {
	if(TLevel < 1)
		TLevel = 1;

	int FinalPower = (int)(PowerBase + Power * (TLevel - 1));
	int FinalPowerRange = (int)(PowerRangeBase + PowerRange * (TLevel - 1));

	TMin = FinalPower - FinalPowerRange;
	TMax = FinalPower + FinalPowerRange;
}

// Returns the range of power rounded
void SkillClass::GetPowerRangeRound(int TLevel, int &TMin, int &TMax) const {
	if(TLevel < 1)
		TLevel = 1;

	int FinalPower = (int)(round_(PowerBase + Power * (TLevel - 1)));
	int FinalPowerRange = (int)(round_(PowerRangeBase + PowerRange * (TLevel - 1)));

	TMin = FinalPower - FinalPowerRange;
	TMax = FinalPower + FinalPowerRange;
}

// Returns the range of power
void SkillClass::GetPowerRange(int TLevel, float &TMin, float &TMax) const {
	if(TLevel < 1)
		TLevel = 1;

	float FinalPower = PowerBase + Power * (TLevel - 1);
	float FinalPowerRange = PowerRangeBase + PowerRange * (TLevel - 1);

	TMin = FinalPower - FinalPowerRange;
	TMax = FinalPower + FinalPowerRange;
}

// Gets the sell cost of a skill
int SkillClass::GetSellCost(int TPlayerLevel) const {

	return SkillCost * (11 + TPlayerLevel);
}

// Resolves the use of a skill in battle.
void SkillClass::ResolveSkill(FighterResultStruct *TResult, FighterResultStruct *TTargetResult) const {
	FighterClass *Fighter = TResult->Fighter;
	FighterClass *TargetFighter = TTargetResult->Fighter;
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
			if(Fighter->GetType() == FighterClass::TYPE_PLAYER) {
				PlayerClass *Player = static_cast<PlayerClass *>(Fighter);

				// Use the potion
				int Type = (ID == 2);
				int Slot = Player->GetPotionBattle(Type);
				if(Slot != -1) {
					int HealthChange = 0, ManaChange = 0;
					Player->UsePotionBattle(Slot, Type, HealthChange, ManaChange);
					Healing += HealthChange;
					ManaRestore += ManaChange;

					// Write packet
					PacketClass Packet(NetworkClass::INVENTORY_USE);
					Packet.WriteChar(Slot);
					ServerNetwork->SendPacketToPeer(&Packet, Player->GetPeer());
				}
			}
		break;
	}

	// Generate defense
	Damage -= TargetFighter->GenerateDefense();
	if(Damage < 0)
		Damage = 0;
	TResult->DamageDealt = Damage;

	// Update results
	TTargetResult->HealthChange += -Damage;
	TResult->HealthChange += Healing;
	TResult->ManaChange += ManaRestore - ManaCost;
}

// Determines if a skill can be used
bool SkillClass::CanUse(FighterClass *TFighter) const {
	int Level = TFighter->GetSkillLevel(ID);

	// Check for bad types
	if(Type == TYPE_PASSIVE)
		return false;

	// Spell cost
	if(TFighter->GetMana() < GetManaCost(Level))
		return false;

	// Potions
	if(Type == TYPE_USEPOTION) {
		if(TFighter->GetType() == FighterClass::TYPE_MONSTER)
			return false;

		PlayerClass *Player = static_cast<PlayerClass *>(TFighter);
		return Player->GetPotionBattle(ID == 2) != -1;
	}

	return true;
}
