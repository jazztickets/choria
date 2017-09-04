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
#include <scripting.h>
#include <ae/buffer.h>
#include <ae/audio.h>
#include <ae/database.h>
#include <ae/assets.h>
#include <ae/random.h>
#include <objects/object.h>
#include <objects/buff.h>
#include <objects/statchange.h>
#include <objects/statuseffect.h>
#include <objects/battle.h>
#include <objects/components/inventory.h>
#include <objects/components/record.h>
#include <objects/components/fighter.h>
#include <objects/components/controller.h>
#include <objects/map.h>
#include <server.h>
#include <stats.h>
#include <stdexcept>
#include <iostream>

// Libraries
luaL_Reg _Scripting::RandomFunctions[] = {
	{"GetInt", &_Scripting::RandomGetInt},
	{nullptr, nullptr}
};

luaL_Reg _Scripting::AudioFunctions[] = {
	{"Play", &_Scripting::AudioPlay},
	{nullptr, nullptr}
};

int luaopen_Random(lua_State *LuaState) {
	luaL_newlib(LuaState, _Scripting::RandomFunctions);

	return 1;
}

int luaopen_Audio(lua_State *LuaState) {
	luaL_newlib(LuaState, _Scripting::AudioFunctions);

	return 1;
}

// Constructor
_Scripting::_Scripting() :
	LuaState(nullptr),
	CurrentTableIndex(0) {

	// Initialize lua object
	LuaState = luaL_newstate();
	luaL_openlibs(LuaState);
	luaL_requiref(LuaState, "Random", luaopen_Random, 1);
	luaL_requiref(LuaState, "Audio", luaopen_Audio, 1);
}

// Destructor
_Scripting::~_Scripting() {

	// Close lua state
	if(LuaState != nullptr)
		lua_close(LuaState);
}

// Set up scripting environment
void _Scripting::Setup(const _Stats *Stats, const std::string &BaseScript) {
	InjectStats(Stats);
	InjectMonsters(Stats);
	InjectItems(Stats);
	LoadScript(BaseScript);
	InjectItemPointers(Stats);
	InjectBuffs(Stats);
}

// Load a script file
void _Scripting::LoadScript(const std::string &Path) {

	// Load the file
	if(luaL_dofile(LuaState, Path.c_str()))
		throw std::runtime_error("Failed to load script " + Path + "\n" + std::string(lua_tostring(LuaState, -1)));
}

// Load global state with enumerations and constants
void _Scripting::InjectStats(const _Stats *Stats) {

	// Add damage types
	lua_newtable(LuaState);
	for(const auto &Iterator : Stats->DamageTypes) {

		// Add pointer to table
		lua_pushstring(LuaState, Iterator.second.c_str());
		lua_pushinteger(LuaState, Iterator.first);
		lua_settable(LuaState, -3);
	}
	lua_setglobal(LuaState, "DamageType");

	// Push bag types
	lua_pushinteger(LuaState, _Bag::BagType::NONE);
	lua_setglobal(LuaState, "BAG_NONE");
	lua_pushinteger(LuaState, _Bag::BagType::EQUIPMENT);
	lua_setglobal(LuaState, "BAG_EQUIPMENT");
	lua_pushinteger(LuaState, _Bag::BagType::INVENTORY);
	lua_setglobal(LuaState, "BAG_INVENTORY");
	lua_pushinteger(LuaState, _Bag::BagType::TRADE);
	lua_setglobal(LuaState, "BAG_TRADE");

	// Push inventory slot types
	lua_pushinteger(LuaState, EquipmentType::HEAD);
	lua_setglobal(LuaState, "INVENTORY_HEAD");
	lua_pushinteger(LuaState, EquipmentType::BODY);
	lua_setglobal(LuaState, "INVENTORY_BODY");
	lua_pushinteger(LuaState, EquipmentType::LEGS);
	lua_setglobal(LuaState, "INVENTORY_LEGS");
	lua_pushinteger(LuaState, EquipmentType::HAND1);
	lua_setglobal(LuaState, "INVENTORY_HAND1");
	lua_pushinteger(LuaState, EquipmentType::HAND2);
	lua_setglobal(LuaState, "INVENTORY_HAND2");
	lua_pushinteger(LuaState, EquipmentType::RING1);
	lua_setglobal(LuaState, "INVENTORY_RING1");
	lua_pushinteger(LuaState, EquipmentType::RING2);
	lua_setglobal(LuaState, "INVENTORY_RING2");
	lua_pushinteger(LuaState, EquipmentType::AMULET);
	lua_setglobal(LuaState, "INVENTORY_AMULET");

	// Push item types
	lua_pushinteger(LuaState, (int)ItemType::SKILL);
	lua_setglobal(LuaState, "ITEM_SKILL");
	lua_pushinteger(LuaState, (int)ItemType::HELMET);
	lua_setglobal(LuaState, "ITEM_HELMET");
	lua_pushinteger(LuaState, (int)ItemType::ARMOR);
	lua_setglobal(LuaState, "ITEM_ARMOR");
	lua_pushinteger(LuaState, (int)ItemType::BOOTS);
	lua_setglobal(LuaState, "ITEM_BOOTS");
	lua_pushinteger(LuaState, (int)ItemType::ONEHANDED_WEAPON);
	lua_setglobal(LuaState, "ITEM_ONEHANDED_WEAPON");
	lua_pushinteger(LuaState, (int)ItemType::TWOHANDED_WEAPON);
	lua_setglobal(LuaState, "ITEM_TWOHANDED_WEAPON");
	lua_pushinteger(LuaState, (int)ItemType::SHIELD);
	lua_setglobal(LuaState, "ITEM_SHIELD");
	lua_pushinteger(LuaState, (int)ItemType::RING);
	lua_setglobal(LuaState, "ITEM_RING");
	lua_pushinteger(LuaState, (int)ItemType::AMULET);
	lua_setglobal(LuaState, "ITEM_AMULET");
	lua_pushinteger(LuaState, (int)ItemType::CONSUMABLE);
	lua_setglobal(LuaState, "ITEM_CONSUMABLE");
	lua_pushinteger(LuaState, (int)ItemType::TRADABLE);
	lua_setglobal(LuaState, "ITEM_TRADABLE");
	lua_pushinteger(LuaState, (int)ItemType::UNLOCKABLE);
	lua_setglobal(LuaState, "ITEM_UNLOCKABLE");
}

