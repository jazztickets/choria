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
#pragma once

// Libraries
#include <instances/battle.h>
#include <constants.h>
#include <IGUIElement.h>
#include <IGUIButton.h>
#include <Keycodes.h>

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
		void HandleInput(irr::EKEY_CODE TKey);
		void HandleGUI(irr::gui::EGUI_EVENT_TYPE TEventType, irr::gui::IGUIElement *TElement);
		void HandleCommand(int TSlot, int TSkillID);

		// Updates
		void Update(uint32_t TDeltaTime);

		// Render
		void Render();
		void GetPositionFromSlot(int TSlot, irr::core::position2di &TPosition);

		// Resolve
		void ResolveTurn(_Packet *TPacket);
		void EndBattle(_Packet *TPacket);

	private:

		void RenderBattle(bool TShowResults);
		void RenderBattleWin();
		void RenderBattleLose();

		void UpdateStats();

		void SendSkill(int TSkillSlot);
		void ChangeTarget(int TDirection);

		// Battle results
		uint32_t ResultTimer;
		bool ShowResults;
		int TotalExperience, TotalGold;

		// Actions
		FighterResultStruct Results[BATTLE_MAXFIGHTERS];

		// Client's player
		PlayerClass *ClientPlayer;
		irr::gui::IGUIButton *SkillButtons[BATTLE_MAXSKILLS], *PassButton;

		// Items
		std::vector<const ItemClass *> MonsterDrops;

};
