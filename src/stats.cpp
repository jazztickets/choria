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
#include <ae/files.h>
#include <objects/object.h>
#include <objects/map.h>
#include <objects/buff.h>
#include <objects/components/character.h>
#include <objects/components/inventory.h>
#include <objects/components/monster.h>
#include <constants.h>
#include <tinyxml2.h>
#include <algorithm>
#include <iostream>

// Compare function for sorting portraits
bool ComparePortrait(const _Portrait *First, const _Portrait *Second) {
	return First->NetworkID < Second->NetworkID;
}

// Compare function for sorting models
bool CompareBuild(const _Object *First, const _Object *Second) {
	return First->NetworkID < Second->NetworkID;
}

// Constructor
_Stats::_Stats(bool Headless) :
	Headless(Headless) {

	// Load database that stores game data
	Database = new ae::_Database("stats/stats.db", true);

	// Load game data
	LoadTypes();
	LoadMapDirectory();
	LoadData("data/stats.xml");

	// Load spreadsheet data
	OldLoadLevels();
	OldLoadBuffs();
	OldLoadStatTypes();
	OldLoadTraders();
	OldLoadBlacksmiths();
	OldLoadMinigames();
	OldLoadScripts();
	OldLoadLights();
}

// Destructor
_Stats::~_Stats() {

	for(const auto &Build : Builds)
		delete Build.second;

	for(const auto &Buff : OldBuffs)
		delete Buff.second;

	delete Database;
}

// Get string attribute from node
const char *_Stats::GetString(tinyxml2::XMLElement *Node, const char *Attribute, bool Required) {
	const char *Value = Node->Attribute(Attribute);
	if(!Value) {
		if(Required)
			throw std::runtime_error("Missing '" + std::string(Attribute) + "' attribute in '" + std::string(Node->Name()) + "' node!");

		return "";
	}

	return Value;
}

// Get a valid texture from an id attribute
const ae::_Texture *_Stats::GetTexture(tinyxml2::XMLElement *Node, const char *Attribute) {
	std::string Value = GetString(Node, Attribute);

	// Search
	const auto &Iterator = ae::Assets.Textures.find(Value);
	if(!Headless && Iterator == ae::Assets.Textures.end())
		throw std::runtime_error("Cannot find texture '" + Value + "' for attribute '" + std::string(Attribute) + "' in element '" + std::string(Node->Name()) + "'");

	return Iterator->second;
}

// Get a valid item from an id attribute
const _BaseItem *_Stats::GetItem(tinyxml2::XMLElement *Node, const char *Attribute, bool AllowNone) {
	std::string Value = GetString(Node, Attribute);
	if(AllowNone && Value == "none")
		return nullptr;

	// Search
	const auto &Iterator = Items.find(Value);
	if(Iterator == Items.end())
		throw std::runtime_error("Cannot find item '" + Value + "' for attribute '" + std::string(Attribute) + "' in element '" + std::string(Node->Name()) + "'");

	return &Iterator->second;
}

// Get a valid monster from an id attribute
const _MonsterStat *_Stats::GetMonster(tinyxml2::XMLElement *Node, const char *Attribute) {
	std::string Value = GetString(Node, Attribute);

	// Search
	const auto &Iterator = Monsters.find(Value);
	if(Iterator == Monsters.end())
		throw std::runtime_error("Cannot find monster '" + Value + "' for attribute '" + std::string(Attribute) + "' in element '" + std::string(Node->Name()) + "'");

	return &Iterator->second;
}