// Inject items pointers into existing lua tables
void _Scripting::InjectItemPointers(const _Stats *Stats) {

	// Add item pointers to lua tables
	for(const auto &Iterator : Stats->Items) {
		const _Item *Item = Iterator.second;
		if(!Item)
			continue;

		// Find table
		lua_getglobal(LuaState, Item->Script.c_str());
		if(!lua_istable(LuaState, -1)) {
			lua_pop(LuaState, 1);
			continue;
		}

		// Add item pointer
		PushItem(LuaState, Item, 0);
		lua_setfield(LuaState, -2, "Item");

		lua_pop(LuaState, 1);
	}
}

// Inject item stats
void _Scripting::InjectItems(const _Stats *Stats) {

	// Add stats to lua table
	lua_newtable(LuaState);
	Stats->Database->PrepareQuery("SELECT * FROM item");
	while(Stats->Database->FetchRow()) {
		uint32_t ID = Stats->Database->GetInt<uint32_t>("id");
		std::string Name = Stats->Database->GetString("name");

		// Make ID the key to the table
		lua_pushinteger(LuaState, ID);

		// Make new table for attributes
		lua_newtable(LuaState);

		// Set attributes
		lua_pushinteger(LuaState, ID);
		lua_setfield(LuaState, -2, "ID");

		lua_pushstring(LuaState, Name.c_str());
		lua_setfield(LuaState, -2, "Name");

		lua_pushinteger(LuaState, Stats->Database->GetInt<int>("cost"));
		lua_setfield(LuaState, -2, "Cost");

		lua_pushinteger(LuaState, Stats->Database->GetInt<int>("itemtype_id"));
		lua_setfield(LuaState, -2, "Type");

		// Add attributes to table
		lua_settable(LuaState, -3);
	}

	// Give name to global table
	lua_setglobal(LuaState, "Items");

	// Free memory
	Stats->Database->CloseQuery();
}

