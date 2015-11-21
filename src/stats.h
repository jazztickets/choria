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
#include <list>
#include <vector>

// Forward Declarations
class _Database;
class _Monster;

// Structures
struct _MapStat {
	std::string File;
	int ViewWidth;
	int ViewHeight;
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
	bool Indexed;
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

		_Stats() : Database(nullptr) { }
		void Init();
		void Close();

		// General Stats
		void GetMonsterStats(int MonsterID, _Monster *Monster);
		const _Portrait *GetPortrait(int PortraitID) { return &Portraits[PortraitID]; }
		const _Skill *GetSkill(int SkillID);
		const _MapStat *GetMap(int MapID) { return &Maps[MapID]; }
		const _Item *GetItem(int ItemID) { return &Items[ItemID]; }
		const _Vendor *GetVendor(int VendorID) { return &Vendors[VendorID]; }
		const _Trader *GetTrader(int TraderID) { return &Traders[TraderID]; }

		void GetPortraits(std::list<_Portrait> &List);

		// Monsters
		void GenerateMonsterListFromZone(int ZoneID, std::vector<int> &Monsters);
		void GenerateMonsterDrops(int MonsterID, int Count, std::vector<int> &Drops);

		std::vector<_Event> Events;
		std::vector<_Level> Levels;

		std::map<int, _Portrait> Portraits;
		std::map<int, _MapStat> Maps;
		std::map<int, _Item> Items;
		std::map<int, _Vendor> Vendors;
		std::map<int, _Trader> Traders;

		// Levels
		const _Level *GetLevel(int Level) const { return &Levels[Level-1]; }
		const _Level *FindLevel(int Experience) const;
		int GetMaxLevel() const { return Levels.size(); }

		// Skills
		std::vector<_Skill> Skills;

	private:

		void LoadPortraits();
		void LoadMaps();
		void LoadEvents();
		void LoadLevels();
		void LoadSkills();
		void LoadItems();
		void LoadVendors();
		void LoadTraders();

		_Database *Database;

};

extern _Stats Stats;
