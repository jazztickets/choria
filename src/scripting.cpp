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
#include <server.h>
#include <stdexcept>

// Constructor
_Scripting::_Scripting() :
	Server(nullptr),
	LuaState(nullptr) {

	// Initialize lua object
	LuaState = luaL_newstate();
	luaopen_base(LuaState);
	luaopen_math(LuaState);

	// Set globals
	lua_pushlightuserdata(LuaState, this);
	lua_setglobal(LuaState, "param_scripting");

	// Register C++ functions used by lua
	lua_register(LuaState, "map_change", &MapChangeFunction);
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

// Execute lua code
void _Scripting::ExecuteLua(const std::string &Code, _Object *Object) {
	lua_pushlightuserdata(LuaState, Object);
	lua_setglobal(LuaState, "param_object");

	int ReturnCode = luaL_dostring(LuaState, Code.c_str());
	if(ReturnCode)
		throw std::runtime_error(lua_tostring(LuaState, -1));
}

// Change maps
int _Scripting::MapChangeFunction(lua_State *LuaState) {
	int ArgumentCount = lua_gettop(LuaState);
	if(ArgumentCount != 1)
		throw std::runtime_error("Wrong argument count for function map_change(map)");

	// Get parameters
	std::string Map = lua_tostring(LuaState, 1);

	// Get object
	//lua_getglobal(LuaState, "param_object");
	//_Object *Object = (_Object *)lua_topointer(LuaState, -1);

	// Get scripting pointer
	//lua_getglobal(LuaState, "param_scripting");
	//_Scripting *Scripting = (_Scripting *)lua_topointer(LuaState, -1);

	// Change maps
	//Scripting->Server->ChangePlayerMap(Map, Object->Peer);

	return 0;
}