// Inject monster stats
void _Scripting::InjectMonsters(const _Stats *Stats) {

	// Add stats to lua table
	lua_newtable(LuaState);
	Stats->Database->PrepareQuery("SELECT * FROM monster");
	while(Stats->Database->FetchRow()) {
		uint32_t ID = Stats->Database->GetInt<uint32_t>("id");
		std::string Name = Stats->Database->GetString("name");

		// Make ID the key to the table
		lua_pushinteger(LuaState, ID);

		// Make new table for attributes
		lua_newtable(LuaState);

		// Set attributes
		lua_pushinteger(LuaState, ID);
		lua_setfield(LuaState, -2, "ID");

		lua_pushstring(LuaState, Name.c_str());
		lua_setfield(LuaState, -2, "Name");

		lua_pushinteger(LuaState, Stats->Database->GetInt<int>("health"));
		lua_setfield(LuaState, -2, "Health");

		lua_pushinteger(LuaState, Stats->Database->GetInt<int>("mana"));
		lua_setfield(LuaState, -2, "Mana");

		lua_pushinteger(LuaState, Stats->Database->GetInt<int>("armor"));
		lua_setfield(LuaState, -2, "Armor");

		lua_pushinteger(LuaState, Stats->Database->GetInt<int>("mindamage"));
		lua_setfield(LuaState, -2, "MinDamage");

		lua_pushinteger(LuaState, Stats->Database->GetInt<int>("maxdamage"));
		lua_setfield(LuaState, -2, "MaxDamage");

		// Add attributes to table
		lua_settable(LuaState, -3);
	}

	// Give name to global table
	lua_setglobal(LuaState, "Monsters");

	// Free memory
	Stats->Database->CloseQuery();
}

// Inject buffs stat data
void _Scripting::InjectBuffs(const _Stats *Stats) {

	// Add buffs
	for(const auto &Iterator : Stats->Buffs) {
		const _Buff *Buff = Iterator.second;
		if(Buff) {

			// Get table
			lua_getglobal(LuaState, Buff->Script.c_str());
			if(!lua_istable(LuaState, -1))
				throw std::runtime_error("InjectBuffs: " + Buff->Script + " is not a table!");

			// Add ID
			lua_pushinteger(LuaState, Buff->ID);
			lua_setfield(LuaState, -2, "ID");

			// Add pointer
			lua_pushlightuserdata(LuaState, (void *)Buff);
			lua_setfield(LuaState, -2, "Pointer");

			// Pop global
			lua_pop(LuaState, 1);
		}
	}
}

// Inject server clock
void _Scripting::InjectTime(double Time) {

	// Push time
	lua_pushnumber(LuaState, Time);
	lua_setglobal(LuaState, "ServerTime");
}

// Create battle table
void _Scripting::CreateBattle(_Battle *Battle) {

	// Get table
	lua_getglobal(LuaState, "Battles");
	if(!lua_istable(LuaState, -1))
		throw std::runtime_error("CreateBattle: Battles is not a table!");

	// Battles[NetworkID] = {}
	lua_pushinteger(LuaState, Battle->NetworkID);
	lua_newtable(LuaState);
	lua_settable(LuaState, -3);
	lua_pop(LuaState, 1);
}

// Remove battle instance from battle table
void _Scripting::DeleteBattle(_Battle *Battle) {

	// Get table
	lua_getglobal(LuaState, "Battles");
	if(!lua_istable(LuaState, -1))
		throw std::runtime_error("CreateBattle: Battles is not a table!");

	// Battles[NetworkID] = nil
	lua_pushinteger(LuaState, Battle->NetworkID);
	lua_pushnil(LuaState);
	lua_settable(LuaState, -3);
	lua_pop(LuaState, 1);
}

