/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2021 Alan Witkowski
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
#include <ae/database.h>
#include <ae/assets.h>
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
	NextBattle(0),
	Invisible(0),
	Hardcore(false),
	Offline(false),
	Status(0),

	CalcLevelStats(true),
	Level(0),
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

	Targets.reserve(BATTLE_MAX_OBJECTS_PER_SIDE);
	ActionBar.resize(ACTIONBAR_MAX_SIZE);
	for(std::size_t i = 0; i < ActionBar.size(); i++)
		ActionBar[i].ActionBarSlot = i;
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
		Attributes[Attribute.second.Name].Int = 0;
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
			if((Attributes["Health"].Int < Attributes["MaxHealth"].Int && Attributes["HealthRegen"].Int > 0) || Attributes["HealthRegen"].Int < 0)
				StatChange.Values["Health"].Int = Attributes["HealthRegen"].Int;
			if((Attributes["Mana"].Int < Attributes["MaxMana"].Int && Attributes["ManaRegen"].Int > 0) || Attributes["ManaRegen"].Int < 0)
				StatChange.Values["Mana"].Int = Attributes["ManaRegen"].Int;

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
		if(StatusEffect->Deleted || (!IsAlive() && !StatusEffect->Buff->Summon)) {
			delete StatusEffect;
			Iterator = StatusEffects.erase(Iterator);

			CalculateStats();
		}
		else
			++Iterator;
	}

	// Update battle cooldowns
	bool SendBossCooldownUpdate = false;
	if(IdleTime <= PLAYER_IDLE_TIME) {
		for(auto Iterator = BossCooldowns.begin(); Iterator != BossCooldowns.end(); ) {
			Iterator->second -= FrameTime;

			// Remove cooldown
			if(Iterator->second <= 0.0) {
				SendBossCooldownUpdate = true;
				Iterator = BossCooldowns.erase(Iterator);
			}
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

	// Notify client of boss being off cooldown
	if(Object->Server && SendBossCooldownUpdate) {
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::PLAYER_BOSSCOOLDOWNS);
		Object->SerializeBossCooldowns(Packet);
		Object->SendPacket(Packet, false);
	}
}

// Update health
void _Character::UpdateHealth(int &Value) {
	if(Object->Server && Value > 0)
		Value *= Attributes["HealthUpdateMultiplier"].Mult();

	Attributes["Health"].Int = std::clamp(Attributes["Health"].Int + Value, 0, Attributes["MaxHealth"].Int);
}

// Update mana
void _Character::UpdateMana(int Value) {
	Attributes["Mana"].Int = std::clamp(Attributes["Mana"].Int + Value, 0, Attributes["MaxMana"].Int);
}

// Update gold amount
void _Character::UpdateGold(int64_t Value) {
	Attributes["Gold"].Int64 = std::clamp(Attributes["Gold"].Int64 + Value, -PLAYER_MAX_GOLD, PLAYER_MAX_GOLD);
}

// Update experience
void _Character::UpdateExperience(int64_t Value) {
	Attributes["Experience"].Int64 = std::max(Attributes["Experience"].Int64 + Value, (int64_t)0);
}

// Update all resistances
void _Character::UpdateAllResist(int Value) {
	for(const auto &Name : _Stats::ResistNames)
		Attributes[Name].Int += Value;
}

// Update elemental resistances
void _Character::UpdateElementalResist(int Value) {
	Attributes["FireResist"].Int += Value;
	Attributes["ColdResist"].Int += Value;
	Attributes["LightningResist"].Int += Value;
}

