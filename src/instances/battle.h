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
#include <constants.h>
#include <list>
#include <cstdint>

// Forward Declarations
class _Object;
class _Stats;
class _Item;
class _Buffer;
class _Element;
class _ServerNetwork;
class _ClientNetwork;

// Structures
struct _ActionResult {
	_ActionResult() : Fighter(nullptr), SkillID(-1), Target(-1), DamageDealt(0), HealthChange(0), ManaChange(0) { }
	_Object *Fighter;
	int SkillID;
	int Target;
	int DamageDealt;
	int HealthChange;
	int ManaChange;
};

struct _BattleResult {
	_BattleResult() : FighterCount(0), PlayerCount(0), MonsterCount(0), ExperienceGiven(0), GoldGiven(0), Dead(true) { }
	int FighterCount;
	int PlayerCount;
	int MonsterCount;
	int ExperienceGiven;
	int GoldGiven;
	bool Dead;
};

// Classes
class _Battle {

	public:

		enum StateType {
			STATE_NONE,
			// server
			STATE_END,
			// client
			STATE_INITWIN,
			STATE_WIN,
			STATE_INITLOSE,
			STATE_LOSE,
		};

		_Battle();
		~_Battle();

		// Objects
		void AddFighter(_Object *Fighter, int Side);
		void RemoveFighter(_Object *RemoveFighter);
		void SetDefaultTargets();
		int GetPeerCount();

		// Updates
		void Update(double FrameTime);
		void Render(double BlendFactor);
		void ClientHandleAction(int Action);

		void ResolveAction(_Object *SourceFighter);

		// CLIENT BATTLE

		// Setup
		void StartBattleClient();

		// Input
		void HandleCommand(int Slot, uint32_t SkillID);

		// Render
		void GetBattleOffset(int SideIndex, _Object *Fighter);

		// Resolve
		void ClientResolveAction(_Buffer *Packet);
		void EndBattle(_Buffer *Packet);

		void UpdateStats();

		// SERVER BATTLE

		// Setup
		void StartBattleServer();

		// Input
		void ServerHandleAction(_Object *Fighter, int ActionBarSlot);

		// Resolve
		void CheckEnd();

		void BroadcastPacket(_Buffer &Packet);
		void SendActionToPlayers(_Object *Player);

		_Stats *Stats;
		_ServerNetwork *ServerNetwork;
		_ClientNetwork *ClientNetwork;
		_Object *ClientPlayer;

	protected:

		void ClientSetAction(int ActionBarSlot);
		void ChangeTarget(int Direction);

		void GetFighterList(int Side, std::list<_Object *> &SideFighters);
		void GetAliveFighterList(int Side, std::list<_Object *> &AliveFighters);
		void GetPlayerList(int Side, std::list<_Object *> &Players);
		int GetFighterFromSlot(int Slot);

		void RenderBattle();
		void RenderBattleWin();
		void RenderBattleLose();

		// State
		int State;
		double Timer;

		// Objects
		std::list<_Object *> Fighters;
		int LeftFighterCount, RightFighterCount;
		int PlayerCount, MonsterCount;

		// CLIENT BATTLE

		// Battle results
		int TotalExperience, TotalGold;

		// UI
		_Element *BattleElement;
		_Element *BattleWinElement;
		_Element *BattleLoseElement;

		// Items
		std::list<const _Item *> MonsterDrops;
};
