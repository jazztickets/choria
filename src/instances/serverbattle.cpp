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
#include "serverbattle.h"
#include "../engine/globals.h"
#include "../engine/stats.h"
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
int ServerBattleClass::RemovePlayer(PlayerClass *Player) {

	int Count = 0;
	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetType() == FighterClass::TYPE_PLAYER) {
			PlayerClass *PlayerIterator = static_cast<PlayerClass *>(Fighters[i]);
			
			// Remove the player from battle
			if(PlayerIterator == Player) {
				PlayerIterator->ExitBattle();
				Fighters[i] = NULL;
			}

			if(Fighters[i])
				Count++;
		}
	}

	// Update player count
	PlayerCount = Count;

	// Check for ending conditions
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
		Packet.WriteBit(Type);
		Packet.WriteBit(Fighters[i]->GetSide());

		if(Type == FighterClass::TYPE_PLAYER) {
			PlayerClass *Player = static_cast<PlayerClass *>(Fighters[i]);

			// Network ID
			Packet.WriteInt(Player->NetworkID);

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

	State = STATE_BATTLE;
}

// Handles input from the client
void ServerBattleClass::HandleInput(PlayerClass *Player, int ActionBarIndex, int Target) {

	// Set inputs
	Player->SetBattleAction(Player->GetActionBar(ActionBarIndex));
	Player->SetTarget(Target);

	// Notify other players
	SendActionToPlayers(Player);
}

// Update the battle system for the server
void ServerBattleClass::Update(u32 FrameTime) {

	switch(State) {
		case STATE_BATTLE:
			for(u32 i = 0; i < Fighters.size(); i++) {
				FighterClass *Fighter = Fighters[i];
				if(Fighter) {
					Fighter->UpdateTurnTimer(FrameTime);
					
					// Update AI
					if(Fighter->UpdateAI(FrameTime)) {
						SendActionToPlayers(Fighter);
					}

					// Take a turn
					if(Fighter->TurnTimerReady() && Fighter->GetBattleAction() != NULL && Fighter->GetHealth() > 0) {
						ResolveAction(Fighter);
					}
				}
			}
		break;
		case STATE_END:
		break;
	}
}

// Resolves an action for a fighter
void ServerBattleClass::ResolveAction(FighterClass *Fighter) {

	// Get action used
	const ActionClass *Action = Fighter->GetBattleAction();
	if(Action == NULL)
		return;

	// Reset fighter state
	Fighter->ResetTurnTimer();
	Fighter->SetBattleAction(NULL);

	// Generate damage
	int Damage = Action->GenerateDamage(Fighter);

	// Generate target defense
	FighterClass *TargetFighter = Fighters[Fighter->GetTarget()];
	int Defense = TargetFighter->GenerateDefense();

	// Subtract defense
	Damage -= Defense;
	if(Damage < 0)
		Damage = 0;

	// Take damage
	TargetFighter->UpdateHealth(-Damage);

	// Get results
	BattleUpdateStruct Update;
	Update.Fighter = Fighter->GetSlot();
	Update.Giver = true;
	Update.DamageDealt = Damage;
	Update.TurnTimer = Fighter->GetTurnTimer();
	Update.TurnTimerMax = Fighter->GetTurnTimerMax();

	// Get target results
	BattleUpdateStruct TargetUpdate;
	TargetUpdate.Fighter = Fighter->GetTarget();
	TargetUpdate.Giver = false;
	TargetUpdate.DamageTaken = Damage;
	TargetUpdate.TurnTimer = TargetFighter->GetTurnTimer();
	TargetUpdate.TurnTimerMax = TargetFighter->GetTurnTimerMax();

	// Send updates
	SendUpdateToPlayers(Update);
	SendUpdateToPlayers(TargetUpdate);

	// End?
	CheckEnd();
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
					Stats.GenerateMonsterDrops(Monsters[i]->GetID(), 1, MonsterDrops);
				}

				// Get a list of players that receive items
				array<PlayerClass *> LeftSidePlayers;
				GetPlayerList(0, LeftSidePlayers);

				// Give out rewards round robin style
				u32 PlayerIndex = Random.GenerateRange(0, (int)(LeftSidePlayers.size()-1));
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
				Players[i]->AddItem(Stats.GetItem(ItemID), 1, -1);
			}
			
			ServerNetwork->SendPacketToPeer(&Packet, Players[i]->GetPeer());
		}

		State = STATE_END;
	}
}

// Sends a fighter update to all players
void ServerBattleClass::SendUpdateToPlayers(BattleUpdateStruct &Update) {
	FighterClass *Fighter = Fighters[Update.Fighter];

	PacketClass Packet(NetworkClass::BATTLE_UPDATE);
	Packet.WriteChar(Update.Fighter);
	Packet.WriteChar(Update.Giver);
	Packet.WriteInt(Update.DamageDealt);
	Packet.WriteInt(Update.DamageTaken);
	Packet.WriteInt(Update.TurnTimer);
	Packet.WriteInt(Update.TurnTimerMax);
	Packet.WriteInt(Fighter->GetHealth());
	Packet.WriteInt(Fighter->GetMana());

	// Send packet
	SendPacketToPlayers(&Packet);
}

// Send a packet to all players
void ServerBattleClass::SendPacketToPlayers(PacketClass *Packet) {
	
	// Send packet to all players
	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetType() == FighterClass::TYPE_PLAYER) {
			PlayerClass *Player = static_cast<PlayerClass *>(Fighters[i]);
			ServerNetwork->SendPacketToPeer(Packet, Player->GetPeer());
		}
	}	
}

// Sends an action to the other players
void ServerBattleClass::SendActionToPlayers(FighterClass *Fighter) {
	const ActionClass *Action = Fighter->GetBattleAction();
	if(Action == NULL)
		return;

	// Build packet
	PacketClass Packet(NetworkClass::BATTLE_ACTION);
	Packet.WriteChar(Fighter->GetSlot());
	Packet.WriteChar(Action->GetType());
	Packet.WriteChar(Action->GetID());

	// Send packet to all players
	for(u32 i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighter != Fighters[i] && Fighters[i]->GetType() == FighterClass::TYPE_PLAYER) {
			PlayerClass *Player = static_cast<PlayerClass *>(Fighters[i]);
			ServerNetwork->SendPacketToPeer(&Packet, Player->GetPeer());
		}
	}
}