// Calculates all of the player stats
void _Character::CalculateStats() {
	_Scripting *Scripting = Object->Scripting;

	// Set default values
	for(const auto &Attribute : Object->Stats->Attributes) {
		if(!Attribute.second.Calculate)
			continue;

		Attributes[Attribute.second.Name].Int = Attribute.second.Default.Int;
	}

	// Get base stats
	CalculateLevelStats();
	SkillPoints += SkillPointsUnlocked;

	Attributes["MaxHealth"].Int = BaseMaxHealth;
	Attributes["MaxMana"].Int = BaseMaxMana;
	Attributes["MinDamage"].Int = BaseMinDamage;
	Attributes["MaxDamage"].Int = BaseMaxDamage;
	Attributes["Armor"].Int = BaseArmor;
	Attributes["DamageBlock"].Int = BaseDamageBlock;
	Attributes["SpellDamage"].Int = BaseSpellDamage;

	Object->Light = 0;
	Invisible = 0;
	Attributes["Difficulty"].Int += Attributes["EternalPain"].Int + Attributes["Rebirths"].Int;
	Attributes["RebirthTier"].Int += Attributes["RebirthPower"].Int + Attributes["Evolves"].Int;
	Attributes["EvolveTier"].Int = Attributes["Rebirths"].Int / 10;
	Sets.clear();

	// Base resistances
	for(const auto &Name : _Stats::ResistNames)
		Attributes[Name].Int = BaseResistances[Name];

	// Eternal Strength
	Attributes["PhysicalPower"].Int += Attributes["EternalStrength"].Int;
	Attributes["FirePower"].Int += Attributes["EternalStrength"].Int;
	Attributes["ColdPower"].Int += Attributes["EternalStrength"].Int;
	Attributes["LightningPower"].Int += Attributes["EternalStrength"].Int;
	Attributes["BleedPower"].Int += Attributes["EternalStrength"].Int;
	Attributes["PoisonPower"].Int += Attributes["EternalStrength"].Int;
	Attributes["SummonPower"].Int += Attributes["EternalStrength"].Int;

	// Eternal Guard
	if(Attributes["EternalGuard"].Int) {
		Attributes["DamageBlock"].Int += Attributes["EternalGuard"].Int;
		Attributes["Armor"].Int += Attributes["EternalGuard"].Int / 3;
		UpdateAllResist(Attributes["EternalGuard"].Int / 4);
	}

	// Eternal Fortitude
	Attributes["HealthBonus"].Int += Attributes["EternalFortitude"].Int;
	Attributes["HealPower"].Int += Attributes["EternalFortitude"].Int;

	// Eternal Spirit
	Attributes["ManaBonus"].Int += Attributes["EternalSpirit"].Int;
	Attributes["ManaPower"].Int += Attributes["EternalSpirit"].Int;

	// Eternal Wisdom
	Attributes["ExperienceBonus"].Int += Attributes["EternalWisdom"].Int;

	// Eternal Wealth
	Attributes["GoldBonus"].Int += Attributes["EternalWealth"].Int;

	// Eternal Knowledge
	SkillPoints += Attributes["EternalKnowledge"].Int;

	// Eternal Alacrity
	Attributes["BattleSpeed"].Int = BaseBattleSpeed + Attributes["EternalAlacrity"].Int;

	// Eternal Command
	Attributes["SummonBattleSpeed"].Int += Attributes["EternalCommand"].Int;

	// Eternal Impatience
	Attributes["Cooldowns"].Int -= Attributes["EternalImpatience"].Int;
	Attributes["BossCooldowns"].Int -= Attributes["RebirthSoul"].Int;

	// Eternal Charisma
	Attributes["VendorDiscount"].Int += Attributes["EternalCharisma"].Int;

	// Get item stats
	std::vector<int> ItemMinDamage(Object->Stats->DamageTypes.size(), 0);
	std::vector<int> ItemMaxDamage(Object->Stats->DamageTypes.size(), 0);
	std::unordered_map<uint32_t, std::vector<int>> SetPieceLevels;
	_Bag &EquipmentBag = Object->Inventory->GetBag(BagType::EQUIPMENT);
	for(std::size_t i = 0; i < EquipmentBag.Slots.size(); i++) {
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
			ItemMinDamage[Item->DamageTypeID] += std::floor(Item->GetAttribute("MinDamage", Upgrades));
			ItemMaxDamage[Item->DamageTypeID] += std::floor(Item->GetAttribute("MaxDamage", Upgrades));
		}

		// Stat changes
		Attributes["Armor"].Int += std::floor(Item->GetAttribute("Armor", Upgrades));
		Attributes["DamageBlock"].Int += std::floor(Item->GetAttribute("DamageBlock", Upgrades));
		Attributes["Pierce"].Int += std::floor(Item->GetAttribute("Pierce", Upgrades));
		Attributes["MaxHealth"].Int += std::floor(Item->GetAttribute("MaxHealth", Upgrades));
		Attributes["MaxMana"].Int += std::floor(Item->GetAttribute("MaxMana", Upgrades));
		Attributes["HealthRegen"].Int += std::floor(Item->GetAttribute("HealthRegen", Upgrades));
		Attributes["ManaRegen"].Int += std::floor(Item->GetAttribute("ManaRegen", Upgrades));
		Attributes["BattleSpeed"].Int += std::floor(Item->GetAttribute("BattleSpeed", Upgrades));
		Attributes["MoveSpeed"].Int += std::floor(Item->GetAttribute("MoveSpeed", Upgrades));
		Attributes["AllSkills"].Int += std::floor(Item->GetAttribute("AllSkills", Upgrades));
		Attributes["SpellDamage"].Int += std::floor(Item->GetAttribute("SpellDamage", Upgrades));
		Attributes["Cooldowns"].Int += std::floor(Item->GetCooldownReduction(Upgrades));
		Attributes["ExperienceBonus"].Int += std::floor(Item->GetAttribute("ExperienceBonus", Upgrades));
		Attributes["GoldBonus"].Int += std::floor(Item->GetAttribute("GoldBonus", Upgrades));
		Attributes["AttackPower"].Int += std::floor(Item->GetAttribute("AttackPower", Upgrades));
		Attributes["SummonPower"].Int += std::floor(Item->GetAttribute("SummonPower", Upgrades));
		Attributes["Initiative"].Int += std::floor(Item->GetAttribute("Initiative", Upgrades));
		Attributes["Evasion"].Int = Attributes["Evasion"].Multiplicative(std::floor(Item->GetAttribute("Evasion", Upgrades)));

		// Handle all resist
		if(Item->ResistanceTypeID == 1)
			UpdateAllResist(std::floor(Item->GetAttribute("Resist", Upgrades)));
		else if(Item->ResistanceTypeID == 9)
			UpdateElementalResist(std::floor(Item->GetAttribute("Resist", Upgrades)));
		else
			Attributes[Object->Stats->DamageTypes.at(Item->ResistanceTypeID).Name + "Resist"].Int += std::floor(Item->GetAttribute("Resist", Upgrades));

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

		// Add to networth
		Attributes["EquippedNetworth"].Int64 += Item->GetPrice(Scripting, Object, nullptr, 1, false, Upgrades);
	}

	// Inventory networth
	for(const auto &InventoryBag : Object->Inventory->GetBags()) {
		if(InventoryBag.Type != BagType::INVENTORY && InventoryBag.Type != BagType::TRADE)
			continue;

		for(std::size_t i = 0; i < InventoryBag.Slots.size(); i++) {
			const _Item *Item = InventoryBag.Slots[i].Item;
			if(!Item)
				continue;

			Attributes["InventoryNetworth"].Int64 += Item->GetPrice(Scripting, Object, nullptr, InventoryBag.Slots[i].Count, false, InventoryBag.Slots[i].Upgrades);
		}
	}

	// Add set bonus
	for(auto &SetData : Sets) {

		// Check for completed set
		const _Set &Set = Object->Stats->Sets.at(SetData.first);
		int SetLimit = std::max(1, Set.Count + Attributes["SetLimit"].Int);
		if(SetData.second.EquippedCount < SetLimit) {
			SetData.second.Level = 0;
			continue;
		}

		// Get set level from nth highest level piece, where n is the set count
		auto SetPieceLevelsArray = SetPieceLevels[SetData.first];
		std::sort(SetPieceLevelsArray.begin(), SetPieceLevelsArray.end(), std::greater<int>());
		SetData.second.Level = SetPieceLevelsArray[SetLimit-1];

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
	for(std::size_t i = 0; i < EquipmentBag.Slots.size(); i++) {
		const _Item *Item = EquipmentBag.Slots[i].Item;
		if(!Item || !Item->SetID)
			continue;

		const _SetData &SetData = Sets.at(Item->SetID);
		const _Set &Set = Object->Stats->Sets.at(Item->SetID);
		int SetLimit = std::max(1, Set.Count + Attributes["SetLimit"].Int);
		if(SetData.EquippedCount < SetLimit)
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
	BattleSpeedBeforeBuffs = Attributes["BattleSpeed"].Int;

	// Get buff stats
	for(const auto &StatusEffect : StatusEffects) {
		_StatChange StatChange;
		StatChange.Object = Object;
		StatusEffect->Buff->ExecuteScript(Scripting, "Stats", StatusEffect->Level, StatChange);
		CalculateStatBonuses(StatChange);
	}

	// Get damage
	if(!Object->Monster->DatabaseID) {
		bool HasWeaponDamage = false;
		for(std::size_t i = 0; i < ItemMinDamage.size(); i++) {
			if(ItemMinDamage[i] != 0 || ItemMaxDamage[i] != 0)
				HasWeaponDamage = true;

			Attributes["MinDamage"].Int += (int)std::roundf(ItemMinDamage[i] * Attributes["AttackPower"].Mult() * GetDamagePowerMultiplier(i));
			Attributes["MaxDamage"].Int += (int)std::roundf(ItemMaxDamage[i] * Attributes["AttackPower"].Mult() * GetDamagePowerMultiplier(i));
		}

		// Add fist damage
		if(!HasWeaponDamage) {
			Attributes["MinDamage"].Int = Level;
			Attributes["MaxDamage"].Int = Level + 1;
		}
	}
	else {
		Attributes["MinDamage"].Int *= Attributes["AttackPower"].Mult();
		Attributes["MaxDamage"].Int *= Attributes["AttackPower"].Mult();
	}

	// Cap resistances
	for(const auto &Name : _Stats::ResistNames)
		Attributes[Name].Int = std::clamp(Attributes[Name].Int, GAME_MIN_RESISTANCE, GAME_MAX_RESISTANCE);

	// Get physical resistance from armor
	float ArmorResist = Attributes["Armor"].Int / (30.0f + std::abs(Attributes["Armor"].Int));

	// Physical resist comes solely from armor
	Attributes["PhysicalResist"].Int += (int)(ArmorResist * 100);

	// Cap stats
	Attributes["Evasion"].Int = std::clamp(100 - Attributes["Evasion"].Int, 0, GAME_MAX_EVASION);
	Attributes["MoveSpeed"].Int = std::max(Attributes["MoveSpeed"].Int, PLAYER_MIN_MOVESPEED);
	Attributes["BattleSpeed"].Int = std::max(Attributes["BattleSpeed"].Int, BATTLE_MIN_SPEED);
	Attributes["Cooldowns"].Int = std::max(Attributes["Cooldowns"].Int, 0);

	Attributes["MinDamage"].Int = std::max(Attributes["MinDamage"].Int, 0);
	Attributes["MaxDamage"].Int = std::max(Attributes["MaxDamage"].Int, 0);
	Attributes["Pierce"].Int = std::max(Attributes["Pierce"].Int, 0);
	Attributes["DamageBlock"].Int = std::max(Attributes["DamageBlock"].Int, 0);
	Attributes["ConsumeChance"].Int = std::clamp(Attributes["ConsumeChance"].Int, 5, 100);
	Attributes["ManaShield"].Int = std::clamp(Attributes["ManaShield"].Int, 0, 100);

	Attributes["MaxHealth"].Int *= Attributes["HealthBonus"].BonusMult();
	Attributes["MaxMana"].Int *= Attributes["ManaBonus"].BonusMult();
	Attributes["Health"].Int = std::min(Attributes["Health"].Int, Attributes["MaxHealth"].Int);
	Attributes["Mana"].Int = std::min(Attributes["Mana"].Int, Attributes["MaxMana"].Int);
	if(Attributes["HealthRegen"].Int > 0)
		Attributes["HealthRegen"].Int *= Attributes["HealPower"].Mult();
	if(Attributes["ManaRegen"].Int > 0)
		Attributes["ManaRegen"].Int *= Attributes["ManaPower"].Mult();
	Attributes["BossCooldowns"].Int = std::clamp(Attributes["BossCooldowns"].Int, 0, 100);
	Attributes["VendorDiscount"].Int = std::clamp(Attributes["VendorDiscount"].Int, 0, GAME_MAX_VENDOR_DISCOUNT);

	RefreshActionBarCount();
}

// Calculate base level stats
void _Character::CalculateLevelStats() {
	if(!Object->Stats || !CalcLevelStats)
		return;

	// Cap experience
	const _Level *MaxLevelStat = Object->Stats->GetLevel(Object->Stats->GetMaxLevel());
	Attributes["Experience"].Int64 = std::clamp(Attributes["Experience"].Int64, (int64_t)0, MaxLevelStat->Experience);

	// Find current level
	const _Level *LevelStat = Object->Stats->FindLevel(Attributes["Experience"].Int64);
	Level = LevelStat->Level;
	Attributes["RebirthTier"].Int = LevelStat->RebirthTier;
	ExperienceNextLevel = LevelStat->NextLevel;
	ExperienceNeeded = (Level == Object->Stats->GetMaxLevel()) ? 0 : LevelStat->NextLevel - (Attributes["Experience"].Int64 - LevelStat->Experience);

	// Set base attributes
	BaseMaxHealth = LevelStat->Health;
	BaseMaxMana = LevelStat->Mana;
	BaseMinDamage = 0;
	BaseMaxDamage = 0;
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
						Attributes[Update.first].Int += Update.second.Int;
					break;
					case StatValueType::INTEGER64:
						Attributes[Update.first].Int64 += Update.second.Int64;
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
				Attributes[Update.first].Int = Update.second.Int;
			break;
			case StatUpdateType::MULTIPLICATIVE:
				Attributes[Update.first].Int = Attributes[Update.first].Multiplicative(Update.second.Int);
			break;
		}
	}

	if(StatChange.HasStat("Invisible"))
		Invisible = StatChange.Values["Invisible"].Int;
	if(StatChange.HasStat("Light"))
		Object->Light = StatChange.Values["Light"].Int;

	if(StatChange.HasStat("AllResist")) {
		UpdateAllResist(StatChange.Values["AllResist"].Int);
	}
	if(StatChange.HasStat("ElementalResist")) {
		UpdateElementalResist(StatChange.Values["ElementalResist"].Int);
	}
}

