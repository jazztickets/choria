/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef STATS_H
#define STATS_H

// Libraries
#include <irrlicht.h>
#include <map>
#include "constants.h"
#include "../objects/item.h"
#include "../objects/skill.h"

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

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
	ZoneStruct(int MonsterID, int Odds) : MonsterID(MonsterID), Odds(Odds) { }
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
	MonsterDropStruct(int ItemID, int Odds) : ItemID(ItemID), Odds(Odds) { }
	int ItemID;
	int Odds;
};

struct SkillTypeStruct {
	stringc Name;
};

// Classes
class StatsClass {

	public:

		int Init();
		int Close();

		// General Stats
		void GetMonsterStats(int MonsterID, MonsterClass *Monster);
		void GetPortraitList(list<PortraitStruct> &List);
		const PortraitStruct *GetPortrait(int PortraitID) { return &Portraits[PortraitID]; }

		const SkillClass *GetSkill(int SkillID);
		const MapStruct *GetMap(int MapID) { return &Maps[MapID]; }
		const ItemClass *GetItem(int ItemID) { return &Items[ItemID]; }
		const VendorStruct *GetVendor(int VendorID) { return &Vendors[VendorID]; }
		const TraderStruct *GetTrader(int TraderID) { return &Traders[TraderID]; }

		// Monsters
		void GenerateMonsterListFromZone(int Zone, array<int> &Monsters);
		void GenerateMonsterDrops(int MonsterID, int Count, array<int> &Drops);

		// Events
		const EventStruct *GetEvent(int Index) const { return &Events[Index]; }
		int GetEventCount() const { return Events.size(); }

		// Levels
		const LevelStruct *GetLevel(int Level) const { return &Levels[Level-1]; }
		const LevelStruct *FindLevel(int Experience) const;
		int GetMaxLevel() const { return Levels.size(); }

		// Skills
		int GetSkillTypeCount() const { return SkillTypes.size(); }
		int GetSkillCount() const { return Skills.size(); }
		const array<SkillTypeStruct> &GetSkillTypeList() const { return SkillTypes; }

	private:

		void LoadPortraits();
		void LoadMaps();
		void LoadEvents();
		void LoadLevels();
		void LoadSkillTypes();
		void LoadSkills();
		void LoadItems();
		void LoadVendors();
		void LoadTraders();

		DatabaseClass *Database;

		array<EventStruct> Events;
		array<LevelStruct> Levels;
		array<SkillTypeStruct> SkillTypes;

		std::map<int, PortraitStruct> Portraits;
		std::map<int, MapStruct> Maps;
		std::map<int, ItemClass> Items;
		std::map<int, VendorStruct> Vendors;
		std::map<int, TraderStruct> Traders;
		std::map<int, SkillClass> Skills;
};

// Singletons
extern StatsClass Stats;

#endif
