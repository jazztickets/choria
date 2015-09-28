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
#include "clientbattle.h"
#include "../engine/globals.h"
#include "../engine/graphics.h"
#include "../network/network.h"
#include "../engine/stats.h"
#include "../network/packetstream.h"
#include "../objects/fighter.h"
#include <objects/player.h>

// Constructor
ClientBattleClass::ClientBattleClass()
:	BattleClass() {

}

// Destructor
ClientBattleClass::~ClientBattleClass() {

}

// Starts the battle on the client
void ClientBattleClass::StartBattle(PlayerClass *TClientPlayer) {

	// Save the client's player
	ClientPlayer = TClientPlayer;
	if(ClientPlayer->GetSide() == 0)
		ClientPlayer->SetTarget(1);
	else
		ClientPlayer->SetTarget(0);
	ClientPlayer->StartBattle(this);

	// Set fighter position offsets
	position2di Offset;
	for(u32 i = 0; i < Fighters.size(); i++) {
		GetPositionFromSlot(Fighters[i]->GetSlot(), Offset);
		Fighters[i]->SetOffset(Offset);
		Fighters[i]->SetSkillUsed(NULL);
		Fighters[i]->SetSkillUsing(NULL);
	}

	// Add skill buttons
	const SkillClass *Skill;
	for(int i = 0; i < BATTLE_MAXSKILLS; i++) {

		// Add button
		SkillButtons[i] = irrGUI->addButton(Graphics::Instance().GetCenteredRect(288 + i * 32, 464, 32, 32), 0, ELEMENT_SKILL1 + i, 0);
		IGUIStaticText *Text = Graphics::Instance().AddText(stringc(i+1).c_str(), 3, 1, GraphicsClass::ALIGN_LEFT, SkillButtons[i]);
		Text->setOverrideFont(Graphics::Instance().GetFont(GraphicsClass::FONT_8));

		// Get skill info
		Skill = ClientPlayer->GetSkillBar(i);
		if(Skill)
			SkillButtons[i]->setImage(Skill->GetImage());
		else
			SkillButtons[i]->setImage(Graphics::Instance().GetImage(GraphicsClass::IMAGE_EMPTYSLOT));
	}
	PassButton = irrGUI->addButton(Graphics::Instance().GetCenteredRect(570, 464, 50, 20), 0, ELEMENT_PASS, L"Pass");
	PassButton->setOverrideFont(Graphics::Instance().GetFont(GraphicsClass::FONT_8));

	State = STATE_GETINPUT;
	TargetState = -1;
	ShowResults = false;
}

// Removes a player from battle
void ClientBattleClass::RemovePlayer(PlayerClass *TPlayer) {
	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] == TPlayer) {
			Fighters[i] = NULL;
			break;
		}
	}
}

// Handles client input
void ClientBattleClass::HandleInput(EKEY_CODE TKey) {

	switch(State) {
		case STATE_GETINPUT: {
			switch(TKey) {
				case KEY_KEY_1:
				case KEY_KEY_2:
				case KEY_KEY_3:
				case KEY_KEY_4:
				case KEY_KEY_5:
				case KEY_KEY_6:
				case KEY_KEY_7:
				case KEY_KEY_8:
					SendSkill(TKey - KEY_KEY_1);
				break;
				case KEY_UP:
					ChangeTarget(-1);
				break;
				case KEY_DOWN:
					ChangeTarget(1);
				break;
				default:
				break;
			}
		}
		break;
		case STATE_WIN:
		case STATE_LOSE: {
			if(Timer > BATTLE_WAITENDTIME) {
				State = STATE_DELETE;

				PacketClass Packet(NetworkClass::BATTLE_CLIENTDONE);
				ClientNetwork->SendPacketToHost(&Packet);
			}
		}
		break;
		default:
		break;
	}
}

// Handles GUI events
void ClientBattleClass::HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement) {

	int ID = TElement->getID();
	switch(State) {
		case STATE_GETINPUT:
			switch(TEventType) {
				case EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_PASS:
							SendSkill(9);
						break;
						default:
							if(ID >= ELEMENT_SKILL1 && ID <= ELEMENT_SKILL8) {
								SendSkill(TElement->getID() - ELEMENT_SKILL1);
							}
						break;
					}
				break;
				default:
				break;
			}
		break;
		default:
		break;
	}
}

// Handles a command from an other player
void ClientBattleClass::HandleCommand(int TSlot, int TSkillID) {
	int Index = GetFighterFromSlot(TSlot);
	if(Index != -1) {
		Fighters[Index]->SetSkillUsing(Stats::Instance().GetSkill(TSkillID));
	}
}

