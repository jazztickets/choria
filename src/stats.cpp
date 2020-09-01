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
#include <stats.h>
#include <ae/database.h>
#include <ae/random.h>
#include <ae/assets.h>
#include <objects/object.h>
#include <objects/buff.h>
#include <objects/components/character.h>
#include <objects/components/inventory.h>
#include <objects/components/monster.h>
#include <scripting.h>
#include <constants.h>
#include <algorithm>
#include <iostream>

// Resistances
std::vector<std::string> _Stats::ResistNames = {
	"FireResist",
	"ColdResist",
	"LightningResist",
	"PoisonResist",
	"BleedResist",
	"StunResist",
};

// Constructor
_Stats::_Stats(bool Headless) :
	Headless(Headless) {

	// Load database that stores game data
	Database = new ae::_Database("stats/stats.db", true);

	// Load spreadsheet data
	LoadAttributes();
	LoadMaps();
	LoadEvents();
	LoadLevels();
	LoadBuffs();
	LoadItemTypes();
	LoadTargetTypes();
	LoadDamageTypes();
	LoadItems();
	LoadVendors();
	LoadTraders();
	LoadBlacksmiths();
	LoadEnchanters();
	LoadMinigames();
	LoadModels();
	LoadBuilds();
	LoadScripts();
	LoadSets();
	LoadUnlocks();
	LoadLights();
}

// Destructor
_Stats::~_Stats() {

	for(const auto &Item : Items)
		delete Item.second;

	for(const auto &Buff : Buffs)
		delete Buff.second;

	for(const auto &Build : Builds)
		delete Build.second;

	delete Database;
}

// Load map data
void _Stats::LoadMaps() {

	// Run query
	Database->PrepareQuery("SELECT * FROM map");

	// Get data
	_MapStat Map;
	while(Database->FetchRow()) {
		Map.File = Database->GetString("file");
		Map.Atlas = Database->GetString("atlas");
		Map.Music = Database->GetString("music");
		Map.Outside = Database->GetInt<int>("outside");
		Map.AmbientLight.r = (float)Database->GetReal("ambient_red");
		Map.AmbientLight.g = (float)Database->GetReal("ambient_green");
		Map.AmbientLight.b = (float)Database->GetReal("ambient_blue");
		Map.AmbientLight.a = 1.0f;
		Map.BackgroundMapID = Database->GetInt<uint32_t>("background_id");
		Map.BackgroundOffset.x = (float)Database->GetReal("background_x");
		Map.BackgroundOffset.y = (float)Database->GetReal("background_y");
		Map.BackgroundOffset.z = (float)Database->GetReal("background_z");

		Maps[Database->GetInt<uint32_t>("id")] = Map;
	}
	Database->CloseQuery();
}

// Load event data
void _Stats::LoadEvents() {

	// Run query
	Database->PrepareQuery("SELECT * FROM event");

	// Get data
	_EventName Event;
	while(Database->FetchRow()) {
		Event.Name = Database->GetString("name");
		Event.ShortName = Database->GetString("shortname");
		EventNames.push_back(Event);
	}
	Database->CloseQuery();
}

// Loads level data
void _Stats::LoadLevels() {

	// Run query
	Database->PrepareQuery("SELECT * FROM level");

	// Get data
	_Level Level;
	while(Database->FetchRow()) {
		Level.Level = Database->GetInt<int>("level");
		Level.Experience = Database->GetInt64("experience");
		Level.SkillPoints = Database->GetInt<int>("skillpoints");
		Level.Health = Database->GetInt<int>("health");
		Level.Mana = Database->GetInt<int>("mana");
		Level.RebirthTier = Database->GetInt<int>("rebirth_tier");
		Levels.push_back(Level);
	}
	Database->CloseQuery();

	// Calculate next level
	for(size_t i = 1; i < Levels.size(); i++) {
		Levels[i-1].NextLevel = Levels[i].Experience - Levels[i-1].Experience;
	}

	Levels[Levels.size()-1].NextLevel = 0;
}

