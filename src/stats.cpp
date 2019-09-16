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
#include <stats.h>
#include <ae/database.h>
#include <ae/random.h>
#include <ae/assets.h>
#include <objects/object.h>
#include <objects/buff.h>
#include <objects/components/character.h>
#include <objects/components/inventory.h>
#include <objects/components/monster.h>
#include <constants.h>
#include <tinyxml2.h>
#include <algorithm>
#include <iostream>

// Compare function for sorting portraits
bool ComparePortrait(const _Portrait &First, const _Portrait &Second) {
	return First.NetworkID < Second.NetworkID;
}

// Constructor
_Stats::_Stats(bool Headless) :
Headless(Headless) {

	// Load database that stores game data
	Database = new ae::_Database("stats/stats.db", true);

	// Load game data
	LoadData("data/stats.xml");

	// Load spreadsheet data
	OldLoadMaps();
	OldLoadEvents();
	OldLoadLevels();
	OldLoadBuffs();
	OldLoadItemTypes();
	OldLoadStatTypes();
	OldLoadTargetTypes();
	OldLoadDamageTypes();
	OldLoadItems();
	OldLoadVendors();
	OldLoadTraders();
	OldLoadBlacksmiths();
	OldLoadMinigames();
	OldLoadModels();
	OldLoadBuilds();
	OldLoadScripts();
	OldLoadLights();
}

// Destructor
_Stats::~_Stats() {

	for(const auto &Build : Builds)
		delete Build.second;

	for(const auto &Item : OldItems)
		delete Item.second;

	for(const auto &Buff : OldBuffs)
		delete Buff.second;

	for(const auto &Build : OldBuilds)
		delete Build.second;

	delete Database;
}

// Get string attribute from node
const char *_Stats::GetString(tinyxml2::XMLElement *Node, const char *Attribute) {
	const char *Value = Node->Attribute(Attribute);
	if(!Value)
		throw std::runtime_error("Missing '" + std::string(Attribute) + "' attribute in '" + std::string(Node->Name()) + "' node!");

	return Value;
}

// Load data
void _Stats::LoadData(const std::string &Path) {

	// Load file
	tinyxml2::XMLDocument Document;
	if(Document.LoadFile(Path.c_str()) != tinyxml2::XML_SUCCESS)
		throw std::runtime_error("Error loading: " + Path);

	// Get data node
	tinyxml2::XMLElement *DataNode = Document.FirstChildElement();

	// Build map of nodes
	std::unordered_map<std::string, tinyxml2::XMLElement *> Nodes(
	{
		{ "scopes", DataNode->FirstChildElement("scopes") },
		{ "damage_types", DataNode->FirstChildElement("damage_types") },
		{ "item_types", DataNode->FirstChildElement("item_types") },
		{ "events", DataNode->FirstChildElement("events") },
		{ "portraits", DataNode->FirstChildElement("portraits") },
		{ "models", DataNode->FirstChildElement("models") },
		{ "items", DataNode->FirstChildElement("items") },
		{ "skills", DataNode->FirstChildElement("skills") },
		{ "builds", DataNode->FirstChildElement("builds") },
		{ "vendors", DataNode->FirstChildElement("vendors") },
		{ "monsters", DataNode->FirstChildElement("monsters") },
		{ "zones", DataNode->FirstChildElement("zones") },
	});

	// Check nodes
	for(const auto &Node : Nodes) {
		if(!Node.second)
			throw std::runtime_error("Missing '" + Node.first + "' node in " + Path);
	}

	uint8_t NetworkID;

	// Load portraits
	NetworkID = 1;
	for(tinyxml2::XMLElement *ChildNode = Nodes["portraits"]->FirstChildElement(); ChildNode != nullptr; ChildNode = ChildNode->NextSiblingElement()) {
		_Portrait Portrait;
		Portrait.ID = GetString(ChildNode, "id");
		Portrait.Texture = ae::Assets.Textures[GetString(ChildNode, "texture")];
		Portrait.NetworkID = NetworkID++;
		Portraits[Portrait.ID] = Portrait;
		PortraitsIndex[Portrait.NetworkID] = &Portraits[Portrait.ID];
	}

	// Load models
	NetworkID = 1;
	for(tinyxml2::XMLElement *ChildNode = Nodes["models"]->FirstChildElement(); ChildNode != nullptr; ChildNode = ChildNode->NextSiblingElement()) {
		_Model Model;
		Model.ID = GetString(ChildNode, "id");
		Model.Texture = ae::Assets.Textures[GetString(ChildNode, "texture")];
		Model.NetworkID = NetworkID++;
		Models[Model.ID] = Model;
	}

	// Load builds
	for(tinyxml2::XMLElement *ChildNode = Nodes["builds"]->FirstChildElement(); ChildNode != nullptr; ChildNode = ChildNode->NextSiblingElement()) {
		_Object *Object = new _Object();
		Object->Name = GetString(ChildNode, "name");
		Object->ModelTexture = ae::Assets.Textures[GetString(ChildNode, "texture")];
		Builds[Object->Name] = Object;
	}
}

