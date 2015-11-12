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
	irr::core::stringc File;
	int ViewWidth;
	int ViewHeight;
};

struct _Portrait {
	int ID;
	irr::video::ITexture *Image;
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
	_Zone(int TMonsterID, int TOdds) : MonsterID(TMonsterID), Odds(TOdds) { }
	int MonsterID;
	int Odds;
};

struct _Event {
	irr::core::stringc Name;
	irr::core::stringc ShortName;
	bool Indexed;
};

struct _Vendor {
	int ID;
	irr::core::stringc Name;
	irr::core::stringc Info;
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
	irr::core::stringc Name;
	const _Item *RewardItem;
	int Count;
	std::vector<_TraderItem> TraderItems;
};

struct _MonsterDrop {
	_MonsterDrop(int TItemID, int TOdds) : ItemID(TItemID), Odds(TOdds) { }
	int ItemID;
	int Odds;
};

// Classes
class _Stats {

	public:

		void Init();
		void Close();

		// General Stats
		void GetMonsterStats(int TMonsterID, _Monster *TMonster);
		const _Portrait *GetPortrait(int TPortraitID) { return &Portraits[TPortraitID]; }
		const _Skill *GetSkill(int TSkillID);
		const _MapStat *GetMap(int TMapID) { return &Maps[TMapID]; }
		const _Item *GetItem(int TItemID) { return &Items[TItemID]; }
		const _Vendor *GetVendor(int TVendorID) { return &Vendors[TVendorID]; }
		const _Trader *GetTrader(int TTraderID) { return &Traders[TTraderID]; }

		void GetPortraitList(std::list<_Portrait> &TList);

		// Monsters
		void GenerateMonsterListFromZone(int TZone, std::vector<int> &TMonsters);
		void GenerateMonsterDrops(int TMonsterID, int TCount, std::vector<int> &TDrops);

		// Events
		const _Event *GetEvent(int TIndex) const { return &Events[TIndex]; }
		int GetEventCount() const { return Events.size(); }

		// Levels
		const _Level *GetLevel(int TLevel) const { return &Levels[TLevel-1]; }
		const _Level *FindLevel(int TExperience) const;
		int GetMaxLevel() const { return Levels.size(); }

		// Skills
		const std::vector<_Skill> &GetSkillList() const { return Skills; }

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

		std::vector<_Event> Events;
		std::vector<_Level> Levels;
		std::vector<_Skill> Skills;

		std::map<int, _Portrait> Portraits;
		std::map<int, _MapStat> Maps;
		std::map<int, _Item> Items;
		std::map<int, _Vendor> Vendors;
		std::map<int, _Trader> Traders;
};

extern _Stats Stats;