// Load buffs
void _Stats::LoadBuffs() {

	// Run query
	Database->PrepareQuery("SELECT * FROM buff");

	// Get data
	while(Database->FetchRow()) {
		_Buff *Buff = new _Buff;
		Buff->ID = Database->GetInt<uint32_t>("id");
		Buff->Name = Database->GetString("name");
		Buff->Script = Database->GetString("script");
		Buff->Texture = ae::Assets.Textures[Database->GetString("texture")];
		Buff->PauseDuringBattle = Database->GetInt<int>("pause");
		Buff->ShowLevel = Database->GetInt<int>("show_level");
		Buff->Dismiss = Database->GetInt<int>("dismiss");
		Buff->WarningTime = (double)Database->GetInt<int>("warning");
		Buffs[Buff->ID] = Buff;
	}
	Database->CloseQuery();

	Buffs[0] = nullptr;
}

// Load item types
void _Stats::LoadItemTypes() {

	// Run query
	Database->PrepareQuery("SELECT * FROM itemtype");

	// Get data
	while(Database->FetchRow()) {
		uint32_t ID = Database->GetInt<uint32_t>("id");
		ItemTypes[ID] = Database->GetString("name");
	}
	Database->CloseQuery();
}

// Load target type strings
void _Stats::LoadTargetTypes() {

	// Run query
	Database->PrepareQuery("SELECT * FROM target");

	// Get data
	while(Database->FetchRow()) {
		uint32_t ID = Database->GetInt<uint32_t>("id");
		TargetTypes[ID] = Database->GetString("name");
	}
	Database->CloseQuery();
}

// Load damage types
void _Stats::LoadDamageTypes() {

	// Run query
	Database->PrepareQuery("SELECT * FROM damagetype");

	// Get data
	while(Database->FetchRow()) {
		_DamageType DamageType;
		uint32_t ID = Database->GetInt<uint32_t>("id");
		DamageType.Name = Database->GetString("name");
		std::string ColorName = Database->GetString("color");
		DamageType.Color = ae::Assets.Colors[ColorName];
		DamageTypes[ID] = DamageType;
	}
	Database->CloseQuery();
}

// Load items
void _Stats::LoadItems() {

	// Run query
	Database->PrepareQuery("SELECT * FROM item");

	// Get data
	while(Database->FetchRow()) {
		uint32_t ItemID = Database->GetInt<uint32_t>("id");
		if(ItemID == 0)
			continue;

		std::string TexturePath = Database->GetString("texture");
		std::string AltTexturePath = Database->GetString("alt_texture");

		_Item *Item = new _Item;
		Item->Stats = this;
		Item->ID = ItemID;
		Item->Name = Database->GetString("name");
		Item->Texture = ae::Assets.Textures[TexturePath];
		Item->AltTexture = ae::Assets.Textures[AltTexturePath];
		Item->Script = Database->GetString("script");
		Item->Proc = Database->GetString("proc");
		Item->Type = (ItemType)Database->GetInt<int>("itemtype_id");
		Item->Category = Database->GetInt<int>("category");
		Item->Level = Database->GetInt<int>("level");
		Item->MaxLevel = Database->GetInt<int>("maxlevel");
		Item->Duration = Database->GetReal("duration");
		Item->Cooldown = Database->GetReal("cooldown");
		Item->Cost = Database->GetInt<int>("cost");
		Item->DamageTypeID = Database->GetInt<uint32_t>("damagetype_id");
		Item->SetID = Database->GetInt<uint32_t>("set_id");
		Item->Attributes["MinDamage"].Int = Database->GetInt<int>("mindamage");
		Item->Attributes["MaxDamage"].Int = Database->GetInt<int>("maxdamage");
		Item->Attributes["Armor"].Int = Database->GetInt<int>("armor");
		Item->Attributes["DamageBlock"].Int = Database->GetInt<int>("block");
		Item->Attributes["Pierce"].Int = Database->GetInt<int>("pierce");
		Item->Attributes["MaxHealth"].Int = Database->GetInt<int>("maxhealth");
		Item->Attributes["MaxMana"].Int = Database->GetInt<int>("maxmana");
		Item->Attributes["HealthRegen"].Int = Database->GetInt<int>("healthregen");
		Item->Attributes["ManaRegen"].Int = Database->GetInt<int>("manaregen");
		Item->Attributes["BattleSpeed"].Int = Database->GetInt<int>("battlespeed");
		Item->Attributes["MoveSpeed"].Int = Database->GetInt<int>("movespeed");
		Item->Attributes["Evasion"].Int = Database->GetInt<int>("evasion");
		Item->Attributes["SpellProc"].Int = Database->GetInt<int>("spellproc");
		Item->Attributes["SpellDamage"].Int = Database->GetInt<int>("spell_damage");
		Item->Attributes["Resist"].Int = Database->GetInt<int>("res");
		Item->Attributes["Cursed"].Int = 0;
		Item->Chance = Database->GetInt<int>("chance");
		Item->ResistanceTypeID = Database->GetInt<uint32_t>("restype_id");
		Item->Tradable = Database->GetInt<int>("tradable");
		Item->TargetAlive = Database->GetInt<int>("target_alive");
		Item->TargetID = (TargetType)Database->GetInt<int>("target_id");
		Item->Scope = (ScopeType)Database->GetInt<int>("scope_id");
		Item->UnlockID = Database->GetInt<uint32_t>("unlock_id");
		Item->Tradable = Database->GetInt<int>("tradable");
		Item->BulkBuy = true;

		// Disable bulk buy on rites
		if(Item->Category == 100 || Item->Type == ItemType::UNLOCKABLE || !Item->IsStackable())
			Item->BulkBuy = false;

		if(!Headless && Item->Texture == nullptr && TexturePath != "")
			throw std::runtime_error("Can't find texture " + TexturePath);
		if(!Headless && Item->AltTexture == nullptr && AltTexturePath != "")
			throw std::runtime_error("Can't find texture " + AltTexturePath);

		Items[Item->ID] = Item;
	}
	Database->CloseQuery();

	Items[0] = nullptr;

	// Load extra attributes
	_Scripting Scripting;
	Scripting.LoadScript(SCRIPTS_DATA);
	Scripting.LoadItemAttributes(this);
}

