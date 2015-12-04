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
#include <server.h>
#include <actions.h>
#include <buffer.h>
#include <graphics.h>
#include <stats.h>
#include <program.h>
#include <assets.h>
#include <font.h>
#include <packet.h>
#include <random.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>

// Constructor
_Battle::_Battle() :
	Stats(nullptr),
	Server(nullptr),
	ClientNetwork(nullptr),
	ClientPlayer(nullptr),
	State(STATE_NONE),
	Done(false),
	Timer(0),
	NextID(0),
	ClientExperienceReceived(0),
	ClientGoldReceived(0),
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

	if(!Done) {

		// Update fighters
		int AliveCount[2] = { 0, 0 };
		for(auto &Fighter : Fighters) {

			// Count alive fighters for each side
			if(Fighter->Health > 0) {
				AliveCount[Fighter->BattleSide]++;

				// Update AI
				if(Server)
					Fighter->UpdateAI(Server->Scripting, Fighters, FrameTime);

				// Check turn timer
				Fighter->TurnTimer += FrameTime * Fighter->BattleSpeed;
				if(Fighter->TurnTimer > 1.0) {
					Fighter->TurnTimer = 1.0;

					if(Server) {
						if(Fighter->BattleAction.IsSet()) {
							ServerResolveAction(Fighter);
						}
					}
				}
			}
			else
				Fighter->TurnTimer = 0;
		}

		// Check for end
		if(Server) {
			if(AliveCount[0] == 0 || AliveCount[1] == 0) {
				ServerEndBattle();
			}
		}
		else {

			// Update action results
			for(auto Iterator = ActionResults.begin(); Iterator != ActionResults.end(); ) {
				_ActionResult &ActionResult = *Iterator;
				ActionResult.Time += FrameTime;
				if(ActionResult.Time >= ACTIONRESULT_TIMEOUT) {
					Iterator = ActionResults.erase(Iterator);
				}
				else
					++Iterator;
			}
		}
	}

	Timer += FrameTime;
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
		Fighter->RenderBattle(ClientPlayer->BattleTarget == Fighter, ClientPlayer->BattleSide == Fighter->BattleSide);
	}

	// Draw action results
	for(auto &ActionResult : ActionResults) {
		RenderActionResults(ActionResult);
	}
}

// Render results of an action
void _Battle::RenderActionResults(_ActionResult &ActionResult) {
	if(!ActionResult.TargetFighter || !ActionResult.SourceFighter)
		return;

	glm::ivec2 TargetDrawPosition = ActionResult.TargetFighter->ResultPosition;
	glm::ivec2 SourceDrawPosition = ActionResult.SourceFighter->ResultPosition;
	TargetDrawPosition.y -= ActionResult.Time * ACTIONRESULT_SPEED;

	// Get alpha
	double TimeLeft = ACTIONRESULT_TIMEOUT - ActionResult.Time;
	float AlphaPercent = 1.0f;
	if(TimeLeft < ACTIONRESULT_FADETIME)
		AlphaPercent = TimeLeft / ACTIONRESULT_FADETIME;

	// Get skill used
	const _Texture *SkillTexture;
	if(ActionResult.SkillUsed)
		SkillTexture = ActionResult.SkillUsed->Image;
	else
		SkillTexture = Assets.Textures["skills/attack.png"];

	// Get color
	glm::vec4 Color = COLOR_WHITE;
	Color.a = AlphaPercent;

	// Draw skill icon
	glm::vec4 WhiteAlpha = glm::vec4(0.5f, 0.5f, 0.5f, AlphaPercent);
	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	Graphics.DrawCenteredImage(TargetDrawPosition, SkillTexture, WhiteAlpha);

	glm::ivec2 BattleActionUsedPosition = SourceDrawPosition - glm::ivec2(ActionResult.SourceFighter->Portrait->Size.x/2 + SkillTexture->Size.x/2 + 10, 0);
	Graphics.DrawCenteredImage(BattleActionUsedPosition, SkillTexture, Color);

	// Draw damage dealt
	std::stringstream Buffer;
	Buffer << std::abs(ActionResult.TargetHealthChange);
	Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), TargetDrawPosition + glm::ivec2(0, 7), Color, CENTER_BASELINE);
}

