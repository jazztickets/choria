/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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
#include <objects/components/monster.h>
#include <objects/object.h>
#include <objects/statuseffect.h>
#include <objects/buff.h>
#include <ae/buffer.h>
#include <ae/random.h>
#include <constants.h>
#include <scripting.h>
#include <packet.h>
#include <stats.h>
#include <cmath>

// Constructor
_Character::_Character(_Object *Object) :
	Object(Object),
	CharacterID(0),
	UpdateTimer(0.0),

	Battle(nullptr),
	HUD(nullptr),

	StatusTexture(nullptr),
	Portrait(nullptr),

	Gold(0),
	NextBattle(0),
	Invisible(0),
	Stunned(0),
	Hardcore(false),
	Status(0),

	PlayTime(0.0),
	BattleTime(0.0),
	Deaths(0),
	MonsterKills(0),
	PlayerKills(0),
	GamesPlayed(0),
	Bounty(0),
	GoldLost(0),

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
	BasePierce(0),
	BaseMoveSpeed(100),
	BaseMaxStamina(100),
	BaseStaminaRegen(25.0f),
	BaseStaminaRegenDelay(1.0),
	BaseEvasion(0),
	BaseHitChance(100),
	BaseDropRate(0),

	Health(1),
	MaxHealth(1),
	Mana(0),
	MaxMana(0),
	HealthRegen(0),
	ManaRegen(0),
	HealPower(0.0f),
	AttackPower(0.0f),
	Stamina(0.0f),
	MaxStamina(0.0f),
	StaminaRegen(0.0f),
	StaminaRegenDelay(0.0),
	StaminaRegenTimer(0.0),
	MinDamage(0),
	MaxDamage(0),
	Armor(0),
	DamageBlock(0),
	Pierce(0),
	MoveSpeed(100),
	Evasion(0),
	HitChance(100),
	DropRate(0),

	Vendor(nullptr),
	Trader(nullptr),
	Blacksmith(nullptr),
	Minigame(nullptr),
	Seed(0),

	TradePlayer(nullptr),
	TradeGold(0),
	WaitingForTrade(false),
	TradeAccepted(false),

	LoadMap(nullptr),
	SpawnMap(nullptr),
	TeleportTime(-1),

	MenuOpen(false),
	InventoryOpen(false),
	SkillsOpen(false),

	Bot(false) {

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
			if(Health < MaxHealth && HealthRegen != 0)
				StatChange.Values[StatType::HEALTH].Integer = HealthRegen;
			if(Mana < MaxMana && ManaRegen != 0)
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

		// Reduce count
		StatusEffect->Duration -= FrameTime;
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
}