// Push object onto stack
void _Scripting::PushObject(_Object *Object) {
	lua_newtable(LuaState);

	PushObjectStatusEffects(Object);
	lua_setfield(LuaState, -2, "StatusEffects");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectAddTarget, 1);
	lua_setfield(LuaState, -2, "AddTarget");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectClearTargets, 1);
	lua_setfield(LuaState, -2, "ClearTargets");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectGetInventoryItem, 1);
	lua_setfield(LuaState, -2, "GetInventoryItem");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectGetSkillPointsAvailable, 1);
	lua_setfield(LuaState, -2, "GetSkillPointsAvailable");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectSpendSkillPoints, 1);
	lua_setfield(LuaState, -2, "SpendSkillPoints");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectSetAction, 1);
	lua_setfield(LuaState, -2, "SetAction");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectGenerateDamage, 1);
	lua_setfield(LuaState, -2, "GenerateDamage");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectGetDamageReduction, 1);
	lua_setfield(LuaState, -2, "GetDamageReduction");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectGetInputStateFromPath, 1);
	lua_setfield(LuaState, -2, "GetInputStateFromPath");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectFindPath, 1);
	lua_setfield(LuaState, -2, "FindPath");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectFindEvent, 1);
	lua_setfield(LuaState, -2, "FindEvent");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectGetTileEvent, 1);
	lua_setfield(LuaState, -2, "GetTileEvent");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectRespawn, 1);
	lua_setfield(LuaState, -2, "Respawn");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectUseCommand, 1);
	lua_setfield(LuaState, -2, "UseCommand");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectCloseWindows, 1);
	lua_setfield(LuaState, -2, "CloseWindows");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectVendorExchange, 1);
	lua_setfield(LuaState, -2, "VendorExchange");

	lua_pushinteger(LuaState, Object->Status);
	lua_setfield(LuaState, -2, "Status");

	lua_pushnumber(LuaState, Object->Fighter->TurnTimer);
	lua_setfield(LuaState, -2, "TurnTimer");

	lua_pushboolean(LuaState, Object->Action.IsSet());
	lua_setfield(LuaState, -2, "BattleActionIsSet");

	lua_pushinteger(LuaState, Object->Fighter->BattleSide);
	lua_setfield(LuaState, -2, "BattleSide");

	lua_pushinteger(LuaState, Object->Character->Gold);
	lua_setfield(LuaState, -2, "Gold");

	lua_pushinteger(LuaState, Object->Character->Health);
	lua_setfield(LuaState, -2, "Health");

	lua_pushinteger(LuaState, Object->Character->MaxHealth);
	lua_setfield(LuaState, -2, "MaxHealth");

	lua_pushinteger(LuaState, Object->Character->Mana);
	lua_setfield(LuaState, -2, "Mana");

	lua_pushinteger(LuaState, Object->Character->MaxMana);
	lua_setfield(LuaState, -2, "MaxMana");

	lua_pushnumber(LuaState, Object->Character->AttackPower);
	lua_setfield(LuaState, -2, "AttackPower");

	lua_pushinteger(LuaState, Object->Character->HitChance);
	lua_setfield(LuaState, -2, "HitChance");

	lua_pushinteger(LuaState, Object->Character->DamageBlock);
	lua_setfield(LuaState, -2, "DamageBlock");

	lua_pushinteger(LuaState, Object->Character->Evasion);
	lua_setfield(LuaState, -2, "Evasion");

	lua_pushinteger(LuaState, Object->Character->CharacterID);
	lua_setfield(LuaState, -2, "CharacterID");

	lua_pushinteger(LuaState, Object->Position.x);
	lua_setfield(LuaState, -2, "X");

	lua_pushinteger(LuaState, Object->Position.y);
	lua_setfield(LuaState, -2, "Y");

	if(Object->Map)
		lua_pushinteger(LuaState, Object->Map->NetworkID);
	else
		lua_pushinteger(LuaState, 0);
	lua_setfield(LuaState, -2, "MapID");

	if(Object->Battle)
		lua_pushinteger(LuaState, Object->Battle->NetworkID);
	else
		lua_pushnil(LuaState);
	lua_setfield(LuaState, -2, "BattleID");

	lua_pushinteger(LuaState, Object->NetworkID);
	lua_setfield(LuaState, -2, "ID");

	lua_pushlightuserdata(LuaState, Object);
	lua_setfield(LuaState, -2, "Pointer");
}

// Push item onto stack
void _Scripting::PushItem(lua_State *LuaState, const _Item *Item, int Upgrades) {
	if(!Item) {
		lua_pushnil(LuaState);
		return;
	}

	lua_newtable(LuaState);

	lua_pushinteger(LuaState, (int)Item->ID);
	lua_setfield(LuaState, -2, "ID");

	lua_pushinteger(LuaState, (int)Item->Type);
	lua_setfield(LuaState, -2, "Type");

	lua_pushinteger(LuaState, (int)Item->Cost);
	lua_setfield(LuaState, -2, "Cost");

	lua_pushlightuserdata(LuaState, (void *)Item);
	lua_pushcclosure(LuaState, &ItemGenerateDamage, 1);
	lua_setfield(LuaState, -2, "GenerateDamage");

	lua_pushinteger(LuaState, Item->DamageTypeID);
	lua_setfield(LuaState, -2, "DamageType");

	lua_pushinteger(LuaState, (int)Item->GetDamageBlock(Upgrades));
	lua_setfield(LuaState, -2, "DamageBlock");

	lua_pushinteger(LuaState, Upgrades);
	lua_setfield(LuaState, -2, "Upgrades");

	lua_pushlightuserdata(LuaState, (void *)Item);
	lua_setfield(LuaState, -2, "Pointer");
}

// Push action result onto stack
void _Scripting::PushActionResult(_ActionResult *ActionResult) {
	lua_newtable(LuaState);

	PushStatChange(&ActionResult->Source);
	lua_setfield(LuaState, -2, "Source");

	PushStatChange(&ActionResult->Target);
	lua_setfield(LuaState, -2, "Target");
}

