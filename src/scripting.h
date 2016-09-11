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
#pragma once

// Libraries
#include <list>
#include <string>
#include <lua.hpp>

// Forward Declarations
class _Object;
class _Item;
class _Stats;
class _StatChange;
class _StatusEffect;
struct _ActionResult;

// Classes
class _Scripting {

	public:

		_Scripting();
		~_Scripting();

		void Setup(_Stats *Stats, const std::string &BaseScript);

		void LoadScript(const std::string &Path);
		void InjectStats(_Stats *Stats);
		void InjectItems(_Stats *Stats);
		void InjectTime(double Time);

		void PushObject(_Object *Object);
		void PushActionResult(_ActionResult *ActionResult);
		void PushStatChange(_StatChange *StatChange);
		void PushStatusEffect(_StatusEffect *StatusEffect);
		void PushObjectList(std::list<_Object *> &Objects);
		void PushObjectStatusEffects(_Object *Object);
		void PushInt(int Value);
		void PushReal(double Value);

		int GetInt(int Index);
		int GetBoolean(int Index);
		double GetReal(int Index);
		std::string GetString(int Index);
		void GetActionResult(int Index, _ActionResult &ActionResult);
		void GetStatChange(int Index, _StatChange &StatChange);

		bool StartMethodCall(const std::string &TableName, const std::string &Function);
		void MethodCall(int ParameterCount, int ReturnCount);
		void FinishMethodCall();

		static void PrintStack(lua_State *LuaState);
		static void PrintTable(lua_State *LuaState);

		static luaL_Reg RandomFunctions[];
		static luaL_Reg AudioFunctions[];

	private:

		static void PushItem(lua_State *LuaState, const _Item *Item, int Upgrades);

		static int RandomGetInt(lua_State *LuaState);
		static int AudioPlay(lua_State *LuaState);

		static int ObjectSetBattleTarget(lua_State *LuaState);
		static int ObjectGetInventoryItem(lua_State *LuaState);
		static int ObjectSetAction(lua_State *LuaState);
		static int ObjectGenerateDamage(lua_State *LuaState);
		static int ObjectGetDamageReduction(lua_State *LuaState);

		static int ItemGenerateDamage(lua_State *LuaState);

		lua_State *LuaState;
		int CurrentTableIndex;

};