// Get percentage to next level
float _Character::GetNextLevelPercent() const {
	float Percent = 0;

	if(ExperienceNextLevel > 0)
		Percent = 1.0f - (float)ExperienceNeeded / ExperienceNextLevel;

	return Percent;
}

// Get adjusted item cost
int64_t _Character::GetItemCost(int64_t ItemCost) {
	if(Attributes["VendorDiscount"].Int > 0) {
		ItemCost *= (100 - Attributes["VendorDiscount"].Int) * 0.01;
		if(ItemCost <= 0)
			ItemCost = 1;
	}

	return ItemCost;
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
	if(Attributes["Attractant"].Int)
		NextBattle = Attributes["Attractant"].Int;
	else
		NextBattle = ae::GetRandomInt(BATTLE_MINSTEPS, BATTLE_MAXSTEPS);
}

// Generate damage
int _Character::GenerateDamage() {
	return ae::GetRandomInt(Attributes["MinDamage"].Int, Attributes["MaxDamage"].Int);
}

// Get damage power from a type
float _Character::GetDamagePowerMultiplier(int DamageTypeID) {

	switch(DamageTypeID) {
		case 2: return Attributes["PhysicalPower"].Mult();
		case 3: return Attributes["FirePower"].Mult();
		case 4: return Attributes["ColdPower"].Mult();
		case 5: return Attributes["LightningPower"].Mult();
		case 6: return Attributes["PoisonPower"].Mult();
		case 7: return Attributes["BleedPower"].Mult();
	}

	return 1.0f;
}

