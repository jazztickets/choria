/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2021 Alan Witkowski
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
class _Object;
class _Buff;

namespace ae {
	class _Database;
}

// Structures
struct _MapStat {
	std::string File;
	std::string Atlas;
	std::string Music;
	glm::vec4 AmbientLight;
	float AmbientBackground;
	glm::vec3 BackgroundOffset;
	uint32_t BackgroundMapID;
	int Outside;
};

struct _Portrait {
	uint32_t ID;
	const ae::_Texture *Texture;
};

struct _Model {
	uint32_t ID;
	const ae::_Texture *Texture;
};

struct _Build {
	uint32_t ID;
	uint32_t ModelID;
	std::string Name;
	const ae::_Texture *Texture;
};

struct _Script {
	uint32_t ID;
	std::string Name;
	std::string Data;
	int Level;
};

struct _Set {
	uint32_t ID;
	std::string Name;
	std::string Script;
	int Count;
};

struct _Level {
	int Level;
	int64_t Experience;
	int64_t NextLevel;
	int Health;
	int Mana;
	int RebirthTier;
	int SkillPoints;
};

struct _DamageType {
	std::string Name;
	glm::vec4 Color;
};

struct _Zone {
	_Zone() : MonsterID(0), Odds(0), Boss(false), Cooldown(0.0), Max(0), Difficulty(0) { }

	uint32_t MonsterID;
	uint32_t Odds;
	bool Boss;
	double Cooldown;
	int Max;
	int Difficulty;
};

struct _EventName {
	std::string Name;
	std::string ShortName;
};

struct _Vendor {
	std::size_t GetSlotFromID(uint32_t ID) const;

	uint32_t ID;
	std::string Name;
	std::string Sort;
	double BuyPercent;
	double SellPercent;
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
	bool CanUpgrade(const _Item *Item, int Upgrades) const;

	uint32_t ID;
	std::string Name;
	int Level;
};

struct _Enchanter {
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
	const _Item *RequiredItem;
	std::vector<_MinigameItem> Items;
};

struct _ItemDrop {
	_ItemDrop(uint32_t ItemID, int Odds) : ItemID(ItemID), Odds(Odds) { }
	uint32_t ItemID;
	int Odds;
};

struct _LightType {
	uint32_t ID;
	std::string Name;
	glm::vec3 Color;
	float Radius;
};

struct _Attribute {
	uint8_t ID;
	std::string Name;
	std::string Label;
	StatValueType Type;
	StatUpdateType UpdateType;
	_Value Default;
	float UpgradeScale;
	int Show;
	bool Calculate;
	bool Save;
	bool Script;
	bool Network;
};

// Classes
class _Stats {

	public:

		static std::vector<std::string> ResistNames;

		_Stats(bool Headless=false);
		~_Stats();

		// General Stats
		void GetMonsterStats(uint32_t MonsterID, _Object *Object, int Difficulty=1.0) const;

		// Menu
		void GetPortraits(std::list<_Portrait> &Portraits) const;
		void GetStartingBuilds(std::list<_Build> &Builds) const;
		const ae::_Texture *GetPortraitImage(uint32_t PortraitID) const;

		// Monsters
		void GetZone(uint32_t ZoneID, _Zone &Zone) const;
		void GenerateMonsterListFromZone(int AdditionalCount, float MonsterCountModifier, uint32_t ZoneID, std::list<_Zone> &Monsters, bool &Boss, double &Cooldown) const;
		void GenerateItemDrops(uint32_t MonsterID, int Count, std::vector<uint32_t> &ItemDrops, float DropRate) const;

		// Maps
		uint32_t GetMapIDByPath(const std::string &Path) const;

		// Levels
		const _Level *GetLevel(int Level) const { return &Levels[(std::size_t)Level-1]; }
		const _Level *FindLevel(int64_t Experience) const;
		int GetMaxLevel() const { return (int)Levels.size(); }

		std::vector<_EventName> EventNames;
		std::vector<_Level> Levels;

		std::vector<std::string> AttributeRank;
		std::unordered_map<std::string, _Attribute> Attributes;
		std::unordered_map<uint32_t, _MapStat> Maps;
		std::unordered_map<uint32_t, _Vendor> Vendors;
		std::unordered_map<uint32_t, _Trader> Traders;
		std::unordered_map<uint32_t, _Blacksmith> Blacksmiths;
		std::unordered_map<uint32_t, _Enchanter> Enchanters;
		std::unordered_map<uint32_t, _MinigameType> Minigames;
		std::unordered_map<uint32_t, _Script> Scripts;
		std::unordered_map<uint32_t, _Set> Sets;
		std::unordered_map<uint32_t, std::string> Unlocks;
		std::unordered_map<uint32_t, _Model> Models;
		std::unordered_map<uint32_t, _LightType> Lights;
		std::unordered_map<uint32_t, std::string> ItemTypes;
		std::unordered_map<uint32_t, std::string> TargetTypes;
		std::unordered_map<uint32_t, _DamageType> DamageTypes;
		std::unordered_map<uint32_t, const _Item *> Items;
		std::unordered_map<uint32_t, const _Buff *> Buffs;
		std::unordered_map<uint32_t, const _Object *> Builds;

		// Database
		ae::_Database *Database;
		bool Headless;

	private:

		void LoadAttributes();
		void LoadMaps();
		void LoadEvents();
		void LoadLevels();
		void LoadBuffs();
		void LoadItemTypes();
		void LoadTargetTypes();
		void LoadDamageTypes();
		void LoadItems();
		void LoadVendors();
		void LoadTraders();
		void LoadBlacksmiths();
		void LoadEnchanters();
		void LoadMinigames();
		void LoadModels();
		void LoadBuilds();
		void LoadScripts();
		void LoadSets();
		void LoadUnlocks();
		void LoadLights();

};