// Push stat change struct onto stack
void _Scripting::PushStatChange(_StatChange *StatChange) {
	lua_newtable(LuaState);
}

// Push status effect
void _Scripting::PushStatusEffect(_StatusEffect *StatusEffect) {
	lua_newtable(LuaState);

	lua_getglobal(LuaState, StatusEffect->Buff->Script.c_str());
	lua_setfield(LuaState, -2, "Buff");

	lua_pushinteger(LuaState, StatusEffect->Level);
	lua_setfield(LuaState, -2, "Level");

	lua_pushnumber(LuaState, StatusEffect->Duration);
	lua_setfield(LuaState, -2, "Duration");
}

// Push list of objects
void _Scripting::PushObjectList(std::list<_Object *> &Objects) {
	lua_newtable(LuaState);

	int Index = 1;
	for(const auto &Object : Objects) {
		PushObject(Object);
		lua_rawseti(LuaState, -2, Index);

		Index++;
	}
}

// Push list of object's current status effects
void _Scripting::PushObjectStatusEffects(_Object *Object) {
	lua_newtable(LuaState);

	int Index = 1;
	for(auto &StatusEffect : Object->Character->StatusEffects) {
		PushStatusEffect(StatusEffect);
		lua_rawseti(LuaState, -2, Index);

		Index++;
	}
}

// Push int value
void _Scripting::PushInt(int Value) {
	lua_pushinteger(LuaState, Value);
}

// Push real value
void _Scripting::PushReal(double Value) {
	lua_pushnumber(LuaState, Value);
}

// Get return value as int
int _Scripting::GetInt(int Index) {

	return (int)lua_tointeger(LuaState, Index + CurrentTableIndex);
}

// Get return value as bool
int _Scripting::GetBoolean(int Index) {

	return lua_toboolean(LuaState, Index + CurrentTableIndex);
}

// Get return value as real
double _Scripting::GetReal(int Index) {

	return (double)lua_tonumber(LuaState, Index + CurrentTableIndex);
}

// Get return value as string
std::string _Scripting::GetString(int Index) {

	return lua_tostring(LuaState, Index + CurrentTableIndex);
}

// Get return value as action result, Index=-1 means top of stack, otherwise index of return value
void _Scripting::GetActionResult(int Index, _ActionResult &ActionResult) {
	if(Index != -1)
		Index += CurrentTableIndex;

	// Check return value
	if(!lua_istable(LuaState, Index))
		throw std::runtime_error("GetActionResult: Value is not a table!");

	lua_pushstring(LuaState, "Source");
	lua_gettable(LuaState, -2);
	GetStatChange(-1, ActionResult.Source);
	lua_pop(LuaState, 1);

	lua_pushstring(LuaState, "Target");
	lua_gettable(LuaState, -2);
	GetStatChange(-1, ActionResult.Target);
	lua_pop(LuaState, 1);

	lua_pushstring(LuaState, "Summon");
	lua_gettable(LuaState, -2);
	GetSummon(-1, ActionResult.Summon);
	lua_pop(LuaState, 1);
}

// Get return value as stat change
void _Scripting::GetStatChange(int Index, _StatChange &StatChange) {
	if(Index != -1)
		Index += CurrentTableIndex;

	// Check return value
	if(!lua_istable(LuaState, Index))
		throw std::runtime_error("GetStatChange: Value is not a table!");

	// Iterate over StatChange table
	lua_pushnil(LuaState);
	while(lua_next(LuaState, -2) != 0) 	{

		// Get key name
		std::string Key = lua_tostring(LuaState, -2);

		// Turn key into StatType
		auto Iterator = StatStringToType.find(Key);

		// Get value type
		if(Iterator != StatStringToType.end()) {

			// Get value from lua
			switch(Iterator->second.ValueType) {
				case StatValueType::INTEGER:
					StatChange.Values[Iterator->second.Type].Integer = (int)lua_tonumber(LuaState, -1);
				break;
				case StatValueType::FLOAT:
					StatChange.Values[Iterator->second.Type].Float = (float)lua_tonumber(LuaState, -1);
				break;
				case StatValueType::BOOLEAN:
					StatChange.Values[Iterator->second.Type].Integer = lua_toboolean(LuaState, -1);
				break;
				case StatValueType::POINTER:
					StatChange.Values[Iterator->second.Type].Pointer = lua_touserdata(LuaState, -1);
				break;
			}
		}

		lua_pop(LuaState, 1);
	}
}