// Update the battle system for the client
void ClientBattleClass::Update(u32 TDeltaTime) {

	ResultTimer += TDeltaTime;
	Timer += TDeltaTime;
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
void ClientBattleClass::Render() {

	// Draw layout
	Graphics::Instance().DrawBackground(GraphicsClass::IMAGE_BLACK, 130, 120, 540, 360, SColor(220, 255, 255, 255));

	if(ShowResults && ResultTimer >= BATTLE_SHOWRESULTTIME) {
		ShowResults = false;
		for(u32 i = 0; i < Fighters.size(); i++) {
			if(Fighters[i])
				Fighters[i]->SetSkillUsed(NULL);
		}
	}

	switch(State) {
		case STATE_GETINPUT:
		case STATE_WAIT:
		case STATE_TURNRESULTS:
			RenderBattle(ShowResults);
		break;
		case STATE_WIN:
			RenderBattleWin();
		break;
		case STATE_LOSE:
			RenderBattleLose();
		break;
	}

}

// Renders the battle part
void ClientBattleClass::RenderBattle(bool TShowResults) {

	// Get a percent of the results timer
	float TimerPercent = 0;
	if(ResultTimer <= BATTLE_SHOWRESULTTIME)
		TimerPercent = 1.0f - (float)ResultTimer / BATTLE_SHOWRESULTTIME;

	// Draw fighters
	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i])
			Fighters[i]->RenderBattle(TShowResults, TimerPercent, &Results[i], ClientPlayer->GetTarget() == Fighters[i]->GetSlot());
	}
}

// Renders the battle win screen
void ClientBattleClass::RenderBattleWin() {

	char String[512];

	// Draw title
	Graphics::Instance().SetFont(GraphicsClass::FONT_18);
	Graphics::Instance().RenderText("You have won", 400, 130, GraphicsClass::ALIGN_CENTER);

	// Draw experience
	int IconX = 180, IconY = 200, IconSpacing = 110, TextOffsetX = IconX + 50, TextOffsetY;
	Graphics::Instance().DrawImage(GraphicsClass::IMAGE_BATTLEEXPERIENCE, IconX, IconY);

	TextOffsetY = IconY - 23;
	Graphics::Instance().SetFont(GraphicsClass::FONT_14);
	sprintf(String, "%d experience", TotalExperience);
	Graphics::Instance().RenderText(String, TextOffsetX, TextOffsetY);

	Graphics::Instance().SetFont(GraphicsClass::FONT_10);
	sprintf(String, "You need %d more experience for your next level", ClientPlayer->GetExperienceNeeded());
	Graphics::Instance().RenderText(String, TextOffsetX, TextOffsetY + 22);

	// Draw gold
	IconY += IconSpacing;
	TextOffsetY = IconY - 20;
	Graphics::Instance().DrawImage(GraphicsClass::IMAGE_BATTLECOINS, IconX, IconY);

	sprintf(String, "%d gold", TotalGold);
	Graphics::Instance().SetFont(GraphicsClass::FONT_14);
	Graphics::Instance().RenderText(String, TextOffsetX, TextOffsetY);

	sprintf(String, "You have %d gold", ClientPlayer->GetGold());
	Graphics::Instance().SetFont(GraphicsClass::FONT_10);
	Graphics::Instance().RenderText(String, TextOffsetX, TextOffsetY + 22);

	// Draw items
	IconY += IconSpacing;
	TextOffsetY = IconY - 10;
	Graphics::Instance().DrawImage(GraphicsClass::IMAGE_BATTLECHEST, IconX, IconY);

	if(MonsterDrops.size() == 0) {
		Graphics::Instance().RenderText("No items found", TextOffsetX, TextOffsetY);
	}
	else {
		int DrawX = TextOffsetX;
		int DrawY = TextOffsetY - 8;

		// Draw items found
		int ColumnIndex = 0;
		for(u32 i = 0; i < MonsterDrops.size(); i++) {
			Graphics::Instance().DrawCenteredImage(MonsterDrops[i]->GetImage(), DrawX + 16, DrawY + 16);

			DrawX += 40;
			ColumnIndex++;
			if(ColumnIndex > 8) {
				DrawX = TextOffsetX;
				DrawY += 40;
				ColumnIndex = 0;
			}
		}
	}

}

// Renders the battle lost screen
void ClientBattleClass::RenderBattleLose() {
	RenderBattle(true);
	char Buffer[256];

	Graphics::Instance().SetFont(GraphicsClass::FONT_14);
	Graphics::Instance().RenderText("You died", 400, 130, GraphicsClass::ALIGN_CENTER);

	sprintf(Buffer, "You lose %d gold", abs(TotalGold));
	Graphics::Instance().SetFont(GraphicsClass::FONT_10);
	Graphics::Instance().RenderText(Buffer, 400, 155, GraphicsClass::ALIGN_CENTER, SColor(255, 200, 200, 200));
}

