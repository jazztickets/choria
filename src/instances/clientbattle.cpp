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
#include "clientbattle.h"
#include "../engine/globals.h"
#include "../engine/graphics.h"
#include "../network/network.h"
#include "../engine/stats.h"
#include "../network/packetstream.h"
#include "../objects/fighter.h"
#include "../objects/player.h"

// Constructor
ClientBattleClass::ClientBattleClass()
:	BattleClass() {

}

// Destructor
ClientBattleClass::~ClientBattleClass() {

}

// Starts the battle on the client
void ClientBattleClass::StartBattle(PlayerClass *Player) {

	// Save the client's player
	ClientPlayer = Player;
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
		Fighters[i]->SetBattleAction(NULL);
	}

	State = STATE_BATTLE;
	TargetState = -1;
}

// Removes a player from battle
void ClientBattleClass::RemovePlayer(PlayerClass *Player) {
	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] == Player) {
			Fighters[i] = NULL;
			break;
		}
	}
}

// Handles client input
void ClientBattleClass::HandleInput(EKEY_CODE Key) {

	switch(State) {
		case STATE_BATTLE: {
			switch(Key) {
				case KEY_KEY_1:
				case KEY_KEY_2:
				case KEY_KEY_3:
				case KEY_KEY_4:
				case KEY_KEY_5:
				case KEY_KEY_6:
				case KEY_KEY_7:
				case KEY_KEY_8:
					SendAction(Key - KEY_KEY_1);
				break;
				case KEY_UP:
					ChangeTarget(-1);
				break;
				case KEY_DOWN:
					ChangeTarget(1);
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
void ClientBattleClass::HandleGUI(EGUI_EVENT_TYPE EventType, IGUIElement *Element) {

	int ID = Element->getID();
	switch(State) {
		case STATE_BATTLE:
			switch(EventType) {
				case EGET_BUTTON_CLICKED:
					switch(ID) {
						case ELEMENT_FLEE:
							//SendSkill(9);
						break;
					}
				break;
			}
		break;
	}
}

// Handles a battle action from another player
void ClientBattleClass::HandleBattleAction(int FighterSlot, int ActionType, int ActionID) {

	Fighters[FighterSlot]->SetBattleAction(Stats.GetSkill(ActionID));
}

// Update the battle system for the client
void ClientBattleClass::Update(u32 FrameTime) {

	Timer += FrameTime;
	switch(State) {
		case STATE_BATTLE:
			for(u32 i = 0; i < Fighters.size(); i++) {
				FighterClass *Fighter = Fighters[i];
				if(Fighter) {
					Fighter->UpdateTurnTimer(FrameTime);
				}
			}

			UpdateAnimations((int)FrameTime);
		break;
		case STATE_WAIT:
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

// Update all the battle animations
void ClientBattleClass::UpdateAnimations(int FrameTime) {

	// Update animations
	for(list<ActionResultStruct>::Iterator Iterator = ActionResults.begin(); Iterator != ActionResults.end(); ) {
		ActionResultStruct &ActionResult = *Iterator;

		// Update time
		ActionResult.Timer += FrameTime;
		if(ActionResult.LifeTime > 0)
			ActionResult.Percent = (float)ActionResult.Timer / ActionResult.LifeTime;

		// Delete animation
		if(ActionResult.Timer >= ActionResult.LifeTime) {
			Iterator = ActionResults.erase(Iterator);
		}
		else {
			++Iterator;
		}
	}
}

// Draw animations
void ClientBattleClass::RenderAnimations() {

/*
	// Draw the skill used
	//if(SkillUsing) {
	//	Graphics.DrawCenteredImage(SkillUsing->GetImage(), Offset.X + 180, Offset.Y + 50, SColor(255, 255, 255, 255));
	//}
*/

	char String[256];
	for(list<ActionResultStruct>::Iterator Iterator = ActionResults.begin(); Iterator != ActionResults.end(); ++Iterator) {
		ActionResultStruct &ActionResult = *Iterator;

		// Draw
		FighterClass *Fighter = Fighters[ActionResult.Fighter];

		// Get offset for drawing
		int OffsetX, OffsetY;
		Fighter->GetOffset(OffsetX, OffsetY);

		// Add animation offset
		OffsetY -= (int)(ActionResult.Percent * 20);
		u32 AlphaPercent;
		if(ActionResult.Percent < 0.25f)
			AlphaPercent = 255;
		else
			AlphaPercent = (u32)(255 * (1.0f - ActionResult.Percent) / 0.75f);

		sprintf(String, "%d", ActionResult.Value);
		Graphics.SetFont(GraphicsClass::FONT_14);
		Graphics.RenderText(String, OffsetX + 110, OffsetY + 10, GraphicsClass::ALIGN_CENTER, SColor(AlphaPercent, 255, 255, 255));
		Graphics.SetFont(GraphicsClass::FONT_10);
	}
}

// Render the battle system
void ClientBattleClass::Render() {

	// Draw layout
	Graphics.DrawBackground(GraphicsClass::IMAGE_BLACK, 130, 120, 540, 360, SColor(220, 255, 255, 255));

	switch(State) {
		case STATE_BATTLE:
		case STATE_WAIT:
			RenderBattle();
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
void ClientBattleClass::RenderBattle() {

	// Draw fighters
	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i])
			Fighters[i]->RenderBattle(ClientPlayer->GetTarget() == Fighters[i]->GetSlot());
	}

	RenderAnimations();
}

// Renders the battle win screen
void ClientBattleClass::RenderBattleWin() {

	char String[512];

	// Draw title
	Graphics.SetFont(GraphicsClass::FONT_18);
	Graphics.RenderText("You have won", 400, 130, GraphicsClass::ALIGN_CENTER);

	// Draw experience
	int IconX = 180, IconY = 200, IconSpacing = 110, TextOffsetX = IconX + 50, TextOffsetY;
	Graphics.DrawImage(GraphicsClass::IMAGE_BATTLEEXPERIENCE, IconX, IconY);

	TextOffsetY = IconY - 23;
	Graphics.SetFont(GraphicsClass::FONT_14);
	sprintf(String, "%d experience", TotalExperience);
	Graphics.RenderText(String, TextOffsetX, TextOffsetY);

	Graphics.SetFont(GraphicsClass::FONT_10);
	sprintf(String, "You need %d more experience for your next level", ClientPlayer->GetExperienceNeeded());
	Graphics.RenderText(String, TextOffsetX, TextOffsetY + 22);

	// Draw gold
	IconY += IconSpacing;
	TextOffsetY = IconY - 20;
	Graphics.DrawImage(GraphicsClass::IMAGE_BATTLECOINS, IconX, IconY);
	
	sprintf(String, "%d gold", TotalGold);
	Graphics.SetFont(GraphicsClass::FONT_14);
	Graphics.RenderText(String, TextOffsetX, TextOffsetY);

	sprintf(String, "You have %d gold", ClientPlayer->GetGold());
	Graphics.SetFont(GraphicsClass::FONT_10);
	Graphics.RenderText(String, TextOffsetX, TextOffsetY + 22);

	// Draw items
	IconY += IconSpacing;
	TextOffsetY = IconY - 10;
	Graphics.DrawImage(GraphicsClass::IMAGE_BATTLECHEST, IconX, IconY);

	if(MonsterDrops.size() == 0) {
		Graphics.RenderText("No items found", TextOffsetX, TextOffsetY);
	}
	else {
		int DrawX = TextOffsetX;
		int DrawY = TextOffsetY - 8;

		// Draw items found
		int ColumnIndex = 0;
		for(u32 i = 0; i < MonsterDrops.size(); i++) {
			Graphics.DrawCenteredImage(MonsterDrops[i]->GetImage(), DrawX + 16, DrawY + 16);

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
	RenderBattle();
	char Buffer[256];

	Graphics.SetFont(GraphicsClass::FONT_14);
	Graphics.RenderText("You died", 400, 130, GraphicsClass::ALIGN_CENTER);

	sprintf(Buffer, "You lose %d gold", abs(TotalGold));
	Graphics.SetFont(GraphicsClass::FONT_10);
	Graphics.RenderText(Buffer, 400, 155, GraphicsClass::ALIGN_CENTER, SColor(255, 200, 200, 200));
}

// Handles a fighter update from the server
void ClientBattleClass::HandleBattleUpdate(PacketClass *Packet) {

	// Get update
	BattleUpdateStruct Update;
	Update.Fighter = Packet->ReadChar();
	Update.Giver = Packet->ReadChar();
	Update.DamageDealt = Packet->ReadInt();
	Update.DamageTaken = Packet->ReadInt();
	Update.TurnTimer = Packet->ReadInt();
	Update.TurnTimerMax = Packet->ReadInt();
	int Health = Packet->ReadInt();
	int Mana = Packet->ReadInt();

	// Update the fighter
	FighterClass *Fighter = Fighters[Update.Fighter];
	Fighter->SetHealth(Health);
	Fighter->SetMana(Mana);
	Fighter->SetTurnTimer(Update.TurnTimer);
	Fighter->SetTurnTimerMax(Update.TurnTimerMax);

	// Change targets if the old one died
	int TargetIndex = GetFighterFromSlot(ClientPlayer->GetTarget());
	if(TargetIndex != -1 && Fighters[TargetIndex]->GetHealth() == 0)
		ChangeTarget(1);

	// Reset battle action
	if(Update.Giver)
		Fighter->SetBattleAction(NULL);

	// Add animation
	if(!Update.Giver) {
		ActionResultStruct ActionResult;
		ActionResult.Fighter = Update.Fighter;
		ActionResult.Value = Update.DamageTaken;
		ActionResult.LifeTime = 1500;
		ActionResults.push_back(ActionResult);
	}
}

// End of a battle
void ClientBattleClass::EndBattle(PacketClass *Packet) {

	// Get ending stats
	bool SideDead[2];
	SideDead[0] = Packet->ReadBit();
	SideDead[1] = Packet->ReadBit();
	int PlayerKills = Packet->ReadChar();
	int MonsterKills = Packet->ReadChar();
	TotalExperience = Packet->ReadInt();
	TotalGold = Packet->ReadInt();
	int ItemCount = Packet->ReadChar();
	for(int i = 0; i < ItemCount; i++) {
		int ItemID = Packet->ReadInt();
		const ItemClass *Item = Stats.GetItem(ItemID); 
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
	State = TargetState;
	TargetState = -1;

	//irrGUI->getRootGUIElement()->removeChild(FleeButton);
}

// Calculates a screen position for a slot
void ClientBattleClass::GetPositionFromSlot(int Slot, position2di &Position) {

	// Get side
	int Side = Slot & 1;

	// Check sides
	int SideCount;
	if(Side == 0) {
		Position.X = 170;
		SideCount = LeftFighterCount;
	}
	else {
		Position.X = 460;
		SideCount = RightFighterCount;
	}

	// Get an index into the side
	int SideIndex = Slot / 2;

	// Get layout based off side count
	switch(SideCount) {
		case 1:
			Position.Y = 247;
		break;
		case 2:
			Position.Y = 240 + (SideIndex * 2 - 1) * 70;
		break;
		default:
			Position.Y = 140 + 300 / SideCount * SideIndex;
		break;
	}
}

// Sends an action to the server
void ClientBattleClass::SendAction(int Index) {
	const ActionClass *Action = ClientPlayer->GetActionBar(Index);
	if(Action == NULL)
		return;

	// Check health
	if(ClientPlayer->GetHealth() == 0)
		return;

	// Set action
	ClientPlayer->SetBattleAction(Action);

	// Send packet
	PacketClass Packet(NetworkClass::BATTLE_ACTION);
	Packet.WriteChar(Index);
	Packet.WriteChar(ClientPlayer->GetTarget());
	ClientNetwork->SendPacketToHost(&Packet);
}

// Changes targets
void ClientBattleClass::ChangeTarget(int Direction) {
	if(ClientPlayer->GetHealth() == 0)
		return;

	// Get a list of fighters on the opposite side
	array<FighterClass *> SideFighters;
	GetFighterList(!ClientPlayer->GetSide(), SideFighters);

	// Find next available target
	int StartIndex, Index;
	StartIndex = Index = ClientPlayer->GetTarget() / 2;
	do {
		Index += Direction;
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
