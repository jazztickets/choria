/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#include <stats.h>
#include <database.h>
#include <globals.h>
#include <random.h>
#include <objects/monster.h>

using namespace irr;

_Stats Stats;

// Initialize
int _Stats::Init() {

	// Load database that stores game data
	Database = new _Database();
	if(!Database->OpenDatabaseCreate("database/data.s3db")) {
		return 0;
	}

	// Load spreadsheet data
	LoadPortraits();
	LoadMaps();
	LoadEvents();
	LoadLevels();
	LoadSkills();
	LoadItems();
	LoadVendors();
	LoadTraders();

	return 1;
}

// Shutdown
int _Stats::Close() {

	delete Database;
	Events.clear();

	return 1;
}

// Load portrait data
void _Stats::LoadPortraits() {

	Portraits.clear();

	// Run query
	Database->RunDataQuery("SELECT * FROM Portraits");

	// Get events
	_Portrait Portrait;
	while(Database->FetchRow()) {
		Portrait.ID = Database->GetInt(0);
		Portrait.Image = irrDriver->getTexture((core::stringc("textures/portraits/") + core::stringc(Database->GetString(1))).c_str());
		Portraits[Portrait.ID] = Portrait;
	}
	Database->CloseQuery();
}

// Load map data
void _Stats::LoadMaps() {

	Maps.clear();

	// Run query
	Database->RunDataQuery("SELECT * FROM Maps");

	// Get events
	_MapStat Map;
	while(Database->FetchRow()) {
		Map.File = core::stringc("maps/") + Database->GetString(1);
		Map.ViewWidth = Database->GetInt(2);
		Map.ViewHeight = Database->GetInt(3);
		Maps[Database->GetInt(0)] = Map;
	}
	Database->CloseQuery();
}

// Load event data
void _Stats::LoadEvents() {

	// Clear events
	Events.clear();

	// Run query
	Database->RunDataQuery("SELECT * FROM Events");

	// Get events
	_Event Event;
	while(Database->FetchRow()) {
		Event.Name = Database->GetString(1);
		Event.ShortName = Database->GetString(2);
		Event.Indexed = !!Database->GetInt(3);
		Events.push_back(Event);
	}
	Database->CloseQuery();
}

// Loads level data
void _Stats::LoadLevels() {

	// Clear events
	Levels.clear();

	// Run query
	Database->RunDataQuery("SELECT * FROM Levels");

	// Get events
	_Level Level;
	while(Database->FetchRow()) {
		Level.Level = Database->GetInt(0);
		Level.Experience = Database->GetInt(1);
		Level.NextLevel = Database->GetInt(2);
		Level.Health = Database->GetInt(3);
		Level.Mana = Database->GetInt(4);
		Level.SkillPoints = Database->GetInt(5);
		Levels.push_back(Level);
	}
	Database->CloseQuery();
}

// Loads skills into a map
void _Stats::LoadSkills() {

	Skills.clear();

	// Run query
	Database->RunDataQuery("SELECT * FROM Skills");

	// Get events
	_Skill Skill;
	while(Database->FetchRow()) {
		Skill.ID = Database->GetInt(0);
		Skill.Name = Database->GetString(1);
		Skill.Info = Database->GetString(2);
		Skill.Image = irrDriver->getTexture((core::stringc("textures/skills/") + core::stringc(Database->GetString(3))).c_str());
		Skill.Type = Database->GetInt(4);
		Skill.SkillCost = Database->GetInt(5);
		Skill.ManaCostBase = Database->GetFloat(6);
		Skill.ManaCost = Database->GetFloat(7);
		Skill.PowerBase = Database->GetFloat(8);
		Skill.PowerRangeBase = Database->GetFloat(9);
		Skill.Power = Database->GetFloat(10);
		Skill.PowerRange = Database->GetFloat(11);
		Skills.push_back(Skill);
	}
	Database->CloseQuery();
}