// Get summon stats
void _Scripting::GetSummon(int Index, _Summon &Summon) {
	if(Index != -1)
		Index += CurrentTableIndex;

	// Check return value
	if(!lua_istable(LuaState, Index))
		return;

	// Get ID
	lua_getfield(LuaState, -1, "ID");
	Summon.ID = (uint32_t)lua_tointeger(LuaState, -1);
	lua_pop(LuaState, 1);

	// Get Health
	lua_getfield(LuaState, -1, "Health");
	Summon.Health = (int)lua_tonumber(LuaState, -1);
	lua_pop(LuaState, 1);

	// Get Mana
	lua_getfield(LuaState, -1, "Mana");
	Summon.Mana = (int)lua_tonumber(LuaState, -1);
	lua_pop(LuaState, 1);

	// Get Armor
	lua_getfield(LuaState, -1, "Armor");
	Summon.Armor = (int)lua_tonumber(LuaState, -1);
	lua_pop(LuaState, 1);

	// Get Damage
	lua_getfield(LuaState, -1, "MinDamage");
	Summon.MinDamage = (int)lua_tonumber(LuaState, -1);
	lua_pop(LuaState, 1);

	lua_getfield(LuaState, -1, "MaxDamage");
	Summon.MaxDamage = (int)lua_tonumber(LuaState, -1);
	lua_pop(LuaState, 1);

}

// Start a call to a lua class method, return table index
bool _Scripting::StartMethodCall(const std::string &TableName, const std::string &Function) {

	// Find table
	lua_getglobal(LuaState, TableName.c_str());
	if(!lua_istable(LuaState, -1)) {
		lua_pop(LuaState, 1);

		return false;
	}

	// Save table index
	CurrentTableIndex = lua_gettop(LuaState);

	// Get function
	lua_getfield(LuaState, CurrentTableIndex, Function.c_str());
	if(!lua_isfunction(LuaState, -1)) {
		lua_pop(LuaState, 1);
		FinishMethodCall();

		return false;
	}

	// Push self parameter
	lua_getglobal(LuaState, TableName.c_str());

	return true;
}

// Run the function started by StartMethodCall
void _Scripting::MethodCall(int ParameterCount, int ReturnCount) {

	// Call function
	if(lua_pcall(LuaState, ParameterCount+1, ReturnCount, 0)) {
		throw std::runtime_error(lua_tostring(LuaState, -1));
	}
}

// Restore state
void _Scripting::FinishMethodCall() {

	// Restore stack
	lua_settop(LuaState, CurrentTableIndex - 1);
}

// Random.GetInt(min, max)
int _Scripting::RandomGetInt(lua_State *LuaState) {
	int Min = (int)lua_tointeger(LuaState, 1);
	int Max = (int)lua_tointeger(LuaState, 2);

	lua_pushinteger(LuaState, GetRandomInt(Min, Max));

	return 1;
}

// Audio.Play(sound)
int _Scripting::AudioPlay(lua_State *LuaState) {

	// Get filename
	std::string Filename = lua_tostring(LuaState, 1);

	// Get volume
	float Volume = 1.0f;
	if(lua_gettop(LuaState) == 2)
		Volume = (float)lua_tonumber(LuaState, 2);

	// Find sound
	auto Sound = Assets.Sounds.find(Filename);
	if(Sound == Assets.Sounds.end())
		return 1;

	// Play sound
	Audio.PlaySound(Sound->second, Volume);

	return 1;
}

// Set battle target
int _Scripting::ObjectAddTarget(lua_State *LuaState) {
	if(!lua_istable(LuaState, 1))
		throw std::runtime_error("ObjectAddTarget: Target is not a table!");

	// Get self pointer
	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));

	// Get pointer of target table
	lua_pushstring(LuaState, "Pointer");
	lua_gettable(LuaState, -2);
	_Object *Target = (_Object *)lua_touserdata(LuaState, -1);
	lua_pop(LuaState, 1);

	Object->Targets.push_back(Target);

	return 0;
}

// Clear battle targets
int _Scripting::ObjectClearTargets(lua_State *LuaState) {
	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	Object->Targets.clear();

	return 0;
}

