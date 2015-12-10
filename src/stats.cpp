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
#include <objects/skill.h>
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
	LoadSkills();
	LoadItems();
	LoadVendors();
	LoadTraders();
}

// Destructor
_Stats::~_Stats() {

	for(const auto &Item : Items)
		delete Item.second;

	for(const auto &Skill : Skills)
		delete Skill.second;

	delete Database;
}

// Load map data
void _Stats::LoadMaps() {

	// Run query
	Database->PrepareQuery("SELECT * FROM map");

	// Get events
	_MapStat Map;
	while(Database->FetchRow()) {
		Map.File = std::string("maps/") + Database->GetString("file");
		Maps[Database->GetInt("id")] = Map;
	}
	Database->CloseQuery();
}

// Load event data
void _Stats::LoadEvents() {

	// Run query
	Database->PrepareQuery("SELECT * FROM event");

	// Get events
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

	// Get events
	_Level Level;
	while(Database->FetchRow()) {
		Level.Level = Database->GetInt("level");
		Level.Experience = Database->GetInt("experience");
		Level.NextLevel = Database->GetInt("nextlevel");
		Level.Health = Database->GetInt("health");
		Level.Mana = Database->GetInt("mana");
		Level.SkillPoints = Database->GetInt("skillpoints");
		Levels.push_back(Level);
	}
	Database->CloseQuery();
}

// Loads skills into a map
void _Stats::LoadSkills() {

	// Run query
	Database->PrepareQuery("SELECT * FROM skill");

	// Get events
	while(Database->FetchRow()) {
		_Skill *Skill = new _Skill;
		Skill->ID = Database->GetInt("id");
		Skill->Name = Database->GetString("name");
		Skill->Script = Database->GetString("script");
		Skill->Image = Assets.Textures[Database->GetString("icon")];
		Skill->TargetID = (TargetType)Database->GetInt("target_id");
		Skill->TargetAlive = Database->GetInt("target_alive");
		Skills[Skill->ID] = Skill;
	}
	Database->CloseQuery();

	Skills[0] = nullptr;
}

