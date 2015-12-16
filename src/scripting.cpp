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
#include <scripting.h>
#include <objects/object.h>
#include <objects/buff.h>
#include <objects/statchange.h>
#include <objects/battle.h>
#include <stats.h>
#include <random.h>
#include <stdexcept>
#include <iostream>

// Controls
luaL_Reg _Scripting::RandomFunctions[] = {
	{"GetInt", &_Scripting::RandomGetInt},
	{nullptr, nullptr}
};

// Lua library functions
int luaopen_Object(lua_State *LuaState) {
	luaL_newlib(LuaState, _Scripting::RandomFunctions);
	return 1;
}

// Constructor
_Scripting::_Scripting() :
	LuaState(nullptr),
	CurrentTableIndex(0) {

	// Initialize lua object
	LuaState = luaL_newstate();
	luaL_openlibs(LuaState);
	luaL_requiref(LuaState, "Random", luaopen_Object, 1);
}

// Destructor
_Scripting::~_Scripting() {

	// Close lua state
	if(LuaState != nullptr)
		lua_close(LuaState);
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

	// Iterate through buffs
	for(const auto &Iterator : Stats->Buffs) {
		const _Buff *Buff = Iterator.second;
		if(Buff) {

			// Add pointer to table
			lua_pushstring(LuaState, Buff->Script.c_str());
			lua_pushlightuserdata(LuaState, (void *)Buff);
			lua_settable(LuaState, -3);
		}
	}

	// Assign table a name
	lua_setglobal(LuaState, "Buffs");
}

// Push pointer onto stack
void _Scripting::PushData(void *Data) {
	lua_pushlightuserdata(LuaState, Data);
}

// Push pointer onto stack
void _Scripting::PushObject(_Object *Object) {
	lua_newtable(LuaState);

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectSetBattleTarget, 1);
	lua_setfield(LuaState, -2, "SetBattleTarget");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectSetAction, 1);
	lua_setfield(LuaState, -2, "SetAction");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectGenerateDamage, 1);
	lua_setfield(LuaState, -2, "GenerateDamage");

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectGenerateDefense, 1);
	lua_setfield(LuaState, -2, "GenerateDefense");

	lua_pushnumber(LuaState, Object->TurnTimer);
	lua_setfield(LuaState, -2, "TurnTimer");

	lua_pushboolean(LuaState, Object->Action.IsSet());
	lua_setfield(LuaState, -2, "BattleActionIsSet");

	lua_pushlightuserdata(LuaState, Object->Targets.front());
	lua_setfield(LuaState, -2, "BattleTarget");

	lua_pushinteger(LuaState, Object->BattleSide);
	lua_setfield(LuaState, -2, "BattleSide");

	lua_pushinteger(LuaState, Object->Health);
	lua_setfield(LuaState, -2, "Health");

	lua_pushinteger(LuaState, Object->MaxHealth);
	lua_setfield(LuaState, -2, "MaxHealth");

	lua_pushinteger(LuaState, Object->Mana);
	lua_setfield(LuaState, -2, "Mana");

	lua_pushinteger(LuaState, Object->MaxMana);
	lua_setfield(LuaState, -2, "MaxMana");

	lua_pushlightuserdata(LuaState, Object);
	lua_setfield(LuaState, -2, "Pointer");
}

// Push action result onto stack
void _Scripting::PushActionResult(_ActionResult *ActionResult) {
	lua_newtable(LuaState);

	lua_pushinteger(LuaState, ActionResult->Target.HealthChange);
	lua_setfield(LuaState, -2, "TargetHealthChange");

	lua_pushinteger(LuaState, ActionResult->Source.ManaChange);
	lua_setfield(LuaState, -2, "SourceManaChange");
}

// Push stat change struct onto stack
void _Scripting::PushStatChange(_StatChange *StatChange) {
	lua_newtable(LuaState);

	lua_pushinteger(LuaState, StatChange->HealthChange);
	lua_setfield(LuaState, -2, "HealthChanges");

	lua_pushinteger(LuaState, StatChange->ManaChange);
	lua_setfield(LuaState, -2, "ManaChange");
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

// Push int value
void _Scripting::PushInt(int Value) {
	lua_pushinteger(LuaState, Value);
}

// Get return value as int
int _Scripting::GetInt(int Index) {

	return (int)lua_tointeger(LuaState, Index + CurrentTableIndex);
}

// Get return value as string
std::string _Scripting::GetString(int Index) {

	return lua_tostring(LuaState, Index + CurrentTableIndex);
}

// Get return value as action result
void _Scripting::GetActionResult(int Index, _ActionResult &ActionResult) {
	if(!lua_istable(LuaState, Index + CurrentTableIndex))
		throw std::runtime_error("GetActionResult: Value is not a table!");

	lua_pushstring(LuaState, "TargetHealthChange");
	lua_gettable(LuaState, -2);
	ActionResult.Target.HealthChange = (int)lua_tointeger(LuaState, -1);
	lua_pop(LuaState, 1);

	lua_pushstring(LuaState, "SourceManaChange");
	lua_gettable(LuaState, -2);
	ActionResult.Source.ManaChange = (int)lua_tointeger(LuaState, -1);
	lua_pop(LuaState, 1);

	lua_pushstring(LuaState, "Buff");
	lua_gettable(LuaState, -2);
	ActionResult.Buff = (_Buff *)lua_touserdata(LuaState, -1);
	lua_pop(LuaState, 1);

	lua_pushstring(LuaState, "BuffLevel");
	lua_gettable(LuaState, -2);
	ActionResult.BuffLevel = (int)lua_tointeger(LuaState, -1);
	lua_pop(LuaState, 1);

	lua_pushstring(LuaState, "BuffDuration");
	lua_gettable(LuaState, -2);
	ActionResult.BuffDuration = (int)lua_tointeger(LuaState, -1);
	lua_pop(LuaState, 1);
}

// Get return value as stat change
void _Scripting::GetStatChange(int Index, _StatChange &StatChange) {
	if(!lua_istable(LuaState, Index + CurrentTableIndex))
		throw std::runtime_error("GetStatChange: Value is not a table!");

	lua_pushstring(LuaState, "HealthChange");
	lua_gettable(LuaState, -2);
	StatChange.HealthChange = (int)lua_tointeger(LuaState, -1);
	lua_pop(LuaState, 1);

	lua_pushstring(LuaState, "ManaChange");
	lua_gettable(LuaState, -2);
	StatChange.ManaChange = (int)lua_tointeger(LuaState, -1);
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

	return true;
}

// Run the function started by StartMethodCall
void _Scripting::MethodCall(int ParameterCount, int ReturnCount) {

	// Call function
	if(lua_pcall(LuaState, ParameterCount, ReturnCount, 0)) {
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

// Set battle action
int _Scripting::ObjectSetAction(lua_State *LuaState) {

	// Get self pointer
	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));

	// Set skill used
	size_t ActionBarIndex = (size_t)lua_tointeger(LuaState, 1);
	if(ActionBarIndex < Object->ActionBar.size())
		Object->Action.Skill = Object->ActionBar[ActionBarIndex].Skill;

	return 0;
}

// Generate damage
int _Scripting::ObjectGenerateDamage(lua_State *LuaState) {

	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	lua_pushinteger(LuaState, Object->GenerateDamage());

	return 1;
}

// Generate defense
int _Scripting::ObjectGenerateDefense(lua_State *LuaState) {

	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	lua_pushinteger(LuaState, Object->GenerateDefense());

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