// Loads vendor data
void _Stats::LoadVendors() {
	Vendors.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM vendor");

	// Get data
	_Vendor Vendor;
	while(Database->FetchRow()) {
		Vendor.ID = Database->GetInt<uint32_t>("id");
		Vendor.Name = Database->GetString("name");
		Vendor.Sort = Database->GetString("sort");
		Vendor.BuyPercent = (float)Database->GetReal("buy_percent");
		Vendor.SellPercent = (float)Database->GetReal("sell_percent");
		Vendor.Items.clear();

		// Set order by for items
		std::string OrderBy = "i.cost";
		if(Vendor.Sort == "set")
			OrderBy = "i.set_id, i.cost";

		// Get items
		Database->PrepareQuery("SELECT item_id FROM vendoritem vi, item i WHERE vi.vendor_id = @vendor_id AND i.id = vi.item_id ORDER BY " + OrderBy, 1);
		Database->BindInt(1, Vendor.ID, 1);
		while(Database->FetchRow(1)) {
			Vendor.Items.push_back(Items[Database->GetInt<uint32_t>("item_id", 1)]);
		}
		Database->CloseQuery(1);

		Vendors[Vendor.ID] = Vendor;
	}
	Database->CloseQuery();
}

// Loads trader data
void _Stats::LoadTraders() {
	Traders.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM trader");

	// Get data
	_Trader Trader;
	while(Database->FetchRow()) {
		Trader.ID = Database->GetInt<uint32_t>("id");
		Trader.Name = Database->GetString("name");
		Trader.RewardItem = Items[Database->GetInt<uint32_t>("item_id")];
		Trader.Upgrades = 0;
		Trader.Count = Database->GetInt<int>("count");
		Trader.Items.clear();

		// Get items
		Database->PrepareQuery("SELECT item_id, count FROM traderitem where trader_id = @trader_id", 1);
		Database->BindInt(1, Trader.ID, 1);
		while(Database->FetchRow(1)) {
			_TraderItem TraderItem;
			TraderItem.Item = Items[Database->GetInt<uint32_t>("item_id", 1)];
			TraderItem.Count = Database->GetInt<int>("count", 1);
			Trader.Items.push_back(TraderItem);
		}
		Database->CloseQuery(1);

		Traders[Trader.ID] = Trader;
	}
	Database->CloseQuery();
}

