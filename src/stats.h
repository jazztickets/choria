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
#pragma once

// Libraries
#include <objects/item.h>
#include <objects/skill.h>
#include <map>
#include <unordered_map>
#include <list>
#include <vector>

// Forward Declarations
class _Database;
class _Object;

// Structures
struct _MapStat {
	std::string File;
};

struct _Portrait {
	int ID;
	const _Texture *Image;
};

struct _Level {
	int Level;
	int Experience;
	int NextLevel;
	int Health;
	int Mana;
	int SkillPoints;
};

struct _Zone {
	_Zone(int MonsterID, int Odds) : MonsterID(MonsterID), Odds(Odds) { }
	int MonsterID;
	int Odds;
};

struct _Event {
	std::string Name;
	std::string ShortName;
};

struct _Vendor {
	int ID;
	std::string Name;
	std::string Info;
	float BuyPercent;
	float SellPercent;
	std::vector<const _Item *> Items;
};

struct _TraderItem {
	const _Item *Item;
	int Count;
};

struct _Trader {
	int ID;
	std::string Name;
	const _Item *RewardItem;
	int Count;
	std::vector<_TraderItem> TraderItems;
};

struct _MonsterDrop {
	_MonsterDrop(int ItemID, int Odds) : ItemID(ItemID), Odds(Odds) { }
	int ItemID;
	int Odds;
};

// Classes
class _Stats {

	public:

		_Stats();
		~_Stats();

		// General Stats
		void GetMonsterStats(uint32_t MonsterID, _Object *Monster);
		const _MapStat *GetMap(uint32_t MapID) { return &Maps[MapID]; }
		const _Item *GetItem(uint32_t ItemID) { return &Items[ItemID]; }
		const _Vendor *GetVendor(uint32_t VendorID) { return &Vendors[VendorID]; }
		const _Trader *GetTrader(uint32_t TraderID) { return &Traders[TraderID]; }

		// Portraits
		void GetPortraits(std::list<_Portrait> &Portraits);
		const _Texture *GetPortraitImage(uint32_t PortraitID);

		// Monsters
		void GenerateMonsterListFromZone(int ZoneID, std::vector<int> &Monsters);
		void GenerateMonsterDrops(int MonsterID, int Count, std::vector<int> &Drops);

		// Levels
		const _Level *GetLevel(int Level) const { return &Levels[Level-1]; }
		const _Level *FindLevel(int Experience) const;
		int GetMaxLevel() const { return Levels.size(); }

		std::vector<_Event> Events;
		std::vector<_Level> Levels;

		std::map<uint32_t, _MapStat> Maps;
		std::map<uint32_t, _Item> Items;
		std::map<uint32_t, _Vendor> Vendors;
		std::map<uint32_t, _Trader> Traders;

		// Skills
		std::unordered_map<uint32_t, const _Skill *> Skills;

	private:

		void LoadMaps();
		void LoadEvents();
		void LoadLevels();
		void LoadSkills();
		void LoadItems();
		void LoadVendors();
		void LoadTraders();

		_Database *Database;

};