// Load types
void _Stats::LoadTypes() {
	EventTypes = {
		{ EventType::NONE,        { "",   "None"         } },
		{ EventType::SCRIPT,      { "SC", "Script"       } },
		{ EventType::SPAWN,       { "S",  "Spawn"        } },
		{ EventType::MAPENTRANCE, { "ME", "Map Entrance" } },
		{ EventType::MAPCHANGE,   { "M",  "Map Change"   } },
		{ EventType::PORTAL,      { "P",  "Portal"       } },
		{ EventType::JUMP,        { "J",  "Jump"         } },
		{ EventType::KEY,         { "K",  "Key"          } },
		{ EventType::VENDOR,      { "V",  "Vendor"       } },
		{ EventType::TRADER,      { "T",  "Trader"       } },
		{ EventType::BLACKSMITH,  { "B",  "Black Smith"  } },
		{ EventType::MINIGAME,    { "G",  "Minigame"     } },
	};

	ScopeTypes = {
		{ ScopeType::NONE,   { "none",   "None"   } },
		{ ScopeType::WORLD,  { "world",  "World"  } },
		{ ScopeType::BATTLE, { "battle", "Battle" } },
		{ ScopeType::ALL,    { "all",    "All"    } },
	};

	TargetTypes = {
		{ TargetType::NONE,              { "none",             "None"             } },
		{ TargetType::SELF,              { "self",             "Self"             } },
		{ TargetType::ENEMY,             { "enemy",            "Enemy"            } },
		{ TargetType::ALLY,              { "ally",             "Ally"             } },
		{ TargetType::MULTIPLE_ENEMIES,  { "multiple_enemies", "Multiple Enemies" } },
		{ TargetType::MULTIPLE_ALLIES,   { "multiple_allies",  "Multiple Allies"  } },
		{ TargetType::ALL_ENEMIES,       { "all_enemies",      "All Enemies"      } },
		{ TargetType::ALL_ALLIES,        { "all_allies",       "All Allies"       } },
		{ TargetType::ANY,               { "any",              "Any"              } },
		{ TargetType::ALL,               { "all",              "All"              } },
	};

	ItemTypes = {
		{ ItemType::NONE,             { "none",       "None"              } },
		{ ItemType::SKILL,            { "skill",      "Skill"             } },
		{ ItemType::HELMET,           { "helmet",     "Helmet"            } },
		{ ItemType::ARMOR,            { "armor",      "Armor"             } },
		{ ItemType::BOOTS,            { "boots",      "Boots"             } },
		{ ItemType::ONEHANDED_WEAPON, { "one_hand",   "One-Handed Weapon" } },
		{ ItemType::TWOHANDED_WEAPON, { "two_hand",   "Two-Handed Weapon" } },
		{ ItemType::SHIELD,           { "shield",     "Shield"            } },
		{ ItemType::RING,             { "ring",       "Ring"              } },
		{ ItemType::AMULET,           { "amulet",     "Amulet"            } },
		{ ItemType::CONSUMABLE,       { "consumable", "Consumable"        } },
		{ ItemType::TRADABLE,         { "tradable",   "Tradable"          } },
		{ ItemType::UNLOCKABLE,       { "unlockable", "Unlockable"        } },
		{ ItemType::KEY,              { "key",        "Key"               } },
	};

	DamageTypes = {
		{ DamageType::NONE,      { "none",      "None"      } },
		{ DamageType::ALL,       { "all",       "All"       } },
		{ DamageType::PHYSICAL,  { "physical",  "Physical"  } },
		{ DamageType::FIRE,      { "fire",      "Fire"      } },
		{ DamageType::COLD,      { "cold",      "Cold"      } },
		{ DamageType::LIGHTNING, { "lightning", "Lightning" } },
		{ DamageType::DIVINE,    { "divine",    "Divine"    } },
		{ DamageType::DARK,      { "dark",      "Dark"      } },
		{ DamageType::POISON,    { "poison",    "Poison"    } },
		{ DamageType::BLEED,     { "bleed",     "Bleed"     } },
	};
}

