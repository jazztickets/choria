/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#include <objects/components/fighter.h>

// Constructor
_Fighter::_Fighter() :
	BaseMaxHealth(0),
	BaseMaxMana(0),
	BaseHealthRegen(0),
	BaseManaRegen(0),
	BaseHealPower(1.0f),
	BaseAttackPower(1.0f),
	BaseMinDamage(0),
	BaseMaxDamage(0),
	BaseArmor(0),
	BaseDamageBlock(0),
	BaseMoveSpeed(100),
	BaseBattleSpeed(100),
	BaseEvasion(0),
	BaseHitChance(100),
	BaseDropRate(0),

	Level(0),
	Health(1),
	MaxHealth(1),
	Mana(0),
	MaxMana(0),
	HealPower(0.0f),
	AttackPower(0.0f),
	MinDamage(0),
	MaxDamage(0),
	Armor(0),
	DamageBlock(0),
	MoveSpeed(100),
	BattleSpeed(100),
	Evasion(0),
	HitChance(100),
	DropRate(0) {

}
