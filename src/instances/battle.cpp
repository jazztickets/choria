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
#include <instances/battle.h>
#include <objects/object.h>
#include <objects/skill.h>
#include <ui/element.h>
#include <ui/label.h>
#include <ui/image.h>
#include <network/servernetwork.h>
#include <actions.h>
#include <buffer.h>
#include <graphics.h>
#include <stats.h>
#include <program.h>
#include <assets.h>
#include <font.h>
#include <packet.h>
#include <random.h>

// Constructor
_Battle::_Battle() :
	Stats(nullptr),
	ServerNetwork(nullptr),
	State(STATE_NONE),
	TargetState(STATE_NONE),
	Timer(0),
	RoundTime(0),
	LeftFighterCount(0),
	RightFighterCount(0),
	PlayerCount(0),
	MonsterCount(0),
	ResultTimer(0),
	ShowResults(false),
	TotalExperience(0),
	TotalGold(0),
	BattleElement(nullptr),
	BattleWinElement(nullptr),
	BattleLoseElement(nullptr),
	ClientPlayer(nullptr) {

}

// Destructor
_Battle::~_Battle() {
	BattleElement->SetVisible(false);
	BattleWinElement->SetVisible(false);
	BattleLoseElement->SetVisible(false);

	// Delete monsters
	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->Type == _Object::MONSTER)
			delete Fighters[i];
	}
}

// Handle player input
void _Battle::HandleAction(int Action) {

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
				Packet.Write<PacketType>(PacketType::BATTLE_CLIENTDONE);
				//ClientState.Network->SendPacket(Packet);
			}
		}
		break;
		default:
		break;
	}
}

// Update battle
void _Battle::Update(double FrameTime) {
	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i]) {
			Fighters[i]->TurnTimer += FrameTime;
			if(Fighters[i]->TurnTimer > 1.0)
				Fighters[i]->TurnTimer = 1.0;
		}
	}

	if(ServerNetwork) {
		UpdateServer(FrameTime);
	}
	else {
		UpdateClient(FrameTime);
	}

}

// Render the battle system
void _Battle::Render(double BlendFactor) {
	if(ShowResults && ResultTimer >= BATTLE_SHOWRESULTTIME) {
		ShowResults = false;
		for(size_t i = 0; i < Fighters.size(); i++) {
			if(Fighters[i])
				Fighters[i]->SkillUsed = nullptr;
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
void _Battle::RenderBattle(bool ShowResults) {
	BattleElement->Render();

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
void _Battle::RenderBattleWin() {
	BattleWinElement->SetVisible(true);
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
			const _Texture *Texture = MonsterDrops[i]->Image;
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
void _Battle::RenderBattleLose() {
	Assets.Labels["label_battlelose_gold"]->Text = "You lost " + std::to_string(std::abs(TotalGold)) + " gold";

	BattleLoseElement->SetVisible(true);
	BattleLoseElement->Render();
}

// Sends a skill selection to the server
void _Battle::SendSkill(int SkillSlot) {
	if(ClientPlayer->Health == 0)
		return;

	const _Skill *Skill = ClientPlayer->GetActionBar(SkillSlot);
	if(Skill == nullptr || !Skill->CanUse(ClientPlayer))
		return;

	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::BATTLE_COMMAND);
	Packet.Write<char>(SkillSlot);
	Packet.Write<char>(ClientPlayer->Target);

	//ClientState.Network->SendPacket(Packet);
	ClientPlayer->SkillUsing = Skill;

	// Update potion count
	if(Skill->Type == _Skill::TYPE_USEPOTION)
		ClientPlayer->UpdatePotionsLeft(Skill->ID == 3);

	State = STATE_WAIT;
}

// Changes targets
void _Battle::ChangeTarget(int Direction) {
	if(ClientPlayer->Health == 0)
		return;

	// Get a list of fighters on the opposite side
	std::vector<_Object *> SideFighters;
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

// Add a fighter to the battle
void _Battle::AddFighter(_Object *Fighter, int Side) {

	// Count fighters and set slots
	if(Side == 0) {
		Fighter->BattleSlot = Side + LeftFighterCount * 2;
		LeftFighterCount++;
	}
	else {
		Fighter->BattleSlot = Side + RightFighterCount * 2;
		RightFighterCount++;
	}

	if(Fighter->Type == _Object::PLAYER)
		PlayerCount++;
	else
		MonsterCount++;

	Fighters.push_back(Fighter);
}

// Removes a player from battle
int _Battle::RemoveFighterClient(_Object *Fighter) {
	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] == Fighter) {
			Fighters[i] = nullptr;
			break;
		}
	}

	return 0;
}

// Get a list of fighters from a side
void _Battle::GetFighterList(int Side, std::vector<_Object *> &SideFighters) {

	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == Side) {
			SideFighters.push_back(Fighters[i]);
		}
	}
}

