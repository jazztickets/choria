/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020 Alan Witkowski
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
#include <objects/components/controller.h>
#include <objects/object.h>
#include <objects/statuseffect.h>
#include <objects/buff.h>
#include <ae/buffer.h>
#include <ae/random.h>
#include <ae/database.h>
#include <scripting.h>
#include <packet.h>
#include <stats.h>
#include <algorithm>
#include <cmath>

// Constructor
_Character::_Character(_Object *Object) :
	Object(Object),
	CharacterID(0),
	BuildID(1),
	UpdateTimer(0.0),

	Battle(nullptr),
	HUD(nullptr),

	StatusTexture(nullptr),
	Portrait(nullptr),
	PortraitID(0),

	IdleTime(0.0),
	Gold(0),
	NextBattle(0),
	Invisible(0),
	Stunned(0),
	DiagonalMovement(false),
	LavaProtection(false),
	Hardcore(false),
	Offline(false),
	Status(0),

	PlayTime(0.0),
	RebirthTime(0.0),
	BattleTime(0.0),
	Deaths(0),
	MonsterKills(0),
	PlayerKills(0),
	GamesPlayed(0),
	Bounty(0),
	GoldLost(0),
	Rebirths(0),

	EternalStrength(0),
	EternalGuard(0),
	EternalFortitude(0),
	EternalSpirit(0),
	EternalWisdom(0),
	EternalWealth(0),
	EternalAlacrity(0),
	EternalKnowledge(0),
	EternalPain(0),
	RebirthWealth(0),
	RebirthWisdom(0),
	RebirthKnowledge(0),
	RebirthPower(0),
	RebirthGirth(0),
	RebirthProficiency(0),
	RebirthInsight(0),
	RebirthPassage(0),

	CalcLevelStats(true),
	Level(0),
	RebirthTier(0),
	Experience(0),
	ExperienceNeeded(0),
	ExperienceNextLevel(0),

	BaseMaxHealth(0),
	BaseMaxMana(0),
	BaseMinDamage(0),
	BaseMaxDamage(0),
	BaseArmor(0),
	BaseDamageBlock(0),
	BaseBattleSpeed(100),
	BaseSpellDamage(100),
	BaseAttackPeriod(BATTLE_DEFAULTATTACKPERIOD),

	Health(1),
	MaxHealth(1),
	Mana(0),
	MaxMana(0),
	HealthRegen(0),
	ManaRegen(0),
	ExperienceMultiplier(1.0f),
	GoldMultiplier(1.0f),
	MaxHealthMultiplier(1.0f),
	MaxManaMultiplier(1.0f),
	ManaReductionRatio(0.0f),
	HealthUpdateMultiplier(0.0f),
	ShieldDamage(100),
	MinDamage(0),
	MaxDamage(0),
	Armor(0),
	DamageBlock(0),
	Pierce(0),
	MoveSpeed(100),
	BattleSpeed(0),
	EquipmentBattleSpeed(0),
	Evasion(0),
	SpellDamage(100),
	HitChance(100),
	AllSkills(0),
	SummonLimit(0),
	Difficulty(0),
	MinigameSpeed(1),
	ConsumeChance(100),
	CooldownMultiplier(0),

	SkillPoints(0),
	SkillPointsUnlocked(0),
	SkillPointsUsed(0),
	SkillPointsOnActionBar(0),
	BeltSize(ACTIONBAR_DEFAULT_BELTSIZE),
	SkillBarSize(ACTIONBAR_DEFAULT_SKILLBARSIZE),

	Vendor(nullptr),
	Trader(nullptr),
	Blacksmith(nullptr),
	Enchanter(nullptr),
	Minigame(nullptr),
	Seed(0),

	TradePlayer(nullptr),
	TradeGold(0),
	WaitingForTrade(false),
	TradeAccepted(false),

	LoadMapID(0),
	SpawnMapID(1),
	SpawnPoint(0),
	TeleportTime(-1),

	MenuOpen(false),
	InventoryOpen(false),
	SkillsOpen(false),

	Bot(false) {

	ActionBar.resize(ACTIONBAR_MAX_SIZE);
}

// Destructor
_Character::~_Character() {
	DeleteStatusEffects();
}

