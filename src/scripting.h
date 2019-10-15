/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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
#include <lua.hpp>
#include <list>
#include <string>

// Constants
const std::string GAME_SCRIPTS = "scripts/game.lua";

// Forward Declarations
class _Object;
class _Light;
class _Battle;
class _BaseItem;
class _Stats;
class _StatChange;
class _StatusEffect;
struct _Summon;
struct _ActionResult;

// Classes
class _Scripting {

	public:

		_Scripting();
		~_Scripting();

		void Setup(const _Stats *Stats, const std::string &BaseScript);

		void LoadScript(const std::string &Path);
		void InjectStats(const _Stats *Stats);
		void InjectItemPointers(const _Stats *Stats);
		void InjectItems(const _Stats *Stats);
		void InjectMonsters(const _Stats *Stats);
		void InjectBuffs(const _Stats *Stats);
		void InjectTime(double Time);

		void CreateBattle(_Battle *Battle);
		void DeleteBattle(_Battle *Battle);

		void PushObject(_Object *Object);
		void PushLight(_Light *Light);
		void PushActionResult(_ActionResult *ActionResult);
		void PushStatChange(_StatChange *StatChange);
		void PushStatusEffect(_StatusEffect *StatusEffect);
		void PushObjectList(std::list<_Object *> &Objects);
		void PushObjectStatusEffects(_Object *Object);
		void PushItemParameters(int Level, double Duration);
		void PushString(const std::string &Value);
		void PushInt(int Value);
		void PushReal(double Value);

		int GetInt(int Index);
		int GetBoolean(int Index);
		double GetReal(int Index);
		std::string GetString(int Index);
		void GetActionResult(int Index, _ActionResult &ActionResult);
		void GetStatChange(int Index, _StatChange &StatChange);
		void GetSummon(int Index, _Summon &Summon);
		void GetLight(int Index, _Light *Light);

		bool StartMethodCall(const std::string &TableName, const std::string &Function);
		void MethodCall(int ParameterCount, int ReturnCount);
		void FinishMethodCall();

		static void PrintStack(lua_State *LuaState);
		static void PrintTable(lua_State *LuaState);

		static luaL_Reg RandomFunctions[];
		static luaL_Reg AudioFunctions[];

	private:

		static void PushItem(lua_State *LuaState, const _BaseItem *Item, int Upgrades);

		static int RandomGetInt(lua_State *LuaState);
		static int RandomGetReal(lua_State *LuaState);
		static int AudioPlay(lua_State *LuaState);

		static int ObjectAddTarget(lua_State *LuaState);
		static int ObjectClearTargets(lua_State *LuaState);
		static int ObjectClearBattleTargets(lua_State *LuaState);
		static int ObjectGetInventoryItem(lua_State *LuaState);
		static int ObjectSetAction(lua_State *LuaState);
		static int ObjectGenerateDamage(lua_State *LuaState);
		static int ObjectGetDamageReduction(lua_State *LuaState);
		static int ObjectFindPath(lua_State *LuaState);
		static int ObjectFindEvent(lua_State *LuaState);
		static int ObjectGetTileEvent(lua_State *LuaState);
		static int ObjectGetInputStateFromPath(lua_State *LuaState);
		static int ObjectRespawn(lua_State *LuaState);
		static int ObjectUseCommand(lua_State *LuaState);
		static int ObjectCloseWindows(lua_State *LuaState);
		static int ObjectVendorExchange(lua_State *LuaState);

		static int ItemGenerateDamage(lua_State *LuaState);

		lua_State *LuaState;
		int CurrentTableIndex;

};
