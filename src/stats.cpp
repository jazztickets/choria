/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2015  Alan Witkowski
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
#include <objects/object.h>
#include <objects/buff.h>
#include <objects/inventory.h>
#include <constants.h>
#include <database.h>
#include <random.h>
#include <assets.h>
#include <iostream>

// Constructor
_Stats::_Stats() {

	// Load database that stores game data
	Database = new _Database("stats/stats.db");

	// Load spreadsheet data
	LoadMaps();
	LoadEvents();
	LoadLevels();
	LoadBuffs();
	LoadTargetTypes();
	LoadItemTypes();
	LoadItems();
	LoadVendors();
	LoadTraders();
	LoadBuilds();
	LoadScripts();
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
		Map.File = std::string("maps/") + Database->GetString("file");
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
		Level.Experience = Database->GetInt<int>("experience");
		Level.SkillPoints = Database->GetInt<int>("skillpoints");
		Level.Health = Database->GetInt<int>("health");
		Level.Mana = Database->GetInt<int>("mana");
		Level.Damage = Database->GetInt<int>("damage");
		Level.Defense = Database->GetInt<int>("defense");
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
		Buff->Texture = Assets.Textures[Database->GetString("texture")];
		Buffs[Buff->ID] = Buff;
	}
	Database->CloseQuery();

	Buffs[0] = nullptr;
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