// Update
void _Character::Update(double FrameTime) {

	// Update stats
	UpdateTimer += FrameTime;
	if(UpdateTimer >= 1.0) {
		UpdateTimer -= 1.0;

		// Update stats on server
		if(Object->Server && IsAlive()) {
			_StatChange StatChange;
			StatChange.Object = Object;

			// Update regen
			if((Health < MaxHealth && HealthRegen != 0) || HealthRegen < 0)
				StatChange.Values[StatType::HEALTH].Integer = HealthRegen;
			if((Mana < MaxMana && ManaRegen != 0) || ManaRegen < 0)
				StatChange.Values[StatType::MANA].Integer = ManaRegen;

			// Update object
			if(StatChange.Values.size() != 0) {
				Object->UpdateStats(StatChange);

				// Build packet
				ae::_Buffer Packet;
				Packet.Write<PacketType>(PacketType::STAT_CHANGE);
				StatChange.Serialize(Packet);

				// Send packet to player
				Object->SendPacket(Packet);
			}
		}
	}

	// Update status effects
	for(auto Iterator = StatusEffects.begin(); Iterator != StatusEffects.end(); ) {
		_StatusEffect *StatusEffect = *Iterator;
		StatusEffect->Time += FrameTime;

		// Call status effect's update every second
		if(StatusEffect->Time >= 1.0) {
			StatusEffect->Time -= 1.0;

			// Resolve effects
			if(Object->Server && IsAlive()) {
				Object->ResolveBuff(StatusEffect, "Update");
			}
		}

		// Update duration if not infinite
		if(!StatusEffect->Infinite) {

			// Don't update during battle if it pauses
			if(!StatusEffect->Buff->PauseDuringBattle || (StatusEffect->Buff->PauseDuringBattle && !Object->Character->Battle))
				StatusEffect->Duration -= FrameTime;

			// Delete
			if(StatusEffect->Duration <= 0.0)
				StatusEffect->Deleted = true;
		}

		// Clean up
		if(StatusEffect->Deleted || !IsAlive()) {
			delete StatusEffect;
			Iterator = StatusEffects.erase(Iterator);

			CalculateStats();
		}
		else
			++Iterator;
	}

	// Update battle cooldowns
	if(IdleTime <= PLAYER_IDLE_TIME) {
		for(auto Iterator = BattleCooldown.begin(); Iterator != BattleCooldown.end(); ) {
			Iterator->second -= FrameTime;

			// Remove cooldown
			if(Iterator->second <= 0.0)
				Iterator = BattleCooldown.erase(Iterator);
			else
				++Iterator;
		}
	}

	// Update cooldowns
	for(auto Iterator = Cooldowns.begin(); Iterator != Cooldowns.end(); ) {
		Iterator->second.Duration -= FrameTime;

		// Remove cooldown
		if(Iterator->second.Duration <= 0.0)
			Iterator = Cooldowns.erase(Iterator);
		else
			++Iterator;
	}
}

// Update health
void _Character::UpdateHealth(int &Value) {
	if(Object->Server && Value > 0)
		Value *= HealthUpdateMultiplier;

	Health += Value;

	if(Health < 0)
		Health = 0;
	else if(Health > MaxHealth)
		Health = MaxHealth;
}

// Update mana
void _Character::UpdateMana(int Value) {
	Mana += Value;

	if(Mana < 0)
		Mana = 0;
	else if(Mana > MaxMana)
		Mana = MaxMana;
}

// Update gold amount
void _Character::UpdateGold(int Value) {
	Gold += Value;
	if(Gold > PLAYER_MAX_GOLD)
		Gold = PLAYER_MAX_GOLD;
}

// Update experience
void _Character::UpdateExperience(int64_t Value) {
	Experience += Value;
	if(Experience < 0)
		Experience = 0;
}

// Update status of character
void _Character::UpdateStatus() {

	Status = STATUS_NONE;
	if(!IsAlive())
		Status = STATUS_DEAD;
	else if(Battle)
		Status = STATUS_BATTLE;
	else if(WaitingForTrade)
		Status = STATUS_TRADE;
	else if(Vendor)
		Status = STATUS_VENDOR;
	else if(Trader)
		Status = STATUS_TRADER;
	else if(Blacksmith)
		Status = STATUS_BLACKSMITH;
	else if(Enchanter)
		Status = STATUS_SKILLS;
	else if(Minigame)
		Status = STATUS_MINIGAME;
	else if(InventoryOpen)
		Status = STATUS_INVENTORY;
	else if(SkillsOpen)
		Status = STATUS_SKILLS;
	else if(MenuOpen)
		Status = STATUS_MENU;
	else if(TeleportTime > 0)
		Status = STATUS_TELEPORT;
}