// Load map data
void _Stats::OldLoadMaps() {

	// Run query
	Database->PrepareQuery("SELECT * FROM map");

	// Get data
	_OldMapStat Map;
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

		OldMaps[Database->GetInt<uint32_t>("id")] = Map;
	}
	Database->CloseQuery();
}

// Load event data
void _Stats::OldLoadEvents() {

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
void _Stats::OldLoadLevels() {

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
		Level.Armor = Database->GetInt<int>("armor");
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
void _Stats::OldLoadBuffs() {

	// Run query
	Database->PrepareQuery("SELECT * FROM buff");

	// Get data
	while(Database->FetchRow()) {
		_Buff *Buff = new _Buff;
		Buff->ID = Database->GetInt<uint32_t>("id");
		Buff->Name = Database->GetString("name");
		Buff->Script = Database->GetString("script");
		Buff->Texture = ae::Assets.Textures[Database->GetString("texture")];
		OldBuffs[Buff->ID] = Buff;
	}
	Database->CloseQuery();

	OldBuffs[0] = nullptr;
}

// Load item types
void _Stats::OldLoadItemTypes() {

	// Run query
	Database->PrepareQuery("SELECT * FROM itemtype");

	// Get data
	while(Database->FetchRow()) {
		uint32_t ID = Database->GetInt<uint32_t>("id");
		OldItemTypes[ID] = Database->GetString("name");
	}
	Database->CloseQuery();
}

// Load upgrade scales from stat types
void _Stats::OldLoadStatTypes() {

	// Run query
	Database->PrepareQuery("SELECT * FROM stattype");

	// Get data
	while(Database->FetchRow()) {
		StatType ID = (StatType)Database->GetInt<uint32_t>("id");
		UpgradeScale[ID] = Database->GetReal("upgrade_scale");
	}
	Database->CloseQuery();
}

// Load target type strings
void _Stats::OldLoadTargetTypes() {

	// Run query
	Database->PrepareQuery("SELECT * FROM target");

	// Get data
	while(Database->FetchRow()) {
		uint32_t ID = Database->GetInt<uint32_t>("id");
		OldTargetTypes[ID] = Database->GetString("name");
	}
	Database->CloseQuery();
}

// Load damage types
void _Stats::OldLoadDamageTypes() {

	// Run query
	Database->PrepareQuery("SELECT * FROM damagetype");

	// Get data
	while(Database->FetchRow()) {
		uint32_t ID = Database->GetInt<uint32_t>("id");
		OldDamageTypes[ID] = Database->GetString("name");
	}
	Database->CloseQuery();
}

// Load items
void _Stats::OldLoadItems() {

	// Run query
	Database->PrepareQuery("SELECT * FROM item");

	// Get data
	while(Database->FetchRow()) {
		uint32_t ItemID = Database->GetInt<uint32_t>("id");
		if(ItemID == 0)
			continue;

		std::string TexturePath = Database->GetString("texture");

		_Item *Item = new _Item;
		Item->Stats = this;
		Item->ID = ItemID;
		Item->Name = Database->GetString("name");
		Item->Texture = ae::Assets.Textures[TexturePath];
		Item->Script = Database->GetString("script");
		Item->Type = (ItemType)Database->GetInt<int>("itemtype_id");
		Item->Level = Database->GetInt<int>("level");
		Item->MaxLevel = Database->GetInt<int>("maxlevel");
		Item->Duration = Database->GetReal("duration");
		Item->Cost = Database->GetInt<int>("cost");
		Item->DamageTypeID = Database->GetInt<uint32_t>("damagetype_id");
		Item->MinDamage = Database->GetInt<int>("mindamage");
		Item->MaxDamage = Database->GetInt<int>("maxdamage");
		Item->Armor = Database->GetInt<int>("armor");
		Item->DamageBlock = Database->GetInt<int>("block");
		Item->Pierce = Database->GetInt<int>("pierce");
		Item->MaxHealth = Database->GetInt<int>("maxhealth");
		Item->MaxMana = Database->GetInt<int>("maxmana");
		Item->HealthRegen = Database->GetInt<int>("healthregen");
		Item->ManaRegen = Database->GetInt<int>("manaregen");
		Item->BattleSpeed = Database->GetInt<int>("battlespeed");
		Item->MoveSpeed = Database->GetInt<int>("movespeed");
		Item->DropRate = Database->GetInt<int>("droprate");
		Item->ResistanceTypeID = Database->GetInt<uint32_t>("restype_id");
		Item->Resistance = Database->GetInt<int>("res");
		Item->AttackDelay = Database->GetReal("attack_delay");
		Item->AttackTime = Database->GetReal("attack_time");
		Item->Cooldown = Database->GetReal("cooldown");
		Item->Tradable = Database->GetInt<int>("tradable");
		Item->TargetAlive = Database->GetInt<int>("target_alive");
		Item->TargetID = (TargetType)Database->GetInt<int>("target_id");
		Item->Scope = (ScopeType)Database->GetInt<int>("scope_id");
		Item->UnlockID = Database->GetInt<uint32_t>("unlock_id");

		if(!Headless && Item->Texture == nullptr && TexturePath != "")
			throw std::runtime_error("Can't find texture " + TexturePath);

		OldItems[Item->ID] = Item;
	}
	Database->CloseQuery();

	OldItems[0] = nullptr;
}

// Loads vendor data
void _Stats::OldLoadVendors() {
	OldVendors.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM vendor");

	// Get data
	_OldVendor Vendor;
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
			Vendor.Items.push_back(OldItems[Database->GetInt<uint32_t>("item_id", 1)]);
		}
		Database->CloseQuery(1);

		OldVendors[Vendor.ID] = Vendor;
	}
	Database->CloseQuery();
}