// Load map directory and build map array
void _Stats::LoadMapDirectory() {
	ae::NetworkIDType NetworkID = 1;
	ae::_Files Files(MAPS_PATH);
	for(const auto &File : Files.Nodes) {

		// Get clean map name
		size_t DotPosition = File.find(".map.gz");
		std::string Name;
		if(DotPosition == 0)
			throw std::runtime_error("Bad map name: " + File);

		Name = File.substr(0, DotPosition);
		MapsIndex[Name] = NetworkID;
	}
	MapsIndex[""] = 0;
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

	// Build index for types
	std::unordered_map<std::string, ScopeType> ScopeTypesIndex;
	for(const auto &Scope : ScopeTypes)
		ScopeTypesIndex[Scope.second.first] = Scope.first;

	std::unordered_map<std::string, TargetType> TargetTypesIndex;
	for(const auto &TargetType : TargetTypes)
		TargetTypesIndex[TargetType.second.first] = TargetType.first;

	std::unordered_map<std::string, ItemType> ItemTypesIndex;
	for(const auto &ItemType : ItemTypes)
		ItemTypesIndex[ItemType.second.first] = ItemType.first;

	// Load portraits
	NetworkID = 1;
	for(tinyxml2::XMLElement *Node = Nodes["portraits"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_Portrait Portrait;
		Portrait.ID = GetString(Node, "id");
		if(Portraits.find(Portrait.ID) != Portraits.end())
			throw std::runtime_error("Duplicate portraits id '" + Portrait.ID + "' in " + Path);

		Portrait.Texture = GetTexture(Node, "texture");
		Portrait.NetworkID = NetworkID++;

		Portraits[Portrait.ID] = Portrait;
		PortraitsIndex[Portrait.NetworkID] = &Portraits[Portrait.ID];
	}

	// Load models
	NetworkID = 1;
	for(tinyxml2::XMLElement *Node = Nodes["models"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_Model Model;
		Model.ID = GetString(Node, "id");
		if(Models.find(Model.ID) != Models.end())
			throw std::runtime_error("Duplicate model id '" + Model.ID + "' in " + Path);

		Model.Texture = GetTexture(Node, "texture");
		Model.NetworkID = NetworkID++;

		Models[Model.ID] = Model;
		ModelsIndex[Model.NetworkID] = &Models[Model.ID];
	}

	// Load skills
	NetworkID = 1;
	for(tinyxml2::XMLElement *Node = Nodes["skills"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_BaseItem Skill;
		Skill.ID = GetString(Node, "id");
		if(Items.find(Skill.ID) != Items.end())
			throw std::runtime_error("Duplicate skill id '" + Skill.ID + "' in " + Path);

		Skill.Name = GetString(Node, "name");
		Skill.Texture = GetTexture(Node, "texture");

		// Get target
		tinyxml2::XMLElement *TargetNode = Node->FirstChildElement("target");
		if(TargetNode) {
			Skill.Target = TargetTypesIndex[GetString(TargetNode, "type")];
			Skill.TargetAlive = TargetNode->BoolAttribute("alive");
		}

		// Get use
		tinyxml2::XMLElement *UseNode = Node->FirstChildElement("use");
		if(UseNode) {
			Skill.Scope = ScopeTypesIndex[GetString(UseNode, "scope")];
			Skill.Script = GetString(UseNode, "script");
		}

		Skill.NetworkID = NetworkID++;
		Items[Skill.ID] = Skill;
		ItemsIndex[Skill.NetworkID] = &Items[Skill.ID];
	}

	// Load items
	NetworkID = 1;
	for(tinyxml2::XMLElement *Node = Nodes["items"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_BaseItem Item;
		Item.ID = GetString(Node, "id");
		if(Items.find(Item.ID) != Items.end())
			throw std::runtime_error("Duplicate item id '" + Item.ID + "' in " + Path);

		Item.Name = GetString(Node, "name");

		// Item type
		Item.Type = ItemTypesIndex[GetString(Node, "type")];
		if(Item.Type == ItemType::NONE)
			throw std::runtime_error("Bad item type for '" + Item.ID + "' in " + Path);

		// Texture
		Item.Texture = GetTexture(Node, "texture");

		// Get prices
		tinyxml2::XMLElement *PriceNode = Node->FirstChildElement("price");
		if(PriceNode) {
			Item.Cost = PriceNode->IntAttribute("buy");
			Item.Tradable = PriceNode->BoolAttribute("tradable");
		}

		// Get damage
		tinyxml2::XMLElement *DamageNode = Node->FirstChildElement("damage");
		if(DamageNode) {
			Item.MinDamage = DamageNode->IntAttribute("min");
			Item.MaxDamage = DamageNode->IntAttribute("max");
		}

		// Get use stats
		tinyxml2::XMLElement *UseNode = Node->FirstChildElement("use");
		if(UseNode) {
			Item.AttackDelay = UseNode->DoubleAttribute("attack_delay");
			Item.AttackTime = UseNode->DoubleAttribute("attack_time");
			Item.Cooldown = UseNode->DoubleAttribute("cooldown");
			Item.Scope = ScopeTypesIndex[GetString(UseNode, "scope", false)];
		}

		Item.NetworkID = NetworkID++;
		Items[Item.ID] = Item;
		ItemsIndex[Item.NetworkID] = &Items[Item.ID];
	}

	// Load builds
	NetworkID = 1;
	for(tinyxml2::XMLElement *Node = Nodes["builds"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_Object *Object = new _Object();
		Object->Name = GetString(Node, "name");
		Object->NetworkID = NetworkID++;
		Object->Model = &Models.at(GetString(Node, "model"));
		Object->BuildTexture = ae::Assets.Textures[GetString(Node, "texture")];
		if(!Object->BuildTexture)
			throw std::runtime_error("Cannot find build texture for build '" + Object->Name + "' in " + Path);
		if(Builds.find(Object->Name) != Builds.end())
			throw std::runtime_error("Duplicate build name '" + Object->Name + "' in " + Path);
		Builds[Object->Name] = Object;
	}

	// Load vendors
	NetworkID = 1;
	for(tinyxml2::XMLElement *Node = Nodes["vendors"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_Vendor Vendor;
		Vendor.ID = GetString(Node, "id");
		if(Vendors.find(Vendor.ID) != Vendors.end())
			throw std::runtime_error("Duplicate vendor id '" + Vendor.ID + "' in " + Path);

		// Load items
		for(tinyxml2::XMLElement *ItemNode = Node->FirstChildElement("item"); ItemNode != nullptr; ItemNode = ItemNode->NextSiblingElement()) {
			const _BaseItem *Item = GetItem(ItemNode, "id");
			Vendor.Items.push_back(Item);
		}

		Vendor.NetworkID = NetworkID++;
		Vendors[Vendor.ID] = Vendor;
	}

	// Load monsters
	NetworkID = 1;
	for(tinyxml2::XMLElement *Node = Nodes["monsters"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_MonsterStat Monster;
		Monster.ID = GetString(Node, "id");
		if(Monsters.find(Monster.ID) != Monsters.end())
			throw std::runtime_error("Duplicate monster id '" + Monster.ID + "' in " + Path);

		Monster.Name = GetString(Node, "name");
		Monster.Texture = GetTexture(Node, "texture");

		// Get stats
		tinyxml2::XMLElement *StatsNode = Node->FirstChildElement("stats");
		if(StatsNode) {
			Monster.AI = GetString(StatsNode, "ai");
			Monster.Health = StatsNode->IntAttribute("health");
			Monster.Mana = StatsNode->IntAttribute("mana");
		}

		// Get damage
		tinyxml2::XMLElement *DamageNode = Node->FirstChildElement("damage");
		if(DamageNode) {
			Monster.MinDamage = DamageNode->IntAttribute("min");
			Monster.MaxDamage = DamageNode->IntAttribute("max");
		}

		// Load actions
		tinyxml2::XMLElement *ActionsNode = Node->FirstChildElement("actions");
		if(!ActionsNode)
			throw std::runtime_error("No skills node for monster id '" + Monster.ID + "' in " + Path);

		for(tinyxml2::XMLElement *ActionNode = ActionsNode->FirstChildElement("action"); ActionNode != nullptr; ActionNode = ActionNode->NextSiblingElement()) {
			const _BaseItem *Item = GetItem(ActionNode, "id");

			//TODO add monster action struct
			Monster.Actions.push_back(Item);
		}

		// Load drops
		tinyxml2::XMLElement *DropsNode = Node->FirstChildElement("drops");
		if(DropsNode) {
			for(tinyxml2::XMLElement *DropNode = DropsNode->FirstChildElement("drop"); DropNode != nullptr; DropNode = DropNode->NextSiblingElement()) {
				_Drop Drop;
				Drop.Item = GetItem(DropNode, "id", true);
				Drop.Odds = DropNode->UnsignedAttribute("odds");
				Monster.Drops.push_back(Drop);
			}
		}

		Monster.NetworkID = NetworkID++;
		Monsters[Monster.ID] = Monster;
	}

	// Load zones
	for(tinyxml2::XMLElement *Node = Nodes["zones"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_Zone Zone;
		Zone.ID = GetString(Node, "id");
		if(Zones.find(Zone.ID) != Zones.end())
			throw std::runtime_error("Duplicate zone id '" + Zone.ID + "' in " + Path);

		Zone.Min = Node->IntAttribute("min", 1);
		Zone.Max = Node->IntAttribute("max", Zone.Min);

		// Load monsters
		for(tinyxml2::XMLElement *MonsterNode = Node->FirstChildElement("monster"); MonsterNode != nullptr; MonsterNode = MonsterNode->NextSiblingElement()) {
			_ZoneMonster ZoneMonster;
			ZoneMonster.Monster = GetMonster(MonsterNode, "id");
			ZoneMonster.Odds = MonsterNode->UnsignedAttribute("odds");
			Zone.Monsters.push_back(ZoneMonster);
		}

		Zones[Zone.ID] = Zone;
	}
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
		//Trader.RewardItem = OldItems[Database->GetInt<uint32_t>("item_id")];
		Trader.Upgrades = 0;
		Trader.Count = Database->GetInt<int>("count");
		Trader.Items.clear();

		// Get items
		Database->PrepareQuery("SELECT item_id, count FROM traderitem where trader_id = @trader_id", 1);
		Database->BindInt(1, Trader.ID, 1);
		while(Database->FetchRow(1)) {
			_TraderItem TraderItem;
			//TraderItem.Item = OldItems[Database->GetInt<uint32_t>("item_id", 1)];
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
		//Minigame.RequiredItem = OldItems[Database->GetInt<uint32_t>("item_id")];

		// Get items
		Database->PrepareQuery("SELECT item_id, count FROM minigameitem where minigame_id = @minigame_id", 1);
		Database->BindInt(1, Minigame.ID, 1);
		while(Database->FetchRow(1)) {
			_MinigameItem MinigameItem;
			//MinigameItem.Item = OldItems[Database->GetInt<uint32_t>("item_id", 1)];
			MinigameItem.Count = Database->GetInt<int>("count", 1);
			Minigame.Items.push_back(MinigameItem);
		}
		Database->CloseQuery(1);

		OldMinigames[Minigame.ID] = Minigame;
	}
	Database->CloseQuery();
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
		Object->Monster->ExperienceGiven = Database->GetInt<int>("experience");
		Object->Monster->GoldGiven = Database->GetInt<int>("gold");
		Object->Monster->AI = Database->GetString("ai_name");
		Object->Character->Health = Object->Character->MaxHealth = Object->Character->BaseMaxHealth;
		Object->Character->Mana = Object->Character->MaxMana = Object->Character->BaseMaxMana;
		Object->Character->Gold = Object->Monster->GoldGiven;
		Object->Character->CalcLevelStats = false;
	}

	// Free memory
	Database->CloseQuery();
}

// Get portrait by network id
const _Portrait *_Stats::GetPortrait(uint8_t NetworkID) const {
	const auto &Iterator = PortraitsIndex.find(NetworkID);
	if(Iterator == PortraitsIndex.end())
		return nullptr;

	return Iterator->second;
}

// Get model by network id
const _Model *_Stats::GetModel(uint8_t NetworkID) const {
	const auto &Iterator = ModelsIndex.find(NetworkID);
	if(Iterator == ModelsIndex.end())
		return nullptr;

	return Iterator->second;
}

// Get list of portraits sorted by rank
void _Stats::GetPortraits(std::list<const _Portrait *> &PortraitList) const {
	for(const auto &Portrait : Portraits)
		PortraitList.push_back(&Portrait.second);

	PortraitList.sort(ComparePortrait);
}

// Get list of starting builds sorted by rank
void _Stats::GetStartingBuilds(std::list<const _Object *> &BuildsList) const {
	for(const auto &Build : Builds)
		BuildsList.push_back(Build.second);

	BuildsList.sort(CompareBuild);
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
size_t _Vendor::GetSlotFromID(const std::string &ItemID) const {
	size_t Index = 0;
	for(const auto &Item : Items) {
		if(Item->ID == ItemID)
			return Index;

		Index++;
	}

	return NOSLOT;
}