// Return an item from the object's inventory
int _Scripting::ObjectGetInventoryItem(lua_State *LuaState) {

	// Get self pointer
	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));

	const _Item *Item = nullptr;
	int Upgrades = 0;

	// Get item
	_Slot Slot;
	Slot.BagType = (_Bag::BagType)lua_tointeger(LuaState, 1);
	Slot.Index = (size_t)lua_tointeger(LuaState, 2);
	if(Object->Inventory->IsValidSlot(Slot)) {
		Item = Object->Inventory->GetSlot(Slot).Item;
		Upgrades = Object->Inventory->GetSlot(Slot).Upgrades;
	}

	// Push item
	PushItem(LuaState, Item, Upgrades);

	return 1;
}

// Return the number of skills available
int _Scripting::ObjectGetSkillPointsAvailable(lua_State *LuaState) {

	// Get self pointer
	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));

	// Push value
	lua_pushinteger(LuaState, Object->Character->GetSkillPointsAvailable());

	return 1;
}

// Spend skill points
int _Scripting::ObjectSpendSkillPoints(lua_State *LuaState) {

	// Get parameters
	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	uint32_t SkillID = (uint32_t)lua_tointeger(LuaState, 1);
	int Amount = (int)lua_tointeger(LuaState, 2);

	// Spend points
	Object->Character->AdjustSkillLevel(SkillID, Amount);
	Object->Character->CalculateStats();

	// Push points available
	lua_pushinteger(LuaState, Object->Character->GetSkillPointsAvailable());

	return 1;
}

// Set battle action
int _Scripting::ObjectSetAction(lua_State *LuaState) {

	// Get self pointer
	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));

	// Set skill used
	size_t ActionBarIndex = (size_t)lua_tointeger(LuaState, 1);
	if(!Object->Character->GetActionFromActionBar(Object->Action, ActionBarIndex)) {
		lua_pushboolean(LuaState, false);
		return 1;
	}

	// Check that the action can be used
	_ActionResult ActionResult;
	ActionResult.Source.Object = Object;
	ActionResult.Scope = ScopeType::BATTLE;
	ActionResult.ActionUsed = Object->Action;
	if(!Object->Action.Item->CanUse(Object->Scripting, ActionResult)) {
		Object->Action.Item = nullptr;
		lua_pushboolean(LuaState, false);
	}
	else
		lua_pushboolean(LuaState, true);

	return 1;
}

// Generate damage
int _Scripting::ObjectGenerateDamage(lua_State *LuaState) {

	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	lua_pushinteger(LuaState, Object->Character->GenerateDamage());

	return 1;
}

// Get damage reduction amount from a type of resistance
int _Scripting::ObjectGetDamageReduction(lua_State *LuaState) {

	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	uint32_t DamageTypeID = (uint32_t)lua_tointeger(LuaState, 1);

	lua_pushnumber(LuaState, 1.0 - (double)Object->Character->Resistances[DamageTypeID] / 100.0);

	return 1;
}

// Pathfind to a position in the map
int _Scripting::ObjectFindPath(lua_State *LuaState) {

	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	int X = (int)lua_tointeger(LuaState, 1);
	int Y = (int)lua_tointeger(LuaState, 2);

	bool Success = Object->Pathfind(Object->Position, glm::ivec2(X, Y));

	lua_pushinteger(LuaState, Success);

	return 1;
}

// Find an event in the map
int _Scripting::ObjectFindEvent(lua_State *LuaState) {

	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	uint32_t Type = (uint32_t)lua_tointeger(LuaState, 1);
	uint32_t Data = (uint32_t)lua_tointeger(LuaState, 2);
	if(!Object->Map)
		return 0;

	glm::ivec2 Position = Object->Position;
	if(!Object->Map->FindEvent(_Event(Type, Data), Position))
		return 0;

	lua_pushinteger(LuaState, Position.x);
	lua_pushinteger(LuaState, Position.y);

	return 2;
}

// Return an event from a tile position
int _Scripting::ObjectGetTileEvent(lua_State *LuaState) {

	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	int X = (int)lua_tointeger(LuaState, 1);
	int Y = (int)lua_tointeger(LuaState, 2);

	if(!Object->Map)
		return 0;

	const _Event &Event = Object->Map->GetTile(glm::ivec2(X, Y))->Event;

	lua_pushinteger(LuaState, Event.Type);
	lua_pushinteger(LuaState, Event.Data);

	return 2;
}

// Get the next input state from a path
int _Scripting::ObjectGetInputStateFromPath(lua_State *LuaState) {

	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	lua_pushinteger(LuaState, Object->GetInputStateFromPath());

	return 1;
}

// Send the respawn command
int _Scripting::ObjectRespawn(lua_State *LuaState) {
	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));

	if(!Object->Server)
		return 0;

	_Buffer Packet;
	Object->Server->HandleRespawn(Packet, Object->Peer);

	return 0;
}

