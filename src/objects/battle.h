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
#include <objects/managerbase.h>
#include <objects/action.h>
#include <packet.h>
#include <list>
#include <cstdint>

// Forward Declarations
class _Element;
class _Scripting;
class _Server;
class _ClientNetwork;
class _StatusEffect;

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

	uint8_t FighterCount;
	uint8_t PlayerCount;
	uint8_t MonsterCount;
	int TotalExperienceGiven;
	int TotalGoldGiven;
	int ExperiencePerFighter;
	int GoldPerFighter;
	bool Dead;
};

// Classes
class _Battle : public _ManagerBase {

	public:

		enum StateType {
			STATE_NONE,
			STATE_WIN,
			STATE_LOSE,
		};

		_Battle();
		~_Battle();

		// Objects
		void AddFighter(_Object *Fighter, uint8_t Side);
		void RemoveFighter(_Object *RemoveFighter);
		int GetPeerCount();

		// Updates
		void Update(double FrameTime) override;
		void OnDelete() override { }
		void Render(double BlendFactor);

		// Network
		void Serialize(_Buffer &Data);
		void Unserialize(_Buffer &Data);
		void BroadcastPacket(_Buffer &Data);

		// Setup
		void ServerEndBattle();
		void ClientEndBattle(_Buffer &Data);

		// Input
		bool ClientHandleInput(int Action);
		void ClientHandlePlayerAction(_Buffer &Data);

		_Stats *Stats;
		_Server *Server;
		_Scripting *Scripting;
		_ClientNetwork *ClientNetwork;
		_Object *ClientPlayer;
		_Manager<_Object> *Manager;

		std::list<_Object *> Fighters;
		std::list<_ActionResult> ActionResults;

		bool Done;

	private:

		void GetBattleOffset(int SideIndex, _Object *Fighter);
		void ClientSetAction(uint8_t ActionBarSlot);
		void ChangeTarget(int Direction, int SideDirection);

		void GetFighterList(int Side, std::list<_Object *> &SideFighters);
		void GetAliveFighterList(int Side, std::list<_Object *> &AliveFighters);

		void RenderBattle(double BlendFactor);
		void RenderActionResults(_ActionResult &ActionResult, double BlendFactor);
		void RenderStatChanges(_StatChange &StatChange, double BlendFactor);
		void RenderBattleWin();
		void RenderBattleLose();

		// State
		int State;
		double Time;
		double WaitTimer;

		// Objects
		int SideCount[2];
		uint8_t NextID;

		// Client battle results
		int ClientExperienceReceived;
		int ClientGoldReceived;
		std::list<const _Item *> ClientItemDrops;

		// UI
		_Element *BattleElement;
		_Element *BattleWinElement;
		_Element *BattleLoseElement;

};