// Get a list of alive fighters from a side
void _Battle::GetAliveFighterList(int Side, std::vector<_Object *> &AliveFighters) {

	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == Side && Fighters[i]->Health > 0) {
			AliveFighters.push_back(Fighters[i]);
		}
	}
}

// Get a list of monster from the right side
void _Battle::GetMonsterList(std::vector<_Object *> &Monsters) {

	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == 1 && Fighters[i]->Type == _Object::MONSTER) {
			Monsters.push_back(Fighters[i]);
		}
	}
}

// Get a list of players from a side
void _Battle::GetPlayerList(int Side, std::vector<_Object *> &Players) {

	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetSide() == Side && Fighters[i]->Type == _Object::PLAYER) {
			Players.push_back(Fighters[i]);
		}
	}
}

// Gets a fighter index from a slot number
int _Battle::GetFighterFromSlot(int Slot) {
	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->BattleSlot == Slot) {
			return i;
		}
	}

	return -1;
}

// Starts the battle on the client
void _Battle::StartBattleClient(_Object *Player) {
	BattleElement = Assets.Elements["element_battle"];
	BattleWinElement = Assets.Elements["element_battlewin"];
	BattleLoseElement = Assets.Elements["element_battlelose"];
	BattleElement->SetVisible(true);

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
		Fighters[i]->BattleOffset = Offset;
		Fighters[i]->SkillUsed = nullptr;
		Fighters[i]->SkillUsing = nullptr;
	}

	State = STATE_GETINPUT;
	TargetState = -1;
	ShowResults = false;
}

// Handles a command from an other player
void _Battle::HandleCommand(int Slot, uint32_t SkillID) {
	int Index = GetFighterFromSlot(Slot);
	if(Index != -1) {
		Fighters[Index]->SkillUsing = Stats->Skills[SkillID];
	}
}

// Update the battle system for the client
void _Battle::UpdateClient(double FrameTime) {
	/*
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
	}*/
}

// Displays turn results from the server
void _Battle::ResolveTurn(_Buffer *Packet) {

	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i]) {
			Results[i].SkillID = Packet->Read<char>();
			Results[i].Target = Packet->Read<int32_t>();
			Results[i].DamageDealt = Packet->Read<int32_t>();
			Results[i].HealthChange = Packet->Read<int32_t>();
			Fighters[i]->Health = Packet->Read<int32_t>();
			Fighters[i]->Mana = Packet->Read<int32_t>();
			Fighters[i]->SkillUsed = Fighters[i]->SkillUsing;
			Fighters[i]->SkillUsing = nullptr;
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
void _Battle::EndBattle(_Buffer *Packet) {

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
		const _Item *Item = Stats->GetItem(ItemID);
		MonsterDrops.push_back(Item);
		ClientPlayer->AddItem(Item, 1, -1);
	}

	// Check win or death
	int PlayerSide = ClientPlayer->GetSide();
	int OtherSide = !PlayerSide;
	if(!SideDead[PlayerSide] && SideDead[OtherSide]) {
		ClientPlayer->PlayerKills += PlayerKills;
		ClientPlayer->MonsterKills += MonsterKills;
		TargetState = STATE_INITWIN;
	}
	else {
		ClientPlayer->Deaths++;
		TargetState = STATE_INITLOSE;
	}

	// Go to the ending state immediately
	if(State != STATE_TURNRESULTS) {
		State = TargetState;
		TargetState = -1;
	}
}