// Update counts on action bar
void _Character::RefreshActionBarCount() {
	SkillPointsOnActionBar = 0;
	for(std::size_t i = 0; i < ActionBar.size(); i++) {
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
bool _Character::GetActionFromActionBar(_Action &ReturnAction, std::size_t Slot) {
	if(Slot < ActionBar.size()) {
		ReturnAction.Item = ActionBar[Slot].Item;
		if(!ReturnAction.Item)
			return false;

		// Determine if item is a skill, then look at object's skill levels
		if(ReturnAction.Item->IsSkill() && HasLearned(ReturnAction.Item)) {
			ReturnAction.Level = Skills[ReturnAction.Item->ID];
			if(ReturnAction.Level > 0) {
				ReturnAction.Level += Attributes["AllSkills"].Int;
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
			ActionResult.Summons.reserve(BATTLE_MAX_OBJECTS_PER_SIDE);
			if(!GetActionFromActionBar(ActionResult.ActionUsed, Action.ActionBarSlot))
				break;

			// Get summon stats
			ActionResult.Source.Object = Object;
			ActionResult.SummonBuff = StatusEffect->Buff;
			Action.Item->Use(Object->Scripting, ActionResult);

			// Get summon limit for skill
			if(SummonLimits.find(SummonSkill) == SummonLimits.end())
				SummonLimits[SummonSkill] = ActionResult.Summons[0].Limit;

			// Add summons to list
			if(ActionResult.Summons[0].ID) {
				int AllowedSummons = std::min(StatusEffect->Level, SummonLimits[SummonSkill]);
				for(int i = 0; i < AllowedSummons; i++) {
					SummonLimits[SummonSkill]--;
					Summons.push_back(std::pair<_Summon, _StatusEffect *>(ActionResult.Summons[0], StatusEffect));
				}
			}
		}
	}
}

// Determine if a skill can be equipped from the skill screen
bool _Character::CanEquipSkill(const _Item *Skill) {

	// Find existing action
	for(int i = 0; i < SkillBarSize; i++) {
		if(ActionBar[i].Item == Skill)
			return false;
	}

	// Find an empty slot
	for(int i = 0; i < SkillBarSize; i++) {
		if(!ActionBar[i].IsSet())
			return true;
	}

	return false;
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
void _Character::AdjustSkillLevel(uint32_t SkillID, int Amount, bool SoftMax) {
	if(SkillID == 0)
		return;

	const _Item *Skill = Object->Stats->Items.at(SkillID);
	if(Skill == nullptr)
		return;

	// Buying
	if(Amount > 0) {

		// Get max level for skill
		int MaxLevel = std::min(MaxSkillLevels[SkillID], Skill->MaxLevel);

		// Get number of points until max level is hit
		int PointsToMax = MaxLevel - Skills[SkillID];
		if(SoftMax) {
			PointsToMax -= Attributes["AllSkills"].Int;

			// All skills brings it over the max, so give one point at level 0 only
			if(PointsToMax < 0)
				PointsToMax = Skills[SkillID] == 0 ? 1 : 0;
		}

		// Cap points to spend
		int PointsToSpend = std::min(GetSkillPointsAvailable(), Amount);
		PointsToSpend = std::min(PointsToSpend, PointsToMax);

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
void _Character::ResetUIState(bool ResetMenuState) {
	InventoryOpen = false;
	SkillsOpen = false;
	Vendor = nullptr;
	Trader = nullptr;
	Blacksmith = nullptr;
	Enchanter = nullptr;
	Minigame = nullptr;
	TeleportTime = -1.0;
	if(ResetMenuState)
		MenuOpen = false;
}

// Add status effect, return true if added
bool _Character::AddStatusEffect(_StatusEffect *StatusEffect) {
	if(!StatusEffect)
		return false;

	// Reduce duration of stun/slow with resist
	if(StatusEffect->Buff->Name == "Stunned" || StatusEffect->Buff->Name == "Slowed" || StatusEffect->Buff->Name == "Taunted") {
		StatusEffect->Duration *= 1.0f - Attributes["StunResist"].Mult();
		StatusEffect->MaxDuration *= 1.0f - Attributes["StunResist"].Mult();
	}

	// Find existing buff
	for(auto &ExistingEffect : StatusEffects) {
		if(StatusEffect->Buff != ExistingEffect->Buff)
			continue;

		// Add to level of summon buff
		if(StatusEffect->Buff->Summon) {
			ExistingEffect->Level += StatusEffect->Level;
		}
		// Update duration
		else if(StatusEffect->Level >= ExistingEffect->Level) {
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

	StatusEffects.push_back(StatusEffect);

	return true;
}

// Delete memory used by status effects
void _Character::DeleteStatusEffects() {
	for(auto &StatusEffect : StatusEffects)
		delete StatusEffect;

	StatusEffects.clear();
}

// Update status of character
uint8_t _Character::GetStatus() {
	if(!IsAlive())
		return STATUS_DEAD;
	else if(Battle)
		return STATUS_BATTLE;
	else if(WaitingForTrade)
		return STATUS_TRADE;
	else if(Vendor)
		return STATUS_VENDOR;
	else if(Trader)
		return STATUS_TRADER;
	else if(Blacksmith)
		return STATUS_BLACKSMITH;
	else if(Enchanter)
		return STATUS_SKILLS;
	else if(Minigame)
		return STATUS_MINIGAME;
	else if(InventoryOpen)
		return STATUS_INVENTORY;
	else if(SkillsOpen)
		return STATUS_SKILLS;
	else if(MenuOpen)
		return STATUS_MENU;
	else if(TeleportTime > 0)
		return STATUS_TELEPORT;

	return STATUS_NONE;
}

// Update status texture from status
void _Character::UpdateStatusTexture() {

	switch(Status) {
		case _Character::STATUS_NONE:
			Object->Character->StatusTexture = nullptr;
		break;
		case _Character::STATUS_MENU:
			Object->Character->StatusTexture = ae::Assets.Textures["textures/status/pause.png"];
		break;
		case _Character::STATUS_INVENTORY:
			Object->Character->StatusTexture = ae::Assets.Textures["textures/status/bag.png"];
		break;
		case _Character::STATUS_VENDOR:
			Object->Character->StatusTexture = ae::Assets.Textures["textures/status/vendor.png"];
		break;
		case _Character::STATUS_SKILLS:
			Object->Character->StatusTexture = ae::Assets.Textures["textures/status/skills.png"];
		break;
		case _Character::STATUS_TRADE:
			Object->Character->StatusTexture = ae::Assets.Textures["textures/status/trade.png"];
		break;
		case _Character::STATUS_TRADER:
			Object->Character->StatusTexture = ae::Assets.Textures["textures/status/vendor.png"];
		break;
		case _Character::STATUS_BLACKSMITH:
			Object->Character->StatusTexture = ae::Assets.Textures["textures/status/vendor.png"];
		break;
		case _Character::STATUS_MINIGAME:
			Object->Character->StatusTexture = ae::Assets.Textures["textures/status/vendor.png"];
		break;
		case _Character::STATUS_TELEPORT:
			Object->Character->StatusTexture = ae::Assets.Textures["textures/status/teleport.png"];
		break;
		case _Character::STATUS_BATTLE:
			Object->Character->StatusTexture = ae::Assets.Textures["textures/status/battle.png"];
		break;
		case _Character::STATUS_DEAD:
			Object->Character->StatusTexture = ae::Assets.Textures["textures/status/dead.png"];
		break;
		default:
		break;
	}
}

// Clear player unlocks
void _Character::ClearUnlocks() {
	Unlocks.clear();
	SkillPointsUnlocked = 0;
	BeltSize = ACTIONBAR_DEFAULT_BELTSIZE;
	SkillBarSize = ACTIONBAR_DEFAULT_SKILLBARSIZE;
	for(std::size_t i = 0; i < ActionBar.size(); i++)
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