// Load items
void _Stats::LoadItems() {

	// Run query
	Database->PrepareQuery("SELECT * FROM item");

	// Get events
	while(Database->FetchRow()) {
		_Item *Item = new _Item;
		Item->ID = Database->GetInt("id");
		Item->Name = Database->GetString("name");
		Item->Level = Database->GetInt("level");
		Item->Type = Database->GetInt("type");
		Item->Image = Assets.Textures[std::string("items/") + Database->GetString("image")];
		Item->LevelRequired = Database->GetInt("levelrequired");
		Item->Cost = Database->GetInt("cost");
		Item->Damage = Database->GetReal("damage");
		Item->DamageRange = Database->GetReal("damagerange");
		Item->Defense = Database->GetReal("defense");
		Item->DefenseRange = Database->GetReal("defenserange");
		Item->DamageType = Database->GetInt("damagetype");
		Item->HealthRestore = Database->GetInt("healthrestore");
		Item->ManaRestore = Database->GetInt("manarestore");
		Item->MaxHealth = Database->GetInt("maxhealth");
		Item->MaxMana = Database->GetInt("maxmana");
		Item->HealthRegen = Database->GetReal("healthregen");
		Item->ManaRegen = Database->GetReal("manaregen");
		Item->InvisPower = Database->GetInt("invispower");
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

	// Get vendors
	_Vendor Vendor;
	while(Database->FetchRow()) {
		Vendor.ID = Database->GetInt("id");
		Vendor.Name = Database->GetString("name");
		Vendor.Info = Database->GetString("info");
		Vendor.BuyPercent = Database->GetReal("buypercent");
		Vendor.SellPercent = Database->GetReal("sellpercent");
		Vendor.Items.clear();

		// Get items
		Database->PrepareQuery("SELECT item_id FROM vendoritem vi, item i where vi.vendor_id = @vendor_id and i.id = vi.item_id order by i.cost", 1);
		Database->BindInt(1, Vendor.ID, 1);
		while(Database->FetchRow(1)) {
			Vendor.Items.push_back(Items[Database->GetInt("item_id", 1)]);
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

	// Get traders
	_Trader Trader;
	while(Database->FetchRow()) {
		Trader.ID = Database->GetInt("id");
		Trader.Name = Database->GetString("name");
		Trader.RewardItem = Items[Database->GetInt("item_id")];
		Trader.Count = Database->GetInt("count");
		Trader.TraderItems.clear();

		// Get items
		Database->PrepareQuery("SELECT item_id, count FROM traderitem where trader_id = @trader_id", 1);
		Database->BindInt(1, Trader.ID, 1);
		while(Database->FetchRow(1)) {
			_TraderItem TraderItem;
			TraderItem.Item = Items[Database->GetInt("item_id", 1)];
			TraderItem.Count = Database->GetInt("count", 1);
			Trader.TraderItems.push_back(TraderItem);
		}
		Database->CloseQuery(1);

		Traders[Trader.ID] = Trader;
	}
	Database->CloseQuery();
}

// Gets monsters stats from the database
void _Stats::GetMonsterStats(uint32_t MonsterID, _Object *Monster) {

	// Run query
	Database->PrepareQuery("SELECT m.*, ai.name as ai_name FROM monster m, ai WHERE m.id = @monster_id");
	Database->BindInt(1, MonsterID);

	// Get data
	float Value, Range;
	if(Database->FetchRow()) {
		Monster->Level = Database->GetInt("level");
		Monster->Name = Database->GetString("name");
		Monster->Portrait = Assets.Textures[std::string("monsters/") + Database->GetString("portrait")];
		Monster->Health = Monster->MaxHealth = Database->GetInt("health");
		Monster->Mana = Monster->MaxMana = Database->GetInt("mana");
		Monster->ExperienceGiven = Database->GetInt("experience");
		Monster->GoldGiven = Database->GetInt("gold");

		Value = Database->GetReal("damage");
		Range = Database->GetReal("damagerange");
		Monster->MinDamage = (int)(Value - Range);
		Monster->MaxDamage = (int)(Value + Range);

		Value = Database->GetReal("defense");
		Range = Database->GetReal("defenserange");
		Monster->MinDefense = (int)(Value - Range);
		Monster->MaxDefense = (int)(Value + Range);

		Monster->AI = Database->GetString("ai_name");

		Monster->ActionBar.resize(ACTIONBAR_STARTING_SIZE);
		Monster->ActionBar[0].Skill = Skills[1];
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
		Portrait.ID = Database->GetInt("id");
		Portrait.Image = Assets.Textures[std::string("portraits/") + Database->GetString("image")];

		Portraits.push_back(Portrait);
	}

	Database->CloseQuery();
}

// Get portrait texture by id
const _Texture *_Stats::GetPortraitImage(uint32_t PortraitID) {
	const _Texture *Image = nullptr;

	// Run query
	Database->PrepareQuery("SELECT image FROM portrait where id = @portrait_id");
	Database->BindInt(1, PortraitID);
	if(Database->FetchRow()) {
		Image = Assets.Textures[std::string("portraits/") + Database->GetString("image")];
	}
	Database->CloseQuery();

	return Image;
}

// Randomly generates a list of monsters from a zone
void _Stats::GenerateMonsterListFromZone(uint32_t ZoneID, std::list<int> &Monsters) {
	if(ZoneID == 0)
		return;

	int MonsterCount = 0;

	// Get zone info
	Database->PrepareQuery("SELECT monstercount FROM zone WHERE id = @zone_id");
	Database->BindInt(1, ZoneID);
	if(Database->FetchRow()) {
		MonsterCount = Database->GetInt("monstercount");
	}
	Database->CloseQuery();

	// No monsters
	if(MonsterCount == 0)
		return;

	// Run query
	Database->PrepareQuery("SELECT monster_id, odds FROM zonedata WHERE zone_id = @zone_id");
	Database->BindInt(1, ZoneID);

	// Get monsters in zone
	std::vector<_Zone> Zone;
	uint32_t OddsSum = 0;
	while(Database->FetchRow()) {
		uint32_t MonsterID = Database->GetInt("monster_id");
		uint32_t Odds = Database->GetInt("odds");
		OddsSum += Odds;

		Zone.push_back(_Zone(MonsterID, OddsSum));
	}

	// Free memory
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
		uint32_t ItemID = Database->GetInt("item_id");
		uint32_t Odds = Database->GetInt("odds");
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

// Find a level from the given experience number
const _Level *_Stats::FindLevel(int Experience) const {

	// Search through levels
	for(size_t i = 1; i < Levels.size(); i++) {
		if(Levels[i].Experience > Experience)
			return &Levels[i-1];
	}

	return &Levels[Levels.size()-1];
}