// Calculates a screen position for a slot
void _Battle::GetPositionFromSlot(int Slot, glm::ivec2 &Position) {
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

// Updates player stats
void _Battle::UpdateStats() {
	ClientPlayer->UpdateExperience(TotalExperience);
	ClientPlayer->UpdateGold(TotalGold);
	ClientPlayer->CalculatePlayerStats();
}



// Removes a player from the battle, return remaining player count
int _Battle::RemoveFighterServer(_Object *RemoveFighter) {

	int Count = 0;
	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->Type == _Object::PLAYER) {
			_Object *Player = Fighters[i];
			if(Player == RemoveFighter) {
				Player->StopBattle();
				Fighters[i] = nullptr;
			}

			if(Fighters[i])
				Count++;
		}
	}
	PlayerCount = Count;

	CheckEnd();

	return Count;
}

// Starts the battle and notifies the players
void _Battle::StartBattleServer() {

	// Build packet
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::BATTLE_START);

	// Write fighter count
	int FighterCount = Fighters.size();
	Packet.Write<char>(FighterCount);

	// Write fighter information
	for(int i = 0; i < FighterCount; i++) {

		// Write fighter type
		Packet.Write<char>(Fighters[i]->Type);
		Packet.Write<char>(Fighters[i]->GetSide());

		if(Fighters[i]->Type == _Object::PLAYER) {
			_Object *Player = Fighters[i];

			// Network ID
			Packet.Write<NetworkIDType>(Player->NetworkID);
			Packet.Write<glm::ivec2>(Player->Position);

			// Player stats
			Packet.Write<int32_t>(Player->Health);
			Packet.Write<int32_t>(Player->MaxHealth);
			Packet.Write<int32_t>(Player->Mana);
			Packet.Write<int32_t>(Player->MaxMana);

			// Start the battle for the player
			Player->StartBattle(this);
		}
		else {
			_Object *Monster = Fighters[i];

			// Monster ID
			Packet.Write<int32_t>(Monster->DatabaseID);
		}
	}

	// Send packet to players
	BroadcastPacket(Packet);

	State = STATE_INPUT;
}

// Handles input from the client
void _Battle::HandleInput(_Object *Player, int Command, int Target) {

	if(State == STATE_INPUT) {

		// Check for needed commands
		if(Player->GetCommand() == -1) {
			Player->Command = Command;
			Player->Target = Target;

			// Notify other players
			SendSkillToPlayers(Player);

			// Check for all commands
			bool Ready = true;
			for(size_t i = 0; i < Fighters.size(); i++) {
				if(Fighters[i] && Fighters[i]->Health > 0 && Fighters[i]->GetCommand() == -1) {
					Ready = false;
					break;
				}
			}

			// All players are ready
			if(Ready)
				State = STATE_RESOLVETURN;
		}
	}
}

// Update the battle system for the server
void _Battle::UpdateServer(double FrameTime) {

	switch(State) {
		case STATE_INPUT:
			RoundTime += FrameTime;
			if(RoundTime > BATTLE_ROUNDTIME)
				State = STATE_RESOLVETURN;
		break;
		case STATE_RESOLVETURN:
			ResolveTurn();
			CheckEnd();
		break;
		case STATE_END:
		break;
	}
}

