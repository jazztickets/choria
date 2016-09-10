/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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

// Forward Declarations
class _Database;
class _Object;
class _Buff;

// Structures
struct _MapStat {
	std::string File;
};

struct _Portrait {
	uint32_t ID;
	const _Texture *Image;
};

struct _Build {
	uint32_t ID;
	std::string Name;
	const _Texture *Image;
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
	_Zone(uint32_t MonsterID, uint32_t Odds) : MonsterID(MonsterID), Odds(Odds) { }
	uint32_t MonsterID;
	uint32_t Odds;
};

struct _EventName {
	std::string Name;
	std::string ShortName;
};

struct _Vendor {
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
	std::vector<_TraderItem> TraderItems;
};

struct _ItemDrop {
	_ItemDrop(uint32_t ItemID, uint32_t Odds) : ItemID(ItemID), Odds(Odds) { }
	uint32_t ItemID;
	uint32_t Odds;
};

// Classes
class _Stats {

	public:

		_Stats();
		~_Stats();

		// General Stats
		void GetMonsterStats(uint32_t MonsterID, _Object *Monster, double Difficulty=1.0);
		const _MapStat *GetMap(uint32_t MapID) { return &Maps[MapID]; }
		const _Vendor *GetVendor(uint32_t VendorID) { return &Vendors[VendorID]; }
		const _Trader *GetTrader(uint32_t TraderID) { return &Traders[TraderID]; }

		// Menu
		void GetPortraits(std::list<_Portrait> &Portraits);
		void GetStartingBuilds(std::list<_Build> &Builds);
		const _Texture *GetPortraitImage(uint32_t PortraitID);

		// Monsters
		void GenerateMonsterListFromZone(int AdditionalCount, uint32_t ZoneID, std::list<uint32_t> &Monsters, bool &Boss);
		void GenerateItemDrops(uint32_t MonsterID, uint32_t Count, std::list<uint32_t> &ItemDrops);

		// Maps
		uint32_t GetMapIDByPath(const std::string &Path);

		// Levels
		const _Level *GetLevel(int Level) const { return &Levels[Level-1]; }
		const _Level *FindLevel(int Experience) const;
		int GetMaxLevel() const { return Levels.size(); }

		std::vector<_EventName> EventNames;
		std::vector<_Level> Levels;

		std::unordered_map<uint32_t, _MapStat> Maps;
		std::unordered_map<uint32_t, _Vendor> Vendors;
		std::unordered_map<uint32_t, _Trader> Traders;
		std::unordered_map<uint32_t, _Script> Scripts;
		std::unordered_map<uint32_t, std::string> ItemTypes;
		std::unordered_map<uint32_t, std::string> TargetTypes;
		std::unordered_map<uint32_t, std::string> DamageTypes;
		std::unordered_map<StatType, double> UpgradeScale;
		std::unordered_map<uint32_t, const _Item *> Items;
		std::unordered_map<uint32_t, const _Buff *> Buffs;
		std::unordered_map<uint32_t, const _Object *> Builds;

		// Database
		_Database *Database;

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
		void LoadBuilds();
		void LoadScripts();

};
