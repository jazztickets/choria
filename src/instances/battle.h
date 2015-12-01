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
#include <cstdint>

// Forward Declarations
class _Object;
class _Skill;
class _Item;
class _Buffer;
class _Element;
class _Stats;
class _ServerNetwork;
class _ClientNetwork;

// Structures
struct _ActionResult {
	_ActionResult() :
		SourceFighter(nullptr),
		TargetFighter(nullptr),
		SkillUsed(nullptr),
		ItemUsed(nullptr),
		DamageDealt(0),
		SourceHealthChange(0),
		SourceManaChange(0),
		TargetHealthChange(0),
		TargetManaChange(0) { }

	_Object *SourceFighter;
	_Object *TargetFighter;
	const _Skill *SkillUsed;
	const _Item *ItemUsed;
	int DamageDealt;
	int SourceHealthChange;
	int SourceManaChange;
	int TargetHealthChange;
	int TargetManaChange;
};

struct _BattleResult {
	_BattleResult() :
		FighterCount(0),
		PlayerCount(0),
		MonsterCount(0),
		TotalExperienceGiven(0),
		TotalGoldGiven(0),
		ExperiencePerFighter(0),
		GoldPerFighter(0),
		Dead(true) { }

	int FighterCount;
	int PlayerCount;
	int MonsterCount;
	int TotalExperienceGiven;
	int TotalGoldGiven;
	int ExperiencePerFighter;
	int GoldPerFighter;
	bool Dead;
};

// Classes
class _Battle {

	public:

		enum StateType {
			STATE_NONE,
			STATE_WIN,
			STATE_LOSE,
		};

		_Battle();
		~_Battle();

		// Objects
		void AddFighter(_Object *Fighter, int Side);
		void RemoveFighter(_Object *RemoveFighter);
		void SetDefaultTargets();
		_Object *GetObjectByID(int BattleID);
		int GetPeerCount();

		// Updates
		void Update(double FrameTime);
		void Render(double BlendFactor);

		// Setup
		void ServerStartBattle();
		void ClientStartBattle();
		void ServerEndBattle();
		void ClientEndBattle(_Buffer &Data);

		// Resolve
		void ResolveAction(_Object *SourceFighter);
		void ClientResolveAction(_Buffer &Data);

		// Input
		bool ClientHandleInput(int Action);
		void ServerHandleAction(_Object *Fighter, int ActionBarSlot);
		void ClientHandlePlayerAction(_Buffer &Data);

		_Stats *Stats;
		_ServerNetwork *ServerNetwork;
		_ClientNetwork *ClientNetwork;
		_Object *ClientPlayer;

	protected:

		void BroadcastPacket(_Buffer &Packet);

		void GetBattleOffset(int SideIndex, _Object *Fighter);
		void ClientSetAction(int ActionBarSlot);
		void ChangeTarget(int Direction, int SideDirection);

		void GetFighterList(int Side, std::list<_Object *> &SideFighters);
		void GetAliveFighterList(int Side, std::list<_Object *> &AliveFighters);

		void RenderBattle();
		void RenderBattleWin();
		void RenderBattleLose();

		// State
		int State;
		bool Done;
		double Timer;

		// Objects
		std::list<_Object *> Fighters;
		int SideCount[2];
		int NextID;

		// Client battle results
		int ClientExperienceReceived;
		int ClientGoldReceived;
		std::list<const _Item *> ClientItemDrops;

		// UI
		_Element *BattleElement;
		_Element *BattleWinElement;
		_Element *BattleLoseElement;

};