// Calculates all of the player stats
void _Character::CalculateStats() {
	_Scripting *Scripting = Object->Scripting;

	// Set default values
	for(const auto &Attribute : Object->Stats->Attributes)
		Attributes[Attribute.second.Name].Integer = Attribute.second.Default.Integer;

	// Get base stats
	CalculateLevelStats();
	SkillPoints += SkillPointsUnlocked;

	MaxHealth = BaseMaxHealth;
	MaxMana = BaseMaxMana;
	BattleSpeed = BaseBattleSpeed;
	MinDamage = BaseMinDamage;
	MaxDamage = BaseMaxDamage;
	Armor = BaseArmor;
	DamageBlock = BaseDamageBlock;
	SpellDamage = BaseSpellDamage;
	HealthRegen = 0;
	ManaRegen = 0;
	HealthUpdateMultiplier = 1.0f;
	ManaReductionRatio = 1.0f;
	MoveSpeed = 100;
	Evasion = 0;
	HitChance = 100;
	ShieldDamage = 100;
	Pierce = 0;
	AllSkills = 0;

	Object->Light = 0;
	Invisible = 0;
	CooldownMultiplier = 1.0f;
	DiagonalMovement = false;
	LavaProtection = false;
	Stunned = 0;
	SummonLimit = 0;
	MinigameSpeed = 1;
	ConsumeChance = 100;
	Difficulty = EternalPain + Rebirths;
	RebirthTier += RebirthPower;
	Resistances.clear();
	Sets.clear();

	// Base resistances
	for(int i = 3; i <= 8; i++)
		Resistances[i] = BaseResistances[i];

	// Eternal Strength
	Attributes["PhysicalPower"].Integer += EternalStrength;
	Attributes["FirePower"].Integer += EternalStrength;
	Attributes["ColdPower"].Integer += EternalStrength;
	Attributes["LightningPower"].Integer += EternalStrength;
	Attributes["BleedPower"].Integer += EternalStrength;
	Attributes["PoisonPower"].Integer += EternalStrength;
	Attributes["PetPower"].Integer += EternalStrength;

	// Eternal Guard
	if(EternalGuard) {
		DamageBlock += EternalGuard;
		Armor += EternalGuard / 3;
		for(int i = GAME_ALL_RESIST_START_ID; i <= GAME_ALL_RESIST_END_ID; i++)
			Resistances[i] += EternalGuard / 4;
	}

	// Eternal Fortitude
	MaxHealthMultiplier = 1.0f + EternalFortitude / 100.0f;
	Attributes["HealPower"].Integer += EternalFortitude;

	// Eternal Spirit
	MaxManaMultiplier = 1.0f + EternalSpirit / 100.0f;
	Attributes["ManaPower"].Integer += EternalSpirit;

	// Eternal Wisdom
	ExperienceMultiplier = 1.0f + EternalWisdom / 100.0f;

	// Eternal Wealth
	GoldMultiplier = 1.0f + EternalWealth / 100.0f;

	// Eternal Alacrity
	BattleSpeed = 100 + EternalAlacrity;

	// Eternal Knowledge
	SkillPoints += EternalKnowledge;

	// Get item stats
	std::vector<int> ItemMinDamage(Object->Stats->DamageTypes.size(), 0);
	std::vector<int> ItemMaxDamage(Object->Stats->DamageTypes.size(), 0);
	std::unordered_map<uint32_t, std::vector<int>> SetPieceLevels;
	_Bag &EquipmentBag = Object->Inventory->GetBag(BagType::EQUIPMENT);
	for(size_t i = 0; i < EquipmentBag.Slots.size(); i++) {
		const _Item *Item = EquipmentBag.Slots[i].Item;
		if(!Item)
			continue;

		// Get upgrade count
		int Upgrades = EquipmentBag.Slots[i].Upgrades;

		// Get stat bonuses
		_ActionResult ActionResult;
		ActionResult.ActionUsed.Level = Upgrades;
		ActionResult.Source.Object = Object;
		Item->GetStats(Scripting, ActionResult, 0, 0);
		CalculateStatBonuses(ActionResult.Source);

		// Add damage
		if(Item->Type != ItemType::SHIELD) {
			ItemMinDamage[Item->DamageTypeID] += Item->GetMinDamage(Upgrades);
			ItemMaxDamage[Item->DamageTypeID] += Item->GetMaxDamage(Upgrades);
		}

		// Stat changes
		Armor += Item->GetArmor(Upgrades);
		DamageBlock += Item->GetDamageBlock(Upgrades);
		Pierce += Item->GetPierce(Upgrades);
		MaxHealth += Item->GetMaxHealth(Upgrades);
		MaxMana += Item->GetMaxMana(Upgrades);
		HealthRegen += Item->GetHealthRegen(Upgrades);
		ManaRegen += Item->GetManaRegen(Upgrades);
		BattleSpeed += Item->GetBattleSpeed(Upgrades);
		MoveSpeed += Item->GetMoveSpeed(Upgrades);
		Evasion += Item->GetEvasion(Upgrades);
		AllSkills += Item->GetAllSkills(Upgrades);
		SpellDamage += Item->GetSpellDamage(Upgrades);
		CooldownMultiplier += Item->GetCooldownReduction(Upgrades) / 100.0f;
		GoldMultiplier += Item->GetGoldBonus(Upgrades) / 100.0f;
		ExperienceMultiplier += Item->GetExpBonus(Upgrades) / 100.0f;

		// Handle all resist
		if(Item->ResistanceTypeID == 1) {
			for(int i = GAME_ALL_RESIST_START_ID; i <= GAME_ALL_RESIST_END_ID; i++)
				Resistances[i] += Item->GetResistance(Upgrades);
		}
		else
			Resistances[Item->ResistanceTypeID] += Item->GetResistance(Upgrades);

		// Increment set count
		if(Item->SetID) {
			auto SetIterator = Sets.find(Item->SetID);
			if(SetIterator == Sets.end()) {
				Sets[Item->SetID].EquippedCount = 1;
				Sets[Item->SetID].MaxLevel = Item->MaxLevel;
			}
			else {
				SetIterator->second.EquippedCount++;
				SetIterator->second.MaxLevel = std::min(SetIterator->second.MaxLevel, Item->MaxLevel);
			}

			SetPieceLevels[Item->SetID].push_back(Upgrades);
		}
	}

	// Add set bonus
	for(auto &SetData : Sets) {

		// Check for completed set
		const _Set &Set = Object->Stats->Sets.at(SetData.first);
		if(SetData.second.EquippedCount < Set.Count) {
			SetData.second.Level = 0;
			continue;
		}

		// Get set level from nth highest level piece, where n is the set count
		auto SetPieceLevelsArray = SetPieceLevels[SetData.first];
		std::sort(SetPieceLevelsArray.begin(), SetPieceLevelsArray.end(), std::greater());
		SetData.second.Level = SetPieceLevelsArray[Set.Count-1];

		// Get stat bonuses when set is complete
		_StatChange StatChange;
		StatChange.Object = Object;
		if(Scripting->StartMethodCall(Set.Script, "SetStats")) {
			Scripting->PushObject(StatChange.Object);
			Scripting->PushInt(SetData.second.Level);
			Scripting->PushInt(SetData.second.MaxLevel);
			Scripting->PushStatChange(&StatChange);
			Scripting->MethodCall(4, 1);
			Scripting->GetStatChange(1, StatChange);
			Scripting->FinishMethodCall();

			CalculateStatBonuses(StatChange);
		}
	}

	// Get added set bonus
	for(size_t i = 0; i < EquipmentBag.Slots.size(); i++) {
		const _Item *Item = EquipmentBag.Slots[i].Item;
		if(!Item || !Item->SetID)
			continue;

		const _SetData &SetData = Sets.at(Item->SetID);
		const _Set &Set = Object->Stats->Sets.at(Item->SetID);
		if(SetData.EquippedCount < Set.Count)
			continue;

		_StatChange StatChange;
		StatChange.Object = Object;
		if(Scripting->StartMethodCall(Item->Script, "SetStats")) {
			const _SetData &SetData = Sets.at(Item->SetID);

			Scripting->PushObject(StatChange.Object);
			Scripting->PushInt(SetData.Level);
			Scripting->PushInt(SetData.MaxLevel);
			Scripting->PushStatChange(&StatChange);
			Scripting->MethodCall(4, 1);
			Scripting->GetStatChange(1, StatChange);
			Scripting->FinishMethodCall();

			CalculateStatBonuses(StatChange);
		}
	}

	// Set max skill levels if necessary
	if(CalcLevelStats)
		InitializeSkillLevels();

	// Calculate skills points used
	SkillPointsUsed = 0;
	for(const auto &SkillLevel : Skills) {
		const _Item *Skill = Object->Stats->Items.at(SkillLevel.first);
		if(Skill)
			SkillPointsUsed += SkillLevel.second;
	}

	// Get skill bonus
	for(int i = 0; i < SkillBarSize; i++) {
		_ActionResult ActionResult;
		ActionResult.Source.Object = Object;
		if(GetActionFromActionBar(ActionResult.ActionUsed, i)) {
			const _Item *Skill = ActionResult.ActionUsed.Item;
			if(Skill->IsSkill() && Skill->TargetID == TargetType::NONE) {

				// Get passive stat changes
				Skill->GetStats(Object->Scripting, ActionResult, 0, 0);
				CalculateStatBonuses(ActionResult.Source);
			}
		}
	}

	// Get speed before buffs
	EquipmentBattleSpeed = BattleSpeed;

	// Get buff stats
	for(const auto &StatusEffect : StatusEffects) {
		_StatChange StatChange;
		StatChange.Object = Object;
		StatusEffect->Buff->ExecuteScript(Scripting, "Stats", StatusEffect->Level, StatChange);
		CalculateStatBonuses(StatChange);
	}

	// Get damage
	for(size_t i = 0; i < ItemMinDamage.size(); i++) {
		MinDamage += (int)std::roundf(ItemMinDamage[i] * Attributes["AttackPower"].Integer * 0.01f * GetDamagePowerMultiplier(i));
		MaxDamage += (int)std::roundf(ItemMaxDamage[i] * Attributes["AttackPower"].Integer * 0.01f * GetDamagePowerMultiplier(i));
	}

	// Cap resistances
	for(auto &Resist : Resistances) {
		Resist.second = std::min(Resist.second, GAME_MAX_RESISTANCE);
		Resist.second = std::max(Resist.second, GAME_MIN_RESISTANCE);
	}

	// Get physical resistance from armor
	float ArmorResist = Armor / (30.0f + std::abs(Armor));

	// Physical resist comes solely from armor
	Resistances[2] = (int)(ArmorResist * 100);

	// Cap stats
	if(Evasion > GAME_MAX_EVASION)
		Evasion = GAME_MAX_EVASION;
	if(BattleSpeed < BATTLE_MIN_SPEED)
		BattleSpeed = BATTLE_MIN_SPEED;
	if(MoveSpeed < PLAYER_MIN_MOVESPEED)
		MoveSpeed = PLAYER_MIN_MOVESPEED;
	if(CooldownMultiplier <= 0.0f)
		CooldownMultiplier = 0.0f;

	MinDamage = std::max(MinDamage, 0);
	MaxDamage = std::max(MaxDamage, 0);
	Pierce = std::max(Pierce, 0);
	DamageBlock = std::max(DamageBlock, 0);
	ManaReductionRatio = std::clamp(1.0f - ManaReductionRatio, 0.0f, 1.0f);
	ConsumeChance = std::clamp(ConsumeChance, 0, 100);

	MaxHealth *= MaxHealthMultiplier;
	MaxMana *= MaxManaMultiplier;
	Health = std::min(Health, MaxHealth);
	Mana = std::min(Mana, MaxMana);
	if(HealthRegen > 0)
		HealthRegen *= Attributes["HealPower"].Integer * 0.01f;
	if(ManaRegen > 0)
		ManaRegen *= Attributes["ManaPower"].Integer * 0.01f;

	RefreshActionBarCount();
}

