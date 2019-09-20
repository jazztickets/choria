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
#include <objects/item.h>
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

namespace ae {
	class _Database;
}

struct _Portrait {
	std::string ID;
	const ae::_Texture *Texture;
	uint8_t NetworkID;
};

struct _Model {
	std::string ID;
	const ae::_Texture *Texture;
	uint8_t NetworkID;
};

struct _Build {
	std::string Name;
	std::string Model;
	const ae::_Texture *Texture;
};

struct _OldScript {
	uint32_t ID;
	std::string Name;
	int Level;
	double Cooldown;
};

struct _Level {
	int Level;
	int Experience;
	int NextLevel;
	int Health;
	int Mana;
	int Damage;
	int Armor;
	int SkillPoints;
};

struct _OldZone {
	uint32_t MonsterID;
	uint32_t Odds;
	int Max;
};

struct _Drop {
	const _BaseItem *Item;
	uint32_t Odds;
};

struct _MonsterStat {
	_MonsterStat() : Health(0), Mana(0), MinDamage(0), MaxDamage(0) { }
	std::string ID;
	std::string Name;
	std::string AI;
	const ae::_Texture *Texture;
	std::vector<_Drop> Drops;
	int Health;
	int Mana;
	int MinDamage;
	int MaxDamage;
	uint16_t NetworkID;
};

struct _Vendor {
	_Vendor() : BuyPercent(1.0f), SellPercent(0.5f) { }
	size_t GetSlotFromID(const std::string &ItemID) const;

	std::string ID;
	float BuyPercent;
	float SellPercent;
	std::vector<const _BaseItem *> Items;
	uint16_t NetworkID;
};

struct _TraderItem {
	const _BaseItem *Item;
	int Count;
};

struct _OldTrader {
	uint32_t ID;
	std::string Name;
	const _BaseItem *RewardItem;
	int Upgrades;
	int Count;
	std::vector<_TraderItem> Items;
};

struct _OldBlacksmith {
	uint32_t ID;
	std::string Name;
	int Level;
};

struct _MinigameItem {
	const _BaseItem *Item;
	int Count;
};

struct _OldMinigameType {
	uint32_t ID;
	std::string Name;
	std::string Script;
	int Cost;
	const _BaseItem *RequiredItem;
	std::vector<_MinigameItem> Items;
};

struct _OldItemDrop {
	_OldItemDrop(uint32_t ItemID, uint32_t Odds) : ItemID(ItemID), Odds(Odds) { }
	uint32_t ItemID;
	uint32_t Odds;
};

struct _OldLightType {
	uint32_t ID;
	std::string Name;
	glm::vec3 Color;
	float Radius;
};

// Classes
class _Stats {

	public:

		_Stats(bool Headless=false);
		~_Stats();

		// General Stats
		void GetMonsterStats(uint32_t MonsterID, _Object *Object, double Difficulty=1.0) const;

		// Menu
		const _Portrait *GetPortrait(uint8_t NetworkID) const;
		const _Model *GetModel(uint8_t NetworkID) const;
		void GetPortraits(std::list<const _Portrait *> &PortraitList) const;
		void GetStartingBuilds(std::list<const _Object *> &BuildsList) const;

		// Monsters
		void GenerateMonsterListFromZone(int AdditionalCount, uint32_t ZoneID, std::list<uint32_t> &Monsters, bool &Boss, double &Cooldown) const;
		void GenerateItemDrops(uint32_t MonsterID, uint32_t Count, int DropRate, std::list<uint32_t> &ItemDrops) const;

		// Levels
		const _Level *GetLevel(int Level) const { return &Levels[(size_t)Level-1]; }
		const _Level *FindLevel(int Experience) const;
		int GetMaxLevel() const { return (int)Levels.size(); }

		std::vector<_Level> Levels;

		std::unordered_map<uint32_t, _OldTrader> OldTraders;
		std::unordered_map<uint32_t, _OldBlacksmith> OldBlacksmiths;
		std::unordered_map<uint32_t, _OldMinigameType> OldMinigames;
		std::unordered_map<uint32_t, _OldScript> OldScripts;
		std::unordered_map<uint32_t, _OldLightType> OldLights;
		std::unordered_map<StatType, double, StatTypeHash> UpgradeScale;
		std::unordered_map<uint32_t, const _Buff *> OldBuffs;

		std::unordered_map<std::string, ae::NetworkIDType> MapsIndex;
		std::unordered_map<DamageType, std::pair<std::string, std::string> > DamageTypes;
		std::unordered_map<EventType, std::pair<std::string, std::string> > EventTypes;
		std::unordered_map<ItemType, std::pair<std::string, std::string> > ItemTypes;
		std::unordered_map<ScopeType, std::pair<std::string, std::string> > ScopeTypes;
		std::unordered_map<TargetType, std::pair<std::string, std::string> > TargetTypes;

		std::unordered_map<std::string, _BaseItem> Items;
		std::unordered_map<uint16_t, const _BaseItem *> ItemsIndex;
		std::unordered_map<std::string, _Portrait> Portraits;
		std::unordered_map<uint8_t, const _Portrait *> PortraitsIndex;
		std::unordered_map<std::string, _Model> Models;
		std::unordered_map<uint8_t, const _Model *> ModelsIndex;
		std::unordered_map<std::string, _Vendor> Vendors;
		std::unordered_map<std::string, _MonsterStat> Monsters;
		std::unordered_map<std::string, const _MonsterStat *> MonstersIndex;

		std::unordered_map<std::string, const _Object *> Builds;

		// Database
		ae::_Database *Database;
		bool Headless;

	private:

		void LoadTypes();
		void LoadMapDirectory();
		void LoadData(const std::string &Path);
		const char *GetString(tinyxml2::XMLElement *Node, const char *Attribute, bool Required=true);
		const ae::_Texture *GetTexture(tinyxml2::XMLElement *Node, const char *Attribute);
		const _BaseItem *GetItem(tinyxml2::XMLElement *Node, const char *Attribute);

		void OldLoadMaps();
		void OldLoadLevels();
		void OldLoadBuffs();
		void OldLoadStatTypes();
		void OldLoadTraders();
		void OldLoadBlacksmiths();
		void OldLoadMinigames();
		void OldLoadScripts();
		void OldLoadLights();

};
