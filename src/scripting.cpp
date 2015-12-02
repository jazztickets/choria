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
#include <stdexcept>

// Constructor
_Scripting::_Scripting() :
	LuaState(nullptr) {

	// Initialize lua object
	LuaState = luaL_newstate();
	luaopen_base(LuaState);
	luaopen_math(LuaState);
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