// Calculate base level stats
void _Character::CalculateLevelStats() {
	if(!Object->Stats || !CalcLevelStats)
		return;

	// Cap min experience
	if(Experience < 0)
		Experience = 0;

	// Cap max experience
	const _Level *MaxLevelStat = Object->Stats->GetLevel(Object->Stats->GetMaxLevel());
	if(Experience > MaxLevelStat->Experience)
		Experience = MaxLevelStat->Experience;

	// Find current level
	const _Level *LevelStat = Object->Stats->FindLevel(Experience);
	Level = LevelStat->Level;
	RebirthTier = LevelStat->RebirthTier;
	ExperienceNextLevel = LevelStat->NextLevel;
	ExperienceNeeded = (Level == Object->Stats->GetMaxLevel()) ? 0 : LevelStat->NextLevel - (Experience - LevelStat->Experience);

	// Set base attributes
	BaseMaxHealth = LevelStat->Health;
	BaseMaxMana = LevelStat->Mana;
	BaseMinDamage = 1;
	BaseMaxDamage = 2;
	BaseArmor = 0;
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

	if(StatChange.HasStat(StatType::MANAREDUCTIONRATIO))
		ManaReductionRatio *= (1.0f - StatChange.Values[StatType::MANAREDUCTIONRATIO].Float);
	if(StatChange.HasStat(StatType::HEALTHUPDATEMULTIPLIER))
		HealthUpdateMultiplier += StatChange.Values[StatType::HEALTHUPDATEMULTIPLIER].Float;
	if(StatChange.HasStat(StatType::ATTACKPOWER))
		Attributes["AttackPower"].Integer += StatChange.Values[StatType::ATTACKPOWER].Integer;
	if(StatChange.HasStat(StatType::PHYSICALPOWER))
		Attributes["PhysicalPower"].Integer += StatChange.Values[StatType::PHYSICALPOWER].Integer;
	if(StatChange.HasStat(StatType::FIREPOWER))
		Attributes["FirePower"].Integer += StatChange.Values[StatType::FIREPOWER].Integer;
	if(StatChange.HasStat(StatType::COLDPOWER))
		Attributes["ColdPower"].Integer += StatChange.Values[StatType::COLDPOWER].Integer;
	if(StatChange.HasStat(StatType::LIGHTNINGPOWER))
		Attributes["LightningPower"].Integer += StatChange.Values[StatType::LIGHTNINGPOWER].Integer;
	if(StatChange.HasStat(StatType::BLEEDPOWER))
		Attributes["BleedPower"].Integer += StatChange.Values[StatType::BLEEDPOWER].Integer;
	if(StatChange.HasStat(StatType::POISONPOWER))
		Attributes["PoisonPower"].Integer += StatChange.Values[StatType::POISONPOWER].Integer;
	if(StatChange.HasStat(StatType::PETPOWER))
		Attributes["PetPower"].Integer += StatChange.Values[StatType::PETPOWER].Integer;
	if(StatChange.HasStat(StatType::HEALPOWER))
		Attributes["HealPower"].Integer += StatChange.Values[StatType::HEALPOWER].Integer;
	if(StatChange.HasStat(StatType::MANAPOWER))
		Attributes["ManaPower"].Integer += StatChange.Values[StatType::MANAPOWER].Integer;
	if(StatChange.HasStat(StatType::SUMMONLIMIT))
		SummonLimit += StatChange.Values[StatType::SUMMONLIMIT].Integer;
	if(StatChange.HasStat(StatType::SPELL_DAMAGE))
		SpellDamage += StatChange.Values[StatType::SPELL_DAMAGE].Integer;

	if(StatChange.HasStat(StatType::BATTLESPEED))
		BattleSpeed += StatChange.Values[StatType::BATTLESPEED].Integer;
	if(StatChange.HasStat(StatType::HITCHANCE))
		HitChance += StatChange.Values[StatType::HITCHANCE].Integer;
	if(StatChange.HasStat(StatType::EVASION))
		Evasion += StatChange.Values[StatType::EVASION].Integer;
	if(StatChange.HasStat(StatType::STUNNED))
		Stunned = StatChange.Values[StatType::STUNNED].Integer;

	if(StatChange.HasStat(StatType::RESISTTYPE) && StatChange.HasStat(StatType::RESIST)) {
		uint32_t ResistType = (uint32_t)StatChange.Values[StatType::RESISTTYPE].Integer;
		if(ResistType == 1) {
			for(int i = GAME_ALL_RESIST_START_ID; i <= GAME_ALL_RESIST_END_ID; i++)
				Resistances[i] += StatChange.Values[StatType::RESIST].Integer;
		}
		else if(ResistType == 9) {
			for(int i = GAME_ELEMENTAL_RESIST_START_ID; i <= GAME_ELEMENTAL_RESIST_END_ID; i++)
				Resistances[i] += StatChange.Values[StatType::RESIST].Integer;
		}
		else
			Resistances[ResistType] += StatChange.Values[StatType::RESIST].Integer;
	}

	if(StatChange.HasStat(StatType::ALL_RESIST)) {
		for(int i = GAME_ALL_RESIST_START_ID; i <= GAME_ALL_RESIST_END_ID; i++)
			Resistances[i] += StatChange.Values[StatType::ALL_RESIST].Integer;
	}
	if(StatChange.HasStat(StatType::PHYSICAL_RESIST))
		Resistances[2] += StatChange.Values[StatType::PHYSICAL_RESIST].Integer;
	if(StatChange.HasStat(StatType::FIRE_RESIST))
		Resistances[3] += StatChange.Values[StatType::FIRE_RESIST].Integer;
	if(StatChange.HasStat(StatType::COLD_RESIST))
		Resistances[4] += StatChange.Values[StatType::COLD_RESIST].Integer;
	if(StatChange.HasStat(StatType::LIGHTNING_RESIST))
		Resistances[5] += StatChange.Values[StatType::LIGHTNING_RESIST].Integer;
	if(StatChange.HasStat(StatType::POISON_RESIST))
		Resistances[6] += StatChange.Values[StatType::POISON_RESIST].Integer;
	if(StatChange.HasStat(StatType::BLEED_RESIST))
		Resistances[7] += StatChange.Values[StatType::BLEED_RESIST].Integer;
	if(StatChange.HasStat(StatType::STUN_RESIST))
		Resistances[8] += StatChange.Values[StatType::STUN_RESIST].Integer;
	if(StatChange.HasStat(StatType::ELEMENTAL_RESIST)) {
		for(int i = GAME_ELEMENTAL_RESIST_START_ID; i <= GAME_ELEMENTAL_RESIST_END_ID; i++)
			Resistances[i] += StatChange.Values[StatType::ELEMENTAL_RESIST].Integer;
	}

	if(StatChange.HasStat(StatType::MINDAMAGE))
		MinDamage += StatChange.Values[StatType::MINDAMAGE].Integer;
	if(StatChange.HasStat(StatType::MAXDAMAGE))
		MaxDamage += StatChange.Values[StatType::MAXDAMAGE].Integer;
	if(StatChange.HasStat(StatType::ARMOR))
		Armor += StatChange.Values[StatType::ARMOR].Integer;
	if(StatChange.HasStat(StatType::DAMAGEBLOCK))
		DamageBlock += StatChange.Values[StatType::DAMAGEBLOCK].Integer;
	if(StatChange.HasStat(StatType::SHIELD_DAMAGE))
		ShieldDamage += StatChange.Values[StatType::SHIELD_DAMAGE].Integer;

	if(StatChange.HasStat(StatType::MOVESPEED))
		MoveSpeed += StatChange.Values[StatType::MOVESPEED].Integer;

	if(StatChange.HasStat(StatType::INVISIBLE))
		Invisible = StatChange.Values[StatType::INVISIBLE].Integer;

	if(StatChange.HasStat(StatType::LIGHT))
		Object->Light = StatChange.Values[StatType::LIGHT].Integer;

	if(StatChange.HasStat(StatType::DIAGONAL_MOVEMENT))
		DiagonalMovement = StatChange.Values[StatType::DIAGONAL_MOVEMENT].Integer;

	if(StatChange.HasStat(StatType::MINIGAME_SPEED))
		MinigameSpeed = StatChange.Values[StatType::MINIGAME_SPEED].Integer;

	if(StatChange.HasStat(StatType::LAVA_PROTECTION))
		LavaProtection = StatChange.Values[StatType::LAVA_PROTECTION].Integer;

	if(StatChange.HasStat(StatType::DIFFICULTY))
		Difficulty += StatChange.Values[StatType::DIFFICULTY].Integer;

	if(StatChange.HasStat(StatType::CONSUME_CHANCE))
		ConsumeChance += StatChange.Values[StatType::CONSUME_CHANCE].Integer;

	if(StatChange.HasStat(StatType::ALLSKILLS))
		AllSkills += StatChange.Values[StatType::ALLSKILLS].Integer;
}