// Load items
void _Stats::LoadItems() {

	Items.clear();

	// Run query
	Database->RunDataQuery("SELECT * FROM Items");

	// Get events
	_Item Item;
	while(Database->FetchRow()) {
		Item.ID = Database->GetInt(0);
		Item.Name = Database->GetString(1);
		Item.Level = Database->GetInt(2);
		Item.Type = Database->GetInt(3);
		Item.Image = irrDriver->getTexture((core::stringc("textures/items/") + core::stringc(Database->GetString(4))).c_str());
		Item.LevelRequired = Database->GetInt(5);
		Item.Cost = Database->GetInt(6);
		Item.Damage = Database->GetFloat(7);
		Item.DamageRange = Database->GetFloat(8);
		Item.Defense = Database->GetFloat(9);
		Item.DefenseRange = Database->GetFloat(10);
		Item.DamageType = Database->GetInt(11);
		Item.HealthRestore = Database->GetInt(12);
		Item.ManaRestore = Database->GetInt(13);
		Item.MaxHealth = Database->GetInt(14);
		Item.MaxMana = Database->GetInt(15);
		Item.HealthRegen = Database->GetFloat(16);
		Item.ManaRegen = Database->GetFloat(17);
		Item.InvisPower = Database->GetInt(18);
		Items[Item.ID] = Item;
	}
	Database->CloseQuery();
}

