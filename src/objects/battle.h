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
#include <ae/baseobject.h>
#include <objects/action.h>
#include <list>
#include <vector>
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
		AlivePlayerCount(0),
		AliveMonsterCount(0),
		MonsterCount(0),
		JoinedCount(0),
		TotalExperienceGiven(0.0f),
		TotalGoldGiven(0.0f),
		TotalBounty(0),
		TotalGoldStolen(0),
		ExperiencePerCharacter(0),
		GoldPerCharacter(0),
		GoldStolenPerCharacter(0),
		Dead(true) { }

	int AliveCount;
	int PlayerCount;
	int AlivePlayerCount;
	int AliveMonsterCount;
	int MonsterCount;
	int JoinedCount;
	float TotalExperienceGiven;
	float TotalGoldGiven;
	int64_t TotalBounty;
	int64_t TotalGoldStolen;
	int64_t ExperiencePerCharacter;
	int64_t GoldPerCharacter;
	int64_t GoldStolenPerCharacter;
	bool Dead;
};

// Classes
class _Battle : public ae::_BaseObject {

	public:

		_Battle();
		~_Battle() override;

		// Objects
		void AddObject(_Object *Object, uint8_t Side, bool Join=false);
		void RemoveObject(_Object *RemoveObject);
		void GetSeparateObjectList(uint8_t Side, std::vector<_Object *> &Allies, std::vector<_Object *> &Enemies);
		void GetObjectList(int Side, std::vector<_Object *> &SideObjects);
		int GetPeerCount();

		// Updates
		void Update(double FrameTime) override;
		void Render(double BlendFactor);

		// Network
		void Serialize(ae::_Buffer &Data);
		void Unserialize(ae::_Buffer &Data, _HUD *HUD);
		void BroadcastPacket(ae::_Buffer &Data);
		void BroadcastStatusEffects(_Object *UpdatedObject);

		// Setup
		void ServerEndBattle();

		// Input
		bool ClientHandleInput(std::size_t Action, bool MouseCombat=false);
		void ClientHandlePlayerAction(ae::_Buffer &Data);
		void ClientSetAction(uint8_t ActionBarSlot);
		void ClientSetTarget(const _Item *Item, int Side, _Object *InitialTarget);
		void ClientChangeTarget(int Direction, bool SideDirection);

		// Pointers
		const _Stats *Stats;
		_Server *Server;
		_Scripting *Scripting;
		ae::_ClientNetwork *ClientNetwork;
		_Object *ClientPlayer;
		ae::_Manager<_Object> *Manager;

		// Objects
		std::vector<_Object *> Objects;
		std::list<_ActionResult> ActionResults;

		// Attributes
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

		void RenderActionResults(_ActionResult &ActionResult, double BlendFactor);

		// State
		double Time;
		double WaitTimer;

		// UI
		ae::_Element *BattleElement;

};