// Send use command
int _Scripting::ObjectUseCommand(lua_State *LuaState) {
	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	Object->Controller->UseCommand = true;

	return 0;
}

// Close all open windows for object
int _Scripting::ObjectCloseWindows(lua_State *LuaState) {
	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	if(!Object->Server)
		return 0;

	_Buffer Packet;
	Packet.Write<uint8_t>(_Object::STATUS_NONE);

	Packet.StartRead();
	Object->Server->HandlePlayerStatus(Packet, Object->Peer);

	return 0;
}

// Interact with vendor
int _Scripting::ObjectVendorExchange(lua_State *LuaState) {
	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	if(!Object->Server || !Object->Character->Vendor)
		return 0;

	_Buffer Packet;
	bool Buy = (bool)lua_toboolean(LuaState, 1);
	Packet.WriteBit(Buy);
	if(Buy) {

		// Get parameters
		uint32_t ItemID = (_Bag::BagType)lua_tointeger(LuaState, 2);
		int Amount = (int)lua_tointeger(LuaState, 3);

		// Build packet
		_Slot VendorSlot;
		_Slot TargetSlot;
		VendorSlot.Index = Object->Character->Vendor->GetSlotFromID(ItemID);
		Packet.Write<uint8_t>((uint8_t)Amount);
		VendorSlot.Serialize(Packet);
		TargetSlot.Serialize(Packet);

		Packet.StartRead();
		Object->Server->HandleVendorExchange(Packet, Object->Peer);
	}
	else {

		// Get parameters
		_Slot Slot;
		Slot.BagType = (_Bag::BagType)lua_tointeger(LuaState, 2);
		Slot.Index = (size_t)lua_tointeger(LuaState, 3);
		uint8_t Amount = (uint8_t)lua_tointeger(LuaState, 4);

		// Build packet
		Packet.Write<uint8_t>((uint8_t)Amount);
		Slot.Serialize(Packet);

		Packet.StartRead();
		Object->Server->HandleVendorExchange(Packet, Object->Peer);
	}

	return 0;
}

// Generate a random damage value for an item
int _Scripting::ItemGenerateDamage(lua_State *LuaState) {

	// Get self pointer
	_Item *Item = (_Item *)lua_touserdata(LuaState, lua_upvalueindex(1));
	int Upgrades = (int)lua_tointeger(LuaState, 1);

	lua_pushinteger(LuaState, GetRandomInt((int)Item->GetMinDamage(Upgrades), (int)Item->GetMaxDamage(Upgrades)));

	return 1;
}

// Print lua stack
void _Scripting::PrintStack(lua_State *LuaState) {
	for(int i = lua_gettop(LuaState); i >= 0; i--) {
		int Type = lua_type(LuaState, i);

		switch(Type) {
			case LUA_TNIL:
				std::cout << i << ": nil" << std::endl;
			break;
			case LUA_TBOOLEAN:
				std::cout << i << ": boolean : " << lua_toboolean(LuaState, i) << std::endl;
			break;
			case LUA_TLIGHTUSERDATA:
				std::cout << i << ": light userdata" << std::endl;
			break;
			case LUA_TNUMBER:
				std::cout << i << ": number : " << lua_tonumber(LuaState, i) << std::endl;
			break;
			case LUA_TSTRING:
				std::cout << i << ": string : " << lua_tostring(LuaState, i) << std::endl;
			break;
			case LUA_TTABLE:
				std::cout << i << ": table" << std::endl;
			break;
			case LUA_TFUNCTION:
				std::cout << i << ": function" << std::endl;
			break;
			case LUA_TUSERDATA:
				std::cout << i << ": userdata" << std::endl;
			break;
			case LUA_TTHREAD:
			break;
		}
	}

	std::cout << "-----------------" << std::endl;
}

// Print lua table
void _Scripting::PrintTable(lua_State *LuaState) {
	lua_pushnil(LuaState);

	while(lua_next(LuaState, -2) != 0) 	{
		if(lua_isstring(LuaState, -1))
			std::cout << lua_tostring(LuaState, -2) << " = " << lua_tostring(LuaState, -1) << std::endl;
		else if(lua_isnumber(LuaState, -1))
			std::cout << lua_tostring(LuaState, -2) << " = " << lua_tonumber(LuaState, -1) << std::endl;
		else if(lua_istable(LuaState, -1))
			PrintTable(LuaState);

		lua_pop(LuaState, 1);
	}
}