// Resolves the turn and sends the result to each player
void _Battle::ResolveTurn() {
	RoundTime = 0;

	// Get a monster list
	std::vector<_Object *> Monsters;
	GetMonsterList(Monsters);

	// Update AI
	if(Monsters.size() > 0) {

		// Get a list of humans on the left side
		std::vector<_Object *> Humans;
		GetAliveFighterList(0, Humans);

		// Update the monster's target
		for(size_t i = 0; i < Monsters.size(); i++) {
			Monsters[i]->UpdateTarget(Humans);
		}
	}

	// Handle each fighter's action
	_ActionResult Results[BATTLE_MAXFIGHTERS];
	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i]) {
			_ActionResult *Result = &Results[i];
			Result->Fighter = Fighters[i];

			// Ignore dead fighters
			if(Fighters[i]->Health > 0) {
				Result->Target = Fighters[i]->Target;

				// Get skill used
				const _Skill *Skill = Fighters[i]->GetActionBar(Fighters[i]->GetCommand());
				if(Skill && Skill->CanUse(Result->Fighter)) {
					int TargetFighterIndex = GetFighterFromSlot(Result->Target);
					Result->SkillID = Skill->ID;

					// Update fighters
					_ActionResult *TargetResult = &Results[TargetFighterIndex];
					TargetResult->Fighter = Fighters[TargetFighterIndex];
					Skill->ResolveSkill(Result, TargetResult);
				}

				// Update health and mana regen
				int HealthUpdate, ManaUpdate;
				Fighters[i]->UpdateRegen(HealthUpdate, ManaUpdate);
				Result->HealthChange += HealthUpdate;
				Result->ManaChange += ManaUpdate;
			}
		}
	}

	// Build packet for results
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::BATTLE_TURNRESULTS);

	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i]) {

			// Update fighters
			Fighters[i]->UpdateHealth(Results[i].HealthChange);
			Fighters[i]->UpdateMana(Results[i].ManaChange);
			Fighters[i]->Command = -1;

			Packet.Write<char>(Results[i].Target);
			Packet.Write<int32_t>(Results[i].SkillID);
			Packet.Write<int32_t>(Results[i].DamageDealt);
			Packet.Write<int32_t>(Results[i].HealthChange);
			Packet.Write<int32_t>(Fighters[i]->Health);
			Packet.Write<int32_t>(Fighters[i]->Mana);
		}
	}

	// Send packet
	BroadcastPacket(Packet);
}

