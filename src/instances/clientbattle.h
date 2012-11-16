/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef CLIENTBATTLE_H
#define CLIENTBATTLE_H

// Libraries
#include "battle.h"
#include "../engine/constants.h"

// Forward Declarations
class ItemClass;

// Structures
struct ActionResultStruct {
	ActionResultStruct() : Fighter(-1), Type(0), Value(0), Timer(0), LifeTime(1500), Percent(0.0f) { }
	int Fighter;
	int Type;
	int Value;
	float Percent;
	u32 Timer, LifeTime;
};

// Classes
class ClientBattleClass : public BattleClass {

	public:

		enum StateType {
			STATE_BATTLE,
			STATE_WAIT,
			STATE_INITWIN,
			STATE_WIN,
			STATE_INITLOSE,
			STATE_LOSE,
			STATE_DELETE,
		};

		enum ElementType {
			ELEMENT_FLEE,
		};

		ClientBattleClass();
		~ClientBattleClass();

		// Setup
		void StartBattle(PlayerClass *Player);
		void RemovePlayer(PlayerClass *Player);

		// Input
		void HandleInput(EKEY_CODE Key);
		void HandleGUI(EGUI_EVENT_TYPE EventType, IGUIElement *Element);

		// Updates
		void Update(u32 FrameTime);

		// Render
		void Render();
		void GetPositionFromSlot(int Slot, position2di &Position);

		// Network updates
		void HandleBattleAction(int FighterSlot, int ActionType, int ActionID);
		void HandleBattleUpdate(PacketClass *Packet);
		void EndBattle(PacketClass *Packet);

	private:

		void RenderBattle();
		void RenderBattleWin();
		void RenderBattleLose();
		void RenderAnimations();

		void UpdateStats();
		void UpdateAnimations(int FrameTime);

		void SendAction(int Index);
		void ChangeTarget(int Direction);
	
		// Action results
		list<ActionResultStruct> ActionResults;

		// Battle results
		int TotalExperience, TotalGold;

		// Actions
		BattleUpdateStruct Results[BATTLE_MAXFIGHTERS];

		// Client's player
		PlayerClass *ClientPlayer;
		IGUIButton *FleeButton;

		// Items
		array<const ItemClass *> MonsterDrops;

};

#endif
