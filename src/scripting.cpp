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
#include <scripting.h>
#include <objects/object.h>
#include <objects/buff.h>
#include <objects/statchange.h>
#include <objects/statuseffect.h>
#include <objects/battle.h>
#include <objects/inventory.h>
#include <stats.h>
#include <audio.h>
#include <assets.h>
#include <random.h>
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
void _Scripting::Setup(_Stats *Stats, const std::string &BaseScript) {
	InjectStats(Stats);
	LoadScript(BaseScript);
	InjectItems(Stats);
}

// Load a script file
void _Scripting::LoadScript(const std::string &Path) {

	// Load the file
	if(luaL_dofile(LuaState, Path.c_str()))
		throw std::runtime_error("Failed to load script " + Path + "\n" + std::string(lua_tostring(LuaState, -1)));
}

// Load global state with stat info
void _Scripting::InjectStats(_Stats *Stats) {
	lua_newtable(LuaState);

	// Add buffs
	for(const auto &Iterator : Stats->Buffs) {
		const _Buff *Buff = Iterator.second;
		if(Buff) {

			// Add pointer to table
			lua_pushstring(LuaState, Buff->Script.c_str());
			lua_pushlightuserdata(LuaState, (void *)Buff);
			lua_settable(LuaState, -3);
		}
	}
	lua_setglobal(LuaState, "Buffs");

	// Add damage types
	lua_newtable(LuaState);
	for(const auto &Iterator : Stats->DamageTypes) {

		// Add pointer to table
		lua_pushstring(LuaState, Iterator.second.c_str());
		lua_pushinteger(LuaState, Iterator.first);
		lua_settable(LuaState, -3);
	}
	lua_setglobal(LuaState, "DamageType");

	// Push inventory slot types
	lua_pushinteger(LuaState, InventoryType::HEAD);
	lua_setglobal(LuaState, "INVENTORY_HEAD");
	lua_pushinteger(LuaState, InventoryType::BODY);
	lua_setglobal(LuaState, "INVENTORY_BODY");
	lua_pushinteger(LuaState, InventoryType::LEGS);
	lua_setglobal(LuaState, "INVENTORY_LEGS");
	lua_pushinteger(LuaState, InventoryType::HAND1);
	lua_setglobal(LuaState, "INVENTORY_HAND1");
	lua_pushinteger(LuaState, InventoryType::HAND2);
	lua_setglobal(LuaState, "INVENTORY_HAND2");
	lua_pushinteger(LuaState, InventoryType::RING1);
	lua_setglobal(LuaState, "INVENTORY_RING1");
	lua_pushinteger(LuaState, InventoryType::RING2);
	lua_setglobal(LuaState, "INVENTORY_RING2");
}

// Inject items
void _Scripting::InjectItems(_Stats *Stats) {

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

// Inject server clock
void _Scripting::InjectTime(double Time) {

	// Push time
	lua_pushnumber(LuaState, Time);
	lua_setglobal(LuaState, "ServerTime");
}

// Push object onto stack
void _Scripting::PushObject(_Object *Object) {
	lua_newtable(LuaState);

	PushObjectStatusEffects(Object);
	lua_setfield(LuaState, -2, "StatusEffects");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectSetBattleTarget, 1);
	lua_setfield(LuaState, -2, "SetBattleTarget");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectGetInventoryItem, 1);
	lua_setfield(LuaState, -2, "GetInventoryItem");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectSetAction, 1);
	lua_setfield(LuaState, -2, "SetAction");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectGenerateDamage, 1);
	lua_setfield(LuaState, -2, "GenerateDamage");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectGetDamageReduction, 1);
	lua_setfield(LuaState, -2, "GetDamageReduction");

	lua_pushnumber(LuaState, Object->TurnTimer);
	lua_setfield(LuaState, -2, "TurnTimer");

	lua_pushboolean(LuaState, Object->Action.IsSet());
	lua_setfield(LuaState, -2, "BattleActionIsSet");

	lua_pushinteger(LuaState, Object->BattleSide);
	lua_setfield(LuaState, -2, "BattleSide");

	lua_pushinteger(LuaState, Object->Gold);
	lua_setfield(LuaState, -2, "Gold");

	lua_pushinteger(LuaState, Object->Health);
	lua_setfield(LuaState, -2, "Health");

	lua_pushinteger(LuaState, Object->MaxHealth);
	lua_setfield(LuaState, -2, "MaxHealth");

	lua_pushinteger(LuaState, Object->Mana);
	lua_setfield(LuaState, -2, "Mana");

	lua_pushinteger(LuaState, Object->MaxMana);
	lua_setfield(LuaState, -2, "MaxMana");

	lua_pushinteger(LuaState, Object->HitChance);
	lua_setfield(LuaState, -2, "HitChance");

	lua_pushinteger(LuaState, Object->DamageBlock);
	lua_setfield(LuaState, -2, "DamageBlock");

	lua_pushinteger(LuaState, Object->Evasion);
	lua_setfield(LuaState, -2, "Evasion");

	lua_pushinteger(LuaState, Object->CharacterID);
	lua_setfield(LuaState, -2, "CharacterID");

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

	lua_pushlightuserdata(LuaState, (void *)Item);
	lua_pushcclosure(LuaState, &ItemGenerateDamage, 1);
	lua_setfield(LuaState, -2, "GenerateDamage");

	lua_pushinteger(LuaState, Item->DamageType);
	lua_setfield(LuaState, -2, "DamageType");

	lua_pushinteger(LuaState, Item->GetDamageBlock(Upgrades));
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
	for(auto &StatusEffect : Object->StatusEffects) {
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
		StatValueType Type;
		if(Iterator != StatStringToType.end()) {

			// Get value from lua
			Type = StatValueTypes[(int)Iterator->second].ValueType;
			switch(Type) {
				case StatValueType::INTEGER:
					StatChange.Values[Iterator->second].Integer = (int)lua_tointeger(LuaState, -1);
				break;
				case StatValueType::FLOAT:
					StatChange.Values[Iterator->second].Float = (float)lua_tonumber(LuaState, -1);
				break;
				case StatValueType::BOOLEAN:
					StatChange.Values[Iterator->second].Integer = lua_toboolean(LuaState, -1);
				break;
				case StatValueType::POINTER:
					StatChange.Values[Iterator->second].Pointer = lua_touserdata(LuaState, -1);
				break;
			}
		}

		lua_pop(LuaState, 1);
	}
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

	// Find sound
	auto Sound = Assets.Sounds.find(Filename);
	if(Sound == Assets.Sounds.end())
		return 1;

	// Play sound
	Audio.PlaySound(Sound->second);

	return 1;
}

