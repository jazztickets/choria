/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#ifndef CLIENTBATTLE_H
#define CLIENTBATTLE_H

// Libraries
#include <instances/battle.h>
#include <engine/constants.h>

// Forward Declarations
class ItemClass;

// Classes
class ClientBattleClass : public BattleClass {

	public:

		enum StateType {
			STATE_GETINPUT,
			STATE_WAIT,
			STATE_TURNRESULTS,
			STATE_INITWIN,
			STATE_WIN,
			STATE_INITLOSE,
			STATE_LOSE,
			STATE_DELETE,
		};

		enum ElementType {
			ELEMENT_PASS,
			ELEMENT_SKILL1,
			ELEMENT_SKILL8=ELEMENT_SKILL1+BATTLE_MAXSKILLS-1,
		};

		ClientBattleClass();
		~ClientBattleClass();

		// Setup
		void StartBattle(PlayerClass *TClientPlayer);
		void RemovePlayer(PlayerClass *TPlayer);

		// Input
		void HandleInput(EKEY_CODE TKey);
		void HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement);
		void HandleCommand(int TSlot, int TSkillID);

		// Updates
		void Update(u32 TDeltaTime);

		// Render
		void Render();
		void GetPositionFromSlot(int TSlot, position2di &TPosition);

		// Resolve
		void ResolveTurn(PacketClass *TPacket);
		void EndBattle(PacketClass *TPacket);

	private:

		void RenderBattle(bool TShowResults);
		void RenderBattleWin();
		void RenderBattleLose();

		void UpdateStats();

		void SendSkill(int TSkillSlot);
		void ChangeTarget(int TDirection);

		// Battle results
		u32 ResultTimer;
		bool ShowResults;
		int TotalExperience, TotalGold;

		// Actions
		FighterResultStruct Results[BATTLE_MAXFIGHTERS];

		// Client's player
		PlayerClass *ClientPlayer;
		IGUIButton *SkillButtons[BATTLE_MAXSKILLS], *PassButton;

		// Items
		array<const ItemClass *> MonsterDrops;

};

#endif
