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
#include <network/servernetwork.h>
#include <network/clientnetwork.h>
#include <objects/object.h>
#include <objects/skill.h>
#include <ui/element.h>
#include <ui/label.h>
#include <ui/image.h>
#include <actions.h>
#include <buffer.h>
#include <graphics.h>
#include <stats.h>
#include <program.h>
#include <assets.h>
#include <font.h>
#include <packet.h>
#include <random.h>
#include <iostream>

// Constructor
_Battle::_Battle() :
	Stats(nullptr),
	ServerNetwork(nullptr),
	ClientNetwork(nullptr),
	ClientPlayer(nullptr),
	State(STATE_NONE),
	Timer(0),
	TotalExperience(0),
	TotalGold(0),
	BattleElement(nullptr),
	BattleWinElement(nullptr),
	BattleLoseElement(nullptr) {

	SideCount[0] = 0;
	SideCount[1] = 0;
}

// Destructor
_Battle::~_Battle() {
	if(BattleElement)
		BattleElement->SetVisible(false);
	if(BattleWinElement)
		BattleWinElement->SetVisible(false);
	if(BattleLoseElement)
		BattleLoseElement->SetVisible(false);

	// Delete monsters
	for(auto &Fighter : Fighters) {
		if(Fighter->DatabaseID)
			delete Fighter;
	}
}

// Update battle
void _Battle::Update(double FrameTime) {

	int AliveCount[2] = { 0, 0 };
	for(auto &Fighter : Fighters) {

		// Count alive fighters for each side
		if(Fighter->Health > 0)
			AliveCount[Fighter->BattleSide]++;

		// Check turn timer
		Fighter->TurnTimer += FrameTime * 0.5f;
		if(Fighter->TurnTimer > 1.0) {
			Fighter->TurnTimer = 1.0;

			if(ServerNetwork) {
				if(Fighter->BattleAction.IsSet()) {
					ResolveAction(Fighter);
				}
			}
		}
	}
}

// Render the battle system
void _Battle::Render(double BlendFactor) {

	switch(State) {
		case STATE_WIN:
			RenderBattleWin();
		break;
		case STATE_LOSE:
			RenderBattleLose();
		break;
		default:
			RenderBattle();
		break;
	}
}

// Renders the battle part
void _Battle::RenderBattle() {
	BattleElement->Render();

	// Draw fighters
	for(auto &Fighter : Fighters) {
		Fighter->RenderBattle(ClientPlayer->BattleTarget == Fighter);
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
		for(auto &MonsterDrop : MonsterDrops) {
			const _Texture *Texture = MonsterDrop->Image;
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

// Sends an action selection to the server
void _Battle::ClientSetAction(int ActionBarSlot) {
	if(ClientPlayer->Health == 0)
		return;

	// Get skill
	const _Skill *Skill = ClientPlayer->GetActionBar(ActionBarSlot);
	if(Skill == nullptr || !Skill->CanUse(ClientPlayer))
		return;

	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::BATTLE_ACTION);
	Packet.Write<char>(ActionBarSlot);

	ClientNetwork->SendPacket(Packet);
	ClientPlayer->BattleAction.Skill = ClientPlayer->ActionBar[ActionBarSlot];

	// Update potion count
	if(Skill->Type == _Skill::TYPE_USEPOTION)
		ClientPlayer->UpdatePotionsLeft(Skill->ID == 3);
}

// Changes targets
void _Battle::ChangeTarget(int Direction, int SideDirection) {
	if(!ClientNetwork || ClientPlayer->Health == 0)
		return;

	// Change sides
	if(SideDirection != 0)
		ClientPlayer->BattleTargetSide = !ClientPlayer->BattleTargetSide;

	// Update target
	ClientPlayer->BattleTargetIndex += Direction;
	if(ClientPlayer->BattleTargetIndex < 0)
		ClientPlayer->BattleTargetIndex = SideCount[ClientPlayer->BattleTargetSide]-1;
	else if(ClientPlayer->BattleTargetIndex >= SideCount[ClientPlayer->BattleTargetSide])
		ClientPlayer->BattleTargetIndex = 0;

	ClientPlayer->BattleTarget = GetObjectFromIndex(ClientPlayer->BattleTargetSide, ClientPlayer->BattleTargetIndex);

	// Send packet
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::BATTLE_CHANGETARGET);
	Packet.Write<char>(ClientPlayer->BattleTargetSide);
	Packet.Write<char>(ClientPlayer->BattleTargetIndex);
	ClientNetwork->SendPacket(Packet);
}

// Resolves an action and sends the result to each player
void _Battle::ResolveAction(_Object *SourceFighter) {
	if(SourceFighter->Health <= 0)
		return;

	_ActionResult ActionResult;
	ActionResult.SourceFighter = SourceFighter;
	ActionResult.TargetFighter = SourceFighter->BattleTarget;
	ActionResult.SkillUsed = SourceFighter->BattleAction.Skill;

	// Update fighters
	if(ActionResult.SkillUsed)
		ActionResult.SkillUsed->ResolveUse(ActionResult);

	// Update source health and mana
	int HealthUpdate, ManaUpdate;
	SourceFighter->UpdateRegen(HealthUpdate, ManaUpdate);
	ActionResult.SourceHealthChange += HealthUpdate;
	ActionResult.SourceManaChange += ManaUpdate;

	// Update fighters
	ActionResult.SourceFighter->UpdateHealth(ActionResult.SourceHealthChange);
	ActionResult.SourceFighter->UpdateMana(ActionResult.SourceManaChange);
	ActionResult.TargetFighter->UpdateHealth(ActionResult.TargetHealthChange);
	ActionResult.TargetFighter->UpdateMana(ActionResult.TargetManaChange);

	// Build packet for results
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::BATTLE_ACTIONRESULTS);

	int SkillID = 0;
	if(ActionResult.SkillUsed)
		SkillID = ActionResult.SkillUsed->ID;

	Packet.Write<char>(ActionResult.SourceFighter->BattleSide);
	Packet.Write<char>(ActionResult.SourceFighter->BattleIndex);
	Packet.Write<char>(ActionResult.TargetFighter->BattleSide);
	Packet.Write<char>(ActionResult.TargetFighter->BattleIndex);
	Packet.Write<uint32_t>(SkillID);
	Packet.Write<int32_t>(ActionResult.DamageDealt);
	Packet.Write<int32_t>(ActionResult.SourceHealthChange);
	Packet.Write<int32_t>(ActionResult.SourceManaChange);
	Packet.Write<int32_t>(ActionResult.TargetHealthChange);
	Packet.Write<int32_t>(ActionResult.TargetManaChange);
	Packet.Write<int32_t>(ActionResult.SourceFighter->Health);
	Packet.Write<int32_t>(ActionResult.SourceFighter->Mana);
	Packet.Write<int32_t>(ActionResult.TargetFighter->Health);
	Packet.Write<int32_t>(ActionResult.TargetFighter->Mana);

	// Send packet
	BroadcastPacket(Packet);

	SourceFighter->TurnTimer = 0.0;
	SourceFighter->BattleAction.Unset();
}

