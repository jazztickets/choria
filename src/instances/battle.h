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
#include <vector>
#include <cstdint>

// Forward Declarations
class _Object;
class _Stats;
class _Item;
class _Buffer;
class _Element;
class _ServerNetwork;

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
			STATE_INPUT,
			STATE_RESOLVETURN,
			STATE_END,
			// client
			STATE_GETINPUT,
			STATE_WAIT,
			STATE_TURNRESULTS,
			STATE_INITWIN,
			STATE_WIN,
			STATE_INITLOSE,
			STATE_LOSE,
			STATE_DELETE,
		};

		_Battle();
		virtual ~_Battle();

		// Objects
		void AddFighter(_Object *Fighter, int Side);
		virtual int RemoveFighter(_Object *RemoveFighter) { return 0; }
		int RemoveFighterClient(_Object *Fighter);

		// Updates
		void Update(double FrameTime);
		void Render(double BlendFactor);
		void HandleAction(int Action);

		// States
		int GetState() const { return State; }

		// CLIENT BATTLE

		// Setup
		void StartBattleClient(_Object *Player);

		// Input
		void HandleCommand(int Slot, uint32_t SkillID);

		// Updates
		void UpdateClient(double FrameTime);

		// Render
		void GetPositionFromSlot(int Slot, glm::ivec2 &Position);

		// Resolve
		void ResolveTurn(_Buffer *Packet);
		void EndBattle(_Buffer *Packet);

		void UpdateStats();

		// SERVER BATTLE

		// Setup
		void StartBattleServer();

		// Objects
		int RemoveFighterServer(_Object *RemoveFighterServer);

		// Input
		void HandleInput(_Object *Player, int Command, int Target);

		// Updates
		void UpdateServer(double FrameTime);

		// Resolve
		void ResolveTurn();
		void CheckEnd();

		void BroadcastPacket(_Buffer &Packet);
		void SendSkillToPlayers(_Object *Player);

		_Stats *Stats;
		_ServerNetwork *ServerNetwork;

	protected:

		void SendSkill(int SkillSlot);
		void ChangeTarget(int Direction);

		void GetFighterList(int Side, std::vector<_Object *> &SideFighters);
		void GetAliveFighterList(int Side, std::vector<_Object *> &AliveFighters);
		void GetMonsterList(std::vector<_Object *> &Monsters);
		void GetPlayerList(int Side, std::vector<_Object *> &Players);
		int GetFighterFromSlot(int Slot);

		void RenderBattle(bool ShowResults);
		void RenderBattleWin();
		void RenderBattleLose();

		// State
		int State, TargetState;
		double Timer;
		double RoundTime;

		// Objects
		std::vector<_Object *> Fighters;
		int LeftFighterCount, RightFighterCount;
		int PlayerCount, MonsterCount;

		// CLIENT BATTLE

		// Battle results
		double ResultTimer;
		bool ShowResults;
		int TotalExperience, TotalGold;

		// UI
		_Element *BattleElement;
		_Element *BattleWinElement;
		_Element *BattleLoseElement;

		// Actions
		_ActionResult Results[BATTLE_MAXFIGHTERS];

		// Client's player
		_Object *ClientPlayer;

		// Items
		std::vector<const _Item *> MonsterDrops;
};
