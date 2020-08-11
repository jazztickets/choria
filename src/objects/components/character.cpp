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
#include <stdexcept>
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
	Hardcore(false),
	Offline(false),
	Status(0),

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
	BaseSpellDamage(100),
	BaseAttackPeriod(BATTLE_DEFAULTATTACKPERIOD),

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

// Initialize attributes
void _Character::Init() {
	if(!Object->Stats)
		throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " Object->Stats is null");

	// Set attributes
	for(const auto &Attribute : Object->Stats->Attributes)
		Attributes[Attribute.second.Name].Integer = 0;
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
			if((Attributes["Health"].Integer < Attributes["MaxHealth"].Integer && Attributes["HealthRegen"].Integer > 0) || Attributes["HealthRegen"].Integer < 0)
				StatChange.Values["Health"].Integer = Attributes["HealthRegen"].Integer;
			if((Attributes["Mana"].Integer < Attributes["MaxMana"].Integer && Attributes["ManaRegen"].Integer > 0) || Attributes["ManaRegen"].Integer < 0)
				StatChange.Values["Mana"].Integer = Attributes["ManaRegen"].Integer;

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
		Value *= Attributes["HealthUpdateMultiplier"].Integer * 0.01f;

	Attributes["Health"].Integer = std::clamp(Attributes["Health"].Integer + Value, 0, Attributes["MaxHealth"].Integer);
}