// Displays turn results from the server
void _Battle::ClientResolveAction(_Buffer &Data) {

	_ActionResult ActionResult;
	int SourceBattleSide = Data.Read<char>();
	int SourceBattleIndex = Data.Read<char>();
	int TargetBattleSide = Data.Read<char>();
	int TargetBattleIndex = Data.Read<char>();

	int SkillID = Data.Read<uint32_t>();
	ActionResult.SkillUsed = Stats->Skills[SkillID];
	ActionResult.DamageDealt = Data.Read<int32_t>();
	ActionResult.SourceHealthChange = Data.Read<int32_t>();
	ActionResult.SourceManaChange = Data.Read<int32_t>();
	ActionResult.TargetHealthChange = Data.Read<int32_t>();
	ActionResult.TargetManaChange = Data.Read<int32_t>();
	int SourceFighterHealth = Data.Read<int32_t>();
	int SourceFighterMana = Data.Read<int32_t>();
	int TargetFighterHealth = Data.Read<int32_t>();
	int TargetFighterMana = Data.Read<int32_t>();

	// Update source fighter
	ActionResult.SourceFighter = GetObjectFromIndex(SourceBattleSide, SourceBattleIndex);
	if(ActionResult.SourceFighter) {
		ActionResult.SourceFighter->Health = SourceFighterHealth;
		ActionResult.SourceFighter->Mana = SourceFighterMana;
		ActionResult.SourceFighter->TurnTimer = 0.0;
		ActionResult.SourceFighter->BattleAction.Unset();
	}

	// Update target fighter
	ActionResult.TargetFighter = GetObjectFromIndex(TargetBattleSide, TargetBattleIndex);
	if(ActionResult.TargetFighter) {
		ActionResult.TargetFighter->Health = TargetFighterHealth;
		ActionResult.TargetFighter->Mana = TargetFighterMana;
	}

/*
	// Change targets if the old one died
	int TargetIndex = GetFighterFromSlot(ClientPlayer->Target);
	if(TargetIndex != -1 && Fighters[TargetIndex]->Health == 0)
		ChangeTarget(1);
*/
}

// Get the object pointer from index and side
_Object *_Battle::GetObjectFromIndex(int BattleSide, int BattleIndex) {
	for(auto &Fighter : Fighters) {
		if(Fighter->BattleSide == BattleSide && Fighter->BattleIndex == BattleIndex) {
			return Fighter;
		}
	}

	return nullptr;
}

