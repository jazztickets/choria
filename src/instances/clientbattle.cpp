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
#include <instances/clientbattle.h>
#include <globals.h>
#include <graphics.h>
#include <stats.h>
#include <buffer.h>
#include <actions.h>
#include <program.h>
#include <assets.h>
#include <font.h>
#include <ui/element.h>
#include <ui/label.h>
#include <ui/image.h>
#include <objects/fighter.h>
#include <objects/player.h>
#include <network/network.h>

// Constructor
_ClientBattle::_ClientBattle()
:	_Battle() {

}

// Destructor
_ClientBattle::~_ClientBattle() {

}

// Starts the battle on the client
void _ClientBattle::StartBattle(_Player *Player) {

	// Save the client's player
	ClientPlayer = Player;
	if(ClientPlayer->GetSide() == 0)
		ClientPlayer->Target = 1;
	else
		ClientPlayer->Target = 0;
	ClientPlayer->StartBattle(this);

	// Set fighter position offsets
	glm::ivec2 Offset;
	for(size_t i = 0; i < Fighters.size(); i++) {
		GetPositionFromSlot(Fighters[i]->BattleSlot, Offset);
		Fighters[i]->SetOffset(Offset);
		Fighters[i]->SetSkillUsed(nullptr);
		Fighters[i]->SetSkillUsing(nullptr);
	}

	State = STATE_GETINPUT;
	TargetState = -1;
	ShowResults = false;
}

// Removes a player from battle
void _ClientBattle::RemovePlayer(_Player *Player) {
	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] == Player) {
			Fighters[i] = nullptr;
			break;
		}
	}
}

// Handles a command from an other player
void _ClientBattle::HandleCommand(int Slot, int SkillID) {
	int Index = GetFighterFromSlot(Slot);
	if(Index != -1) {
		Fighters[Index]->SetSkillUsing(Stats.GetSkill(SkillID));
	}
}

// Handle player input
void _ClientBattle::HandleAction(int Action) {

	switch(State) {
		case STATE_GETINPUT: {
			switch(Action) {
				case _Actions::SKILL1:
				case _Actions::SKILL2:
				case _Actions::SKILL3:
				case _Actions::SKILL4:
				case _Actions::SKILL5:
				case _Actions::SKILL6:
				case _Actions::SKILL7:
				case _Actions::SKILL8:
					SendSkill(Action - _Actions::SKILL1);
				break;
				case _Actions::UP:
					ChangeTarget(-1);
				break;
				case _Actions::DOWN:
					ChangeTarget(1);
				break;
			}
		} break;
		case STATE_WIN:
		case STATE_LOSE: {
			if(Timer > BATTLE_WAITENDTIME) {
				State = STATE_DELETE;

				_Buffer Packet;
				Packet.Write<char>(_Network::BATTLE_CLIENTDONE);
				ClientNetwork->SendPacketToHost(&Packet);
			}
		}
		break;
		default:
		break;
	}
}

// Update the battle system for the client
void _ClientBattle::Update(double FrameTime) {

	ResultTimer += FrameTime;
	Timer += FrameTime;
	switch(State) {
		case STATE_GETINPUT:
		break;
		case STATE_WAIT:
		break;
		case STATE_TURNRESULTS:
			if(Timer > BATTLE_WAITRESULTTIME) {
				State = TargetState;
				TargetState = -1;
			}
		break;
		case STATE_INITWIN:
			UpdateStats();
			Timer = 0;
			State = STATE_WIN;
		break;
		case STATE_WIN:
		break;
		case STATE_INITLOSE:
			Timer = 0;
			State = STATE_LOSE;
		break;
		case STATE_LOSE:
		break;
		default:
		break;
	}
}

