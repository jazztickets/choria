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

// Constants
const int DEFAULT_ACTIONBAR_SIZE = 4;

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
	OldLoadStatTypes();
	OldLoadLights();
}

// Destructor
_Stats::~_Stats() {

	for(const auto &Build : Builds)
		delete Build.second;

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
		{ "buffs", DataNode->FirstChildElement("buffs") },
		{ "skills", DataNode->FirstChildElement("skills") },
		{ "weapon_types", DataNode->FirstChildElement("weapon_types") },
		{ "items", DataNode->FirstChildElement("items") },
		{ "builds", DataNode->FirstChildElement("builds") },
		{ "vendors", DataNode->FirstChildElement("vendors") },
		{ "traders", DataNode->FirstChildElement("traders") },
		{ "blacksmiths", DataNode->FirstChildElement("blacksmiths") },
		{ "minigames", DataNode->FirstChildElement("minigames") },
		{ "monsters", DataNode->FirstChildElement("monsters") },
		{ "zones", DataNode->FirstChildElement("zones") },
	});

	// Check nodes
	for(const auto &Node : Nodes) {
		if(!Node.second)
			throw std::runtime_error("Missing '" + Node.first + "' node in " + Path);
	}

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
	uint8_t NetworkID = 1;
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

	// Load buffs
	NetworkID = 1;
	for(tinyxml2::XMLElement *Node = Nodes["buffs"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_Buff Buff;
		Buff.ID = GetString(Node, "id");
		if(Buffs.find(Buff.ID) != Buffs.end())
			throw std::runtime_error("Duplicate buff id '" + Buff.ID + "' in " + Path);

		Buff.Name = GetString(Node, "name");
		Buff.Script = GetString(Node, "script");
		Buff.Texture = GetTexture(Node, "texture");

		Buff.NetworkID = NetworkID++;
		Buffs[Buff.ID] = Buff;
		BuffsIndex[Buff.NetworkID] = &Buffs[Buff.ID];
	}

	// Load skills
	NetworkID = 1;
	for(tinyxml2::XMLElement *Node = Nodes["skills"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_BaseItem Skill;
		Skill.ID = GetString(Node, "id");
		if(Items.find(Skill.ID) != Items.end())
			throw std::runtime_error("Duplicate skill id '" + Skill.ID + "' in " + Path);

		Skill.Type = ItemType::SKILL;
		Skill.Name = GetString(Node, "name");
		Skill.Texture = GetTexture(Node, "texture");
		Skill.MaxLevel = Node->IntAttribute("max_level", 1);

		// Get target
		tinyxml2::XMLElement *TargetNode = Node->FirstChildElement("target");
		if(TargetNode) {
			Skill.Target = TargetTypesIndex[GetString(TargetNode, "type")];
			Skill.TargetAlive = TargetNode->BoolAttribute("alive", true);
		}

		// Get use
		tinyxml2::XMLElement *UseNode = Node->FirstChildElement("use");
		if(UseNode) {
			Skill.Scope = ScopeTypesIndex[GetString(UseNode, "scope")];
			Skill.Script = GetString(UseNode, "script");
		}

		Skill.Stats = this;
		Skill.NetworkID = NetworkID++;
		Items[Skill.ID] = Skill;
		ItemsIndex[Skill.NetworkID] = &Items[Skill.ID];
	}

	// Load weapon types
	for(tinyxml2::XMLElement *Node = Nodes["weapon_types"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_WeaponType WeaponType;
		WeaponType.ID = GetString(Node, "id");
		if(WeaponTypes.find(WeaponType.ID) != WeaponTypes.end())
			throw std::runtime_error("Duplicate weapon_type id '" + WeaponType.ID + "' in " + Path);

		WeaponType.Name = GetString(Node, "name");

		// Load skills granted
		for(tinyxml2::XMLElement *SkillNode = Node->FirstChildElement("skills"); SkillNode != nullptr; SkillNode = SkillNode->NextSiblingElement()) {
			const _BaseItem *Skill = GetItem(SkillNode, "id");
			WeaponType.Skills.push_back(Skill);
		}

		WeaponTypes[WeaponType.ID] = WeaponType;
	}

	// Load items
	for(tinyxml2::XMLElement *Node = Nodes["items"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_BaseItem Item;
		Item.ID = GetString(Node, "id");
		if(Items.find(Item.ID) != Items.end())
			throw std::runtime_error("Duplicate item id '" + Item.ID + "' in " + Path);

		Item.Name = GetString(Node, "name");
		Item.Script = GetString(Node, "script", false);

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
			Item.Tradable = PriceNode->BoolAttribute("tradable", true);
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

		Item.Stats = this;
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
		Object->Character->ActionBar.resize(DEFAULT_ACTIONBAR_SIZE);
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

	// Load traders
	NetworkID = 1;
	for(tinyxml2::XMLElement *Node = Nodes["traders"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_Trader Trader;
		Trader.ID = GetString(Node, "id");
		if(Traders.find(Trader.ID) != Traders	.end())
			throw std::runtime_error("Duplicate trader id '" + Trader.ID + "' in " + Path);

		Trader.RewardItem = GetItem(Node, "reward_item");
		Trader.RewardCount = Node->IntAttribute("reward_count", 1);

		// Load required items
		for(tinyxml2::XMLElement *ItemNode = Node->FirstChildElement("item"); ItemNode != nullptr; ItemNode = ItemNode->NextSiblingElement()) {
			_TraderItem TraderItem;
			TraderItem.Item = GetItem(ItemNode, "id");
			TraderItem.Count = ItemNode->IntAttribute("count", 1);
			Trader.Items.push_back(TraderItem);
		}

		Trader.NetworkID = NetworkID++;
		Traders[Trader.ID] = Trader;
	}

	// Load blacksmiths
	NetworkID = 1;
	for(tinyxml2::XMLElement *Node = Nodes["blacksmiths"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_Blacksmith Blacksmith;
		Blacksmith.ID = GetString(Node, "id");
		if(Blacksmiths.find(Blacksmith.ID) != Blacksmiths.end())
			throw std::runtime_error("Duplicate blacksmith id '" + Blacksmith.ID + "' in " + Path);

		Blacksmith.Name = GetString(Node, "name");
		Blacksmith.Level = Node->IntAttribute("level");
		Blacksmith.NetworkID = NetworkID++;
		Blacksmiths[Blacksmith.ID] = Blacksmith;
	}

	// Load minigames
	NetworkID = 1;
	for(tinyxml2::XMLElement *Node = Nodes["minigames"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_MinigameStat Minigame;
		Minigame.ID = GetString(Node, "id");
		if(Minigames.find(Minigame.ID) != Minigames.end())
			throw std::runtime_error("Duplicate minigame id '" + Minigame.ID + "' in " + Path);

		Minigame.RequiredItem = GetItem(Node, "required_item");
		Minigame.Cost = Node->IntAttribute("count", 1);

		// Load prizes
		for(tinyxml2::XMLElement *ItemNode = Node->FirstChildElement("item"); ItemNode != nullptr; ItemNode = ItemNode->NextSiblingElement()) {
			_MinigameItem MinigameItem;
			MinigameItem.Item = GetItem(ItemNode, "id");
			MinigameItem.Count = ItemNode->IntAttribute("count", 1);
			Minigame.Items.push_back(MinigameItem);
		}

		Minigame.NetworkID = NetworkID++;
		Minigames[Minigame.ID] = Minigame;
	}

	// Load monsters
	NetworkID = 1;
	for(tinyxml2::XMLElement *Node = Nodes["monsters"]->FirstChildElement(); Node != nullptr; Node = Node->NextSiblingElement()) {
		_MonsterStat Monster;
		Monster.ID = GetString(Node, "id");
		if(Monsters.find(Monster.ID) != Monsters.end())
			throw std::runtime_error("Duplicate monster id '" + Monster.ID + "' in " + Path);

		Monster.Name = GetString(Node, "name");
		Monster.Portrait = &Portraits.at(GetString(Node, "portrait"));

		// Get stats
		tinyxml2::XMLElement *StatsNode = Node->FirstChildElement("stats");
		if(StatsNode) {
			Monster.AI = GetString(StatsNode, "ai");
			Monster.Health = StatsNode->IntAttribute("health");
			Monster.Mana = StatsNode->IntAttribute("mana");
			Monster.Experience = StatsNode->IntAttribute("experience");
			Monster.Gold = StatsNode->IntAttribute("gold");
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
				Drop.Odds = DropNode->IntAttribute("odds");
				Monster.Drops.push_back(Drop);
			}
		}

		Monster.NetworkID = NetworkID++;
		Monsters[Monster.ID] = Monster;
		MonstersIndex[Monster.NetworkID] = &Monsters[Monster.ID];
	}

	// Load zones
	NetworkID = 1;
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
			ZoneMonster.Odds = MonsterNode->IntAttribute("odds");
			ZoneMonster.Max = MonsterNode->IntAttribute("max");
			Zone.Monsters.push_back(ZoneMonster);
		}

		Zone.NetworkID = NetworkID++;
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

// Set object's monster stats
void _Stats::GetMonsterStats(const _MonsterStat *MonsterStat, _Object *Object, double Difficulty) const {
	Object->Monster->MonsterStat = MonsterStat;

	// Get data
	Object->Name = MonsterStat->Name;
	Object->Character->Portrait = MonsterStat->Portrait;
	Object->Character->BaseMaxHealth = (int)(MonsterStat->Health * Difficulty);
	Object->Character->BaseMaxMana = MonsterStat->Mana;
	Object->Character->BaseMinDamage = MonsterStat->MinDamage;
	Object->Character->BaseMaxDamage = MonsterStat->MaxDamage;
	Object->Character->BaseArmor = 0;
	Object->Character->BaseDamageBlock = 0;
	Object->Character->Health = Object->Character->MaxHealth = Object->Character->BaseMaxHealth;
	Object->Character->Mana = Object->Character->MaxMana = Object->Character->BaseMaxMana;
	Object->Character->Gold = MonsterStat->Gold;
	Object->Character->CalcLevelStats = false;
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
void _Stats::GenerateMonsterListFromZone(int AdditionalCount, const std::string &ZoneID, std::list<const _MonsterStat *> &Monsters, bool &Boss, double &Cooldown) const {
	if(ZoneID.empty())
		return;

	if(Zones.find(ZoneID) == Zones.end())
		return;

	// Get zone info
	const _Zone *Zone = &Zones.at(ZoneID);

	// If boss zone then use odds parameter as monster count
	if(Boss) {
		for(const auto &ZoneMonster : Zone->Monsters) {
			for(int i = 0; i < ZoneMonster.Odds; i++) {
				Monsters.push_back(ZoneMonster.Monster);
			}
		}
	}
	else {

		// Get random number of monsters
		int MonsterCount = ae::GetRandomInt(Zone->Min, Zone->Max);
		if(!MonsterCount)
			return;

		// Cap monster count
		MonsterCount += AdditionalCount;
		MonsterCount = std::min(MonsterCount, BATTLE_MAX_OBJECTS_PER_SIDE);

		// Build CDT for monsters
		std::vector<_ZoneMonster> ZoneCDT;
		int OddsSum = 0;
		int MaxTotal = 0;
		bool HasZeroMax = true;
		for(const auto &ZoneMonster : Zone->Monsters) {

			// Get zone data
			_ZoneMonster ZoneData;
			ZoneData.Monster = ZoneMonster.Monster;
			ZoneData.Odds = ZoneMonster.Odds;
			ZoneData.Max = ZoneMonster.Max;

			// Increase max for each player if set
			if(ZoneData.Max > 0) {
				MaxTotal += ZoneData.Max + AdditionalCount;
				ZoneData.Max += AdditionalCount;
			}
			else
				HasZeroMax = false;

			OddsSum += ZoneData.Odds;
			ZoneData.Odds = OddsSum;
			ZoneCDT.push_back(ZoneData);
		}

		// Check for monsters in zone
		if(OddsSum > 0) {

			// Cap monster count if all monsters have a max set
			if(HasZeroMax)
				MonsterCount = std::min(MaxTotal, MonsterCount);

			// Generate monsters
			std::unordered_map<const _MonsterStat *, int> MonsterTotals;
			while((int)Monsters.size() < MonsterCount) {

				// Find monster in CDT
				int RandomNumber = ae::GetRandomInt(1, OddsSum);
				for(const auto &ZoneData : ZoneCDT) {
					if(RandomNumber <= ZoneData.Odds) {

						// Check monster max
						if(ZoneData.Max == 0 || (ZoneData.Max > 0 && MonsterTotals[ZoneData.Monster] < ZoneData.Max)) {
							MonsterTotals[ZoneData.Monster]++;
							Monsters.push_back(ZoneData.Monster);
						}
						break;
					}
				}
			}
		}
	}
}

// Generates a list of items dropped from a monster
void _Stats::GenerateItemDrops(const _MonsterStat *MonsterStat, int Count, int DropRate, std::list<const _BaseItem *> &ItemDrops) const {
	if(!MonsterStat)
		return;

	// Get list of possible drops and build CDT
	int OddsSum = 0;
	std::list<_Drop> PossibleItemDrops;
	for(const auto &MonsterDrop : MonsterStat->Drops) {
		int Odds = 100 * MonsterDrop.Odds;

		// Reduce chance that nothing drops
		float Scale = 1.0f;
		if(!MonsterDrop.Item)
			Scale = BATTLE_NOTHINGDROP_SCALE;

		// Improve odds of items
		Odds *= 1.0f + DropRate / 100.0f * Scale;
		OddsSum += Odds;

		// Add to list of drops
		_Drop Drop;
		Drop.Item = MonsterDrop.Item;
		Drop.Odds = OddsSum;
		PossibleItemDrops.push_back(Drop);
	}

	// Check for items
	if(OddsSum > 0) {

		// Generate items
		for(int i = 0; i < Count; i++) {
			int RandomNumber = ae::GetRandomInt(1, OddsSum);

			// Find item id in CDT
			const _BaseItem *Item = nullptr;
			for(auto &MonsterDrop : PossibleItemDrops) {
				if(RandomNumber <= MonsterDrop.Odds) {
					Item = MonsterDrop.Item;
					break;
				}
			}

			// Populate item list
			if(Item)
				ItemDrops.push_back(Item);
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