// Get percentage to next level
float _Character::GetNextLevelPercent() const {
	float Percent = 0;

	if(ExperienceNextLevel > 0)
		Percent = 1.0f - (float)ExperienceNeeded / ExperienceNextLevel;

	return Percent;
}

// Determines if the player can accept movement keys held down
bool _Character::AcceptingMoveInput() {
	if(Battle)
		return false;

	if(Object->Controller->WaitForServer)
		return false;

	if(Vendor)
		return false;

	if(Trader)
		return false;

	if(Blacksmith)
		return false;

	if(Enchanter)
		return false;

	if(Minigame)
		return false;

	if(!IsAlive())
		return false;

	return true;
}

// Generates the number of moves until the next battle
void _Character::GenerateNextBattle() {
	NextBattle = ae::GetRandomInt(BATTLE_MINSTEPS, BATTLE_MAXSTEPS);
}

// Generate damage
int _Character::GenerateDamage() {
	return ae::GetRandomInt(MinDamage, MaxDamage);
}

// Get damage power from a type
float _Character::GetDamagePowerMultiplier(int DamageTypeID) {

	switch(DamageTypeID) {
		case 2: return Attributes["PhysicalPower"].Integer * 0.01f;
		case 3: return Attributes["FirePower"].Integer * 0.01f;
		case 4: return Attributes["ColdPower"].Integer * 0.01f;
		case 5: return Attributes["LightningPower"].Integer * 0.01f;
		case 6: return Attributes["PoisonPower"].Integer * 0.01f;
		case 7: return Attributes["BleedPower"].Integer * 0.01f;
	}

	return 1.0f;
}