// Loads blacksmith data
void _Stats::LoadBlacksmiths() {
	Blacksmiths.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM blacksmith");

	// Get data
	_Blacksmith Blacksmith;
	while(Database->FetchRow()) {
		Blacksmith.ID = Database->GetInt<uint32_t>("id");
		Blacksmith.Name = Database->GetString("name");
		Blacksmith.Level = Database->GetInt<int>("level");
		Blacksmiths[Blacksmith.ID] = Blacksmith;
	}
	Database->CloseQuery();
}

// Loads enchanter data
void _Stats::LoadEnchanters() {
	Enchanters.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM enchanter");

	// Get data
	_Enchanter Enchanter;
	while(Database->FetchRow()) {
		Enchanter.ID = Database->GetInt<uint32_t>("id");
		Enchanter.Name = Database->GetString("name");
		Enchanter.Level = Database->GetInt<int>("level");
		Enchanters[Enchanter.ID] = Enchanter;
	}
	Database->CloseQuery();
}

// Load minigames
void _Stats::LoadMinigames() {
	Minigames.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM minigame");

	// Get data
	while(Database->FetchRow()) {
		_MinigameType Minigame;
		Minigame.ID = Database->GetInt<uint32_t>("id");
		Minigame.Name = Database->GetString("name");
		Minigame.Script = Database->GetString("script");
		Minigame.Cost = Database->GetInt<int>("cost");
		Minigame.RequiredItem = Items[Database->GetInt<uint32_t>("item_id")];

		// Get items
		Database->PrepareQuery("SELECT item_id, count FROM minigameitem where minigame_id = @minigame_id", 1);
		Database->BindInt(1, Minigame.ID, 1);
		while(Database->FetchRow(1)) {
			_MinigameItem MinigameItem;
			MinigameItem.Item = Items[Database->GetInt<uint32_t>("item_id", 1)];
			MinigameItem.Count = Database->GetInt<int>("count", 1);
			Minigame.Items.push_back(MinigameItem);
		}
		Database->CloseQuery(1);

		Minigames[Minigame.ID] = Minigame;
	}
	Database->CloseQuery();
}

// Load model textures
void _Stats::LoadModels() {
	Models.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM model");

	// Get data
	_Model Model;
	while(Database->FetchRow()) {
		Model.ID = Database->GetInt<uint32_t>("id");
		Model.Texture = ae::Assets.Textures[Database->GetString("texture")];

		Models[Model.ID] = Model;
	}
	Database->CloseQuery();
}

// Load preset builds
void _Stats::LoadBuilds() {

	// Get build info
	Database->PrepareQuery("SELECT * FROM build");
	while(Database->FetchRow()) {
		uint32_t BuildID = Database->GetInt<uint32_t>("id");
		if(BuildID == 0)
			continue;

		// Create object
		_Object *Object = new _Object();
		Object->CreateComponents();
		Object->Name = std::string("build_") + Database->GetString("name");
		Object->ModelID = Database->GetInt<uint32_t>("model_id");

		// Get items
		Database->PrepareQuery("SELECT * FROM builditem WHERE build_id = @build_id", 1);
		Database->BindInt(1, BuildID, 1);
		while(Database->FetchRow(1)) {
			const _Item *Item = Items[Database->GetInt<uint32_t>("item_id", 1)];
			if(Item)
				Object->Inventory->AddItem(Item, 0, Database->GetInt<int>("count", 1));
		}
		Database->CloseQuery(1);

		// Get skills
		Database->PrepareQuery("SELECT * FROM buildskill WHERE build_id = @build_id", 1);
		Database->BindInt(1, BuildID, 1);
		while(Database->FetchRow(1)) {
			const _Item *Item = Items[Database->GetInt<uint32_t>("item_id", 1)];
			if(Item)
				Object->Character->Skills[Item->ID] = Database->GetInt<int>("level", 1);
		}
		Database->CloseQuery(1);

		// Get actionbar
		Database->PrepareQuery("SELECT * FROM buildactionbar WHERE build_id = @build_id", 1);
		Database->BindInt(1, BuildID, 1);
		while(Database->FetchRow(1)) {
			const _Item *Item = Items[Database->GetInt<uint32_t>("item_id", 1)];
			if(Item) {
				size_t Slot = (size_t)Database->GetInt<uint32_t>("slot", 1);
				if(Slot < Object->Character->ActionBar.size())
					Object->Character->ActionBar[Slot].Item = Item;
			}
		}
		Database->CloseQuery(1);

		// Save build
		Builds[BuildID] = Object;
	}
	Database->CloseQuery();

	Builds[0] = nullptr;
}