// Renders the battle win screen
void _Battle::RenderBattleWin() {
	BattleWinElement->SetVisible(true);
	Assets.Labels["label_battlewin_experience"]->Text = std::to_string(ClientExperienceReceived) + " experience";
	Assets.Labels["label_battlewin_coins"]->Text = std::to_string(ClientGoldReceived) + " gold";
	Assets.Labels["label_battlewin_chest"]->Visible = ClientItemDrops.size() == 0;
	BattleWinElement->Render();

	// Draw item drops
	if(ClientItemDrops.size() > 0) {

		// Get positions
		_Image *Image = Assets.Images["image_battlewin_chest"];
		glm::ivec2 StartPosition = (Image->Bounds.Start + Image->Bounds.End) / 2;
		StartPosition.x += Image->Size.x;
		glm::ivec2 DrawPosition(StartPosition);

		// Draw items found
		for(auto &MonsterDrop : ClientItemDrops) {
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
	Assets.Labels["label_battlelose_gold"]->Text = "You lost " + std::to_string(std::abs(ClientGoldReceived)) + " gold";

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
	Packet.Write<PacketType>(PacketType::BATTLE_SETACTION);
	Packet.Write<char>(ActionBarSlot);

	ClientNetwork->SendPacket(Packet);
	ClientPlayer->BattleAction.Skill = ClientPlayer->ActionBar[ActionBarSlot];
}

// Changes targets
void _Battle::ChangeTarget(int Direction, int SideDirection) {
	if(!ClientNetwork || ClientPlayer->Health == 0)
		return;

	// Get current target side
	int BattleTargetSide = ClientPlayer->BattleTarget->BattleSide;

	// Change sides
	if(SideDirection != 0)
		BattleTargetSide = !BattleTargetSide;

	// Get list of fighters on target side
	std::list<_Object *> FighterList;
	GetFighterList(BattleTargetSide, FighterList);

	// Get new target
	_Object *NewTarget = nullptr;
	if(FighterList.size()) {

		// Get iterator to current target
		auto Iterator = std::find(FighterList.begin(), FighterList.end(), ClientPlayer->BattleTarget);

		if(Iterator == FighterList.end())
			Iterator = FighterList.begin();

		// Update target
		if(Direction > 0) {
			Iterator++;
			if(Iterator == FighterList.end())
				Iterator = FighterList.begin();

			NewTarget = *Iterator;
		}
		else if(Direction < 0) {
			if(Iterator == FighterList.begin())
				NewTarget = FighterList.back();
			else {
				Iterator--;
				NewTarget = *Iterator;
			}
		}
		else
			NewTarget = *Iterator;
	}

	ClientPlayer->BattleTarget = NewTarget;
	int BattleTargetID = -1;
	if(ClientPlayer->BattleTarget)
		BattleTargetID = ClientPlayer->BattleTarget->BattleID;

	// Send packet
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::BATTLE_CHANGETARGET);
	Packet.Write<char>(BattleTargetID);
	ClientNetwork->SendPacket(Packet);
}

// Resolves an action and sends the result to each player
void _Battle::ServerResolveAction(_Object *SourceFighter) {
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

	Packet.Write<char>(ActionResult.SourceFighter->BattleID);
	Packet.Write<char>(ActionResult.TargetFighter->BattleID);
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
	int SourceBattleID = Data.Read<char>();
	int TargetBattleID = Data.Read<char>();

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
	ActionResult.SourceFighter = GetObjectByID(SourceBattleID);
	if(ActionResult.SourceFighter) {
		ActionResult.SourceFighter->Health = SourceFighterHealth;
		ActionResult.SourceFighter->Mana = SourceFighterMana;
		ActionResult.SourceFighter->TurnTimer = 0.0;
		ActionResult.SourceFighter->BattleAction.Unset();
	}

	// Update target fighter
	ActionResult.TargetFighter = GetObjectByID(TargetBattleID);
	if(ActionResult.TargetFighter) {
		ActionResult.TargetFighter->Health = TargetFighterHealth;
		ActionResult.TargetFighter->Mana = TargetFighterMana;
	}

	ActionResults.push_back(ActionResult);

/*
	// Change targets if the old one died
	int TargetIndex = GetFighterFromSlot(ClientPlayer->Target);
	if(TargetIndex != -1 && Fighters[TargetIndex]->Health == 0)
		ChangeTarget(1);
*/
}

// Get the object pointer battle id
_Object *_Battle::GetObjectByID(int BattleID) {
	if(BattleID == -1)
		return nullptr;

	// Search fighters
	for(auto &Fighter : Fighters) {
		if(Fighter->BattleID == BattleID)
			return Fighter;
	}

	return nullptr;
}

// Add a fighter to the battle
void _Battle::AddFighter(_Object *Fighter, int Side) {
	Fighter->Battle = this;
	Fighter->BattleID = NextID++;
	Fighter->BattleSide = Side;
	Fighter->BattleAction.Unset();

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

// Starts the battle and notifies the players
void _Battle::ServerStartBattle() {

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
		Fighter->TurnTimer = GetRandomReal(0, BATTLE_MAX_START_TURNTIMER);

		// Write fighter type
		Packet.Write<int>(Fighter->DatabaseID);
		Packet.Write<char>(Fighter->BattleSide);
		Packet.Write<float>(Fighter->TurnTimer);

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

// Starts the battle on the client
void _Battle::ClientStartBattle() {
	BattleElement = Assets.Elements["element_battle"];
	BattleWinElement = Assets.Elements["element_battlewin"];
	BattleLoseElement = Assets.Elements["element_battlelose"];
	BattleElement->SetVisible(true);

	// Set up targets
	SetDefaultTargets();

	// Set fighter position offsets
	int SideCount[2] = { 0, 0 };
	for(auto &Fighter : Fighters) {
		GetBattleOffset(SideCount[Fighter->BattleSide], Fighter);
		SideCount[Fighter->BattleSide]++;
	}
}

// Checks for the end of a battle
void _Battle::ServerEndBattle() {

	// Get statistics for each side
	_BattleResult SideStats[2];
	std::list<_Object *> SideFighters[2];
	for(int Side = 0; Side < 2; Side++) {

		// Get a list of fighters that are still in the battle
		GetFighterList(Side, SideFighters[Side]);

		// Loop through fighters
		for(auto &Fighter : SideFighters[Side]) {

			// Keep track of players
			if(Fighter->DatabaseID == 0) {
				SideStats[Side].PlayerCount++;
			}
			else
				SideStats[Side].MonsterCount++;

			// Tally alive fighters
			if(Fighter->Health > 0) {
				SideStats[Side].Dead = false;
			}

			// Sum experience and gold
			SideStats[Side].TotalExperienceGiven += Fighter->GetExperienceGiven();
			SideStats[Side].TotalGoldGiven += Fighter->GetGoldGiven();
			SideStats[Side].FighterCount++;
		}
	}

	// Get winning side
	int WinningSide;
	if(SideStats[0].Dead && SideStats[1].Dead)
		WinningSide = -1;
	else if(SideStats[0].Dead)
		WinningSide = 1;
	else
		WinningSide = 0;

	// Check for a winning side
	if(WinningSide != -1 && SideFighters[WinningSide].size()) {

		// Divide up rewards
		for(int Side = 0; Side < 2; Side++) {
			int OtherSide = !Side;

			// Divide experience up
			if(SideStats[OtherSide].TotalExperienceGiven > 0) {
				SideStats[Side].ExperiencePerFighter = SideStats[OtherSide].TotalExperienceGiven / SideStats[Side].FighterCount;
				if(SideStats[Side].ExperiencePerFighter <= 0)
					SideStats[Side].ExperiencePerFighter = 1;
			}

			// Divide gold up
			if(SideStats[OtherSide].TotalGoldGiven > 0) {
				SideStats[Side].GoldPerFighter = SideStats[OtherSide].TotalGoldGiven / SideStats[Side].FighterCount;
				if(SideStats[Side].GoldPerFighter <= 0)
					SideStats[Side].GoldPerFighter = 1;
			}
		}

		// Convert winning side list to array
		std::vector<_Object *> FighterArray { std::begin(SideFighters[WinningSide]), std::end(SideFighters[WinningSide]) };

		// Generate items drops
		std::list<uint32_t> ItemDrops;
		for(auto &Fighter : SideFighters[!WinningSide]) {
			if(Fighter->DatabaseID)
				Stats->GenerateItemDrops(Fighter->DatabaseID, 1, ItemDrops);
		}

		// Give out drops
		for(auto &ItemID : ItemDrops) {
			std::shuffle(FighterArray.begin(), FighterArray.end(), RandomGenerator);
			_Object *Fighter = FighterArray[0];
			Fighter->ItemDropsReceived.push_back(ItemID);
		}
	}

	// Send data
	for(auto &Fighter : Fighters) {
		Fighter->InputState = 0;

		// Get rewards
		int ExperienceEarned = 0;
		int GoldEarned = 0;
		if(Fighter->BattleSide != WinningSide) {
			GoldEarned = (int)(-Fighter->Gold * 0.1f);
			Fighter->Deaths++;

			if(Fighter->Peer)
				Server->SendMessage(Fighter->Peer, std::string("You lost " + std::to_string(std::abs(GoldEarned)) + " gold"), COLOR_RED);
		}
		else {
			ExperienceEarned = SideStats[WinningSide].ExperiencePerFighter;
			GoldEarned = SideStats[WinningSide].GoldPerFighter;
			Fighter->PlayerKills += SideStats[!WinningSide].PlayerCount;
			Fighter->MonsterKills += SideStats[!WinningSide].MonsterCount;
		}

		// Update stats
		int CurrentLevel = Fighter->Level;
		Fighter->Experience += ExperienceEarned;
		Fighter->UpdateGold(GoldEarned);
		Fighter->CalculatePlayerStats();
		int NewLevel = Fighter->Level;
		if(NewLevel > CurrentLevel) {
			Fighter->RestoreHealthMana();
			if(Fighter->Peer)
				Server->SendMessage(Fighter->Peer, std::string("You are now level " + std::to_string(NewLevel) + "!"), COLOR_GOLD);
		}

		// Write results
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::BATTLE_END);
		Packet.WriteBit(SideStats[0].Dead);
		Packet.WriteBit(SideStats[1].Dead);
		Packet.Write<char>(SideStats[!Fighter->BattleSide].PlayerCount);
		Packet.Write<char>(SideStats[!Fighter->BattleSide].MonsterCount);
		Packet.Write<int32_t>(ExperienceEarned);
		Packet.Write<int32_t>(GoldEarned);

		// Write items
		int ItemCount = Fighter->ItemDropsReceived.size();
		Packet.Write<char>(ItemCount);

		// Write items
		for(auto &ItemID : Fighter->ItemDropsReceived) {
			Packet.Write<uint32_t>(ItemID);
			Fighter->AddItem(Stats->GetItem(ItemID), 1, -1);
		}
		Fighter->ItemDropsReceived.clear();

		// Send info
		if(Fighter->Peer) {
			Server->Network->SendPacket(Packet, Fighter->Peer);
			Server->SendHUD(Fighter->Peer);
		}
	}

	Done = true;
}

// End of a battle
void _Battle::ClientEndBattle(_Buffer &Data) {

	// Get ending stats
	bool SideDead[2];
	SideDead[0] = Data.ReadBit();
	SideDead[1] = Data.ReadBit();
	int PlayerKills = Data.Read<char>();
	int MonsterKills = Data.Read<char>();
	ClientExperienceReceived = Data.Read<int32_t>();
	ClientGoldReceived = Data.Read<int32_t>();
	int ItemCount = Data.Read<char>();
	for(int i = 0; i < ItemCount; i++) {
		int ItemID = Data.Read<int32_t>();
		const _Item *Item = Stats->GetItem(ItemID);
		ClientItemDrops.push_back(Item);
		ClientPlayer->AddItem(Item, 1, -1);
	}

	// Check win or death
	int PlayerSide = ClientPlayer->BattleSide;
	int OtherSide = !PlayerSide;
	if(!SideDead[PlayerSide] && SideDead[OtherSide]) {
		ClientPlayer->PlayerKills += PlayerKills;
		ClientPlayer->MonsterKills += MonsterKills;
		State = STATE_WIN;
	}
	else {
		ClientPlayer->Deaths++;
		State = STATE_LOSE;
	}

	Done = true;
	Timer = 0.0;
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

// Removes a player from the battle, return remaining player count
void _Battle::RemoveFighter(_Object *RemoveFighter) {

	// Remove action results
	for(auto Iterator = ActionResults.begin(); Iterator != ActionResults.end(); ) {
		_ActionResult &ActionResult = *Iterator;
		if(ActionResult.SourceFighter == RemoveFighter || ActionResult.TargetFighter == RemoveFighter) {
			Iterator = ActionResults.erase(Iterator);
		}
		else
			++Iterator;
	}

	// Remove fighters
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

	// Get list of fighters on each side
	std::list<_Object *> FighterList[2];
	GetFighterList(0, FighterList[0]);
	GetFighterList(1, FighterList[1]);

	// Set opposite targets
	for(int i = 0; i < 2; i++) {
		for(auto &Fighter :  FighterList[i]) {
			Fighter->BattleTarget = *FighterList[!i].begin();
		}
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

// Handle player input
bool _Battle::ClientHandleInput(int Action) {

	switch(State) {
		case STATE_WIN:
		case STATE_LOSE: {
			if(Timer > BATTLE_WAITENDTIME) {
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::BATTLE_CLIENTDONE);
				ClientNetwork->SendPacket(Packet);

				return true;
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

	return false;
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
		Packet.Write<char>(Fighter->BattleID);
		Packet.Write<int>(SkillID);
		Packet.Write<int>(0);

		BroadcastPacket(Packet);
	}
}

// Handle action from another player
void _Battle::ClientHandlePlayerAction(_Buffer &Data) {

	int BattleID = Data.Read<char>();
	int SkillID = Data.Read<int>();
	//int ItemID = Data.Read<int>();

	_Object *Fighter = GetObjectByID(BattleID);
	if(Fighter)
		Fighter->BattleAction.Skill = Stats->Skills[SkillID];
}

// Send a packet to all players
void _Battle::BroadcastPacket(_Buffer &Packet) {

	// Send packet to all players
	for(auto &Fighter : Fighters) {
		if(Fighter->Peer) {
			Server->Network->SendPacket(Packet, Fighter->Peer);
		}
	}
}