// Update counts on action bar
void _Character::RefreshActionBarCount() {
	SkillPointsOnActionBar = 0;
	for(size_t i = 0; i < ActionBar.size(); i++) {
		const _Item *Item = ActionBar[i].Item;
		if(Item) {
			if(Item->IsSkill() && HasLearned(Item))
				SkillPointsOnActionBar += Skills[Item->ID];
			else
				ActionBar[i].Count = Object->Inventory->CountItem(Item);
		}
		else
			ActionBar[i].Count = 0;
	}
}

// Return an action struct from an action bar slot
bool _Character::GetActionFromActionBar(_Action &ReturnAction, size_t Slot) {
	if(Slot < ActionBar.size()) {
		ReturnAction.Item = ActionBar[Slot].Item;
		if(!ReturnAction.Item)
			return false;

		// Determine if item is a skill, then look at object's skill levels
		if(ReturnAction.Item->IsSkill() && HasLearned(ReturnAction.Item)) {
			ReturnAction.Level = Skills[ReturnAction.Item->ID];
			if(ReturnAction.Level > 0) {
				ReturnAction.Level += AllSkills;
				if(MaxSkillLevels.find(ReturnAction.Item->ID) != MaxSkillLevels.end())
					ReturnAction.Level = std::min(ReturnAction.Level, MaxSkillLevels.at(ReturnAction.Item->ID));
			}
		}
		else
			ReturnAction.Level = ReturnAction.Item->Level;

		// Set duration for certain items
		ReturnAction.Duration = ReturnAction.Item->Duration;

		return true;
	}

	return false;
}