// Load scripts
void _Stats::LoadScripts() {
	Scripts.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM script");

	// Get data
	_Script Script;
	while(Database->FetchRow()) {
		Script.ID = Database->GetInt<uint32_t>("id");
		Script.Name = Database->GetString("name");
		Script.Level = Database->GetInt<int>("level");
		Script.Data = Database->GetString("data");

		Scripts[Script.ID] = Script;
	}
	Database->CloseQuery();
}

// Load sets
void _Stats::LoadSets() {
	Sets.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM \"set\"");

	// Get data
	_Set Set;
	while(Database->FetchRow()) {
		Set.ID = Database->GetInt<uint32_t>("id");
		Set.Name = Database->GetString("name");
		Set.Script = Database->GetString("script");
		Set.Count = Database->GetInt<int>("count");

		Sets[Set.ID] = Set;
	}
	Database->CloseQuery();
}

// Load unlocks
void _Stats::LoadUnlocks() {
	Unlocks.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM unlock");

	// Get data
	while(Database->FetchRow()) {
		uint32_t ID = Database->GetInt<uint32_t>("id");
		std::string Name = Database->GetString("name");

		Unlocks[ID] = Name;
	}
	Database->CloseQuery();
}

// Load lights
void _Stats::LoadLights() {
	Lights.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM light");

	// Get data
	_LightType Light;
	while(Database->FetchRow()) {
		Light.ID = Database->GetInt<uint32_t>("id");
		Light.Name = Database->GetString("name");
		Light.Color[0] = Database->GetReal("r");
		Light.Color[1] = Database->GetReal("g");
		Light.Color[2] = Database->GetReal("b");
		Light.Radius = Database->GetReal("radius");

		Lights[Light.ID] = Light;
	}
	Database->CloseQuery();
}

// Gets monsters stats from the database
void _Stats::GetMonsterStats(uint32_t MonsterID, _Object *Object, int Difficulty) const {
	float DifficultyMultiplier = (100 + Difficulty) * 0.01f;
	float DamageMultiplier = 1.0f;
	if(Difficulty >= BATTLE_DIFFICULTY_DAMAGE_START)
		DamageMultiplier += (Difficulty - BATTLE_DIFFICULTY_DAMAGE_START) * BATTLE_DIFFICULTY_DAMAGE * 0.01f;

	Object->Monster->DatabaseID = MonsterID;

	// Run query
	Database->PrepareQuery("SELECT m.*, ai.name as ai_name FROM monster m, ai WHERE m.ai_id = ai.id AND m.id = @monster_id");
	Database->BindInt(1, MonsterID);

	// Get data
	if(Database->FetchRow()) {
		Object->Name = Database->GetString("name");
		Object->Character->Portrait = ae::Assets.Textures[Database->GetString("portrait")];
		Object->Character->BaseMaxHealth = (int)(Database->GetInt<int>("health") * DifficultyMultiplier);
		Object->Character->BaseMaxMana = Database->GetInt<int>("mana");
		Object->Character->BaseMinDamage = Database->GetInt<int>("mindamage") * DamageMultiplier;
		Object->Character->BaseMaxDamage = Database->GetInt<int>("maxdamage") * DamageMultiplier;
		Object->Character->BaseArmor = Database->GetInt<int>("armor");
		Object->Character->BaseDamageBlock = Database->GetInt<int>("block");
		Object->Character->BaseAttackPeriod = Database->GetReal("attackperiod");
		Object->Character->BaseSpellDamage *= DamageMultiplier;
		Object->Monster->ExperienceGiven = Database->GetInt<int>("experience") * DifficultyMultiplier;
		Object->Monster->GoldGiven = Database->GetInt<int>("gold") * DifficultyMultiplier;
		Object->Monster->AI = Database->GetString("ai_name");
		uint32_t BuildID = Database->GetInt<uint32_t>("build_id");

		// Load build
		const auto &BuildIterator = Builds.find(BuildID);
		if(BuildIterator == Builds.end())
			throw std::runtime_error("Can't find build_id " + std::to_string(BuildID));

		const _Object *Build = BuildIterator->second;

		// Copy build
		Object->Inventory->Bags = Build->Inventory->GetBags();
		Object->Character->SkillBarSize = ACTIONBAR_MAX_SKILLBARSIZE;
		Object->Character->BeltSize = ACTIONBAR_MAX_BELTSIZE;
		Object->Character->ActionBar = Build->Character->ActionBar;
		Object->Character->Skills = Build->Character->Skills;
		Object->Character->Attributes["Health"].Int = Object->Character->Attributes["MaxHealth"].Int = Object->Character->BaseMaxHealth;
		Object->Character->Attributes["Mana"].Int = Object->Character->Attributes["MaxMana"].Int = Object->Character->BaseMaxMana;
		Object->Character->Attributes["Gold"].Int = Object->Monster->GoldGiven;
		Object->Character->CalcLevelStats = false;
		Object->Character->BaseResistances["FireResist"] = Database->GetInt<int>("fire_res");
		Object->Character->BaseResistances["ColdResist"] = Database->GetInt<int>("cold_res");
		Object->Character->BaseResistances["LightningResist"] = Database->GetInt<int>("lightning_res");
		Object->Character->BaseResistances["PoisonResist"] = Database->GetInt<int>("poison_res");
		Object->Character->BaseResistances["BleedResist"] = Database->GetInt<int>("bleed_res");
		Object->Character->BaseResistances["StunResist"] = Database->GetInt<int>("stun_res");
	}

	// Free memory
	Database->CloseQuery();
}

