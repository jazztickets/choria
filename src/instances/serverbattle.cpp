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
#include <instances/serverbattle.h>
#include <globals.h>
#include <stats.h>
#include <constants.h>
#include <network/packetstream.h>
#include <network/network.h>
#include <random.h>
#include <instances.h>
#include <objects/player.h>
#include <objects/monster.h>

// Constructor
ServerBattleClass::ServerBattleClass()
:	_Battle() {

	RoundTime = 0;
}

// Destructor
ServerBattleClass::~ServerBattleClass() {

}

// Removes a player from the battle
int ServerBattleClass::RemovePlayer(_Player *TPlayer) {

	int Count = 0;
	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetType() == _Fighter::TYPE_PLAYER) {
			_Player *Player = static_cast<_Player *>(Fighters[i]);

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
	_Packet Packet(_Network::WORLD_STARTBATTLE);

	// Write fighter count
	int FighterCount = Fighters.size();
	Packet.WriteChar(FighterCount);

	// Write fighter information
	for(int i = 0; i < FighterCount; i++) {

		// Write fighter type
		int Type = Fighters[i]->GetType();
		Packet.WriteBit(!!Type);
		Packet.WriteBit(!!Fighters[i]->GetSide());

		if(Type == _Fighter::TYPE_PLAYER) {
			_Player *Player = static_cast<_Player *>(Fighters[i]);

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
			_Monster *Monster = static_cast<_Monster *>(Fighters[i]);

			// Monster ID
			Packet.WriteInt(Monster->GetID());
		}
	}

	// Send packet to players
	SendPacketToPlayers(&Packet);

	State = STATE_INPUT;
}

// Handles input from the client
void ServerBattleClass::HandleInput(_Player *TPlayer, int TCommand, int TTarget) {

	if(State == STATE_INPUT) {

		// Check for needed commands
		if(TPlayer->GetCommand() == -1) {
			TPlayer->SetCommand(TCommand);
			TPlayer->SetTarget(TTarget);

			// Notify other players
			SendSkillToPlayers(TPlayer);

			// Check for all commands
			bool Ready = true;
			for(size_t i = 0; i < Fighters.size(); i++) {
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
void ServerBattleClass::Update(uint32_t TDeltaTime) {

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
	std::vector<_Monster *> Monsters;
	GetMonsterList(Monsters);

	// Update AI
	if(Monsters.size() > 0) {

		// Get a list of humans on the left side
		std::vector<_Fighter *> Humans;
		GetAliveFighterList(0, Humans);

		// Update the monster's target
		for(size_t i = 0; i < Monsters.size(); i++) {
			Monsters[i]->UpdateTarget(Humans);
		}
	}

	// Handle each fighter's action
	_FighterResult Results[BATTLE_MAXFIGHTERS];
	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i]) {
			_FighterResult *Result = &Results[i];
			Result->Fighter = Fighters[i];

			// Ignore dead fighters
			if(Fighters[i]->GetHealth() > 0) {
				Result->Target = Fighters[i]->GetTarget();

				// Get skill used
				const _Skill *Skill = Fighters[i]->GetSkillBar(Fighters[i]->GetCommand());
				if(Skill && Skill->CanUse(Result->Fighter)) {
					int TargetFighterIndex = GetFighterFromSlot(Result->Target);
					Result->SkillID = Skill->GetID();

					// Update fighters
					_FighterResult *TargetResult = &Results[TargetFighterIndex];
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
	_Packet Packet(_Network::BATTLE_TURNRESULTS);
	for(size_t i = 0; i < Fighters.size(); i++) {
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
	std::vector<_Player *> Players;

	// Get statistics for each side
	_BattleResult Side[2];
	for(int i = 0; i < 2; i++) {

		// Get a list of fighters that are still in the battle
		std::vector<_Fighter *> SideFighters;
		GetFighterList(i, SideFighters);

		// Loop through fighters
		for(size_t j = 0; j < SideFighters.size(); j++) {

			// Keep track of players
			if(SideFighters[j]->GetType() == _Fighter::TYPE_PLAYER) {
				Players.push_back(static_cast<_Player *>(SideFighters[j]));
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
		std::vector<int> PlayerItems[3];
		if(!Side[0].Dead) {

			// Get a monster list
			std::vector<_Monster *> Monsters;
			GetMonsterList(Monsters);

			// Make sure there are monsters
			if(Monsters.size() > 0) {

				// Generate monster drops in player vs monster situations
				std::vector<int> MonsterDrops;
				for(size_t i = 0; i < Monsters.size(); i++) {
					Stats.GenerateMonsterDrops(Monsters[i]->GetID(), 1, MonsterDrops);
				}

				// Get a list of players that receive items
				std::vector<_Player *> LeftSidePlayers;
				GetPlayerList(0, LeftSidePlayers);

				// Give out rewards round robin style
				std::uniform_int_distribution<size_t> Distribution(0, LeftSidePlayers.size()-1);
				size_t PlayerIndex = Distribution(RandomGenerator);
				for(size_t i = 0; i < MonsterDrops.size(); i++) {
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
		for(size_t i = 0; i < Players.size(); i++) {

			// Get rewards
			int ExperienceEarned = 0;
			int GoldEarned = 0;
			_BattleResult *PlayerSide = &Side[Players[i]->GetSide()];
			_BattleResult *OppositeSide = &Side[!Players[i]->GetSide()];
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
			_Packet Packet(_Network::BATTLE_END);
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
	else
		State = STATE_INPUT;
}

// Send a packet to all players
void ServerBattleClass::SendPacketToPlayers(_Packet *TPacket) {

	// Send packet to all players
	for(size_t i = 0; i < Fighters.size(); i++) {
		if(Fighters[i] && Fighters[i]->GetType() == _Fighter::TYPE_PLAYER) {
			_Player *Player = static_cast<_Player *>(Fighters[i]);
			ServerNetwork->SendPacketToPeer(TPacket, Player->GetPeer());
		}
	}
}

// Send the player's skill to the other players
void ServerBattleClass::SendSkillToPlayers(_Player *TPlayer) {

	// Get all the players on the player's side
	std::vector<_Player *> SidePlayers;
	GetPlayerList(TPlayer->GetSide(), SidePlayers);
	if(SidePlayers.size() == 1)
		return;

	// Get skill id
	const _Skill *Skill = TPlayer->GetSkillBar(TPlayer->GetCommand());
	int SkillID = -1;
	if(Skill)
		SkillID = Skill->GetID();

	// Build packet
	_Packet Packet(_Network::BATTLE_COMMAND);
	Packet.WriteChar(TPlayer->GetSlot());
	Packet.WriteChar(SkillID);

	// Send packet to all players
	for(size_t i = 0; i < SidePlayers.size(); i++) {
		if(SidePlayers[i] != TPlayer) {
			ServerNetwork->SendPacketToPeer(&Packet, SidePlayers[i]->GetPeer());
		}
	}
}
