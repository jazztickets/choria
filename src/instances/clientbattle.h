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
#include <instances/battle.h>
#include <constants.h>

// Forward Declarations
class _Item;
class _Buffer;
class _Element;

// Classes
class _ClientBattle : public _Battle {

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

		_ClientBattle();
		~_ClientBattle();

		// Setup
		void StartBattle(_Player *Player);
		void RemovePlayer(_Player *Player);

		// Input
		void HandleCommand(int Slot, int SkillID);
		void HandleAction(int Action);

		// Updates
		void Update(double FrameTime);

		// Render
		void Render();
		void GetPositionFromSlot(int Slot, glm::ivec2 &Position);

		// Resolve
		void ResolveTurn(_Buffer *Packet);
		void EndBattle(_Buffer *Packet);

	private:

		void RenderBattle(bool ShowResults);
		void RenderBattleWin();
		void RenderBattleLose();

		void UpdateStats();

		void SendSkill(int SkillSlot);
		void ChangeTarget(int Direction);

		// Battle results
		double ResultTimer;
		bool ShowResults;
		int TotalExperience, TotalGold;

		// UI
		_Element *BattleElement;
		_Element *BattleWinElement;
		_Element *BattleLoseElement;

		// Actions
		_FighterResult Results[BATTLE_MAXFIGHTERS];

		// Client's player
		_Player *ClientPlayer;

		// Items
		std::vector<const _Item *> MonsterDrops;

};