// Get list of portraits
void _Stats::GetPortraits(std::list<_Portrait> &Portraits) const {

	// Run query
	Database->PrepareQuery("SELECT * FROM portrait ORDER BY rank");
	while(Database->FetchRow()) {
		_Portrait Portrait;
		Portrait.ID = Database->GetInt<uint32_t>("id");
		Portrait.Texture = ae::Assets.Textures[Database->GetString("texture")];

		Portraits.push_back(Portrait);
	}

	Database->CloseQuery();
}

// Get list of builds
void _Stats::GetStartingBuilds(std::list<_Build> &Builds) const {

	// Run query
	Database->PrepareQuery("SELECT * FROM build WHERE starting = 1 ORDER BY rank");
	while(Database->FetchRow()) {
		_Build Build;
		Build.ID = Database->GetInt<uint32_t>("id");
		Build.Name = Database->GetString("name");
		Build.Texture = ae::Assets.Textures[Database->GetString("texture")];

		Builds.push_back(Build);
	}

	Database->CloseQuery();
}

// Get portrait texture by id
const ae::_Texture *_Stats::GetPortraitImage(uint32_t PortraitID) const {
	const ae::_Texture *Image = nullptr;

	// Run query
	Database->PrepareQuery("SELECT texture FROM portrait WHERE id = @portrait_id");
	Database->BindInt(1, PortraitID);
	if(Database->FetchRow()) {
		Image = ae::Assets.Textures[Database->GetString("texture")];
	}
	Database->CloseQuery();

	return Image;
}

// Get information about zone
void _Stats::GetZone(uint32_t ZoneID, _Zone &Zone) const {

	// Get zone info
	Database->PrepareQuery("SELECT boss, cooldown, minspawn, maxspawn FROM zone WHERE id = @zone_id");
	Database->BindInt(1, ZoneID);
	if(Database->FetchRow()) {
		Zone.Boss = Database->GetInt<int>("boss");
		Zone.Cooldown = Database->GetReal("cooldown");
	}
	Database->CloseQuery();
}

