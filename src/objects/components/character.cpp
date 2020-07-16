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

	Gold(0),
	NextBattle(0),
	Invisible(0),
	Stunned(0),
	DiagonalMovement(false),
	LavaProtection(false),
	Hardcore(false),
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

	CalcLevelStats(true),
	Level(0),
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
	AttackPower(0.0f),
	PhysicalPower(0.0f),
	FirePower(0.0f),
	ColdPower(0.0f),
	LightningPower(0.0f),
	BleedPower(0.0f),
	PoisonPower(0.0f),
	PetPower(0.0f),
	HealPower(0.0f),
	ManaPower(0.0f),
	MinDamage(0),
	MaxDamage(0),
	Armor(0),
	DamageBlock(0),
	Pierce(0),
	MoveSpeed(100),
	BattleSpeed(0),
	EquipmentBattleSpeed(0),
	Evasion(0),
	HitChance(100),
	AllSkills(0),
	SummonLimit(0),
	Difficulty(0),

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

		// Update duration
		if(!StatusEffect->Buff->PauseDuringBattle || (StatusEffect->Buff->PauseDuringBattle && !Object->Character->Battle))
			StatusEffect->Duration -= FrameTime;

		// Remove when expired
		if(StatusEffect->Duration <= 0 || !IsAlive()) {
			delete StatusEffect;
			Iterator = StatusEffects.erase(Iterator);

			CalculateStats();
		}
		else
			++Iterator;
	}

	// Update battle cooldowns
	for(auto Iterator = BattleCooldown.begin(); Iterator != BattleCooldown.end(); ) {
		Iterator->second -= FrameTime;

		// Remove cooldown
		if(Iterator->second <= 0.0)
			Iterator = BattleCooldown.erase(Iterator);
		else
			++Iterator;
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
void _Character::UpdateExperience(int Value) {
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
	HealthRegen = 0;
	ManaRegen = 0;
	HealthUpdateMultiplier = 1.0f;
	ManaReductionRatio = 0.0f;
	AttackPower = 1.0f;
	MoveSpeed = 100;
	Evasion = 0;
	HitChance = 100;
	Pierce = 0;
	AllSkills = 0;

	Object->Light = 0;
	Invisible = 0;
	DiagonalMovement = false;
	LavaProtection = false;
	Stunned = 0;
	SummonLimit = 0;
	Difficulty = EternalPain;
	Resistances.clear();

	// Base resistances
	for(int i = 3; i <= 8; i++)
		Resistances[i] = BaseResistances[i];

	// Eternal Strength
	float AllDamage = EternalStrength / 100.0f;
	PhysicalPower = 1.0f + AllDamage;
	FirePower = 1.0f + AllDamage;
	ColdPower = 1.0f + AllDamage;
	LightningPower = 1.0f + AllDamage;
	BleedPower = 1.0f + AllDamage;
	PoisonPower = 1.0f + AllDamage;
	PetPower = 1.0f + AllDamage;

	// Eternal Guard
	if(EternalGuard) {
		Armor += EternalGuard;
		DamageBlock += EternalGuard;
		for(int i = 3; i <= 7; i++)
			Resistances[i] += EternalGuard;
	}

	// Eternal Fortitude
	MaxHealthMultiplier = 1.0f + EternalFortitude / 100.0f;
	HealPower = 1.0f + EternalFortitude / 100.0f;

	// Eternal Spirit
	MaxManaMultiplier = 1.0f + EternalSpirit / 100.0f;
	ManaPower = 1.0f + EternalSpirit / 100.0f;

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
	int ItemArmor = 0;
	int ItemDamageBlock = 0;
	_Bag &EquipmentBag = Object->Inventory->GetBag(BagType::EQUIPMENT);
	for(size_t i = 0; i < EquipmentBag.Slots.size(); i++) {

		// Check each item
		const _Item *Item = EquipmentBag.Slots[i].Item;
		int Upgrades = EquipmentBag.Slots[i].Upgrades;
		if(Item) {
			_ActionResult ActionResult;
			ActionResult.Source.Object = Object;
			Item->GetStats(Object->Scripting, ActionResult);
			CalculateStatBonuses(ActionResult.Source);

			// Add damage
			if(Item->Type != ItemType::SHIELD) {
				ItemMinDamage[Item->DamageTypeID] += Item->GetMinDamage(Upgrades);
				ItemMaxDamage[Item->DamageTypeID] += Item->GetMaxDamage(Upgrades);
			}
			Pierce += Item->GetPierce(Upgrades);

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
			Evasion += Item->GetEvasion(Upgrades);
			AllSkills += Item->GetAllSkills(Upgrades);
			GoldMultiplier += Item->GetGoldBonus(Upgrades) / 100.0f;
			ExperienceMultiplier += Item->GetExpBonus(Upgrades) / 100.0f;

			// Handle all resist
			if(Item->ResistanceTypeID == 1) {
				for(int i = 3; i <= 7; i++)
					Resistances[i] += Item->GetResistance(Upgrades);
			}
			else
				Resistances[Item->ResistanceTypeID] += Item->GetResistance(Upgrades);
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
				Skill->GetStats(Object->Scripting, ActionResult);
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
		StatusEffect->Buff->ExecuteScript(Object->Scripting, "Stats", StatusEffect->Level, StatChange);
		CalculateStatBonuses(StatChange);
	}

	// Get damage
	for(size_t i = 0; i < ItemMinDamage.size(); i++) {
		MinDamage += (int)std::roundf(ItemMinDamage[i] * AttackPower * GetDamagePower(i));
		MaxDamage += (int)std::roundf(ItemMaxDamage[i] * AttackPower * GetDamagePower(i));
	}
	MinDamage = std::max(MinDamage, 0);
	MaxDamage = std::max(MaxDamage, 0);
	Pierce = std::max(Pierce, 0);

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

	// Cap stats
	if(Evasion > GAME_MAX_EVASION)
		Evasion = GAME_MAX_EVASION;
	if(BattleSpeed < BATTLE_MIN_SPEED)
		BattleSpeed = BATTLE_MIN_SPEED;
	if(MoveSpeed < PLAYER_MIN_MOVESPEED)
		MoveSpeed = PLAYER_MIN_MOVESPEED;

	MaxHealth *= MaxHealthMultiplier;
	MaxMana *= MaxManaMultiplier;
	Health = std::min(Health, MaxHealth);
	Mana = std::min(Mana, MaxMana);
	if(HealthRegen > 0)
		HealthRegen *= HealPower;
	if(ManaRegen > 0)
		ManaRegen *= ManaPower;

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
	ExperienceNextLevel = LevelStat->NextLevel;
	ExperienceNeeded = (Level == Object->Stats->GetMaxLevel()) ? 0 : LevelStat->NextLevel - (Experience - LevelStat->Experience);

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

	if(StatChange.HasStat(StatType::MANAREDUCTIONRATIO))
		ManaReductionRatio += StatChange.Values[StatType::MANAREDUCTIONRATIO].Float;
	if(StatChange.HasStat(StatType::HEALTHUPDATEMULTIPLIER))
		HealthUpdateMultiplier += StatChange.Values[StatType::HEALTHUPDATEMULTIPLIER].Float;
	if(StatChange.HasStat(StatType::ATTACKPOWER))
		AttackPower += StatChange.Values[StatType::ATTACKPOWER].Float;
	if(StatChange.HasStat(StatType::PHYSICALPOWER))
		PhysicalPower += StatChange.Values[StatType::PHYSICALPOWER].Float;
	if(StatChange.HasStat(StatType::FIREPOWER))
		FirePower += StatChange.Values[StatType::FIREPOWER].Float;
	if(StatChange.HasStat(StatType::COLDPOWER))
		ColdPower += StatChange.Values[StatType::COLDPOWER].Float;
	if(StatChange.HasStat(StatType::LIGHTNINGPOWER))
		LightningPower += StatChange.Values[StatType::LIGHTNINGPOWER].Float;
	if(StatChange.HasStat(StatType::BLEEDPOWER))
		BleedPower += StatChange.Values[StatType::BLEEDPOWER].Float;
	if(StatChange.HasStat(StatType::POISONPOWER))
		PoisonPower += StatChange.Values[StatType::POISONPOWER].Float;
	if(StatChange.HasStat(StatType::PETPOWER))
		PetPower += StatChange.Values[StatType::PETPOWER].Float;
	if(StatChange.HasStat(StatType::HEALPOWER))
		HealPower += StatChange.Values[StatType::HEALPOWER].Float;
	if(StatChange.HasStat(StatType::MANAPOWER))
		ManaPower += StatChange.Values[StatType::MANAPOWER].Float;
	if(StatChange.HasStat(StatType::SUMMONLIMIT))
		SummonLimit += StatChange.Values[StatType::SUMMONLIMIT].Integer;

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
			for(int i = 3; i <= 7; i++)
				Resistances[i] += StatChange.Values[StatType::RESIST].Integer;
		}
		else
			Resistances[ResistType] += StatChange.Values[StatType::RESIST].Integer;
	}

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

	if(StatChange.HasStat(StatType::INVISIBLE))
		Invisible = StatChange.Values[StatType::INVISIBLE].Integer;

	if(StatChange.HasStat(StatType::LIGHT))
		Object->Light = StatChange.Values[StatType::LIGHT].Integer;

	if(StatChange.HasStat(StatType::DIAGONAL_MOVEMENT))
		DiagonalMovement = StatChange.Values[StatType::DIAGONAL_MOVEMENT].Integer;

	if(StatChange.HasStat(StatType::LAVA_PROTECTION))
		LavaProtection = StatChange.Values[StatType::LAVA_PROTECTION].Integer;

	if(StatChange.HasStat(StatType::DIFFICULTY))
		Difficulty += StatChange.Values[StatType::DIFFICULTY].Integer;
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
float _Character::GetDamagePower(int DamageTypeID) {

	switch(DamageTypeID) {
		case 2: return PhysicalPower;
		case 3: return FirePower;
		case 4: return ColdPower;
		case 5: return LightningPower;
		case 6: return PoisonPower;
		case 7: return BleedPower;
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
			if(ActionResult.Summon.ID) {
				for(int i = 0; i < StatusEffect->Level; i++)
					Summons.push_back(std::pair<_Summon, _StatusEffect *>(ActionResult.Summon, StatusEffect));
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
				ExistingEffect->MaxDuration = ExistingEffect->Duration = StatusEffect->Duration;
				ExistingEffect->Level = StatusEffect->Level;
				ExistingEffect->Time = 0.0;
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

// Unlock items based on search term and count
void _Character::UnlockBySearch(const std::string &Search, int Count) {

	// Get unlock ids
	ae::_Database *Database = Object->Stats->Database;
	Database->PrepareQuery("SELECT id FROM unlock WHERE name LIKE @search ORDER BY id LIMIT @limit");
	Database->BindString(1, Search);
	Database->BindInt(2, Count);
	while(Database->FetchRow()) {
		uint32_t ID = Database->GetInt<uint32_t>("id");
		Unlocks[ID].Level = 1;
	}
	Database->CloseQuery();
}

// Return true if the object has the item unlocked
bool _Character::HasUnlocked(const _Item *Item) const {
	if(!Item)
		return false;

	if(Unlocks.find(Item->UnlockID) != Unlocks.end())
		return true;

	return false;
}
