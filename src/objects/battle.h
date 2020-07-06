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
#include <ae/baseobject.h>
#include <objects/action.h>
#include <list>
#include <cstdint>

// Forward Declarations
class _Scripting;
class _Server;
class _Monster;
class _StatusEffect;
class _HUD;

namespace ae {
	class _Element;
	class _ClientNetwork;
}

struct _BattleResult {
	_BattleResult() :
		AliveCount(0),
		PlayerCount(0),
		MonsterCount(0),
		JoinedCount(0),
		TotalExperienceGiven(0),
		TotalGoldGiven(0),
		ExperiencePerCharacter(0),
		GoldPerCharacter(0),
		Dead(true) { }

	int AliveCount;
	int PlayerCount;
	int MonsterCount;
	int JoinedCount;
	int TotalExperienceGiven;
	int TotalGoldGiven;
	int ExperiencePerCharacter;
	int GoldPerCharacter;
	bool Dead;
};

struct _SurvivedSummon {
	_SurvivedSummon() : Monster(nullptr), Count(0) { }

	const _Monster *Monster;
	int Count;
};

// Classes
class _Battle : public ae::_BaseObject {

	public:

		_Battle();
		~_Battle();

		// Objects
		void AddObject(_Object *Object, uint8_t Side, bool Join=false);
		void RemoveObject(_Object *RemoveObject);
		void GetSeparateObjectList(uint8_t Side, std::list<_Object *> &Allies, std::list<_Object *> &Enemies);
		int GetPeerCount();

		// Updates
		void Update(double FrameTime) override;
		void Render(double BlendFactor);

		// Network
		void Serialize(ae::_Buffer &Data);
		void Unserialize(ae::_Buffer &Data, _HUD *HUD);
		void BroadcastPacket(ae::_Buffer &Data);

		// Setup
		void ServerEndBattle();

		// Input
		bool ClientHandleInput(size_t Action, bool MouseCombat=false);
		void ClientHandlePlayerAction(ae::_Buffer &Data);
		void ClientSetAction(uint8_t ActionBarSlot);
		void ClientSetTarget(const _Item *Item, int Side, _Object *InitialTarget);

		// Pointers
		const _Stats *Stats;
		_Server *Server;
		_Scripting *Scripting;
		ae::_ClientNetwork *ClientNetwork;
		_Object *ClientPlayer;
		ae::_Manager<_Object> *Manager;

		// Objects
		std::list<_Object *> Objects;
		std::list<_ActionResult> ActionResults;

		// Attributes
		double Difficulty[2];
		double Cooldown;
		uint32_t Zone;
		int SideCount[2];
		bool PVP;
		float BountyEarned;
		float BountyClaimed;
		bool Boss;

	private:

		void GetBattleOffset(int SideIndex, _Object *Object);
		void AdjustBattleElements(int SideIndex, _Object *Object);
		void CreateBattleElements(int SideIndex, _Object *Object);

		void ChangeTarget(int Direction, bool SideDirection);

		void GetObjectList(int Side, std::list<_Object *> &SideObjects);
		void GetAliveObjectList(int Side, std::list<_Object *> &AliveObjects);

		void RenderActionResults(_ActionResult &ActionResult, double BlendFactor);

		// State
		double Time;
		double WaitTimer;

		// UI
		ae::_Element *BattleElement;

};