// Randomly generates a list of monsters from a zone
void _Stats::GenerateMonsterListFromZone(int AdditionalCount, float MonsterCountModifier, uint32_t ZoneID, std::list<_Zone> &Monsters, bool &Boss, double &Cooldown) const {
	if(ZoneID == 0)
		return;

	// Get zone info
	Database->PrepareQuery("SELECT boss, cooldown, minspawn, maxspawn FROM zone WHERE id = @zone_id");
	Database->BindInt(1, ZoneID);

	// Get spawn range
	int MinSpawn = 0;
	int MaxSpawn = 0;
	if(Database->FetchRow()) {
		Boss = Database->GetInt<int>("boss");
		Cooldown = Database->GetReal("cooldown");
		MinSpawn = Database->GetInt<int>("minspawn");
		MaxSpawn = Database->GetInt<int>("maxspawn");
	}
	Database->CloseQuery();

	// If boss zone then use odds parameter as monster count
	if(Boss) {

		for(int i = 0; i < (int)MonsterCountModifier; i++) {

			// Run query
			Database->PrepareQuery("SELECT monster_id, odds, difficulty FROM zonedata WHERE zone_id = @zone_id");
			Database->BindInt(1, ZoneID);
			while(Database->FetchRow()) {
				_Zone ZoneData;
				ZoneData.MonsterID = Database->GetInt<uint32_t>("monster_id");
				ZoneData.Difficulty = Database->GetInt<int>("difficulty");
				uint32_t Count = Database->GetInt<uint32_t>("odds");

				// Populate monster list
				for(uint32_t i = 0; i < Count; i++) {
					if(Monsters.size() < BATTLE_MAX_OBJECTS_PER_SIDE)
						Monsters.push_back(ZoneData);
					else
						break;
				}
			}
			Database->CloseQuery();
		}
	}
	else {

		// Get monster count
		int MonsterCount = ae::GetRandomInt(MinSpawn, MaxSpawn);
		MonsterCount *= MonsterCountModifier;

		// No monsters
		if(MonsterCount <= 0)
			return;

		MonsterCount += AdditionalCount;

		// Cap monster count
		MonsterCount = std::min(MonsterCount, BATTLE_MAX_OBJECTS_PER_SIDE);

		// Run query
		Database->PrepareQuery("SELECT * FROM zonedata WHERE zone_id = @zone_id");
		Database->BindInt(1, ZoneID);

		// Get monsters in zone
		std::vector<_Zone> Zone;
		uint32_t OddsSum = 0;
		int MaxTotal = 0;
		bool HasZeroMax = true;
		while(Database->FetchRow()) {

			// Get zone data
			_Zone ZoneData;
			ZoneData.MonsterID = Database->GetInt<uint32_t>("monster_id");
			ZoneData.Odds = Database->GetInt<uint32_t>("odds");
			ZoneData.Max = Database->GetInt<int>("max");
			ZoneData.Difficulty = Database->GetInt<int>("difficulty");

			// Increase max for each player if set
			if(ZoneData.Max > 0) {
				MaxTotal += ZoneData.Max + AdditionalCount;
				ZoneData.Max += AdditionalCount;
			}
			else
				HasZeroMax = false;

			OddsSum += ZoneData.Odds;
			ZoneData.Odds = OddsSum;
			Zone.push_back(ZoneData);
		}
		Database->CloseQuery();

		// Check for monsters in zone
		if(OddsSum > 0) {

			// Cap monster count if all monsters have a max set
			if(HasZeroMax)
				MonsterCount = std::min(MaxTotal, MonsterCount);

			// Generate monsters
			std::unordered_map<uint32_t, int> MonsterTotals;
			while((int)Monsters.size() < MonsterCount) {

				// Find monster in CDT
				uint32_t RandomNumber = ae::GetRandomInt((uint32_t)1, OddsSum);
				for(const auto &ZoneData : Zone) {
					if(RandomNumber <= ZoneData.Odds) {

						// Check monster max
						if(ZoneData.Max == 0 || (ZoneData.Max > 0 && MonsterTotals[ZoneData.MonsterID] < ZoneData.Max)) {
							MonsterTotals[ZoneData.MonsterID]++;
							Monsters.push_back(ZoneData);
						}
						break;
					}
				}
			}
		}
	}
}