// Loads trader data
void _Stats::OldLoadTraders() {
	OldTraders.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM trader");

	// Get data
	_OldTrader Trader;
	while(Database->FetchRow()) {
		Trader.ID = Database->GetInt<uint32_t>("id");
		Trader.Name = Database->GetString("name");
		Trader.RewardItem = OldItems[Database->GetInt<uint32_t>("item_id")];
		Trader.Upgrades = 0;
		Trader.Count = Database->GetInt<int>("count");
		Trader.Items.clear();

		// Get items
		Database->PrepareQuery("SELECT item_id, count FROM traderitem where trader_id = @trader_id", 1);
		Database->BindInt(1, Trader.ID, 1);
		while(Database->FetchRow(1)) {
			_TraderItem TraderItem;
			TraderItem.Item = OldItems[Database->GetInt<uint32_t>("item_id", 1)];
			TraderItem.Count = Database->GetInt<int>("count", 1);
			Trader.Items.push_back(TraderItem);
		}
		Database->CloseQuery(1);

		OldTraders[Trader.ID] = Trader;
	}
	Database->CloseQuery();
}

// Loads blacksmith data
void _Stats::OldLoadBlacksmiths() {
	OldBlacksmiths.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM blacksmith");

	// Get data
	_OldBlacksmith Blacksmith;
	while(Database->FetchRow()) {
		Blacksmith.ID = Database->GetInt<uint32_t>("id");
		Blacksmith.Name = Database->GetString("name");
		Blacksmith.Level = Database->GetInt<int>("level");
		OldBlacksmiths[Blacksmith.ID] = Blacksmith;
	}
	Database->CloseQuery();
}