// Displays turn results from the server
void ClientBattleClass::ResolveTurn(PacketClass *TPacket) {

	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i]) {
			Results[i].SkillID = TPacket->ReadChar();
			Results[i].Target = TPacket->ReadInt();
			Results[i].DamageDealt = TPacket->ReadInt();
			Results[i].HealthChange = TPacket->ReadInt();
			int Health = TPacket->ReadInt();
			int Mana = TPacket->ReadInt();
			Fighters[i]->SetHealth(Health);
			Fighters[i]->SetMana(Mana);
			Fighters[i]->SetSkillUsed(Fighters[i]->GetSkillUsing());
			Fighters[i]->SetSkillUsing(NULL);
		}
	}

	// Change targets if the old one died
	int TargetIndex = GetFighterFromSlot(ClientPlayer->GetTarget());
	if(TargetIndex != -1 && Fighters[TargetIndex]->GetHealth() == 0)
		ChangeTarget(1);

	Timer = 0;
	ResultTimer = 0;
	ShowResults = true;
	State = STATE_TURNRESULTS;
	TargetState = STATE_GETINPUT;
}

// End of a battle
void ClientBattleClass::EndBattle(PacketClass *TPacket) {

	// Get ending stats
	bool SideDead[2];
	SideDead[0] = TPacket->ReadBit();
	SideDead[1] = TPacket->ReadBit();
	int PlayerKills = TPacket->ReadChar();
	int MonsterKills = TPacket->ReadChar();
	TotalExperience = TPacket->ReadInt();
	TotalGold = TPacket->ReadInt();
	int ItemCount = TPacket->ReadChar();
	for(int i = 0; i < ItemCount; i++) {
		int ItemID = TPacket->ReadInt();
		const ItemClass *Item = Stats::Instance().GetItem(ItemID);
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

	for(int i = 0; i < BATTLE_MAXSKILLS; i++) {
		irrGUI->getRootGUIElement()->removeChild(SkillButtons[i]);
	}
	irrGUI->getRootGUIElement()->removeChild(PassButton);
}

// Calculates a screen position for a slot
void ClientBattleClass::GetPositionFromSlot(int TSlot, position2di &TPosition) {

	// Get side
	int TSide = TSlot & 1;

	// Check sides
	int SideCount;
	if(TSide == 0) {
		TPosition.X = 170;
		SideCount = LeftFighterCount;
	}
	else {
		TPosition.X = 460;
		SideCount = RightFighterCount;
	}

	// Get an index into the side
	int SideIndex = TSlot / 2;

	// Get layout based off side count
	switch(SideCount) {
		case 1:
			TPosition.Y = 247;
		break;
		case 2:
			TPosition.Y = 240 + (SideIndex * 2 - 1) * 70;
		break;
		default:
			TPosition.Y = 140 + 300 / SideCount * SideIndex;
		break;
	}
}

// Sends a skill selection to the server
void ClientBattleClass::SendSkill(int TSkillSlot) {
	if(ClientPlayer->GetHealth() == 0)
		return;

	const SkillClass *Skill = ClientPlayer->GetSkillBar(TSkillSlot);
	if(TSkillSlot != 9 && (Skill == NULL || !Skill->CanUse(ClientPlayer)))
		return;

	PacketClass Packet(NetworkClass::BATTLE_COMMAND);
	Packet.WriteChar(TSkillSlot);
	Packet.WriteChar(ClientPlayer->GetTarget());

	ClientNetwork->SendPacketToHost(&Packet);
	ClientPlayer->SetSkillUsing(Skill);

	// Update potion count
	if(TSkillSlot != 9 && Skill->GetType() == SkillClass::TYPE_USEPOTION)
		ClientPlayer->UpdatePotionsLeft(Skill->GetID() == 2);

	State = STATE_WAIT;
}

// Changes targets
void ClientBattleClass::ChangeTarget(int TDirection) {
	if(ClientPlayer->GetHealth() == 0)
		return;

	// Get a list of fighters on the opposite side
	array<FighterClass *> SideFighters;
	GetFighterList(!ClientPlayer->GetSide(), SideFighters);

	// Find next available target
	int StartIndex, Index;
	StartIndex = Index = ClientPlayer->GetTarget() / 2;
	do {
		Index += TDirection;
		if(Index >= (int)SideFighters.size())
			Index = 0;
		else if(Index < 0)
			Index = SideFighters.size()-1;

	} while(StartIndex != Index && SideFighters[Index]->GetHealth() == 0);

	// Set new target
	ClientPlayer->SetTarget(SideFighters[Index]->GetSlot());
}

// Updates player stats
void ClientBattleClass::UpdateStats() {
	ClientPlayer->UpdateExperience(TotalExperience);
	ClientPlayer->UpdateGold(TotalGold);
	ClientPlayer->CalculatePlayerStats();
}