// Loads vendor data
void _Stats::LoadVendors() {
	Vendors.clear();

	// Run query
	Database->RunDataQuery("SELECT * FROM Vendors");

	// Get vendors
	char Buffer[256];
	_Vendor Vendor;
	while(Database->FetchRow()) {
		Vendor.ID = Database->GetInt(0);
		Vendor.Name = Database->GetString(1);
		Vendor.Info = Database->GetString(2);
		Vendor.BuyPercent = Database->GetFloat(3);
		Vendor.SellPercent = Database->GetFloat(4);
		Vendor.Items.clear();

		// Get items
		sprintf(Buffer, "SELECT ItemsID FROM VendorItems, Items where VendorItems.VendorsID = %d and Items.ID = VendorItems.ItemsID order by Items.Cost", Vendor.ID);
		Database->RunDataQuery(Buffer, 1);
		while(Database->FetchRow(1)) {
			Vendor.Items.push_back(GetItem(Database->GetInt(0, 1)));
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
	Database->RunDataQuery("SELECT * FROM Traders");

	// Get traders
	char Buffer[256];
	_Trader Trader;
	while(Database->FetchRow()) {
		Trader.ID = Database->GetInt(0);
		Trader.Name = Database->GetString(1);
		Trader.RewardItem = GetItem(Database->GetInt(2));
		Trader.Count = Database->GetInt(3);
		Trader.TraderItems.clear();

		// Get items
		sprintf(Buffer, "SELECT ItemsID, Count FROM TraderItems where TradersID = %d", Trader.ID);
		Database->RunDataQuery(Buffer, 1);
		while(Database->FetchRow(1)) {
			_TraderItem TraderItem;
			TraderItem.Item = GetItem(Database->GetInt(0, 1));
			TraderItem.Count = Database->GetInt(1, 1);
			Trader.TraderItems.push_back(TraderItem);
		}
		Database->CloseQuery(1);

		Traders[Trader.ID] = Trader;
	}
	Database->CloseQuery();
}

// Gets monsters stats from the database
void _Stats::GetMonsterStats(int TMonsterID, _Monster *TMonster) {

	// Run query
	char QueryString[256];
	sprintf(QueryString, "SELECT * FROM Monsters WHERE ID = %d", TMonsterID);
	Database->RunDataQuery(QueryString);

	// Get data
	float Value, Range;
	if(Database->FetchRow()) {
		TMonster->Level = Database->GetInt(1);
		TMonster->Name = Database->GetString(2);
		TMonster->Portrait = irrDriver->getTexture((core::stringc("textures/monsters/") + core::stringc(Database->GetString(3))).c_str());
		TMonster->Health = TMonster->MaxHealth = Database->GetInt(4);
		TMonster->Mana = TMonster->MaxMana = Database->GetInt(5);
		TMonster->ExperienceGiven = Database->GetInt(6);
		TMonster->GoldGiven = Database->GetInt(7);

		Value = Database->GetFloat(8);
		Range = Database->GetFloat(9);
		TMonster->MinDamage = (int)(Value - Range);
		TMonster->MaxDamage = (int)(Value + Range);

		Value = Database->GetFloat(10);
		Range = Database->GetFloat(11);
		TMonster->MinDefense = (int)(Value - Range);
		TMonster->MaxDefense = (int)(Value + Range);

		TMonster->AI = Database->GetInt(12);

		TMonster->SkillBar[0] = Stats.GetSkill(0);
	}

	// Free memory
	Database->CloseQuery();
}

// Gets a skill by id
const _Skill *_Stats::GetSkill(int TSkillID) {
	if(TSkillID < 0 || TSkillID >= (int)Skills.size())
		return nullptr;

	return &Skills[TSkillID];
}

// Gets a list of portraits
void _Stats::GetPortraitList(std::list<_Portrait> &TList) {

	for(std::map<int, _Portrait>::iterator Iterator = Portraits.begin(); Iterator != Portraits.end(); ++Iterator)
		TList.push_back(Iterator->second);
}

// Randomly generates a list of monsters from a zone
void _Stats::GenerateMonsterListFromZone(int TZone, std::vector<int> &TMonsters) {
	if(TZone == 0)
		return;

	char QueryString[256];
	int MonsterCount = 0;

	// Get zone info
	sprintf(QueryString, "SELECT MonsterCount FROM Zones WHERE ID = %d", TZone);
	Database->RunDataQuery(QueryString);
	if(Database->FetchRow()) {
		MonsterCount = Database->GetInt(0);
	}
	Database->CloseQuery();

	// No monsters
	if(MonsterCount == 0)
		return;

	// Run query
	sprintf(QueryString, "SELECT MonstersID, Odds FROM ZoneData WHERE ZonesID = %d", TZone);
	Database->RunDataQuery(QueryString);

	// Get monsters in zone
	std::vector<_Zone> Zone;
	int OddsSum = 0;
	while(Database->FetchRow()) {
		int MonsterID = Database->GetInt(0);
		int Odds = Database->GetInt(1);
		OddsSum += Odds;

		Zone.push_back(_Zone(MonsterID, OddsSum));
	}

	// Free memory
	Database->CloseQuery();

	// Check for monsters in zone
	if(OddsSum > 0) {

		// Generate monsters
		int RandomNumber;
		size_t MonsterIndex;
		for(int i = 0; i < MonsterCount; i++) {
			std::uniform_int_distribution<int> Distribution(1, OddsSum);
			RandomNumber = Distribution(RandomGenerator);
			for(MonsterIndex = 0; MonsterIndex < Zone.size(); MonsterIndex++) {
				if(RandomNumber <= Zone[MonsterIndex].Odds)
					break;
			}

			// Populate monster list
			TMonsters.push_back(Zone[MonsterIndex].MonsterID);
		}
	}
}

// Generates a list of items dropped from a monster
void _Stats::GenerateMonsterDrops(int TMonsterID, int TCount, std::vector<int> &TDrops) {
	if(TMonsterID == 0)
		return;

	// Run query
	char QueryString[256];
	sprintf(QueryString, "SELECT ItemsID, Odds FROM MonsterDrops WHERE MonstersID = %d", TMonsterID);
	Database->RunDataQuery(QueryString);

	// Get items from monster
	std::vector<_MonsterDrop> MonsterDrop;
	int OddsSum = 0;
	while(Database->FetchRow()) {
		int ItemID = Database->GetInt(0);
		int Odds = Database->GetInt(1);
		OddsSum += Odds;

		MonsterDrop.push_back(_MonsterDrop(ItemID, OddsSum));
	}

	// Free memory
	Database->CloseQuery();

	// Check for items
	if(OddsSum > 0) {

		// Generate items
		int RandomNumber;
		size_t ItemIndex;
		for(int i = 0; i < TCount; i++) {
			std::uniform_int_distribution<int> Distribution(1, OddsSum);
			RandomNumber = Distribution(RandomGenerator);
			for(ItemIndex = 0; ItemIndex < MonsterDrop.size(); ItemIndex++) {
				if(RandomNumber <= MonsterDrop[ItemIndex].Odds)
					break;
			}

			// Populate item list
			TDrops.push_back(MonsterDrop[ItemIndex].ItemID);
		}
	}
}

// Find a level from the given experience number
const _Level *_Stats::FindLevel(int TExperience) const {

	// Search through levels
	for(size_t i = 1; i < Levels.size(); i++) {
		if(Levels[i].Experience > TExperience)
			return &Levels[i-1];
	}

	return &Levels[Levels.size()-1];
}
