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
#include <objects/components/character.h>
#include <objects/object.h>
#include <ae/buffer.h>
#include <packet.h>
#include <stats.h>

// Constructor
_Character::_Character(_Object *Object) :
	Object(Object),
	UpdateTimer(0.0),

	CalcLevelStats(true),
	Level(0),
	Experience(0),
	ExperienceNeeded(0),
	ExperienceNextLevel(0),

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
	DropRate(0),

	SkillPoints(0),
	SkillPointsUsed(0),
	SkillPointsOnActionBar(0) {

}

// Update
void _Character::Update(double FrameTime) {
	UpdateTimer += FrameTime;
	if(UpdateTimer >= 1.0) {
		UpdateTimer -= 1.0;

		// Update stats
		if(Object->Server && Object->IsAlive()) {
			_StatChange StatChange;
			StatChange.Object = Object;

			// Update regen
			if(Health < MaxHealth && HealthRegen != 0)
				StatChange.Values[StatType::HEALTH].Integer = HealthRegen;
			if(Mana < MaxMana && ManaRegen != 0)
				StatChange.Values[StatType::MANA].Integer = ManaRegen;

			// Update object
			if(StatChange.GetChangedFlag() != 0) {
				Object->UpdateStats(StatChange);

				// Build packet
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::STAT_CHANGE);
				StatChange.Serialize(Packet);

				// Send packet to player
				Object->SendPacket(Packet);
			}
		}
	}
}

// Calculate base level stats
void _Character::CalculateLevelStats(const _Stats *Stats) {
	if(!Stats || !CalcLevelStats)
		return;

	// Cap min experience
	if(Experience < 0)
		Experience = 0;

	// Cap max experience
	const _Level *MaxLevelStat = Stats->GetLevel(Stats->GetMaxLevel());
	if(Experience > MaxLevelStat->Experience)
		Experience = MaxLevelStat->Experience;

	// Find current level
	const _Level *LevelStat = Stats->FindLevel(Experience);
	Level = LevelStat->Level;
	ExperienceNextLevel = LevelStat->NextLevel;
	ExperienceNeeded = (Level == Stats->GetMaxLevel()) ? 0 : LevelStat->NextLevel - (Experience - LevelStat->Experience);

	// Set base attributes
	BaseMaxHealth = LevelStat->Health;
	BaseMaxMana = LevelStat->Mana;
	BaseMinDamage = LevelStat->Damage;
	BaseMaxDamage = LevelStat->Damage + 1;
	BaseArmor = LevelStat->Armor;
	BaseDamageBlock = 0;
	SkillPoints = LevelStat->SkillPoints;
}