// Checks for the end of a battle
void _Battle::CheckEnd() {
	if(State == STATE_END)
		return;

	// Players that get a reward
	std::vector<_Object *> Players;

	// Get statistics for each side
	_BattleResult Side[2];
	for(int i = 0; i < 2; i++) {

		// Get a list of fighters that are still in the battle
		std::vector<_Object *> SideFighters;
		GetFighterList(i, SideFighters);

		// Loop through fighters
		for(size_t j = 0; j < SideFighters.size(); j++) {

			// Keep track of players
			if(SideFighters[j]->Type == _Object::PLAYER) {
				Players.push_back(SideFighters[j]);
				Side[i].PlayerCount++;
			}
			else
				Side[i].MonsterCount++;

			// Tally alive fighters
			if(SideFighters[j]->Health > 0) {
				Side[i].Dead = false;
			}

			// Sum experience and gold
			Side[i].ExperienceGiven += SideFighters[j]->GetExperienceGiven();
			Side[i].GoldGiven += SideFighters[j]->GetGoldGiven();
			Side[i].FighterCount++;
		}
	}

	// Check end conditions
	if(Side[0].Dead || Side[1].Dead || Side[0].FighterCount == 0 || Side[1].FighterCount == 0) {

		// Cap experience
		for(int i = 0; i < 2; i++) {
			if(Side[i].FighterCount > 0) {
				int OtherSide = !i;

				// Divide experience up
				if(Side[OtherSide].ExperienceGiven > 0) {
					Side[OtherSide].ExperienceGiven /= Side[i].FighterCount;
					if(Side[OtherSide].ExperienceGiven <= 0)
						Side[OtherSide].ExperienceGiven = 1;
				}

				// Divide gold up
				if(Side[OtherSide].GoldGiven > 0) {
					Side[OtherSide].GoldGiven /= Side[i].FighterCount;
					if(Side[OtherSide].GoldGiven <= 0)
						Side[OtherSide].GoldGiven = 1;
				}
			}
		}

		// Hand out monster drops
		std::vector<int> PlayerItems[3];
		if(!Side[0].Dead) {

			// Get a monster list
			std::vector<_Object *> Monsters;
			GetMonsterList(Monsters);

			// Make sure there are monsters
			if(Monsters.size() > 0) {

				// Generate monster drops in player vs monster situations
				std::vector<int> MonsterDrops;
				for(size_t i = 0; i < Monsters.size(); i++) {
					Stats->GenerateMonsterDrops(Monsters[i]->DatabaseID, 1, MonsterDrops);
				}

				// Get a list of players that receive items
				std::vector<_Object *> LeftSidePlayers;
				GetPlayerList(0, LeftSidePlayers);

				// Give out rewards round robin style
				std::uniform_int_distribution<size_t> Distribution(0, LeftSidePlayers.size()-1);
				size_t PlayerIndex = Distribution(RandomGenerator);
				for(size_t i = 0; i < MonsterDrops.size(); i++) {
					int LeftSideSlot = LeftSidePlayers[PlayerIndex]->BattleSlot / 2;
					if(MonsterDrops[i] > 0) {
						PlayerItems[LeftSideSlot].push_back(MonsterDrops[i]);

						// Go to next player
						PlayerIndex++;
						if(PlayerIndex >= LeftSidePlayers.size())
							PlayerIndex = 0;
					}
				}
			}
		}

		// Award each player
		for(size_t i = 0; i < Players.size(); i++) {

			// Get rewards
			int ExperienceEarned = 0;
			int GoldEarned = 0;
			_BattleResult *PlayerSide = &Side[Players[i]->GetSide()];
			_BattleResult *OppositeSide = &Side[!Players[i]->GetSide()];
			if(PlayerSide->Dead) {
				GoldEarned = (int)(-Players[i]->Gold * 0.1f);
				Players[i]->Deaths++;

				//OldServerState.SendMessage(Players[i], std::string("You lost " + std::to_string(std::abs(GoldEarned)) + " gold"), COLOR_RED);
			}
			else {
				ExperienceEarned = OppositeSide->ExperienceGiven;
				GoldEarned = OppositeSide->GoldGiven;
				Players[i]->PlayerKills += OppositeSide->PlayerCount;
				Players[i]->MonsterKills += OppositeSide->MonsterCount;

				// Revive dead players and give them one health
				//if(Players[i]->Health == 0)
				//	Players[i]->Health = 1;
			}

			// Update stats
			int CurrentLevel = Players[i]->Level;
			Players[i]->UpdateExperience(ExperienceEarned);
			Players[i]->UpdateGold(GoldEarned);
			Players[i]->CalculatePlayerStats();
			int NewLevel = Players[i]->Level;
			if(NewLevel > CurrentLevel) {
				Players[i]->RestoreHealthMana();
				//OldServerState.SendMessage(Players[i], std::string("You are now level " + std::to_string(NewLevel) + "!"), COLOR_GOLD);
			}

			// Write results
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::BATTLE_END);
			Packet.WriteBit(Side[0].Dead);
			Packet.WriteBit(Side[1].Dead);
			Packet.Write<char>(OppositeSide->PlayerCount);
			Packet.Write<char>(OppositeSide->MonsterCount);
			Packet.Write<int32_t>(ExperienceEarned);
			Packet.Write<int32_t>(GoldEarned);

			// Write items
			int PlayerIndex = Players[i]->BattleSlot / 2;
			int ItemCount = PlayerItems[PlayerIndex].size();
			Packet.Write<char>(ItemCount);

			// Write items
			for(int j = 0; j < ItemCount; j++) {
				int ItemID = PlayerItems[PlayerIndex][j];
				Packet.Write<int32_t>(ItemID);
				Players[i]->AddItem(Stats->GetItem(ItemID), 1, -1);
			}

			//OldServerNetwork->SendPacketToPeer(&Packet, Players[i]->Peer);
		}

		State = STATE_END;
	}
	else
		State = STATE_INPUT;
}

// Send a packet to all players
void _Battle::BroadcastPacket(_Buffer &Packet) {

	// Send packet to all players
	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->Peer) {
			ServerNetwork->SendPacket(Packet, Fighters[i]->Peer);
		}
	}
}

// Send the player's skill to the other players
void _Battle::SendSkillToPlayers(_Object *Player) {

	// Get all the players on the player's side
	std::vector<_Object *> SidePlayers;
	GetPlayerList(Player->GetSide(), SidePlayers);
	if(SidePlayers.size() == 1)
		return;

	// Get skill id
	const _Skill *Skill = Player->GetActionBar(Player->GetCommand());
	uint32_t SkillID = 0;
	if(Skill)
		SkillID = Skill->ID;

	// Build packet
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::BATTLE_COMMAND);
	Packet.Write<char>(Player->BattleSlot);
	Packet.Write<char>(SkillID);

	// Send packet to all players
	for(size_t i = 0; i < SidePlayers.size(); i++) {
		if(SidePlayers[i] != Player) {
			//OldServerNetwork->SendPacketToPeer(&Packet, SidePlayers[i]->Peer);
		}
	}
}