// Load minigames
void _Stats::OldLoadMinigames() {
	OldMinigames.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM minigame");

	// Get data
	_OldMinigameType Minigame;
	while(Database->FetchRow()) {
		Minigame.ID = Database->GetInt<uint32_t>("id");
		Minigame.Name = Database->GetString("name");
		Minigame.Script = Database->GetString("script");
		Minigame.Cost = Database->GetInt<int>("cost");
		Minigame.RequiredItem = OldItems[Database->GetInt<uint32_t>("item_id")];

		// Get items
		Database->PrepareQuery("SELECT item_id, count FROM minigameitem where minigame_id = @minigame_id", 1);
		Database->BindInt(1, Minigame.ID, 1);
		while(Database->FetchRow(1)) {
			_MinigameItem MinigameItem;
			MinigameItem.Item = OldItems[Database->GetInt<uint32_t>("item_id", 1)];
			MinigameItem.Count = Database->GetInt<int>("count", 1);
			Minigame.Items.push_back(MinigameItem);
		}
		Database->CloseQuery(1);

		OldMinigames[Minigame.ID] = Minigame;
	}
	Database->CloseQuery();
}

// Load model textures
void _Stats::OldLoadModels() {
	OldModels.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM model");

	// Get data
	_OldModel Model;
	while(Database->FetchRow()) {
		Model.ID = Database->GetInt<uint32_t>("id");
		Model.Texture = ae::Assets.Textures[Database->GetString("texture")];

		OldModels[Model.ID] = Model;
	}
	Database->CloseQuery();
}

// Load preset builds
void _Stats::OldLoadBuilds() {

	// Get build info
	Database->PrepareQuery("SELECT * FROM build");
	while(Database->FetchRow()) {
		uint32_t BuildID = Database->GetInt<uint32_t>("id");
		if(BuildID == 0)
			continue;

		// Create object
		_Object *Object = new _Object();
		Object->Name = std::string("build_") + Database->GetString("name");
		Object->ModelID = Database->GetInt<uint32_t>("model_id");
		Object->Character->ActionBar.resize(Database->GetInt<uint32_t>("actionbarsize"));

		// Get items
		Database->PrepareQuery("SELECT * FROM builditem WHERE build_id = @build_id", 1);
		Database->BindInt(1, BuildID, 1);
		while(Database->FetchRow(1)) {
			const _Item *Item = OldItems[Database->GetInt<uint32_t>("item_id", 1)];
			if(Item)
				Object->Inventory->AddItem(Item, 0, Database->GetInt<int>("count", 1));
		}
		Database->CloseQuery(1);

		// Get skills
		Database->PrepareQuery("SELECT * FROM buildskill WHERE build_id = @build_id", 1);
		Database->BindInt(1, BuildID, 1);
		while(Database->FetchRow(1)) {
			const _Item *Item = OldItems[Database->GetInt<uint32_t>("item_id", 1)];
			if(Item)
				Object->Character->Skills[Item->ID] = Database->GetInt<int>("level", 1);
		}
		Database->CloseQuery(1);

		// Get actionbar
		Database->PrepareQuery("SELECT * FROM buildactionbar WHERE build_id = @build_id", 1);
		Database->BindInt(1, BuildID, 1);
		while(Database->FetchRow(1)) {
			const _Item *Item = OldItems[Database->GetInt<uint32_t>("item_id", 1)];
			if(Item) {
				size_t Slot = (size_t)Database->GetInt<uint32_t>("slot", 1);
				if(Slot < Object->Character->ActionBar.size())
					Object->Character->ActionBar[Slot].Item = Item;
			}
		}
		Database->CloseQuery(1);

		// Save build
		OldBuilds[BuildID] = Object;
	}
	Database->CloseQuery();

	OldBuilds[0] = nullptr;
}

