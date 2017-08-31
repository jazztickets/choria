/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#include <objects/item.h>
#include <unordered_map>
#include <list>
#include <vector>
#include <glm/vec3.hpp>

// Forward Declarations
class _Database;
class _Object;
class _Buff;

// Structures
struct _MapStat {
	std::string File;
	std::string Atlas;
	std::string Music;
	glm::vec4 AmbientLight;
	glm::vec3 BackgroundOffset;
	uint32_t BackgroundMapID;
	bool Outside;
};

struct _Portrait {
	uint32_t ID;
	const _Texture *Texture;
};

struct _Model {
	uint32_t ID;
	const _Texture *Texture;
};

struct _Build {
	uint32_t ID;
	uint32_t ModelID;
	std::string Name;
	const _Texture *Texture;
};

struct _Script {
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

struct _DamageType {
	std::string Name;
};

struct _Zone {
	uint32_t MonsterID;
	uint32_t Odds;
	int Max;
};

struct _EventName {
	std::string Name;
	std::string ShortName;
};

struct _Vendor {
	size_t GetSlotFromID(uint32_t ID) const;

	uint32_t ID;
	std::string Name;
	float BuyPercent;
	float SellPercent;
	std::vector<const _Item *> Items;
};

struct _TraderItem {
	const _Item *Item;
	int Count;
};

struct _Trader {
	uint32_t ID;
	std::string Name;
	const _Item *RewardItem;
	int Upgrades;
	int Count;
	std::vector<_TraderItem> Items;
};

struct _Blacksmith {
	uint32_t ID;
	std::string Name;
	int Level;
};

struct _MinigameItem {
	const _Item *Item;
	int Count;
};

struct _MinigameType {
	uint32_t ID;
	std::string Name;
	std::string Script;
	int Cost;
	std::vector<_MinigameItem> Items;
};

struct _ItemDrop {
	_ItemDrop(uint32_t ItemID, uint32_t Odds) : ItemID(ItemID), Odds(Odds) { }
	uint32_t ItemID;
	uint32_t Odds;
};

// Classes
class _Stats {

	public:

		_Stats(bool Headless=false);
		~_Stats();

		// General Stats
		void GetMonsterStats(uint32_t MonsterID, _Object *Monster, double Difficulty=1.0) const;

		// Menu
		void GetPortraits(std::list<_Portrait> &Portraits) const;
		void GetStartingBuilds(std::list<_Build> &Builds) const;
		const _Texture *GetPortraitImage(uint32_t PortraitID) const;

		// Monsters
		void GenerateMonsterListFromZone(int AdditionalCount, uint32_t ZoneID, std::list<uint32_t> &Monsters, bool &Boss, double &Cooldown) const;
		void GenerateItemDrops(uint32_t MonsterID, uint32_t Count, int DropRate, std::list<uint32_t> &ItemDrops) const;

		// Maps
		uint32_t GetMapIDByPath(const std::string &Path) const;

		// Levels
		const _Level *GetLevel(int Level) const { return &Levels[(size_t)Level-1]; }
		const _Level *FindLevel(int Experience) const;
		int GetMaxLevel() const { return (int)Levels.size(); }

		std::vector<_EventName> EventNames;
		std::vector<_Level> Levels;

		std::unordered_map<uint32_t, _MapStat> Maps;
		std::unordered_map<uint32_t, _Vendor> Vendors;
		std::unordered_map<uint32_t, _Trader> Traders;
		std::unordered_map<uint32_t, _Blacksmith> Blacksmiths;
		std::unordered_map<uint32_t, _MinigameType> Minigames;
		std::unordered_map<uint32_t, _Script> Scripts;
		std::unordered_map<uint32_t, _Model> Models;
		std::unordered_map<uint32_t, std::string> ItemTypes;
		std::unordered_map<uint32_t, std::string> TargetTypes;
		std::unordered_map<uint32_t, std::string> DamageTypes;
		std::unordered_map<StatType, double, StatTypeHash> UpgradeScale;
		std::unordered_map<uint32_t, const _Item *> Items;
		std::unordered_map<uint32_t, const _Buff *> Buffs;
		std::unordered_map<uint32_t, const _Object *> Builds;

		// Database
		_Database *Database;
		bool Headless;

	private:

		void LoadMaps();
		void LoadEvents();
		void LoadLevels();
		void LoadBuffs();
		void LoadItemTypes();
		void LoadStatTypes();
		void LoadTargetTypes();
		void LoadDamageTypes();
		void LoadItems();
		void LoadVendors();
		void LoadTraders();
		void LoadBlacksmiths();
		void LoadMinigames();
		void LoadModels();
		void LoadBuilds();
		void LoadScripts();

};