// Add a fighter to the battle
void _Battle::AddFighter(_Object *Fighter, int Side) {
	Fighter->Battle = this;
	Fighter->BattleSide = Side;
	Fighter->BattleIndex = SideCount[Side];
	Fighter->BattleAction.Unset();
	for(int i = 0; i < 2; i++)
		Fighter->PotionsLeft[i] = Fighter->MaxPotions[i];

	// Count fighters and set slots
	SideCount[Side]++;
	Fighters.push_back(Fighter);
}

// Get a list of fighters from a side
void _Battle::GetFighterList(int Side, std::list<_Object *> &SideFighters) {

	for(auto &Fighter : Fighters) {
		if(Fighter->BattleSide == Side) {
			SideFighters.push_back(Fighter);
		}
	}
}

// Get a list of alive fighters from a side
void _Battle::GetAliveFighterList(int Side, std::list<_Object *> &AliveFighters) {

	for(auto &Fighter : Fighters) {
		if(Fighter->BattleSide == Side && Fighter->Health > 0) {
			AliveFighters.push_back(Fighter);
		}
	}
}

// Starts the battle on the client
void _Battle::StartBattleClient() {
	BattleElement = Assets.Elements["element_battle"];
	BattleWinElement = Assets.Elements["element_battlewin"];
	BattleLoseElement = Assets.Elements["element_battlelose"];
	BattleElement->SetVisible(true);

	// Set fighter position offsets
	for(auto &Fighter : Fighters) {
		GetBattleOffset(Fighter->BattleIndex, Fighter);
		Fighter->BattleTarget = GetObjectFromIndex(Fighter->BattleTargetSide, Fighter->BattleTargetIndex);
	}
}

// End of a battle
void _Battle::EndBattle(_Buffer &Data) {
/*
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
	int PlayerSide = ClientPlayer->BattleSide;
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
*/
	// Go to the ending state immediately
	/*
	if(State != STATE_TURNRESULTS) {
		State = TargetState;
		TargetState = -1;
	}*/
}

// Calculates a screen position for a slot
void _Battle::GetBattleOffset(int SideIndex, _Object *Fighter) {
	_Element *BattleElement = Assets.Elements["element_battle"];
	glm::ivec2 Center = (BattleElement->Bounds.End + BattleElement->Bounds.Start) / 2;

	// Check sides
	if(Fighter->BattleSide == 0) {
		Fighter->BattleOffset.x = Center.x - 180;
	}
	else {
		Fighter->BattleOffset.x = Center.x + 100;
	}

	// Divide space into SideCount parts, then divide that by 2
	int SpacingY = (BattleElement->Size.y / SideCount[Fighter->BattleSide]) / 2;

	// Place slots in between main divisions
	Fighter->BattleOffset.y = BattleElement->Bounds.Start.y + SpacingY * (2 * SideIndex + 1) + 10;

	// Convert position to relative offset from center
	Fighter->BattleOffset = Fighter->BattleOffset - Center;
}

// Updates player stats
void _Battle::UpdateStats() {
	ClientPlayer->UpdateExperience(TotalExperience);
	ClientPlayer->UpdateGold(TotalGold);
	ClientPlayer->CalculatePlayerStats();
}

// Removes a player from the battle, return remaining player count
void _Battle::RemoveFighter(_Object *RemoveFighter) {

	for(auto Iterator = Fighters.begin(); Iterator != Fighters.end(); ++Iterator) {
		_Object *Fighter = *Iterator;
		if(Fighter == RemoveFighter) {
			SideCount[Fighter->BattleSide]--;
			Fighter->StopBattle();
			if(Fighter->DatabaseID)
				delete Fighter;

			Fighters.erase(Iterator);
			return;
		}
	}
}

// Give each fighter an initial target
void _Battle::SetDefaultTargets() {
	for(auto &Fighter : Fighters) {
		Fighter->BattleTargetSide = !Fighter->BattleSide;
		Fighter->BattleTargetIndex = 0;
		Fighter->BattleTarget = GetObjectFromIndex(Fighter->BattleTargetSide, Fighter->BattleTargetIndex);
	}
}

// Get number of peers in battle
int _Battle::GetPeerCount() {
	int PeerCount = 0;
	for(auto &Fighter : Fighters) {
		if(Fighter->Peer)
			PeerCount++;
	}

	return PeerCount;
}