// Load scripts
void _Stats::OldLoadScripts() {
	OldScripts.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM script");

	// Get data
	_OldScript Script;
	while(Database->FetchRow()) {
		Script.ID = Database->GetInt<uint32_t>("id");
		Script.Name = Database->GetString("name");
		Script.Level = Database->GetInt<int>("level");
		Script.Cooldown = Database->GetReal("cooldown");

		OldScripts[Script.ID] = Script;
	}
	Database->CloseQuery();
}

// Load lights
void _Stats::OldLoadLights() {
	OldLights.clear();

	// Run query
	Database->PrepareQuery("SELECT * FROM light");

	// Get data
	_OldLightType Light;
	while(Database->FetchRow()) {
		Light.ID = Database->GetInt<uint32_t>("id");
		Light.Name = Database->GetString("name");
		Light.Color[0] = Database->GetReal("r");
		Light.Color[1] = Database->GetReal("g");
		Light.Color[2] = Database->GetReal("b");
		Light.Radius = Database->GetReal("radius");

		OldLights[Light.ID] = Light;
	}
	Database->CloseQuery();
}

// Gets monsters stats from the database
void _Stats::GetMonsterStats(uint32_t MonsterID, _Object *Object, double Difficulty) const {
	Object->Monster->DatabaseID = MonsterID;

	// Run query
	Database->PrepareQuery("SELECT m.*, ai.name as ai_name FROM monster m, ai WHERE m.ai_id = ai.id AND m.id = @monster_id");
	Database->BindInt(1, MonsterID);

	// Get data
	if(Database->FetchRow()) {
		Object->Character->Level = Database->GetInt<int>("level");
		Object->Name = Database->GetString("name");
		//Object->Character->Portrait = ae::Assets.Textures[Database->GetString("portrait")];
		Object->Character->BaseMaxHealth = (int)(Database->GetInt<int>("health") * Difficulty);
		Object->Character->BaseMaxMana = Database->GetInt<int>("mana");
		Object->Character->BaseMinDamage = Database->GetInt<int>("mindamage");
		Object->Character->BaseMaxDamage = Database->GetInt<int>("maxdamage");
		Object->Character->BaseArmor = Database->GetInt<int>("armor");
		Object->Character->BaseDamageBlock = Database->GetInt<int>("block");
		Object->Character->BaseAttackPeriod = Database->GetReal("attackperiod");
		Object->Monster->ExperienceGiven = Database->GetInt<int>("experience");
		Object->Monster->GoldGiven = Database->GetInt<int>("gold");
		Object->Monster->AI = Database->GetString("ai_name");
		uint32_t BuildID = Database->GetInt<uint32_t>("build_id");

		// Load build
		const auto &BuildIterator = OldBuilds.find(BuildID);
		if(BuildIterator == OldBuilds.end())
			throw std::runtime_error("Can't find build_id " + std::to_string(BuildID));

		const _Object *Build = BuildIterator->second;

		// Copy build
		Object->Inventory->Bags = Build->Inventory->GetBags();
		Object->Character->ActionBar = Build->Character->ActionBar;
		Object->Character->Skills = Build->Character->Skills;
		Object->Character->Health = Object->Character->MaxHealth = Object->Character->BaseMaxHealth;
		Object->Character->Mana = Object->Character->MaxMana = Object->Character->BaseMaxMana;
		Object->Character->Gold = Object->Monster->GoldGiven;
		Object->Character->CalcLevelStats = false;
	}

	// Free memory
	Database->CloseQuery();
}