// Set battle target
int _Scripting::ObjectSetBattleTarget(lua_State *LuaState) {
	if(!lua_istable(LuaState, 1))
		throw std::runtime_error("ObjectSetBattleTarget: Target is not a table!");

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

// Return an item from the object's inventory
int _Scripting::ObjectGetInventoryItem(lua_State *LuaState) {
	if(!lua_isinteger(LuaState, 1))
		throw std::runtime_error("ObjectGetInventoryItem: Slot is not an integer!");

	// Get self pointer
	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));

	const _Item *Item = nullptr;
	int Upgrades = 0;

	// Get item
	size_t Slot = (size_t)lua_tointeger(LuaState, 1);
	if(Slot < Object->Inventory->Slots.size()) {
		Item = Object->Inventory->Slots[Slot].Item;
		Upgrades = Object->Inventory->Slots[Slot].Upgrades;
	}

	// Push item
	PushItem(LuaState, Item, Upgrades);

	return 1;
}

// Set battle action
int _Scripting::ObjectSetAction(lua_State *LuaState) {

	// Get self pointer
	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));

	// Set skill used
	size_t ActionBarIndex = (size_t)lua_tointeger(LuaState, 1);
	if(!Object->GetActionFromSkillbar(Object->Action, ActionBarIndex)) {
		lua_pushboolean(LuaState, false);
		return 1;
	}

	// Check that the action can be used
	_ActionResult ActionResult;
	ActionResult.Source.Object = Object;
	ActionResult.Scope = ScopeType::BATTLE;
	ActionResult.ActionUsed = Object->Action;
	if(Object->Action.Item->CanUse(Object->Scripting, ActionResult))
		lua_pushboolean(LuaState, true);
	else
		lua_pushboolean(LuaState, false);

	return 1;
}

// Generate damage
int _Scripting::ObjectGenerateDamage(lua_State *LuaState) {

	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	lua_pushinteger(LuaState, Object->GenerateDamage());

	return 1;
}

// Get damage reduction amount from a type of resistance
int _Scripting::ObjectGetDamageReduction(lua_State *LuaState) {

	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	uint32_t DamageTypeID = (uint32_t)lua_tointeger(LuaState, 1);

	lua_pushnumber(LuaState, 1.0 - (double)Object->Resistances[DamageTypeID] / 100.0);

	return 1;
}

// Generate a random damage value for an item
int _Scripting::ItemGenerateDamage(lua_State *LuaState) {

	// Get self pointer
	_Item *Item = (_Item *)lua_touserdata(LuaState, lua_upvalueindex(1));
	int Upgrades = (int)lua_tointeger(LuaState, 1);

	lua_pushinteger(LuaState, GetRandomInt(Item->GetMinDamage(Upgrades), Item->GetMaxDamage(Upgrades)));

	return 1;
}

// Print lua stack
void _Scripting::PrintStack(lua_State *LuaState) {
	for(int i = lua_gettop(LuaState); i >= 0; i--) {
		if(lua_isnumber(LuaState, i))
			std::cout << i << ": number : " << lua_tonumber(LuaState, i) << std::endl;
		else if(lua_isstring(LuaState, i))
			std::cout << i << ": string : " << lua_tostring(LuaState, i) << std::endl;
		else if(lua_istable(LuaState, i))
			std::cout << i << ": table" << std::endl;
		else if(lua_iscfunction(LuaState, i))
			std::cout << i << ": cfunction" << std::endl;
		else if(lua_isfunction(LuaState, i))
			std::cout << i << ": function" << std::endl;
		else if(lua_isuserdata(LuaState, i))
			std::cout << i << ": userdata" << std::endl;
		else if(lua_isnil(LuaState, i))
			std::cout << i << ": nil" << std::endl;
		else if(lua_islightuserdata(LuaState, i))
			std::cout << i << ": light userdata" << std::endl;
		else if(lua_isboolean(LuaState, i))
			std::cout << i << ": boolean : " << lua_toboolean(LuaState, i) << std::endl;
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