// Update health
void _Character::UpdateHealth(int &Value) {
	if(Object->Server && Value > 0)
		Value *= HealPower;

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

	// Get level stats
	CalculateLevelStats();

	// Set base stats
	if(!Object->Monster->MonsterStat) {
		BaseMaxHealth = 100;
		BaseMaxMana = 0;
		BaseMinDamage = 1;
		BaseMaxDamage = 2;
		BaseArmor = 0;
		BaseDamageBlock = 0;
	}

	MaxHealth = BaseMaxHealth;
	MaxMana = BaseMaxMana;
	MaxStamina = BaseMaxStamina;
	StaminaRegen = BaseStaminaRegen;
	StaminaRegenDelay = BaseStaminaRegenDelay;
	HealthRegen = BaseHealthRegen;
	ManaRegen = BaseManaRegen;
	HealPower = BaseHealPower;
	AttackPower = BaseAttackPower;
	Evasion = BaseEvasion;
	HitChance = BaseHitChance;
	MinDamage = BaseMinDamage;
	MaxDamage = BaseMaxDamage;
	Armor = BaseArmor;
	DamageBlock = BaseDamageBlock;
	Pierce = BasePierce;
	MoveSpeed = BaseMoveSpeed;
	DropRate = BaseDropRate;
	Resistances.clear();

	Object->Light = 0;
	Invisible = 0;
	Stunned = 0;

	// Get item stats
	int ItemMinDamage = 0;
	int ItemMaxDamage = 0;
	int ItemArmor = 0;
	int ItemDamageBlock = 0;
	float WeaponDamageModifier = 1.0f;
	_Bag &EquipmentBag = Object->Inventory->GetBag(BagType::EQUIPMENT);
	for(size_t i = 0; i < EquipmentBag.Slots.size(); i++) {
		const _BaseItem *Item = EquipmentBag.Slots[i].Item;
		if(!Item)
			continue;

		// Get upgrade level
		int Upgrades = EquipmentBag.Slots[i].Upgrades;

		// Add damage
		if(Item->Type != ItemType::SHIELD) {
			ItemMinDamage += Item->GetMinDamage(Upgrades);
			ItemMaxDamage += Item->GetMaxDamage(Upgrades);
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
		MoveSpeed += Item->GetMoveSpeed(Upgrades);
		DropRate += Item->GetDropRate(Upgrades);

		// Add resistances
		Resistances[Item->ResistanceTypeID] += Item->GetResistance(Upgrades);

		// Get skills unlocked
		if(Item->WeaponType) {
			for(const auto &Skill : Item->WeaponType->Skills) {
				if(Skills.find(Skill->ID) == Skills.end())
					Skills[Skill->ID] = 0;
			}
		}
	}

	// Get skill bonus
	for(size_t i = 0; i < ActionBar.size(); i++) {
		_ActionResult ActionResult;
		ActionResult.Source.Object = Object;
		if(GetActionFromActionBar(ActionResult.ActionUsed, i)) {
			const _Usable *Usable = ActionResult.ActionUsed.Usable;
			if(Usable->IsSkill() && Usable->Target == TargetType::NONE) {

				// Get passive stat changes
				Usable->CallStats(Object->Scripting, ActionResult);
				CalculateStatBonuses(ActionResult.Source);
			}
		}
	}

	// Get buff stats
	for(const auto &StatusEffect : StatusEffects) {
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

	// Cap speed
	if(MoveSpeed < PLAYER_MIN_MOVESPEED)
		MoveSpeed = PLAYER_MIN_MOVESPEED;

	Health = std::min(Health, MaxHealth);
	Mana = std::min(Mana, MaxMana);

	RefreshActionBarCount();
}

// Calculate base level stats
void _Character::CalculateLevelStats() {
	if(!Object->Stats)
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

	if(StatChange.HasStat(StatType::LIGHT))
		Object->Light = StatChange.Values[StatType::LIGHT].Integer;
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

// Update counts on action bar
void _Character::RefreshActionBarCount() {
	for(size_t i = 0; i < ActionBar.size(); i++) {
		const _Usable *Usable = ActionBar[i].Usable;
		if(Usable) {
			if(!Usable->IsSkill())
				ActionBar[i].Count = Object->Inventory->CountItem(Usable->AsItem());
		}
		else
			ActionBar[i].Count = 0;
	}
}

// Return an action struct from an action bar slot
bool _Character::GetActionFromActionBar(_Action &ReturnAction, size_t Slot) {
	if(Slot < ActionBar.size()) {
		ReturnAction.Usable = ActionBar[Slot].Usable;
		if(!ReturnAction.Usable)
			return false;

		// Determine if item is a skill, then look at object's skill levels
		if(ReturnAction.Usable->IsSkill() && HasLearned(ReturnAction.Usable->AsSkill()))
			ReturnAction.Level = Skills[ReturnAction.Usable->ID];
		else
			ReturnAction.Level = ReturnAction.Usable->Level;

		// Set duration for certain items
		ReturnAction.Duration = ReturnAction.Usable->Duration;

		return true;
	}

	return false;
}

// Return true if the object has the skill unlocked
bool _Character::HasLearned(const _Skill *Skill) const {
	if(!Skill)
		return false;

	if(Skills.find(Skill->ID) != Skills.end())
		return true;

	return false;
}

// Reset ui state variables
void _Character::ResetUIState() {
	InventoryOpen = false;
	SkillsOpen = false;
	MenuOpen = false;
	Vendor = nullptr;
	Trader = nullptr;
	Blacksmith = nullptr;
	Minigame = nullptr;
	TeleportTime = -1.0;
}

// Add status effect, return true if added
bool _Character::AddStatusEffect(_StatusEffect *StatusEffect) {
	if(!StatusEffect)
		return false;

	// Find existing buff
	for(auto &ExistingEffect : StatusEffects) {

		// If buff exists, refresh duration
		if(StatusEffect->Buff == ExistingEffect->Buff) {
			if(StatusEffect->Level >= ExistingEffect->Level) {
				ExistingEffect->MaxDuration = ExistingEffect->Duration = StatusEffect->Duration;
				ExistingEffect->Level = StatusEffect->Level;
				ExistingEffect->Time = 0.0;
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

// Return true if the object has the item unlocked
bool _Character::HasUnlocked(const _BaseItem *Item) const {
	if(!Item)
		return false;

	if(Unlocks.find(Item->ID) != Unlocks.end())
		return true;

	return false;
}
