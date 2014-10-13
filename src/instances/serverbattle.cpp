/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2010  Alan Witkowski
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
**************************************************************************************/
#include "serverbattle.h"
#include "../engine/globals.h"
#include "../engine/stats.h"
#include "../engine/constants.h"
#include "../network/packetstream.h"
#include "../network/network.h"
#include "../engine/random.h"
#include "../engine/instances.h"
#include "../objects/player.h"
#include "../objects/monster.h"

// Constructor
ServerBattleClass::ServerBattleClass()
:	BattleClass() {

	RoundTime = 0;
}

// Destructor
ServerBattleClass::~ServerBattleClass() {

}

// Removes a player from the battle
int ServerBattleClass::RemovePlayer(PlayerClass *TPlayer) {

	int Count = 0;
	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetType() == FighterClass::TYPE_PLAYER) {
			PlayerClass *Player = static_cast<PlayerClass *>(Fighters[i]);
			
			if(Player == TPlayer) {
				Player->StopBattle();
				Fighters[i] = NULL;
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
void ServerBattleClass::StartBattle() {

	// Build packet
	PacketClass Packet(NetworkClass::WORLD_STARTBATTLE);

	// Write fighter count
	int FighterCount = Fighters.size();
	Packet.WriteChar(FighterCount);

	// Write fighter information
	for(int i = 0; i < FighterCount; i++) {

		// Write fighter type
		int Type = Fighters[i]->GetType();
		Packet.WriteBit(!!Type);
		Packet.WriteBit(!!Fighters[i]->GetSide());

		if(Type == FighterClass::TYPE_PLAYER) {
			PlayerClass *Player = static_cast<PlayerClass *>(Fighters[i]);

			// Network ID
			Packet.WriteChar(Player->GetNetworkID());

			// Player stats
			Packet.WriteInt(Player->GetHealth());
			Packet.WriteInt(Player->GetMaxHealth());
			Packet.WriteInt(Player->GetMana());
			Packet.WriteInt(Player->GetMaxMana());

			// Start the battle for the player
			Player->StartBattle(this);
		}
		else {
			MonsterClass *Monster = static_cast<MonsterClass *>(Fighters[i]);

			// Monster ID
			Packet.WriteInt(Monster->GetID());
		}
	}

	// Send packet to players
	SendPacketToPlayers(&Packet);

	State = STATE_INPUT;
}

// Handles input from the client
void ServerBattleClass::HandleInput(PlayerClass *TPlayer, int TCommand, int TTarget) {

	if(State == STATE_INPUT) {

		// Check for needed commands
		if(TPlayer->GetCommand() == -1) {
			TPlayer->SetCommand(TCommand);
			TPlayer->SetTarget(TTarget);

			// Notify other players
			SendSkillToPlayers(TPlayer);

			// Check for all commands
			bool Ready = true;
			for(u32 i = 0; i < Fighters.size(); i++) {
				if(Fighters[i] && Fighters[i]->GetHealth() > 0 && Fighters[i]->GetCommand() == -1) {
					Ready = false;
					break;
				}
			}
			if(Ready)
				State = STATE_RESOLVETURN;
		}
	}
}

// Update the battle system for the server
void ServerBattleClass::Update(u32 TDeltaTime) {

	switch(State) {
		case STATE_INPUT:
			RoundTime += TDeltaTime;
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
void ServerBattleClass::ResolveTurn() {
	RoundTime = 0;

	// Get a monster list
	array<MonsterClass *> Monsters;
	GetMonsterList(Monsters);

	// Update AI
	if(Monsters.size() > 0) {

		// Get a list of humans on the left side
		array<FighterClass *> Humans;
		GetAliveFighterList(0, Humans);

		// Update the monster's target
		for(u32 i = 0; i < Monsters.size(); i++) {
			Monsters[i]->UpdateTarget(Humans);
		}
	}

	// Handle each fighter's action
	FighterResultStruct Results[BATTLE_MAXFIGHTERS];
	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i]) {
			FighterResultStruct *Result = &Results[i];
			Result->Fighter = Fighters[i];

			// Ignore dead fighters
			if(Fighters[i]->GetHealth() > 0) {
				Result->Target = Fighters[i]->GetTarget();
				
				// Get skill used
				const SkillClass *Skill = Fighters[i]->GetSkillBar(Fighters[i]->GetCommand());
				if(Skill && Skill->CanUse(Result->Fighter)) {
					int TargetFighterIndex = GetFighterFromSlot(Result->Target);
					Result->SkillID = Skill->GetID();

					// Update fighters
					FighterResultStruct *TargetResult = &Results[TargetFighterIndex];
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
	PacketClass Packet(NetworkClass::BATTLE_TURNRESULTS);
	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i]) {

			// Update fighters
			Fighters[i]->UpdateHealth(Results[i].HealthChange);
			Fighters[i]->UpdateMana(Results[i].ManaChange);
			Fighters[i]->SetCommand(-1);

			Packet.WriteChar(Results[i].Target);
			Packet.WriteInt(Results[i].SkillID);
			Packet.WriteInt(Results[i].DamageDealt);
			Packet.WriteInt(Results[i].HealthChange);
			Packet.WriteInt(Fighters[i]->GetHealth());
			Packet.WriteInt(Fighters[i]->GetMana());
		}
	}

	// Send packet
	SendPacketToPlayers(&Packet);
}

// Checks for the end of a battle
void ServerBattleClass::CheckEnd() {
	if(State == STATE_END)
		return;

	// Players that get a reward
	array<PlayerClass *> Players;

	// Get statistics for each side
	BattleResultStruct Side[2];
	for(int i = 0; i < 2; i++) {

		// Get a list of fighters that are still in the battle
		array<FighterClass *> SideFighters;
		GetFighterList(i, SideFighters);

		// Loop through fighters
		for(u32 j = 0; j < SideFighters.size(); j++) {

			// Keep track of players
			if(SideFighters[j]->GetType() == FighterClass::TYPE_PLAYER) {
				Players.push_back(static_cast<PlayerClass *>(SideFighters[j]));
				Side[i].PlayerCount++;
			}
			else
				Side[i].MonsterCount++;

			// Tally alive fighters
			if(SideFighters[j]->GetHealth() > 0) {
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
		array<int> PlayerItems[3];
		if(!Side[0].Dead) {

			// Get a monster list
			array<MonsterClass *> Monsters;
			GetMonsterList(Monsters);

			// Make sure there are monsters
			if(Monsters.size() > 0) {

				// Generate monster drops in player vs monster situations
				array<int> MonsterDrops;
				for(u32 i = 0; i < Monsters.size(); i++) {
					Stats::Instance().GenerateMonsterDrops(Monsters[i]->GetID(), 1, MonsterDrops);
				}

				// Get a list of players that receive items
				array<PlayerClass *> LeftSidePlayers;
				GetPlayerList(0, LeftSidePlayers);

				// Give out rewards round robin style
				u32 PlayerIndex = Random::Instance().GenerateRange(0, (int)(LeftSidePlayers.size()-1));
				for(u32 i = 0; i < MonsterDrops.size(); i++) {
					int LeftSideSlot = LeftSidePlayers[PlayerIndex]->GetSlot() / 2;
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
		for(u32 i = 0; i < Players.size(); i++) {

			// Get rewards
			int ExperienceEarned = 0;
			int GoldEarned = 0;
			BattleResultStruct *PlayerSide = &Side[Players[i]->GetSide()];
			BattleResultStruct *OppositeSide = &Side[!Players[i]->GetSide()];
			if(PlayerSide->Dead) {
				GoldEarned = (int)(-Players[i]->GetGold() * 0.1f);
				Players[i]->UpdateDeaths(1);
			}
			else {
				ExperienceEarned = OppositeSide->ExperienceGiven;
				GoldEarned = OppositeSide->GoldGiven;
				Players[i]->UpdatePlayerKills(OppositeSide->PlayerCount);
				Players[i]->UpdateMonsterKills(OppositeSide->MonsterCount);

				// Revive dead players and give them one health
				if(Players[i]->GetHealth() == 0)
					Players[i]->SetHealth(1);
			}

			// Update stats
			int CurrentLevel = Players[i]->GetLevel();
			Players[i]->UpdateExperience(ExperienceEarned);
			Players[i]->UpdateGold(GoldEarned);
			Players[i]->CalculatePlayerStats();
			int NewLevel = Players[i]->GetLevel();
			if(NewLevel > CurrentLevel) {
				Players[i]->RestoreHealthMana();
			}

			// Write results
			PacketClass Packet(NetworkClass::BATTLE_END);
			Packet.WriteBit(Side[0].Dead);
			Packet.WriteBit(Side[1].Dead);
			Packet.WriteChar(OppositeSide->PlayerCount);
			Packet.WriteChar(OppositeSide->MonsterCount);
			Packet.WriteInt(ExperienceEarned);
			Packet.WriteInt(GoldEarned);

			// Write items
			int PlayerIndex = Players[i]->GetSlot() / 2;
			int ItemCount = PlayerItems[PlayerIndex].size();
			Packet.WriteChar(ItemCount);

			// Write items
			for(int j = 0; j < ItemCount; j++) {
				int ItemID = PlayerItems[PlayerIndex][j];
				Packet.WriteInt(ItemID);
				Players[i]->AddItem(Stats::Instance().GetItem(ItemID), 1, -1);
			}
			
			ServerNetwork->SendPacketToPeer(&Packet, Players[i]->GetPeer());
		}

		State = STATE_END;
	}
	else
		State = STATE_INPUT;
}

// Send a packet to all players
void ServerBattleClass::SendPacketToPlayers(PacketClass *TPacket) {
	
	// Send packet to all players
	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetType() == FighterClass::TYPE_PLAYER) {
			PlayerClass *Player = static_cast<PlayerClass *>(Fighters[i]);
			ServerNetwork->SendPacketToPeer(TPacket, Player->GetPeer());
		}
	}	
}

// Send the player's skill to the other players
void ServerBattleClass::SendSkillToPlayers(PlayerClass *TPlayer) {

	// Get all the players on the player's side
	array<PlayerClass *> SidePlayers;
	GetPlayerList(TPlayer->GetSide(), SidePlayers);
	if(SidePlayers.size() == 1)
		return;

	// Get skill id
	const SkillClass *Skill = TPlayer->GetSkillBar(TPlayer->GetCommand());
	int SkillID = -1;
	if(Skill)
		SkillID = Skill->GetID();

	// Build packet
	PacketClass Packet(NetworkClass::BATTLE_COMMAND);
	Packet.WriteChar(TPlayer->GetSlot());
	Packet.WriteChar(SkillID);

	// Send packet to all players
	for(u32 i = 0; i < SidePlayers.size(); i++) {
		if(SidePlayers[i] != TPlayer) {
			ServerNetwork->SendPacketToPeer(&Packet, SidePlayers[i]->GetPeer());
		}
	}
}