// Render the battle system
void _ClientBattle::Render() {
	if(ShowResults && ResultTimer >= BATTLE_SHOWRESULTTIME) {
		ShowResults = false;
		for(size_t i = 0; i < Fighters.size(); i++) {
			if(Fighters[i])
				Fighters[i]->SetSkillUsed(nullptr);
		}
	}

	switch(State) {
		case STATE_GETINPUT:
		case STATE_WAIT:
		case STATE_TURNRESULTS:
			RenderBattle(ShowResults);
		break;
		case STATE_INITWIN:
		case STATE_WIN:
			RenderBattleWin();
		break;
		case STATE_INITLOSE:
		case STATE_LOSE:
			RenderBattleLose();
		break;
	}

}

// Renders the battle part
void _ClientBattle::RenderBattle(bool ShowResults) {
	Assets.Elements["element_battle"]->Render();

	// Get a percent of the results timer
	float TimerPercent = 0;
	if(ResultTimer <= BATTLE_SHOWRESULTTIME)
		TimerPercent = 1.0f - (float)ResultTimer / BATTLE_SHOWRESULTTIME;

	// Draw fighters
	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i])
			Fighters[i]->RenderBattle(ShowResults, TimerPercent, &Results[i], ClientPlayer->Target == Fighters[i]->BattleSlot);
	}
}

// Renders the battle win screen
void _ClientBattle::RenderBattleWin() {
	_Element *BattleWinElement = Assets.Elements["element_battlewin"];
	Assets.Labels["label_battlewin_experience"]->Text = std::to_string(TotalExperience) + " experience";
	Assets.Labels["label_battlewin_coins"]->Text = std::to_string(TotalGold) + " gold";
	Assets.Labels["label_battlewin_chest"]->Visible = MonsterDrops.size() == 0;
	BattleWinElement->Render();

	// Draw item drops
	if(MonsterDrops.size() > 0) {

		// Get positions
		_Image *Image = Assets.Images["image_battlewin_chest"];
		glm::ivec2 StartPosition = (Image->Bounds.Start + Image->Bounds.End) / 2;
		StartPosition.x += Image->Size.x;
		glm::ivec2 DrawPosition(StartPosition);

		// Draw items found
		for(size_t i = 0; i < MonsterDrops.size(); i++) {
			const _Texture *Texture = MonsterDrops[i]->GetImage();
			Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
			Graphics.DrawCenteredImage(DrawPosition, Texture);

			// Move draw position down
			DrawPosition.x += Texture->Size.x + 10;
			if(DrawPosition.x > BattleWinElement->Bounds.End.x) {
				DrawPosition.x = StartPosition.x;
				DrawPosition.y += Texture->Size.y + 10;;
			}
		}
	}
}

// Renders the battle lost screen
void _ClientBattle::RenderBattleLose() {
	Assets.Labels["label_battlelose_gold"]->Text = "You lost " + std::to_string(abs(TotalGold)) + " gold";
	Assets.Elements["element_battlelose"]->Render();
}

// Displays turn results from the server
void _ClientBattle::ResolveTurn(_Buffer *Packet) {

	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i]) {
			Results[i].SkillID = Packet->Read<char>();
			Results[i].Target = Packet->Read<int32_t>();
			Results[i].DamageDealt = Packet->Read<int32_t>();
			Results[i].HealthChange = Packet->Read<int32_t>();
			Fighters[i]->Health = Packet->Read<int32_t>();
			Fighters[i]->Mana = Packet->Read<int32_t>();
			Fighters[i]->SetSkillUsed(Fighters[i]->GetSkillUsing());
			Fighters[i]->SetSkillUsing(nullptr);
		}
	}

	// Change targets if the old one died
	int TargetIndex = GetFighterFromSlot(ClientPlayer->Target);
	if(TargetIndex != -1 && Fighters[TargetIndex]->Health == 0)
		ChangeTarget(1);

	Timer = 0;
	ResultTimer = 0;
	ShowResults = true;
	State = STATE_TURNRESULTS;
	TargetState = STATE_GETINPUT;
}