// Get a vector of summon structs from the character's list of status effects
void _Character::GetSummonsFromBuffs(std::vector<std::pair<_Summon, _StatusEffect *> > &Summons) {
	std::unordered_map<_Item *, int> SummonLimits;
	for(auto &StatusEffect : StatusEffects) {

		// See if the buff has a summon skill attached to it
		_Item *SummonSkill = nullptr;
		if(Object->Scripting->StartMethodCall(StatusEffect->Buff->Script, "GetSummonSkill")) {
			Object->Scripting->MethodCall(0, 1);
			SummonSkill = (_Item *)Object->Scripting->GetPointer(1);
			Object->Scripting->FinishMethodCall();
		}

		// Check for skill
		if(!SummonSkill)
			continue;

		// Find summon skill on action bar
		for(auto &Action : ActionBar) {
			if(Action.Item != SummonSkill)
				continue;

			// Get level of skill
			_ActionResult ActionResult;
			if(!GetActionFromActionBar(ActionResult.ActionUsed, Action.ActionBarSlot))
				break;

			// Get summon stats
			ActionResult.Source.Object = Object;
			ActionResult.Summon.SummonBuff = StatusEffect->Buff;
			Action.Item->Use(Object->Scripting, ActionResult);

			// Get summon limit for skill
			if(SummonLimits.find(SummonSkill) == SummonLimits.end())
				SummonLimits[SummonSkill] = ActionResult.Summon.Limit;

			// Add summons to list
			if(ActionResult.Summon.ID) {
				int AllowedSummons = std::min(StatusEffect->Level, SummonLimits[SummonSkill]);
				for(int i = 0; i < AllowedSummons; i++) {
					SummonLimits[SummonSkill]--;
					Summons.push_back(std::pair<_Summon, _StatusEffect *>(ActionResult.Summon, StatusEffect));
				}
			}
		}
	}
}

// Return true if the object has the skill unlocked
bool _Character::HasLearned(const _Item *Skill) const {
	if(!Skill)
		return false;

	if(Skills.find(Skill->ID) != Skills.end())
		return true;

	return false;
}

// Updates a skill level
void _Character::AdjustSkillLevel(uint32_t SkillID, int Amount) {
	if(SkillID == 0)
		return;

	const _Item *Skill = Object->Stats->Items.at(SkillID);
	if(Skill == nullptr)
		return;

	// Buying
	if(Amount > 0) {

		// Cap points
		int PointsToSpend = std::min(GetSkillPointsAvailable(), Amount);
		PointsToSpend = std::min(PointsToSpend, Skill->MaxLevel - Skills[SkillID]);

		// Update level
		Skills[SkillID] += PointsToSpend;
	}
	else if(Amount < 0) {

		// Update level
		Skills[SkillID] += Amount;
		if(Skills[SkillID] < 0)
			Skills[SkillID] = 0;

		// Update action bar
		if(Skills[SkillID] == 0) {
			for(int i = 0; i < SkillBarSize; i++) {
				if(ActionBar[i].Item == Skill) {
					ActionBar[i].Unset();
					break;
				}
			}
		}
	}
}

