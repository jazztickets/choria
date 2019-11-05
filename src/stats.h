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
#pragma once

// Libraries
#include <ae/type.h>
#include <objects/buff.h>
#include <objects/item.h>
#include <objects/skill.h>
#include <objects/statchange.h>
#include <enums.h>
#include <unordered_map>
#include <list>
#include <vector>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>

// Forward Declarations
class _Object;
class _Buff;

namespace tinyxml2 {
	class XMLElement;
}

struct _Portrait {
	std::string ID;
	const ae::_Texture *Texture;
	bool Starting;
	uint8_t NetworkID;
};

struct _Model {
	std::string ID;
	const ae::_Texture *Texture;
	uint16_t NetworkID;
};

struct _Build {
	std::string Name;
	std::string Model;
	const ae::_Texture *Texture;
};

struct _Level {
	int Level;
	int Experience;
	int NextLevel;
};

struct _Drop {
	const _BaseItem *Item;
	int Odds;
};

struct _MonsterStat {
	_MonsterStat() : Health(0), Mana(0), Armor(0), MinDamage(0), MaxDamage(0), Experience(0), Gold(0) { }
	std::string ID;
	std::string Name;
	std::string AI;
	const _Portrait *Portrait;
	const ae::_Texture *Texture;
	//std::vector<const _Usable *> Actions;
	std::vector<_Drop> Drops;
	int Health;
	int Mana;
	int Armor;
	int MinDamage;
	int MaxDamage;
	int Experience;
	int Gold;
	uint16_t NetworkID;
};

struct _ZoneMonster {
	const _MonsterStat *Monster;
	int Odds;
	int Max;
};

struct _Zone {
	std::string ID;
	std::vector<_ZoneMonster> Monsters;
	int Min;
	int Max;
	uint16_t NetworkID;
};

struct _Vendor {
	_Vendor() : BuyPercent(1.0f), SellPercent(0.5f) { }
	size_t GetSlotFromID(const std::string &ItemID) const;

	std::string ID;
	std::string Name;
	std::string Text;
	float BuyPercent;
	float SellPercent;
	std::vector<const _BaseItem *> Items;
	uint16_t NetworkID;
};

struct _TraderItem {
	const _BaseItem *Item;
	int Count;
};

struct _Trader {
	std::string ID;
	std::string Name;
	std::string Text;
	const _BaseItem *RewardItem;
	int Upgrades;
	int RewardCount;
	std::vector<_TraderItem> Items;
	uint16_t NetworkID;
};

struct _Blacksmith {
	std::string ID;
	std::string Name;
	int Level;
	uint16_t NetworkID;
};

struct _MinigameItem {
	const _BaseItem *Item;
	int Count;
};

struct _MinigameStat {
	std::string ID;
	std::string Name;
	int Cost;
	const _BaseItem *RequiredItem;
	std::vector<_MinigameItem> Items;
	uint16_t NetworkID;
};

struct _WeaponType {
	std::string ID;
	std::string Name;
	std::vector<const _BaseSkill *> Skills;
};

struct _LightType {
	_LightType() : Texture(nullptr), Color(1.0f), HalfSize(0.0f) { }
	const ae::_Texture *Texture;
	std::string Script;
	glm::vec4 Color;
	glm::vec2 HalfSize;
};

// Classes
class _Stats {

	public:

		_Stats(bool Headless=false);
		~_Stats();

		// Menu
		const _Portrait *GetPortrait(uint8_t NetworkID) const;
		const _Model *GetModel(uint8_t NetworkID) const;
		void GetStartingPortraits(std::list<const _Portrait *> &PortraitList) const;
		void GetStartingBuilds(std::list<const _Object *> &BuildsList) const;

		// Monsters
		void GetMonsterStats(const _MonsterStat *MonsterStat, _Object *Object, double Difficulty=1.0) const;
		void GenerateMonsterListFromZone(int AdditionalCount, const std::string &ZoneID, std::list<const _MonsterStat *> &Monsters, bool &Boss, double &Cooldown) const;
		void GenerateItemDrops(const _MonsterStat *MonsterStat, int Count, int DropRate, std::list<const _BaseItem *> &ItemDrops) const;

		// Levels
		const _Level *GetLevel(int Level) const { return &Levels[(size_t)Level-1]; }
		const _Level *FindLevel(int Experience) const;
		int GetMaxLevel() const { return (int)Levels.size(); }

		// Types
		std::unordered_map<DamageType, std::pair<std::string, std::string> > DamageTypes;
		std::unordered_map<EventType, std::pair<std::string, std::string> > EventTypes;
		std::unordered_map<ItemType, std::pair<std::string, std::string> > ItemTypes;
		std::unordered_map<ScopeType, std::pair<std::string, std::string> > ScopeTypes;
		std::unordered_map<TargetType, std::pair<std::string, std::string> > TargetTypes;

		// Objects
		std::unordered_map<std::string, _BaseItem> Items;
		std::unordered_map<std::string, _BaseSkill> Skills;
		std::unordered_map<std::string, _WeaponType> WeaponTypes;
		std::unordered_map<std::string, _Buff> Buffs;
		std::unordered_map<std::string, _Portrait> Portraits;
		std::unordered_map<std::string, _Model> Models;
		std::unordered_map<std::string, _Vendor> Vendors;
		std::unordered_map<std::string, _Trader> Traders;
		std::unordered_map<std::string, _Blacksmith> Blacksmiths;
		std::unordered_map<std::string, _MinigameStat> Minigames;
		std::unordered_map<std::string, _MonsterStat> Monsters;
		std::unordered_map<std::string, _Zone> Zones;
		std::unordered_map<std::string, const _Object *> Builds;
		std::unordered_map<uint32_t, _LightType> Lights;
		std::unordered_map<std::string, std::string> Strings;
		std::unordered_map<StatType, double, StatTypeHash> UpgradeScale;
		std::vector<_Level> Levels;

		// Indexes
		std::unordered_map<std::string, ae::NetworkIDType> MapsIndex;
		std::unordered_map<std::string, EventType> EventsIndex;
		std::unordered_map<uint16_t, const _BaseItem *> ItemsIndex;
		std::unordered_map<uint16_t, const _BaseSkill *> SkillsIndex;
		std::unordered_map<uint16_t, const _Buff *> BuffsIndex;
		std::unordered_map<uint8_t, const _Model *> ModelsIndex;
		std::unordered_map<uint8_t, const _Portrait *> PortraitsIndex;
		std::unordered_map<uint16_t, const _MonsterStat *> MonstersIndex;

	private:

		void LoadTypes();
		void LoadMapDirectory();
		void LoadPortraitsDirectory(const std::string &Path);
		void LoadModelsDirectory(const std::string &Path);
		void LoadLevels(const std::string &Path);
		void LoadLights(const std::string &Path);
		void LoadStrings(const std::string &Path);
		void LoadData(const std::string &Path);

		const char *GetString(tinyxml2::XMLElement *Node, const char *Attribute, bool Required=true);
		const ae::_Texture *GetTexture(tinyxml2::XMLElement *Node, const char *Attribute);
		const _BaseItem *GetItem(tinyxml2::XMLElement *Node, const char *Attribute, bool AllowNone=false);
		const _BaseSkill *GetSkill(tinyxml2::XMLElement *Node, const char *Attribute);
		const _MonsterStat *GetMonster(tinyxml2::XMLElement *Node, const char *Attribute);
		ScopeType GetScope(tinyxml2::XMLElement *Node, const char *Attribute);
		const _WeaponType *GetWeaponType(tinyxml2::XMLElement *Node, const char *Attribute);

		bool Headless;
};
