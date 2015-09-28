/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#pragma once

// Libraries
#include <irrlicht.h>
#include <map>
#include <engine/singleton.h>
#include <objects/item.h>
#include <objects/skill.h>

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Constants
const int STATS_MAXGOLD = 1000000;

// Forward Declarations
class DatabaseClass;
class MonsterClass;

// Structures
struct MapStruct {
	stringc File;
	int ViewWidth;
	int ViewHeight;
};

struct PortraitStruct {
	int ID;
	ITexture *Image;
};

struct LevelStruct {
	int Level;
	int Experience;
	int NextLevel;
	int Health;
	int Mana;
	int SkillPoints;
};

struct ZoneStruct {
	ZoneStruct(int TMonsterID, int TOdds) : MonsterID(TMonsterID), Odds(TOdds) { }
	int MonsterID;
	int Odds;
};

struct EventStruct {
	stringc Name;
	stringc ShortName;
	bool Indexed;
};

struct VendorStruct {
	int ID;
	stringc Name;
	stringc Info;
	float BuyPercent;
	float SellPercent;
	array<const ItemClass *> Items;
};

struct TraderItemStruct {
	const ItemClass *Item;
	int Count;
};

struct TraderStruct {
	int ID;
	stringc Name;
	const ItemClass *RewardItem;
	int Count;
	array<TraderItemStruct> TraderItems;
};

struct MonsterDropStruct {
	MonsterDropStruct(int TItemID, int TOdds) : ItemID(TItemID), Odds(TOdds) { }
	int ItemID;
	int Odds;
};

// Classes
class StatsClass {

	public:

		int Init();
		int Close();

		// General Stats
		void GetMonsterStats(int TMonsterID, MonsterClass *TMonster);
		const PortraitStruct *GetPortrait(int TPortraitID) { return &Portraits[TPortraitID]; }
		const SkillClass *GetSkill(int TSkillID);
		const MapStruct *GetMap(int TMapID) { return &Maps[TMapID]; }
		const ItemClass *GetItem(int TItemID) { return &Items[TItemID]; }
		const VendorStruct *GetVendor(int TVendorID) { return &Vendors[TVendorID]; }
		const TraderStruct *GetTrader(int TTraderID) { return &Traders[TTraderID]; }

		void GetPortraitList(list<PortraitStruct> &TList);

		// Monsters
		void GenerateMonsterListFromZone(int TZone, array<int> &TMonsters);
		void GenerateMonsterDrops(int TMonsterID, int TCount, array<int> &TDrops);

		// Events
		const EventStruct *GetEvent(int TIndex) const { return &Events[TIndex]; }
		int GetEventCount() const { return Events.size(); }

		// Levels
		const LevelStruct *GetLevel(int TLevel) const { return &Levels[TLevel-1]; }
		const LevelStruct *FindLevel(int TExperience) const;
		int GetMaxLevel() const { return Levels.size(); }

		// Skills
		const array<SkillClass> &GetSkillList() const { return Skills; }

	private:

		void LoadPortraits();
		void LoadMaps();
		void LoadEvents();
		void LoadLevels();
		void LoadSkills();
		void LoadItems();
		void LoadVendors();
		void LoadTraders();

		DatabaseClass *Database;

		array<EventStruct> Events;
		array<LevelStruct> Levels;
		array<SkillClass> Skills;

		std::map<int, PortraitStruct> Portraits;
		std::map<int, MapStruct> Maps;
		std::map<int, ItemClass> Items;
		std::map<int, VendorStruct> Vendors;
		std::map<int, TraderStruct> Traders;
};

// Singletons
typedef SingletonClass<StatsClass> Stats;