// Update mana
void _Character::UpdateMana(int Value) {
	Attributes["Mana"].Integer = std::clamp(Attributes["Mana"].Integer + Value, 0, Attributes["MaxMana"].Integer);
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
	for(const auto &Attribute : Object->Stats->Attributes) {
		if(!Attribute.second.Calculate)
			continue;

		Attributes[Attribute.second.Name].Integer = Attribute.second.Default.Integer;
	}

	// Get base stats
	CalculateLevelStats();
	SkillPoints += SkillPointsUnlocked;

	Attributes["MaxHealth"].Integer = BaseMaxHealth;
	Attributes["MaxMana"].Integer = BaseMaxMana;
	Attributes["MinDamage"].Integer = BaseMinDamage;
	Attributes["MaxDamage"].Integer = BaseMaxDamage;
	Attributes["Armor"].Integer = BaseArmor;
	Attributes["DamageBlock"].Integer = BaseDamageBlock;
	Attributes["SpellDamage"].Integer = BaseSpellDamage;

	Object->Light = 0;
	Invisible = 0;
	Attributes["Difficulty"].Integer += Attributes["EternalPain"].Integer + Attributes["Rebirths"].Integer;
	Attributes["RebirthTier"].Integer += Attributes["RebirthPower"].Integer;
	Resistances.clear();
	Sets.clear();

	// Base resistances
	for(int i = GAME_ALL_RESIST_START_ID; i <= GAME_ALL_RESIST_END_ID; i++)
		Resistances[i] = BaseResistances[i];

	// Eternal Strength
	Attributes["PhysicalPower"].Integer += Attributes["EternalStrength"].Integer;
	Attributes["FirePower"].Integer += Attributes["EternalStrength"].Integer;
	Attributes["ColdPower"].Integer += Attributes["EternalStrength"].Integer;
	Attributes["LightningPower"].Integer += Attributes["EternalStrength"].Integer;
	Attributes["BleedPower"].Integer += Attributes["EternalStrength"].Integer;
	Attributes["PoisonPower"].Integer += Attributes["EternalStrength"].Integer;
	Attributes["PetPower"].Integer += Attributes["EternalStrength"].Integer;

	// Eternal Guard
	if(Attributes["EternalGuard"].Integer) {
		Attributes["DamageBlock"].Integer += Attributes["EternalGuard"].Integer;
		Attributes["Armor"].Integer += Attributes["EternalGuard"].Integer / 3;
		for(int i = GAME_ALL_RESIST_START_ID; i <= GAME_ALL_RESIST_END_ID; i++)
			Resistances[i] += Attributes["EternalGuard"].Integer / 4;
	}

	// Eternal Fortitude
	Attributes["HealthBonus"].Integer += Attributes["EternalFortitude"].Integer;
	Attributes["HealPower"].Integer += Attributes["EternalFortitude"].Integer;

	// Eternal Spirit
	Attributes["ManaBonus"].Integer += Attributes["EternalSpirit"].Integer;
	Attributes["ManaPower"].Integer += Attributes["EternalSpirit"].Integer;

	// Eternal Wisdom
	Attributes["ExperienceBonus"].Integer += Attributes["EternalWisdom"].Integer;

	// Eternal Wealth
	Attributes["GoldBonus"].Integer += Attributes["EternalWealth"].Integer;

	// Eternal Alacrity
	Attributes["BattleSpeed"].Integer = BaseBattleSpeed + Attributes["EternalAlacrity"].Integer;

	// Eternal Knowledge
	SkillPoints += Attributes["EternalKnowledge"].Integer;

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
		Attributes["Armor"].Integer += Item->GetArmor(Upgrades);
		Attributes["DamageBlock"].Integer += Item->GetDamageBlock(Upgrades);
		Attributes["Pierce"].Integer += Item->GetPierce(Upgrades);
		Attributes["MaxHealth"].Integer += Item->GetMaxHealth(Upgrades);
		Attributes["MaxMana"].Integer += Item->GetMaxMana(Upgrades);
		Attributes["HealthRegen"].Integer += Item->GetHealthRegen(Upgrades);
		Attributes["ManaRegen"].Integer += Item->GetManaRegen(Upgrades);
		Attributes["BattleSpeed"].Integer += Item->GetBattleSpeed(Upgrades);
		Attributes["MoveSpeed"].Integer += Item->GetMoveSpeed(Upgrades);
		Attributes["Evasion"].Integer += Item->GetEvasion(Upgrades);
		Attributes["AllSkills"].Integer += Item->GetAllSkills(Upgrades);
		Attributes["SpellDamage"].Integer += Item->GetSpellDamage(Upgrades);
		Attributes["Cooldown"].Integer += Item->GetCooldownReduction(Upgrades);
		Attributes["ExperienceBonus"].Integer += Item->GetExperienceBonus(Upgrades);
		Attributes["GoldBonus"].Integer += Item->GetGoldBonus(Upgrades);

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
			Scripting->GetStatChange(1, Object->Stats, StatChange);
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
			Scripting->GetStatChange(1, Object->Stats, StatChange);
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
	EquipmentBattleSpeed = Attributes["BattleSpeed"].Integer;

	// Get buff stats
	for(const auto &StatusEffect : StatusEffects) {
		_StatChange StatChange;
		StatChange.Object = Object;
		StatusEffect->Buff->ExecuteScript(Scripting, "Stats", StatusEffect->Level, StatChange);
		CalculateStatBonuses(StatChange);
	}

	// Get damage
	for(size_t i = 0; i < ItemMinDamage.size(); i++) {
		Attributes["MinDamage"].Integer += (int)std::roundf(ItemMinDamage[i] * Attributes["AttackPower"].Integer * 0.01f * GetDamagePowerMultiplier(i));
		Attributes["MaxDamage"].Integer += (int)std::roundf(ItemMaxDamage[i] * Attributes["AttackPower"].Integer * 0.01f * GetDamagePowerMultiplier(i));
	}

	// Cap resistances
	for(auto &Resist : Resistances) {
		Resist.second = std::min(Resist.second, GAME_MAX_RESISTANCE);
		Resist.second = std::max(Resist.second, GAME_MIN_RESISTANCE);
	}

	// Get physical resistance from armor
	float ArmorResist = Attributes["Armor"].Integer / (30.0f + std::abs(Attributes["Armor"].Integer));

	// Physical resist comes solely from armor
	Resistances[2] = (int)(ArmorResist * 100);

	// Cap stats
	Attributes["Evasion"].Integer = std::clamp(Attributes["Evasion"].Integer, 0, GAME_MAX_EVASION);
	Attributes["MoveSpeed"].Integer = std::max(Attributes["MoveSpeed"].Integer, PLAYER_MIN_MOVESPEED);
	Attributes["BattleSpeed"].Integer = std::max(Attributes["BattleSpeed"].Integer, BATTLE_MIN_SPEED);
	Attributes["Cooldown"].Integer = std::max(Attributes["Cooldown"].Integer, 0);

	Attributes["MinDamage"].Integer = std::max(Attributes["MinDamage"].Integer, 0);
	Attributes["MaxDamage"].Integer = std::max(Attributes["MaxDamage"].Integer, 0);
	Attributes["Pierce"].Integer = std::max(Attributes["Pierce"].Integer, 0);
	Attributes["DamageBlock"].Integer = std::max(Attributes["DamageBlock"].Integer, 0);
	Attributes["ConsumeChance"].Integer = std::clamp(Attributes["ConsumeChance"].Integer, 0, 100);
	Attributes["EnergyField"].Integer = std::clamp(100 - Attributes["EnergyField"].Integer, 0, 100);

	Attributes["MaxHealth"].Integer *= Attributes["HealthBonus"].Integer * 0.01f;
	Attributes["MaxMana"].Integer *= Attributes["ManaBonus"].Integer * 0.01f;
	Attributes["Health"].Integer = std::min(Attributes["Health"].Integer, Attributes["MaxHealth"].Integer);
	Attributes["Mana"].Integer = std::min(Attributes["Mana"].Integer, Attributes["MaxMana"].Integer);
	if(Attributes["HealthRegen"].Integer > 0)
		Attributes["HealthRegen"].Integer *= Attributes["HealPower"].Integer * 0.01f;
	if(Attributes["ManaRegen"].Integer > 0)
		Attributes["ManaRegen"].Integer *= Attributes["ManaPower"].Integer * 0.01f;

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
	Attributes["RebirthTier"].Integer = LevelStat->RebirthTier;
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

	// Update attributes
	for(const auto &Update : StatChange.Values) {
		const auto &Attribute = Object->Stats->Attributes.at(Update.first);
		switch(Attribute.UpdateType) {
			case StatUpdateType::NONE:
				continue;
			break;
			case StatUpdateType::ADD: {
				switch(Attribute.Type) {
					case StatValueType::INTEGER:
					case StatValueType::PERCENT:
						Attributes[Update.first].Integer += Update.second.Integer;
					break;
					case StatValueType::FLOAT:
						Attributes[Update.first].Float += Update.second.Float;
					break;
					default:
						throw std::runtime_error("Bad update type: " + Update.first);
					break;
				}
			} break;
			case StatUpdateType::SET:
				Attributes[Update.first].Integer = Update.second.Integer;
			break;
			case StatUpdateType::MULTIPLICATIVE:
				Attributes[Update.first].Integer = (Attributes[Update.first].Integer * 0.01f * (100 - Update.second.Integer) * 0.01f) * 100;
			break;
		}
	}

	if(StatChange.HasStat("Invisible"))
		Invisible = StatChange.Values["Invisible"].Integer;
	if(StatChange.HasStat("Light"))
		Object->Light = StatChange.Values["Light"].Integer;

	if(StatChange.HasStat("AllResist")) {
		for(int i = GAME_ALL_RESIST_START_ID; i <= GAME_ALL_RESIST_END_ID; i++)
			Resistances[i] += StatChange.Values["AllResist"].Integer;
	}
	if(StatChange.HasStat("PhysicalResist"))
		Resistances[2] += StatChange.Values["PhysicalResist"].Integer;
	if(StatChange.HasStat("FireResist"))
		Resistances[3] += StatChange.Values["FireResist"].Integer;
	if(StatChange.HasStat("ColdResist"))
		Resistances[4] += StatChange.Values["ColdResist"].Integer;
	if(StatChange.HasStat("LightningResist"))
		Resistances[5] += StatChange.Values["LightningResist"].Integer;
	if(StatChange.HasStat("PoisonResist"))
		Resistances[6] += StatChange.Values["PoisonResist"].Integer;
	if(StatChange.HasStat("BleedResist"))
		Resistances[7] += StatChange.Values["BleedResist"].Integer;
	if(StatChange.HasStat("StunResist"))
		Resistances[8] += StatChange.Values["StunResist"].Integer;
	if(StatChange.HasStat("ElementalResist")) {
		for(int i = GAME_ELEMENTAL_RESIST_START_ID; i <= GAME_ELEMENTAL_RESIST_END_ID; i++)
			Resistances[i] += StatChange.Values["ElementalResist"].Integer;
	}
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
	return ae::GetRandomInt(Attributes["MinDamage"].Integer, Attributes["MaxDamage"].Integer);
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
				ReturnAction.Level += Attributes["AllSkills"].Integer;
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