// Increase max skill
void _Character::AdjustMaxSkillLevel(uint32_t SkillID, int Amount) {

	// Get skill from database
	const _Item *Skill = Object->Stats->Items.at(SkillID);
	if(Skill == nullptr)
		return;

	// Check for skill unlocked
	if(Skills.find(SkillID) == Skills.end())
		return;

	// Set max skill if absent
	if(MaxSkillLevels.find(SkillID) == MaxSkillLevels.end())
		MaxSkillLevels[SkillID] = GAME_DEFAULT_MAX_SKILL_LEVEL;

	MaxSkillLevels[SkillID] += Amount;
}

// Initialize max skill levels based on learned skills
void _Character::InitializeSkillLevels() {

	// Populate max skill levels map and cap current skill levels
	for(auto &Skill : Skills) {
		auto MaxSkillLevelIterator = MaxSkillLevels.find(Skill.first);
		if(MaxSkillLevelIterator != MaxSkillLevels.end()) {
			Skill.second = std::min(MaxSkillLevelIterator->second, Skill.second);
		}
		else {
			MaxSkillLevels[Skill.first] = std::min(GAME_DEFAULT_MAX_SKILL_LEVEL, Object->Stats->Items.at(Skill.first)->MaxLevel);
		}
	}
}

// Determine if character has unlocked trading
bool _Character::CanTrade() const {
	return Object->ModelID == 7 || Level >= GAME_TRADING_LEVEL;
}

// Reset ui state variables
void _Character::ResetUIState() {
	InventoryOpen = false;
	SkillsOpen = false;
	MenuOpen = false;
	Vendor = nullptr;
	Trader = nullptr;
	Blacksmith = nullptr;
	Enchanter = nullptr;
	Minigame = nullptr;
	TeleportTime = -1.0;
}

// Add status effect, return true if added
bool _Character::AddStatusEffect(_StatusEffect *StatusEffect) {
	if(!StatusEffect)
		return false;

	// Reduce duration of stun with resist
	if(StatusEffect->Buff->Name == "Stunned")
		StatusEffect->Duration *= 1.0f - (Resistances[8] / 100.0f);

	// Find existing buff
	for(auto &ExistingEffect : StatusEffects) {

		// If buff exists, refresh duration
		if(StatusEffect->Buff == ExistingEffect->Buff) {
			if(StatusEffect->Level >= ExistingEffect->Level) {
				double ExistingRemainder = std::fmod(ExistingEffect->Duration, 1.0);
				double Offset = 0.0;
				if(ExistingRemainder > 0.0 && ExistingRemainder < 1.0)
					Offset = 1.0 - ExistingRemainder;

				ExistingEffect->MaxDuration = StatusEffect->MaxDuration;
				ExistingEffect->Duration = StatusEffect->Duration - Offset;
				ExistingEffect->Level = StatusEffect->Level;
				ExistingEffect->Time = Offset;
				ExistingEffect->Source = StatusEffect->Source;
			}

			return false;
		}
	}

	StatusEffects.push_back(StatusEffect);

	return true;
}

// Delete memory used by status effects
void _Character::DeleteStatusEffects() {
	for(auto &StatusEffect : StatusEffects)
		delete StatusEffect;

	StatusEffects.clear();
}

// Clear player unlocks
void _Character::ClearUnlocks() {
	Unlocks.clear();
	SkillPointsUnlocked = 0;
	BeltSize = ACTIONBAR_DEFAULT_BELTSIZE;
	SkillBarSize = ACTIONBAR_DEFAULT_SKILLBARSIZE;
	for(size_t i = 0; i < ActionBar.size(); i++)
		ActionBar[i].Unset();
}

// Unlock items based on search term and count. Return sum of levels
int _Character::UnlockBySearch(const std::string &Search, int Count) {

	std::vector<uint32_t> UnlockIDs;
	UnlockIDs.reserve(Count);

	// Get unlock ids
	ae::_Database *Database = Object->Stats->Database;
	Database->PrepareQuery("SELECT id FROM unlock WHERE name LIKE @search ORDER BY id LIMIT @limit");
	Database->BindString(1, Search);
	Database->BindInt(2, Count);
	while(Database->FetchRow()) {
		uint32_t ID = Database->GetInt<uint32_t>("id");
		UnlockIDs.push_back(ID);

		// Unlock for character
		Unlocks[ID].Level = 1;
	}
	Database->CloseQuery();

	// Get level
	int Sum = 0;
	for(const auto &UnlockID : UnlockIDs) {
		Database->PrepareQuery("SELECT level FROM item WHERE unlock_id = @unlock_id");
		Database->BindInt(1, UnlockID);
		if(Database->FetchRow()) {
			Sum += Database->GetInt<int>("level");
		}
		Database->CloseQuery();
	}

	return Sum;
}

// Return true if the object has the item unlocked
bool _Character::HasUnlocked(const _Item *Item) const {
	if(!Item)
		return false;

	if(Unlocks.find(Item->UnlockID) != Unlocks.end())
		return true;

	return false;
}