// Starts the battle and notifies the players
void _Battle::StartBattleServer() {

	// Set targets
	SetDefaultTargets();

	// Build packet
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::BATTLE_START);

	// Write fighter count
	int FighterCount = Fighters.size();
	Packet.Write<int>(FighterCount);

	// Write fighter information
	for(auto &Fighter : Fighters) {

		// Write fighter type
		Packet.Write<int>(Fighter->DatabaseID);
		Packet.Write<char>(Fighter->BattleSide);
		Packet.Write<char>(Fighter->BattleTargetSide);
		Packet.Write<char>(Fighter->BattleTargetIndex);

		if(Fighter->DatabaseID == 0) {

			// Network ID
			Packet.Write<NetworkIDType>(Fighter->NetworkID);
			Packet.Write<glm::ivec2>(Fighter->Position);

			// Player stats
			Packet.Write<int32_t>(Fighter->Health);
			Packet.Write<int32_t>(Fighter->MaxHealth);
			Packet.Write<int32_t>(Fighter->Mana);
			Packet.Write<int32_t>(Fighter->MaxMana);
		}
	}

	// Send packet to players
	BroadcastPacket(Packet);
}

// Handle player input
void _Battle::ClientHandleInput(int Action) {

	switch(State) {
		case STATE_WIN:
		case STATE_LOSE: {
			if(Timer > BATTLE_WAITENDTIME) {

				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::BATTLE_CLIENTDONE);
				//ClientState.Network->SendPacket(Packet);
			}
		}
		break;
		default:
			switch(Action) {
				case _Actions::SKILL1:
				case _Actions::SKILL2:
				case _Actions::SKILL3:
				case _Actions::SKILL4:
				case _Actions::SKILL5:
				case _Actions::SKILL6:
				case _Actions::SKILL7:
				case _Actions::SKILL8:
					ClientSetAction(Action - _Actions::SKILL1);
				break;
				case _Actions::UP:
					ChangeTarget(-1, 0);
				break;
				case _Actions::DOWN:
					ChangeTarget(1, 0);
				break;
				case _Actions::LEFT:
					ChangeTarget(0, -1);
				break;
				case _Actions::RIGHT:
					ChangeTarget(0, 1);
				break;
			}
		break;
	}
}

// Handles input from the client
void _Battle::ServerHandleAction(_Object *Fighter, int ActionBarSlot) {

	// Check for needed commands
	if(!Fighter->BattleAction.IsSet()) {

		int SkillID = 0;
		if(ActionBarSlot != -1) {
			Fighter->BattleAction.Skill = Fighter->ActionBar[ActionBarSlot];
			if(Fighter->BattleAction.Skill)
				SkillID = Fighter->BattleAction.Skill->ID;
		}

		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::BATTLE_ACTION);
		Packet.Write<char>(Fighter->BattleSide);
		Packet.Write<char>(Fighter->BattleIndex);
		Packet.Write<int>(SkillID);
		Packet.Write<int>(0);

		BroadcastPacket(Packet);
	}
}

// Handle action from another player
void _Battle::ClientHandlePlayerAction(_Buffer &Data) {

	int BattleSide = Data.Read<char>();
	int BattleIndex = Data.Read<char>();
	int SkillID = Data.Read<int>();
	//int ItemID = Data.Read<int>();

	_Object *Fighter = GetObjectFromIndex(BattleSide, BattleIndex);
	if(Fighter)
		Fighter->BattleAction.Skill = Stats->Skills[SkillID];
}

// Checks for the end of a battle
void _Battle::CheckEnd() {
	/*
	if(State == STATE_END)
		return;

	// Players that get a reward
	std::list<_Object *> Players;

	// Get statistics for each side
	_BattleResult Side[2];
	for(int i = 0; i < 2; i++) {

		// Get a list of fighters that are still in the battle
		std::list<_Object *> SideFighters;
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
		std::list<int> PlayerItems[3];
		if(!Side[0].Dead) {

			// Get a monster list
			std::list<_Object *> Monsters;
			GetMonsterList(Monsters);

			// Make sure there are monsters
			if(Monsters.size() > 0) {

				// Generate monster drops in player vs monster situations
				std::list<int> MonsterDrops;
				for(size_t i = 0; i < Monsters.size(); i++) {
					Stats->GenerateMonsterDrops(Monsters[i]->DatabaseID, 1, MonsterDrops);
				}

				// Get a list of players that receive items
				std::list<_Object *> LeftSidePlayers;
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
			_BattleResult *PlayerSide = &Side[Players[i]->BattleSide];
			_BattleResult *OppositeSide = &Side[!Players[i]->BattleSide];
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
		*/
}

// Send a packet to all players
void _Battle::BroadcastPacket(_Buffer &Packet) {

	// Send packet to all players
	for(auto &Fighter : Fighters) {
		if(Fighter->Peer) {
			ServerNetwork->SendPacket(Packet, Fighter->Peer);
		}
	}
}