// End of a battle
void _ClientBattle::EndBattle(_Buffer *Packet) {

	// Get ending stats
	bool SideDead[2];
	SideDead[0] = Packet->ReadBit();
	SideDead[1] = Packet->ReadBit();
	int PlayerKills = Packet->Read<char>();
	int MonsterKills = Packet->Read<char>();
	TotalExperience = Packet->Read<int32_t>();
	TotalGold = Packet->Read<int32_t>();
	int ItemCount = Packet->Read<char>();
	for(int i = 0; i < ItemCount; i++) {
		int ItemID = Packet->Read<int32_t>();
		const _Item *Item = Stats.GetItem(ItemID);
		MonsterDrops.push_back(Item);
		ClientPlayer->AddItem(Item, 1, -1);
	}

	// Check win or death
	int PlayerSide = ClientPlayer->GetSide();
	int OtherSide = !PlayerSide;
	if(!SideDead[PlayerSide] && SideDead[OtherSide]) {
		ClientPlayer->UpdatePlayerKills(PlayerKills);
		ClientPlayer->UpdateMonsterKills(MonsterKills);
		TargetState = STATE_INITWIN;
	}
	else {
		ClientPlayer->UpdateDeaths(1);
		TargetState = STATE_INITLOSE;
	}

	// Go to the ending state immediately
	if(State != STATE_TURNRESULTS) {
		State = TargetState;
		TargetState = -1;
	}
}

// Calculates a screen position for a slot
void _ClientBattle::GetPositionFromSlot(int Slot, glm::ivec2 &Position) {
	_Element *BattleElement = Assets.Elements["element_battle"];
	glm::ivec2 Center = (BattleElement->Bounds.End + BattleElement->Bounds.Start) / 2;

	// Get side
	int Side = Slot & 1;

	// Check sides
	int SideCount;
	if(Side == 0) {
		Position.x = Center.x - 180;
		SideCount = LeftFighterCount;
	}
	else {
		Position.x = Center.x + 100;
		SideCount = RightFighterCount;
	}

	// Get an index into the side
	int SideIndex = Slot / 2;

	// Divide space into SideCount parts, then divide that by 2
	int SpacingY = (BattleElement->Size.y / SideCount) / 2;

	// Place slots in between main divisions
	Position.y = BattleElement->Bounds.Start.y + SpacingY * (2 * SideIndex + 1) + 10;

	// Convert position to relative offset from center
	Position = Position - Center;
}

// Sends a skill selection to the server
void _ClientBattle::SendSkill(int SkillSlot) {
	if(ClientPlayer->Health == 0)
		return;

	const _Skill *Skill = ClientPlayer->GetSkillBar(SkillSlot);
	if(SkillSlot != 9 && (Skill == nullptr || !Skill->CanUse(ClientPlayer)))
		return;

	_Buffer Packet;
	Packet.Write<char>(_Network::BATTLE_COMMAND);
	Packet.Write<char>(SkillSlot);
	Packet.Write<char>(ClientPlayer->Target);

	ClientNetwork->SendPacketToHost(&Packet);
	ClientPlayer->SetSkillUsing(Skill);

	// Update potion count
	if(SkillSlot != 9 && Skill->Type == _Skill::TYPE_USEPOTION)
		ClientPlayer->UpdatePotionsLeft(Skill->ID == 2);

	State = STATE_WAIT;
}

// Changes targets
void _ClientBattle::ChangeTarget(int Direction) {
	if(ClientPlayer->Health == 0)
		return;

	// Get a list of fighters on the opposite side
	std::vector<_Fighter *> SideFighters;
	GetFighterList(!ClientPlayer->GetSide(), SideFighters);

	// Find next available target
	int StartIndex, Index;
	StartIndex = Index = ClientPlayer->Target / 2;
	do {
		Index += Direction;
		if(Index >= (int)SideFighters.size())
			Index = 0;
		else if(Index < 0)
			Index = SideFighters.size()-1;

	} while(StartIndex != Index && SideFighters[Index]->Health == 0);

	// Set new target
	ClientPlayer->Target = SideFighters[Index]->BattleSlot;
}

// Updates player stats
void _ClientBattle::UpdateStats() {
	ClientPlayer->UpdateExperience(TotalExperience);
	ClientPlayer->UpdateGold(TotalGold);
	ClientPlayer->CalculatePlayerStats();
}
