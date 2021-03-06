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
#include <objects/statchange.h>
#include <lua.hpp>
#include <list>
#include <string>
#include <vector>

// Forward Declarations
class _Object;
class _Battle;
class _Item;
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
		void LoadItemAttributes(_Stats *Stats);

		void CreateBattle(_Battle *Battle);
		void DeleteBattle(_Battle *Battle);

		void PushObject(_Object *Object);
		void PushActionResult(_ActionResult *ActionResult);
		void PushStatChange(_StatChange *StatChange);
		void PushStatusEffect(_StatusEffect *StatusEffect);
		void PushObjectList(std::vector<_Object *> &Objects);
		void PushObjectStatusEffects(_Object *Object);
		void PushItemParameters(uint32_t ID, int Chance, int Level, double Duration, int Upgrades, int SetLevel, int MaxSetLevel, int MoreInfo);
		void PushBoolean(bool Value);
		void PushInt(int Value);
		void PushReal(double Value);

		int GetInt(int Index);
		int64_t GetInt64(int Index);
		int GetBoolean(int Index);
		double GetReal(int Index);
		std::string GetString(int Index);
		void *GetPointer(int Index);
		void GetActionResult(int Index, _ActionResult &ActionResult);
		void GetStatChange(int Index, const _Stats *Stats, _StatChange &StatChange);
		void GetSummons(int Index, std::vector<_Summon> &Summons);
		void GetValue(StatValueType Type, _Value &Value);

		bool StartMethodCall(const std::string &TableName, const std::string &Function);
		void MethodCall(int ParameterCount, int ReturnCount);
		void FinishMethodCall();

		static void PrintStack(lua_State *LuaState);
		static void PrintTable(lua_State *LuaState, int Level=0);

		static luaL_Reg RandomFunctions[];
		static luaL_Reg AudioFunctions[];

	private:

		static void PushItem(lua_State *LuaState, const _Stats *Stats, const _Item *Item, int Upgrades);

		static int RandomGetInt(lua_State *LuaState);
		static int AudioPlay(lua_State *LuaState);

		static int ObjectAddTarget(lua_State *LuaState);
		static int ObjectClearTargets(lua_State *LuaState);
		static int ObjectClearBattleTargets(lua_State *LuaState);
		static int ObjectGetInventoryItem(lua_State *LuaState);
		static int ObjectGetInventoryItemCount(lua_State *LuaState);
		static int ObjectGetSkillPointsAvailable(lua_State *LuaState);
		static int ObjectSpendSkillPoints(lua_State *LuaState);
		static int ObjectSetAction(lua_State *LuaState);
		static int ObjectGenerateDamage(lua_State *LuaState);
		static int ObjectGetAverageDamage(lua_State *LuaState);
		static int ObjectGetDamageReduction(lua_State *LuaState);
		static int ObjectFindPath(lua_State *LuaState);
		static int ObjectFindEvent(lua_State *LuaState);
		static int ObjectGetTileEvent(lua_State *LuaState);
		static int ObjectGetTileZone(lua_State *LuaState);
		static int ObjectGetInputStateFromPath(lua_State *LuaState);
		static int ObjectRespawn(lua_State *LuaState);
		static int ObjectUseCommand(lua_State *LuaState);
		static int ObjectCloseWindows(lua_State *LuaState);
		static int ObjectVendorExchange(lua_State *LuaState);
		static int ObjectUpdateBuff(lua_State *LuaState);
		static int ObjectHasBuff(lua_State *LuaState);

		static int ItemGenerateDamage(lua_State *LuaState);
		static int ItemGetAverageDamage(lua_State *LuaState);

		lua_State *LuaState;
		int CurrentTableIndex;

};