// Get portrait texture by network id
const _Portrait *_Stats::GetPortrait(uint8_t NetworkID) const {
	const auto &Iterator = PortraitsIndex.find(NetworkID);
	if(Iterator == PortraitsIndex.end())
		return nullptr;

	return Iterator->second;
}

// Get list of portraits sorted by rank
void _Stats::GetPortraits(std::list<_Portrait> &PortraitList) const {
	for(const auto &Portrait : Portraits)
		PortraitList.push_back(Portrait.second);

	PortraitList.sort(ComparePortrait);
}

// Get list of builds
void _Stats::GetStartingBuilds(std::list<_OldBuild> &Builds) const {

	// Run query
	Database->PrepareQuery("SELECT * FROM build WHERE starting = 1 ORDER BY rank");
	while(Database->FetchRow()) {
		_OldBuild Build;
		Build.ID = Database->GetInt<uint32_t>("id");
		Build.Name = Database->GetString("name");
		Build.Texture = ae::Assets.Textures[Database->GetString("texture")];

		Builds.push_back(Build);
	}

	Database->CloseQuery();
}

// Randomly generates a list of monsters from a zone
void _Stats::GenerateMonsterListFromZone(int AdditionalCount, uint32_t ZoneID, std::list<uint32_t> &Monsters, bool &Boss, double &Cooldown) const {
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
		int MonsterCount = ae::GetRandomInt(MinSpawn, MaxSpawn);

		// No monsters
		if(MonsterCount == 0)
			return;

		MonsterCount += AdditionalCount;

		// Cap monster count
		MonsterCount = std::min(MonsterCount, BATTLE_MAX_OBJECTS_PER_SIDE);

		// Run query
		Database->PrepareQuery("SELECT * FROM zonedata WHERE zone_id = @zone_id");
		Database->BindInt(1, ZoneID);

		// Get monsters in zone
		std::vector<_OldZone> Zone;
		uint32_t OddsSum = 0;
		int MaxTotal = 0;
		bool HasZeroMax = true;
		while(Database->FetchRow()) {

			// Get zone data
			_OldZone ZoneData;
			ZoneData.MonsterID = Database->GetInt<uint32_t>("monster_id");
			ZoneData.Odds = Database->GetInt<uint32_t>("odds");
			ZoneData.Max = Database->GetInt<int>("max");

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
							Monsters.push_back(ZoneData.MonsterID);
						}
						break;
					}
				}
			}
		}
	}
}

// Generates a list of items dropped from a monster
void _Stats::GenerateItemDrops(uint32_t MonsterID, uint32_t Count, int DropRate, std::list<uint32_t> &ItemDrops) const {
	if(MonsterID == 0)
		return;

	// Run query
	Database->PrepareQuery("SELECT item_id, odds FROM monsterdrop WHERE monster_id = @monster_id");
	Database->BindInt(1, MonsterID);

	// Get list of possible drops and build CDT
	std::list<_OldItemDrop> PossibleItemDrops;
	uint32_t OddsSum = 0;
	while(Database->FetchRow()) {
		uint32_t ItemID = Database->GetInt<uint32_t>("item_id");
		uint32_t Odds = 100 * Database->GetInt<uint32_t>("odds");

		float Scale = 1.0f;
		if(ItemID == 0)
			Scale = BATTLE_NOTHINGDROP_SCALE;

		// Improve odds of items
		Odds *= 1.0f + DropRate / 100.0f * Scale;

		OddsSum += Odds;
		PossibleItemDrops.push_back(_OldItemDrop(ItemID, OddsSum));
	}
	Database->CloseQuery();

	// Check for items
	if(OddsSum > 0) {

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

// Convert vendor slot from item id
size_t _OldVendor::GetSlotFromID(uint32_t ID) const {

	size_t Index = 0;
	for(const auto &Item : Items) {
		if(Item->ID == ID)
			return Index;

		Index++;
	}

	return NOSLOT;
}