// Generates a list of items dropped from a monster
void _Stats::GenerateItemDrops(uint32_t MonsterID, uint32_t Count, std::vector<uint32_t> &ItemDrops, float DropRate) const {
	if(MonsterID == 0)
		return;

	// Run query
	Database->PrepareQuery("SELECT item_id, odds FROM monsterdrop WHERE monster_id = @monster_id");
	Database->BindInt(1, MonsterID);

	// Get list of possible drops and build CDT
	std::vector<_ItemDrop> PossibleItemDrops;
	PossibleItemDrops.reserve(10);
	uint32_t OddsSum = 0;
	uint32_t AddedOdds = 0;
	while(Database->FetchRow()) {
		uint32_t ItemID = Database->GetInt<uint32_t>("item_id");
		uint32_t Odds = 100 * Database->GetInt<uint32_t>("odds");
		if(ItemID) {
			uint32_t IncreasedOdds = Odds * DropRate;
			AddedOdds += IncreasedOdds - Odds;
			Odds = IncreasedOdds;
		}

		OddsSum += Odds;
		PossibleItemDrops.push_back(_ItemDrop(ItemID, OddsSum));
	}
	Database->CloseQuery();

	// Adjust odds by added odds
	OddsSum -= AddedOdds;
	for(auto &DropChance : PossibleItemDrops) {
		DropChance.Odds -= AddedOdds;
	}

	// Check for items
	if(OddsSum <= 0)
		return;

	// Generate items
	for(uint32_t i = 0; i < Count; i++) {
		uint32_t RandomNumber = ae::GetRandomInt((uint32_t)1, OddsSum);

		// Find item id in CDT
		uint32_t ItemID = 0;
		for(auto &MonsterDrop : PossibleItemDrops) {
			if(RandomNumber <= MonsterDrop.Odds) {
				ItemID = MonsterDrop.ItemID;
				break;
			}
		}

		// Populate item list
		if(ItemID)
			ItemDrops.push_back(ItemID);
	}
}

// Get map id by path
uint32_t _Stats::GetMapIDByPath(const std::string &Path) const {

	for(const auto &MapStat : Maps) {
		if(MapStat.second.File == Path) {
			return MapStat.first;
		}
	}

	return 0;
}

// Find a level from the given experience number
const _Level *_Stats::FindLevel(int64_t Experience) const {

	// Search through levels
	for(size_t i = 1; i < Levels.size(); i++) {
		if(Levels[i].Experience > Experience)
			return &Levels[i-1];
	}

	return &Levels[Levels.size()-1];
}

// Load attributes
void _Stats::LoadAttributes() {
	Attributes.clear();
	AttributeRank.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM attribute");

	// Get data
	_Attribute Attribute;
	uint8_t ID = 0;
	while(Database->FetchRow()) {
		Attribute.ID = ID++;
		if(ID == 0)
			throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - Hit attribute limit");
		Attribute.Name = Database->GetString("name");
		Attribute.Label = Database->GetString("label");
		Attribute.Type = (StatValueType)Database->GetInt<int>("valuetype_id");
		Attribute.UpdateType = (StatUpdateType)Database->GetInt<int>("updatetype_id");
		Attribute.UpgradeScale = Database->GetReal("upgrade_scale");
		Attribute.Show = Database->GetInt<int>("show");
		Attribute.Calculate = Database->GetInt<int>("calc");
		Attribute.Save = Database->GetInt<int>("save");
		Attribute.Script = Database->GetInt<int>("script");
		Attribute.Network = Database->GetInt<int>("network");
		switch(Attribute.Type) {
			case StatValueType::BOOLEAN:
			case StatValueType::INTEGER:
			case StatValueType::PERCENT:
				Attribute.Default.Int = Database->GetInt<int>("default");
			break;
			case StatValueType::INTEGER64:
				Attribute.Default.Int64 = Database->GetInt64("default");
			break;
			case StatValueType::FLOAT:
				Attribute.Default.Float = (float)Database->GetReal("default");
			break;
			case StatValueType::POINTER:
				Attribute.Default.Pointer = nullptr;
			break;
			case StatValueType::TIME:
				Attribute.Default.Double = Database->GetReal("default");
			break;
		}

		Attributes[Attribute.Name] = Attribute;
		AttributeRank.push_back(Attribute.Name);
	}
	Database->CloseQuery();
}

// Convert vendor slot from item id
size_t _Vendor::GetSlotFromID(uint32_t ID) const {

	size_t Index = 0;
	for(const auto &Item : Items) {
		if(Item->ID == ID)
			return Index;

		Index++;
	}

	return NOSLOT;
}

// Determine if a blacksmith can upgrade an item
bool _Blacksmith::CanUpgrade(const _Item *Item, int Upgrades) const {
	if(!Item)
	   return false;

	if(!Item->IsEquippable())
		return false;

	if(Upgrades >= Item->MaxLevel)
		return false;

	if(Upgrades >= Level)
		return false;

	return true;
}
