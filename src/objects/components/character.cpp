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
#include <objects/components/inventory.h>
#include <objects/object.h>
#include <objects/buff.h>
#include <ae/buffer.h>
#include <scripting.h>
#include <packet.h>
#include <stats.h>
#include <cmath>

// Constructor
_Character::_Character(_Object *Object) :
	Object(Object),
	CharacterID(0),
	UpdateTimer(0.0),
	Gold(0),

	Invisible(0),
	Stunned(0),
	Hardcore(false),

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
		if(Object->Server && IsAlive()) {
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

// Calculates all of the player stats
void _Character::CalculateStats() {

	// Get base stats
	CalculateLevelStats(Object->Stats);

	MaxHealth = BaseMaxHealth;
	MaxMana = BaseMaxMana;
	HealthRegen = BaseHealthRegen;
	ManaRegen = BaseManaRegen;
	HealPower = BaseHealPower;
	AttackPower = BaseAttackPower;
	BattleSpeed = 0;
	Evasion = BaseEvasion;
	HitChance = BaseHitChance;
	MinDamage = BaseMinDamage;
	MaxDamage = BaseMaxDamage;
	Armor = BaseArmor;
	DamageBlock = BaseDamageBlock;
	MoveSpeed = BaseMoveSpeed;
	DropRate = BaseDropRate;
	Resistances.clear();

	Invisible = 0;
	Stunned = 0;

	// Get item stats
	int ItemMinDamage = 0;
	int ItemMaxDamage = 0;
	int ItemArmor = 0;
	int ItemDamageBlock = 0;
	float WeaponDamageModifier = 1.0f;
	_Bag &EquipmentBag = Object->Inventory->Bags[_Bag::EQUIPMENT];
	for(size_t i = 0; i < EquipmentBag.Slots.size(); i++) {

		// Check each item
		const _Item *Item = EquipmentBag.Slots[i].Item;
		int Upgrades = EquipmentBag.Slots[i].Upgrades;
		if(Item) {

			// Add damage
			if(Item->Type != ItemType::SHIELD) {
				ItemMinDamage += Item->GetMinDamage(Upgrades);
				ItemMaxDamage += Item->GetMaxDamage(Upgrades);
			}

			// Add defense
			ItemArmor += Item->GetArmor(Upgrades);
			ItemDamageBlock += Item->GetDamageBlock(Upgrades);

			// Stat changes
			MaxHealth += Item->GetMaxHealth(Upgrades);
			MaxMana += Item->GetMaxMana(Upgrades);
			HealthRegen += Item->GetHealthRegen(Upgrades);
			ManaRegen += Item->GetManaRegen(Upgrades);
			BattleSpeed += Item->GetBattleSpeed(Upgrades);
			MoveSpeed += Item->GetMoveSpeed(Upgrades);
			DropRate += Item->GetDropRate(Upgrades);

			// Add resistances
			Resistances[Item->ResistanceTypeID] += Item->GetResistance(Upgrades);
		}
	}

	// Calculate skills points used
	SkillPointsUsed = 0;
	for(const auto &SkillLevel : Object->Skills) {
		const _Item *Skill = Object->Stats->Items.at(SkillLevel.first);
		if(Skill)
			SkillPointsUsed += SkillLevel.second;
	}

	// Get skill bonus
	for(size_t i = 0; i < ActionBar.size(); i++) {
		_ActionResult ActionResult;
		ActionResult.Source.Object = Object;
		if(Object->GetActionFromSkillbar(ActionResult.ActionUsed, i)) {
			const _Item *Skill = ActionResult.ActionUsed.Item;
			if(Skill->IsSkill() && Skill->TargetID == TargetType::NONE) {

				// Get passive stat changes
				Skill->GetStats(Object->Scripting, ActionResult);
				CalculateStatBonuses(ActionResult.Source);
			}
		}
	}

	// Get buff stats
	for(const auto &StatusEffect : Object->StatusEffects) {
		_StatChange StatChange;
		StatChange.Object = Object;
		StatusEffect->Buff->ExecuteScript(Object->Scripting, "Stats", StatusEffect->Level, StatChange);
		CalculateStatBonuses(StatChange);
	}

	// Get damage
	MinDamage += (int)std::roundf(ItemMinDamage * WeaponDamageModifier);
	MaxDamage += (int)std::roundf(ItemMaxDamage * WeaponDamageModifier);
	MinDamage = std::max(MinDamage, 0);
	MaxDamage = std::max(MaxDamage, 0);

	// Get defense
	Armor += ItemArmor;
	DamageBlock += ItemDamageBlock;
	DamageBlock = std::max(DamageBlock, 0);

	// Cap resistances
	for(auto &Resist : Resistances) {
		Resist.second = std::min(Resist.second, GAME_MAX_RESISTANCE);
		Resist.second = std::max(Resist.second, -GAME_MAX_RESISTANCE);
	}

	// Get physical resistance from armor
	float ArmorResist = Armor / (30.0f + std::abs(Armor));

	// Physical resist comes solely from armor
	Resistances[2] = (int)(ArmorResist * 100);

	BattleSpeed = (int)(BaseBattleSpeed * BattleSpeed / 100.0 + BaseBattleSpeed);
	if(BattleSpeed < BATTLE_MIN_SPEED)
		BattleSpeed = BATTLE_MIN_SPEED;

	if(MoveSpeed < PLAYER_MIN_MOVESPEED)
		MoveSpeed = PLAYER_MIN_MOVESPEED;

	Health = std::min(Health, MaxHealth);
	Mana = std::min(Mana, MaxMana);

	RefreshActionBarCount();
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

// Update an object's stats from a statchange
void _Character::CalculateStatBonuses(_StatChange &StatChange) {
	if(StatChange.HasStat(StatType::MAXHEALTH))
		MaxHealth += StatChange.Values[StatType::MAXHEALTH].Integer;
	if(StatChange.HasStat(StatType::MAXMANA))
		MaxMana += StatChange.Values[StatType::MAXMANA].Integer;
	if(StatChange.HasStat(StatType::HEALTHREGEN))
		HealthRegen += StatChange.Values[StatType::HEALTHREGEN].Integer;
	if(StatChange.HasStat(StatType::MANAREGEN))
		ManaRegen += StatChange.Values[StatType::MANAREGEN].Integer;

	if(StatChange.HasStat(StatType::HEALPOWER))
		HealPower += StatChange.Values[StatType::HEALPOWER].Float;
	if(StatChange.HasStat(StatType::ATTACKPOWER))
		AttackPower += StatChange.Values[StatType::ATTACKPOWER].Float;

	if(StatChange.HasStat(StatType::BATTLESPEED))
		BattleSpeed += StatChange.Values[StatType::BATTLESPEED].Integer;
	if(StatChange.HasStat(StatType::HITCHANCE))
		HitChance += StatChange.Values[StatType::HITCHANCE].Integer;
	if(StatChange.HasStat(StatType::EVASION))
		Evasion += StatChange.Values[StatType::EVASION].Integer;
	if(StatChange.HasStat(StatType::STUNNED))
		Stunned = StatChange.Values[StatType::STUNNED].Integer;

	if(StatChange.HasStat(StatType::RESISTTYPE))
		Resistances[(uint32_t)StatChange.Values[StatType::RESISTTYPE].Integer] += StatChange.Values[StatType::RESIST].Integer;

	if(StatChange.HasStat(StatType::MINDAMAGE))
		MinDamage += StatChange.Values[StatType::MINDAMAGE].Integer;
	if(StatChange.HasStat(StatType::MAXDAMAGE))
		MaxDamage += StatChange.Values[StatType::MAXDAMAGE].Integer;
	if(StatChange.HasStat(StatType::ARMOR))
		Armor += StatChange.Values[StatType::ARMOR].Integer;
	if(StatChange.HasStat(StatType::DAMAGEBLOCK))
		DamageBlock += StatChange.Values[StatType::DAMAGEBLOCK].Integer;

	if(StatChange.HasStat(StatType::MOVESPEED))
		MoveSpeed += StatChange.Values[StatType::MOVESPEED].Integer;

	if(StatChange.HasStat(StatType::DROPRATE))
		DropRate += StatChange.Values[StatType::DROPRATE].Integer;

	if(StatChange.HasStat(StatType::INVISIBLE))
		Invisible = StatChange.Values[StatType::INVISIBLE].Integer;
}

// Update counts on action bar
void _Character::RefreshActionBarCount() {
	SkillPointsOnActionBar = 0;
	for(size_t i = 0; i < ActionBar.size(); i++) {
		const _Item *Item = ActionBar[i].Item;
		if(Item) {
			if(Item->IsSkill() && Object->HasLearned(Item))
				SkillPointsOnActionBar += Object->Skills[Item->ID];
			else
				ActionBar[i].Count = Object->Inventory->CountItem(Item);
		}
		else
			ActionBar[i].Count = 0;
	}
}