// Load items
void _Stats::LoadItems() {

	// Run query
	Database->PrepareQuery("SELECT * FROM item");

	// Get data
	while(Database->FetchRow()) {
		uint32_t ItemID = Database->GetInt<uint32_t>("id");
		if(ItemID == 0)
			continue;

		_Item *Item = new _Item;
		Item->ID = ItemID;
		Item->Name = Database->GetString("name");
		Item->Texture = Assets.Textures[Database->GetString("texture")];
		Item->Script = Database->GetString("script");
		Item->Type = (ItemType)Database->GetInt<int>("itemtype_id");
		Item->Level = Database->GetInt<int>("level");
		Item->MaxLevel = Database->GetInt<int>("maxlevel");
		Item->LevelRequired = Database->GetInt<int>("levelrequired");
		Item->Cost = Database->GetInt<int>("cost");
		Item->DamageType = Database->GetInt<int>("damagetype_id");
		Item->MinDamage = Database->GetInt<int>("mindamage");
		Item->MaxDamage = Database->GetInt<int>("maxdamage");
		Item->MinDefense = Database->GetInt<int>("mindefense");
		Item->MaxDefense = Database->GetInt<int>("maxdefense");
		Item->MaxHealth = Database->GetInt<int>("maxhealth");
		Item->MaxMana = Database->GetInt<int>("maxmana");
		Item->HealthRegen = (float)Database->GetReal("healthregen");
		Item->ManaRegen = (float)Database->GetReal("manaregen");
		Item->BattleSpeed = Database->GetReal("battlespeed");
		Item->Tradable = Database->GetInt<int>("tradable");
		Item->TargetAlive = Database->GetInt<int>("target_alive");
		Item->TargetID = (TargetType)Database->GetInt<int>("target_id");
		Item->Scope = (ScopeType)Database->GetInt<int>("scope_id");
		Item->UnlockID = Database->GetInt<uint32_t>("unlock_id");
		Items[Item->ID] = Item;
	}
	Database->CloseQuery();

	Items[0] = nullptr;
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
		Vendor.BuyPercent = (float)Database->GetReal("buy_percent");
		Vendor.SellPercent = (float)Database->GetReal("sell_percent");
		Vendor.Items.clear();

		// Get items
		Database->PrepareQuery("SELECT item_id FROM vendoritem vi, item i where vi.vendor_id = @vendor_id and i.id = vi.item_id order by i.cost", 1);
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
		Trader.Count = Database->GetInt<int>("count");
		Trader.TraderItems.clear();

		// Get items
		Database->PrepareQuery("SELECT item_id, count FROM traderitem where trader_id = @trader_id", 1);
		Database->BindInt(1, Trader.ID, 1);
		while(Database->FetchRow(1)) {
			_TraderItem TraderItem;
			TraderItem.Item = Items[Database->GetInt<uint32_t>("item_id", 1)];
			TraderItem.Count = Database->GetInt<int>("count", 1);
			Trader.TraderItems.push_back(TraderItem);
		}
		Database->CloseQuery(1);

		Traders[Trader.ID] = Trader;
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
		Object->ActionBar.resize(Database->GetInt<uint32_t>("actionbarsize"));

		// Get items
		Database->PrepareQuery("SELECT * FROM builditem where build_id = @build_id", 1);
		Database->BindInt(1, BuildID, 1);
		while(Database->FetchRow(1)) {
			const _Item *Item = Items[Database->GetInt<uint32_t>("item_id", 1)];
			if(Item)
				Object->Inventory->AddItem(Item, Database->GetInt<int>("count", 1));
		}
		Database->CloseQuery(1);

		// Get skills
		Database->PrepareQuery("SELECT * FROM buildskill where build_id = @build_id", 1);
		Database->BindInt(1, BuildID, 1);
		while(Database->FetchRow(1)) {
			const _Item *Item = Items[Database->GetInt<uint32_t>("item_id", 1)];
			if(Item)
				Object->Skills[Item->ID] = Database->GetInt<int>("level", 1);
		}
		Database->CloseQuery(1);

		// Get actionbar
		Database->PrepareQuery("SELECT * FROM buildactionbar where build_id = @build_id", 1);
		Database->BindInt(1, BuildID, 1);
		while(Database->FetchRow(1)) {
			const _Item *Item = Items[Database->GetInt<uint32_t>("item_id", 1)];
			if(Item) {
				size_t Slot = (size_t)Database->GetInt<uint32_t>("slot", 1);
				if(Slot < Object->ActionBar.size())
					Object->ActionBar[Slot].Item = Item;
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
void _Stats::LoadScripts(){
	Scripts.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM script");

	// Get data
	_Script Script;
	while(Database->FetchRow()) {
		Script.ID = Database->GetInt<uint32_t>("id");
		Script.Name = Database->GetString("name");
		Script.Level = Database->GetInt<int>("level");

		Scripts[Script.ID] = Script;
	}
	Database->CloseQuery();
}

// Gets monsters stats from the database
void _Stats::GetMonsterStats(uint32_t MonsterID, _Object *Monster) {
	Monster->DatabaseID = MonsterID;

	// Run query
	Database->PrepareQuery("SELECT m.*, ai.name as ai_name FROM monster m, ai WHERE m.ai_id = ai.id AND m.id = @monster_id");
	Database->BindInt(1, MonsterID);

	// Get data
	if(Database->FetchRow()) {
		Monster->Level = Database->GetInt<int>("level");
		Monster->Name = Database->GetString("name");
		Monster->Portrait = Assets.Textures[Database->GetString("portrait")];
		Monster->ExperienceGiven = Database->GetInt<int>("experience");
		Monster->GoldGiven = Database->GetInt<int>("gold");
		Monster->BaseMaxHealth = Database->GetInt<int>("health");
		Monster->BaseMaxMana = Database->GetInt<int>("mana");
		Monster->BaseMinDamage = Database->GetInt<int>("mindamage");
		Monster->BaseMaxDamage = Database->GetInt<int>("maxdamage");
		Monster->BaseMinDefense = Database->GetInt<int>("mindefense");
		Monster->BaseMaxDefense = Database->GetInt<int>("maxdefense");
		Monster->BaseBattleSpeed = Database->GetReal("battlespeed");
		Monster->AI = Database->GetString("ai_name");
		uint32_t BuildID = Database->GetInt<uint32_t>("build_id");

		// Load build
		const _Object *Build = Builds[BuildID];
		if(!Build)
			throw std::runtime_error("Can't find build_id " + std::to_string(BuildID));

		// Copy build
		Monster->ActionBar = Build->ActionBar;
		Monster->Inventory->Slots = Build->Inventory->Slots;
		Monster->Skills = Build->Skills;

		Monster->Health = Monster->MaxHealth = Monster->BaseMaxHealth;
		Monster->Mana = Monster->MaxMana = Monster->BaseMaxMana;
		Monster->Gold = Monster->GoldGiven;
		Monster->CalcLevelStats = false;
	}

	// Free memory
	Database->CloseQuery();
}

// Get list of portraits
void _Stats::GetPortraits(std::list<_Portrait> &Portraits) {

	// Run query
	Database->PrepareQuery("SELECT * FROM portrait");
	while(Database->FetchRow()) {
		_Portrait Portrait;
		Portrait.ID = Database->GetInt<uint32_t>("id");
		Portrait.Image = Assets.Textures[Database->GetString("texture")];

		Portraits.push_back(Portrait);
	}

	Database->CloseQuery();
}

// Get list of builds
void _Stats::GetStartingBuilds(std::list<_Build> &Builds) {

	// Run query
	Database->PrepareQuery("SELECT * FROM build WHERE starting = 1");
	while(Database->FetchRow()) {
		_Build Build;
		Build.ID = Database->GetInt<uint32_t>("id");
		Build.Name = Database->GetString("name");
		Build.Image = Assets.Textures[Database->GetString("texture")];

		Builds.push_back(Build);
	}

	Database->CloseQuery();
}

// Get portrait texture by id
const _Texture *_Stats::GetPortraitImage(uint32_t PortraitID) {
	const _Texture *Image = nullptr;

	// Run query
	Database->PrepareQuery("SELECT texture FROM portrait where id = @portrait_id");
	Database->BindInt(1, PortraitID);
	if(Database->FetchRow()) {
		Image = Assets.Textures[Database->GetString("texture")];
	}
	Database->CloseQuery();

	return Image;
}

// Randomly generates a list of monsters from a zone
void _Stats::GenerateMonsterListFromZone(int AdditionalCount, uint32_t ZoneID, std::list<uint32_t> &Monsters, bool &Boss) {
	if(ZoneID == 0)
		return;

	// Get zone info
	Database->PrepareQuery("SELECT boss, minspawn, maxspawn FROM zone WHERE id = @zone_id");
	Database->BindInt(1, ZoneID);

	// Get spawn range
	int MinSpawn = 0;
	int MaxSpawn = 0;
	if(Database->FetchRow()) {
		Boss = Database->GetInt<int>("boss");
		MinSpawn = Database->GetInt<int>("minspawn");
		MaxSpawn = Database->GetInt<int>("maxspawn");
	}
	Database->CloseQuery();

	// If boss zone then use odds parameter as monster count
	if(Boss) {

		// Run query
		Database->PrepareQuery("SELECT monster_id, odds FROM zonedata WHERE zone_id = @zone_id");
		Database->BindInt(1, ZoneID);
		while(Database->FetchRow()) {
			uint32_t MonsterID = Database->GetInt<uint32_t>("monster_id");
			uint32_t Count = Database->GetInt<uint32_t>("odds");

			// Populate monster list
			for(uint32_t i = 0; i < Count; i++)
				Monsters.push_back(MonsterID);
		}
		Database->CloseQuery();
	}
	else {

		// Get monster count
		int MonsterCount = GetRandomInt(MinSpawn, MaxSpawn);

		// No monsters
		if(MonsterCount == 0)
			return;

		MonsterCount += AdditionalCount;

		// Cap monster count
		MonsterCount = std::min(MonsterCount, BATTLE_MAXFIGHTERS_SIDE);

		// Run query
		Database->PrepareQuery("SELECT monster_id, odds FROM zonedata WHERE zone_id = @zone_id");
		Database->BindInt(1, ZoneID);

		// Get monsters in zone
		std::vector<_Zone> Zone;
		uint32_t OddsSum = 0;
		while(Database->FetchRow()) {
			uint32_t MonsterID = Database->GetInt<uint32_t>("monster_id");
			uint32_t Odds = Database->GetInt<uint32_t>("odds");
			OddsSum += Odds;

			Zone.push_back(_Zone(MonsterID, OddsSum));
		}
		Database->CloseQuery();

		// Check for monsters in zone
		if(OddsSum > 0) {

			// Generate monsters
			uint32_t RandomNumber;
			size_t MonsterIndex;
			for(int i = 0; i < MonsterCount; i++) {
				RandomNumber = GetRandomInt((uint32_t)1, OddsSum);
				for(MonsterIndex = 0; MonsterIndex < Zone.size(); MonsterIndex++) {
					if(RandomNumber <= Zone[MonsterIndex].Odds)
						break;
				}

				// Populate monster list
				Monsters.push_back(Zone[MonsterIndex].MonsterID);
			}
		}
	}
}

// Generates a list of items dropped from a monster
void _Stats::GenerateItemDrops(uint32_t MonsterID, uint32_t Count, std::list<uint32_t> &ItemDrops) {
	if(MonsterID == 0)
		return;

	// Run query
	Database->PrepareQuery("SELECT item_id, odds FROM monsterdrop WHERE monster_id = @monster_id");
	Database->BindInt(1, MonsterID);

	// Get list of possible drops and build CDT
	std::list<_ItemDrop> PossibleItemDrops;
	uint32_t OddsSum = 0;
	while(Database->FetchRow()) {
		uint32_t ItemID = Database->GetInt<uint32_t>("item_id");
		uint32_t Odds = Database->GetInt<uint32_t>("odds");
		OddsSum += Odds;

		PossibleItemDrops.push_back(_ItemDrop(ItemID, OddsSum));
	}
	Database->CloseQuery();

	// Check for items
	if(OddsSum > 0) {

		// Generate items
		for(uint32_t i = 0; i < Count; i++) {
			uint32_t RandomNumber = GetRandomInt((uint32_t)1, OddsSum);

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
}

// Get map id by path
uint32_t _Stats::GetMapIDByPath(const std::string &Path) {

	for(const auto &MapStat : Maps) {
		if(MapStat.second.File == Path) {
			return MapStat.first;
		}
	}

	return 0;
}

// Find a level from the given experience number
const _Level *_Stats::FindLevel(int Experience) const {

	// Search through levels
	for(size_t i = 1; i < Levels.size(); i++) {
		if(Levels[i].Experience > Experience)
			return &Levels[i-1];
	}

	return &Levels[Levels.size()-1];
}
