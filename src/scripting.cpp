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
	LuaState(nullptr) {

	// Initialize lua object
	LuaState = luaL_newstate();
	luaopen_base(LuaState);
	luaopen_math(LuaState);

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
	if(luaL_dofile(LuaState, Path.c_str()) != 0)
		throw std::runtime_error("Failed to load script " + Path + "\n" + std::string(lua_tostring(LuaState, -1)));
}

// Push pointer onto stack
void _Scripting::PushData(void *Data) {
	lua_pushlightuserdata(LuaState, Data);
}

// Push pointer onto stack
void _Scripting::PushObject(_Object *Object) {
	lua_newtable(LuaState);

	lua_pushlightuserdata(LuaState, Object);
	lua_pushcclosure(LuaState, &ObjectSetTarget, 1);
	lua_setfield(LuaState, -2, "SetTarget");

	lua_pushinteger(LuaState, Object->BattleSide);
	lua_setfield(LuaState, -2, "BattleSide");

	lua_pushinteger(LuaState, Object->Health);
	lua_setfield(LuaState, -2, "Health");
}

// Start a call to a lua class method, return table index
int _Scripting::StartMethodCall(const std::string &TableName, const std::string &Function) {

	// Find table
	lua_getglobal(LuaState, TableName.c_str());
	if(!lua_istable(LuaState, -1)) {
		lua_pop(LuaState, 1);

		throw std::runtime_error("Failed to find table " + TableName);
	}

	// Save table index
	int TableIndex = lua_gettop(LuaState);

	// Get function
	lua_getfield(LuaState, TableIndex, Function.c_str());
	if(!lua_isfunction(LuaState, -1)) {
		lua_pop(LuaState, 1);

		throw std::runtime_error("Failed to find function " + Function);
	}

	return TableIndex;
}

// Run the function started by StartMethodCall
void _Scripting::FinishMethodCall(int TableIndex, int Parameters) {

	// Call function
	if(lua_pcall(LuaState, Parameters, 0, 0)) {
		throw std::runtime_error(lua_tostring(LuaState, -1));
	}

	// Restore stack
	lua_settop(LuaState, TableIndex - 1);
}

// Random.GetInt(min, max)
int _Scripting::RandomGetInt(lua_State *LuaState) {
	int Min = lua_tointeger(LuaState, 1);
	int Max = lua_tointeger(LuaState, 2);

	lua_pushinteger(LuaState, GetRandomInt(Min, Max));

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

// Set battle target
int _Scripting::ObjectSetTarget(lua_State *LuaState) {
	_Object *Object = (_Object *)lua_touserdata(LuaState, lua_upvalueindex(1));
	int Target = lua_tointeger(LuaState, 1);

	std::cout << "Target: " << Object->Name << ":" << Target << std::endl;

	return 0;
}