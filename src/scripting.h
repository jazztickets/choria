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
#include <list>
#include <string>
#include <lua.hpp>

// Forward Declarations
class _Object;
class _Stats;
struct _ActionResult;

// Classes
class _Scripting {

	public:

		_Scripting();
		~_Scripting();

		void LoadScript(const std::string &Path);
		void InjectStats(_Stats *Stats);

		void PushData(void *Data);
		void PushObject(_Object *Object);
		void PushActionResult(_ActionResult *ActionResult);
		void PushObjectList(std::list<_Object *> &Objects);
		void PushInt(int Value);

		int GetInt(int Index);
		std::string GetString(int Index);
		void GetActionResult(int Index, _ActionResult &ActionResult);

		bool StartMethodCall(const std::string &TableName, const std::string &Function);
		void MethodCall(int ParameterCount, int ReturnCount);
		void FinishMethodCall();

		static void PrintStack(lua_State *LuaState);

		static luaL_Reg RandomFunctions[];

	private:

		static int RandomGetInt(lua_State *LuaState);

		static int ObjectSetBattleTarget(lua_State *LuaState);
		static int ObjectSetAction(lua_State *LuaState);
		static int ObjectGenerateDamage(lua_State *LuaState);
		static int ObjectGenerateDefense(lua_State *LuaState);

		lua_State *LuaState;
		int CurrentTableIndex;

};
