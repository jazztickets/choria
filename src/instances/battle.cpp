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
#include <scripting.h>
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
#include <glm/gtc/type_ptr.hpp>

// Constructor
_Battle::_Battle() :
	Stats(nullptr),
	Server(nullptr),
	Scripting(nullptr),
	ClientNetwork(nullptr),
	ClientPlayer(nullptr),
	State(STATE_NONE),
	Done(false),
	Time(0),
	WaitTimer(0),
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
					Fighter->UpdateAI(Scripting, Fighters, FrameTime);

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
				WaitTimer += FrameTime;
				if(WaitTimer >= BATTLE_WAITDEADTIME)
					ServerEndBattle();
			}
		}
		else {

			// Update action results
			for(auto Iterator = ActionResults.begin(); Iterator != ActionResults.end(); ) {
				_ActionResult &ActionResult = *Iterator;

				// Find start position
				glm::vec2 StartPosition = ActionResult.SourceFighter->ResultPosition - glm::vec2(ActionResult.SourceFighter->Portrait->Size.x/2 + ActionResult.Texture->Size.x/2 + 10, 0);
				ActionResult.LastPosition = ActionResult.Position;

				// Interpolate between start and end position of action used
				ActionResult.Position = glm::mix(StartPosition, ActionResult.TargetFighter->ResultPosition, std::min(ActionResult.Time * ACTIONRESULT_SPEED / ACTIONRESULT_TIMEOUT, 1.0));
				if(ActionResult.Time == 0)
					ActionResult.LastPosition = ActionResult.Position;

				ActionResult.Time += FrameTime;
				if(ActionResult.Time >= ACTIONRESULT_TIMEOUT) {
					Iterator = ActionResults.erase(Iterator);
				}
				else
					++Iterator;
			}
		}
	}

	Time += FrameTime;
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
			RenderBattle(BlendFactor);
		break;
	}
}

// Renders the battle part
void _Battle::RenderBattle(double BlendFactor) {
	BattleElement->Render();

	// Draw fighters
	for(auto &Fighter : Fighters) {
		Fighter->RenderBattle(ClientPlayer, Time);
	}

	// Draw action results
	for(auto &ActionResult : ActionResults) {
		RenderActionResults(ActionResult, BlendFactor);
	}
}

// Render results of an action
void _Battle::RenderActionResults(_ActionResult &ActionResult, double BlendFactor) {
	if(!ActionResult.TargetFighter || !ActionResult.SourceFighter)
		return;

	// Get alpha
	double TimeLeft = ACTIONRESULT_TIMEOUT - ActionResult.Time;
	float AlphaPercent = 1.0f;
	if(TimeLeft < ACTIONRESULT_FADETIME)
		AlphaPercent = TimeLeft / ACTIONRESULT_FADETIME;

	// Get final draw position
	glm::vec2 DrawPosition = glm::mix(ActionResult.LastPosition, ActionResult.Position, BlendFactor);

	// Draw icon
	glm::vec4 WhiteAlpha = glm::vec4(0.5f, 0.5f, 0.5f, AlphaPercent);
	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	Graphics.DrawCenteredImage(DrawPosition, ActionResult.Texture, WhiteAlpha);

	// Draw damage dealt
	glm::vec4 TextColor = COLOR_WHITE;
	if(ActionResult.TargetHealthChange > 0)
		TextColor = COLOR_GREEN;

	std::stringstream Buffer;
	Buffer << std::abs(ActionResult.TargetHealthChange);
	Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + glm::vec2(0, 7), TextColor, CENTER_BASELINE);
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
void _Battle::ClientSetAction(uint8_t ActionBarSlot) {

	// Check for dead
	if(ClientPlayer->Health == 0)
		return;

	// Player already locked in
	if(ClientPlayer->BattleAction.IsSet())
		return;

	// Get skill
	_Action Action = ClientPlayer->ActionBar[ActionBarSlot];

	// Check for changing an action
	bool ChangingAction = false;
	if(ClientPlayer->PotentialAction.IsSet() && Action != ClientPlayer->PotentialAction)
		ChangingAction = true;

	// Choose an action to use
	if(!ClientPlayer->PotentialAction.IsSet() || ChangingAction) {

		const _Skill *Skill = ClientPlayer->ActionBar[ActionBarSlot].Skill;
		if(Skill && !Skill->CanUse(Scripting, ClientPlayer))
			Skill = nullptr;

		const _Item *Item = ClientPlayer->ActionBar[ActionBarSlot].Item;
		if(Item && (Item->Type != _Item::TYPE_POTION || ClientPlayer->ActionBar[ActionBarSlot].Count == 0))
			Item = nullptr;

		// Set up initial target
		if(Skill || Item) {

			// Get opposite side
			int StartingSide = !ClientPlayer->BattleSide;

			// Pick sides depending on action
			bool TargetAlive = true;
			bool Multiple = false;
			if(Skill) {
				switch(Skill->TargetID) {
					case TargetType::ALLY_ALL:
						Multiple = true;
					case TargetType::SELF:
					case TargetType::ALLY:
						StartingSide = ClientPlayer->BattleSide;
					break;
					case TargetType::ENEMY_ALL:
						Multiple = true;
					break;
					default:
					break;
				}

				TargetAlive = Skill->TargetAlive;
			}
			else if(Item && Item->Type == _Item::TYPE_POTION) {
				StartingSide = ClientPlayer->BattleSide;
			}

			// Get list of fighters on each side
			std::list<_Object *> FighterList;
			GetFighterList(StartingSide, FighterList);

			// Find targets
			ClientPlayer->BattleTargets.clear();
			for(auto &Fighter :  FighterList) {
				if((TargetAlive && Fighter->Health > 0) || !TargetAlive) {
					ClientPlayer->BattleTargets.push_back(Fighter);
					if(!Multiple)
						break;
				}
			}
		}

		// Set potential skill
		ClientPlayer->PotentialAction.Skill = Skill;
		ClientPlayer->PotentialAction.Item = Item;
	}
	// Apply action
	else if(ClientPlayer->BattleTargets.size()) {

		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::BATTLE_SETACTION);
		Packet.Write<uint8_t>(ActionBarSlot);
		Packet.Write<uint8_t>(ClientPlayer->BattleTargets.size());
		for(const auto &BattleTarget : ClientPlayer->BattleTargets)
			Packet.Write<uint8_t>(BattleTarget->BattleID);

		ClientNetwork->SendPacket(Packet);

		ClientPlayer->BattleAction.Skill = ClientPlayer->ActionBar[ActionBarSlot].Skill;
		ClientPlayer->BattleAction.Item = ClientPlayer->ActionBar[ActionBarSlot].Item;
		ClientPlayer->PotentialAction.Unset();
	}
}

// Changes targets
void _Battle::ChangeTarget(int Direction, int SideDirection) {
	if(!ClientNetwork || !ClientPlayer->PotentialAction.IsSet() || ClientPlayer->Health == 0 || ClientPlayer->BattleTargets.size() > 1)
		return;

	// Get current target side
	int BattleTargetSide = ClientPlayer->BattleTargets.front()->BattleSide;

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
		auto Iterator = std::find(FighterList.begin(), FighterList.end(), ClientPlayer->BattleTargets.front());

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

	ClientPlayer->BattleTargets.clear();
	ClientPlayer->BattleTargets.push_back(NewTarget);
}

// Resolves an action and sends the result to each player
void _Battle::ServerResolveAction(_Object *SourceFighter) {
	if(SourceFighter->Health <= 0)
		return;

	_ActionResult ActionResult;
	ActionResult.SourceFighter = SourceFighter;
	ActionResult.SkillUsed = SourceFighter->BattleAction.Skill;
	ActionResult.ItemUsed = SourceFighter->BattleAction.Item;

	// Use item
	int Index = -1;
	if(ActionResult.ItemUsed && ActionResult.SourceFighter->FindItem(ActionResult.ItemUsed, Index)) {
		ActionResult.SourceFighter->UpdateInventory(Index, -1);
	}

	if(ActionResult.SkillUsed) {
		if(!ActionResult.SkillUsed->CanUse(Scripting, SourceFighter))
			return;

		ActionResult.SkillUsed->ApplyCost(Scripting, SourceFighter, ActionResult);
	}

	ActionResult.SourceFighter->UpdateHealth(ActionResult.SourceHealthChange);
	ActionResult.SourceFighter->UpdateMana(ActionResult.SourceManaChange);

	// Build packet for results
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::BATTLE_ACTIONRESULTS);

	// Source fighter
	Packet.Write<uint8_t>(ActionResult.SourceFighter->BattleID);

	// Write action used
	uint32_t SkillID = ActionResult.SkillUsed ? ActionResult.SkillUsed->ID : 0;
	uint32_t ItemID = ActionResult.ItemUsed ? ActionResult.ItemUsed->ID : 0;
	Packet.Write<uint32_t>(SkillID);
	Packet.Write<uint32_t>(ItemID);
	Packet.Write<int32_t>(ActionResult.SourceHealthChange);
	Packet.Write<int32_t>(ActionResult.SourceManaChange);
	Packet.Write<int32_t>(ActionResult.SourceFighter->Health);
	Packet.Write<int32_t>(ActionResult.SourceFighter->Mana);

	// Update each target
	Packet.Write<uint8_t>(SourceFighter->BattleTargets.size());
	for(auto &BattleTarget : SourceFighter->BattleTargets) {
		ActionResult.TargetFighter = BattleTarget;

		// Update fighters
		if(ActionResult.SkillUsed && ActionResult.SkillUsed->Script.length()) {
			Scripting->StartMethodCall(ActionResult.SkillUsed->Script, "ResolveBattleUse");
			Scripting->PushInt(ActionResult.SourceFighter->SkillLevels[ActionResult.SkillUsed->ID]);
			Scripting->PushObject(ActionResult.SourceFighter);
			Scripting->PushObject(ActionResult.TargetFighter);
			Scripting->PushActionResult(&ActionResult);
			Scripting->MethodCall(4, 1);
			Scripting->GetActionResult(1, ActionResult);
			Scripting->FinishMethodCall();
		}
		else if(ActionResult.ItemUsed) {
			ActionResult.TargetHealthChange = ActionResult.ItemUsed->HealthRestore;
			ActionResult.TargetManaChange = ActionResult.ItemUsed->ManaRestore;
		}

		// Update target
		ActionResult.TargetFighter->UpdateHealth(ActionResult.TargetHealthChange);
		ActionResult.TargetFighter->UpdateMana(ActionResult.TargetManaChange);

		// Update source health and mana
		//int HealthUpdate, ManaUpdate;
		//SourceFighter->UpdateRegen(HealthUpdate, ManaUpdate);
		//ActionResult.SourceHealthChange += HealthUpdate;
		//ActionResult.SourceManaChange += ManaUpdate;

		Packet.Write<char>(ActionResult.TargetFighter->BattleID);
		Packet.Write<int32_t>(ActionResult.DamageDealt);
		Packet.Write<int32_t>(ActionResult.TargetHealthChange);
		Packet.Write<int32_t>(ActionResult.TargetManaChange);
		Packet.Write<int32_t>(ActionResult.TargetFighter->Health);
		Packet.Write<int32_t>(ActionResult.TargetFighter->Mana);
	}

	// Send packet
	BroadcastPacket(Packet);

	// Reset fighter
	SourceFighter->TurnTimer = 0.0;
	SourceFighter->BattleAction.Unset();
	SourceFighter->BattleTargets.clear();
}

// Displays turn results from the server
void _Battle::ClientResolveAction(_Buffer &Data) {

	_ActionResult ActionResult;
	uint8_t SourceBattleID = Data.Read<uint8_t>();
	uint32_t SkillID = Data.Read<uint32_t>();
	uint32_t ItemID = Data.Read<uint32_t>();
	ActionResult.SkillUsed = Stats->Skills[SkillID];
	ActionResult.ItemUsed = Stats->Items[ItemID];
	ActionResult.SourceHealthChange = Data.Read<int32_t>();
	ActionResult.SourceManaChange = Data.Read<int32_t>();
	int SourceFighterHealth = Data.Read<int32_t>();
	int SourceFighterMana = Data.Read<int32_t>();

	// Set texture
	if(ActionResult.SkillUsed)
		ActionResult.Texture = ActionResult.SkillUsed->Texture;
	else if(ActionResult.ItemUsed)
		ActionResult.Texture = ActionResult.ItemUsed->Image;
	else
		ActionResult.Texture = Assets.Textures["skills/attack.png"];

	// Update source fighter
	ActionResult.SourceFighter = GetObjectByID(SourceBattleID);
	if(ActionResult.SourceFighter) {
		ActionResult.SourceFighter->Health = SourceFighterHealth;
		ActionResult.SourceFighter->Mana = SourceFighterMana;
		ActionResult.SourceFighter->TurnTimer = 0.0;
		ActionResult.SourceFighter->BattleAction.Unset();
		ActionResult.SourceFighter->BattleTargets.clear();

		// Use item on client
		if(ClientPlayer == ActionResult.SourceFighter && ActionResult.ItemUsed) {
			int Index = -1;
			if(ClientPlayer->FindItem(ActionResult.ItemUsed, Index)) {
				ClientPlayer->UpdateInventory(Index, -1);
				ClientPlayer->RefreshActionBarCount();
			}
		}
	}

	// Update targets
	uint8_t TargetCount = Data.Read<uint8_t>();
	for(uint8_t i = 0; i < TargetCount; i++) {
		uint8_t TargetBattleID = Data.Read<uint8_t>();
		ActionResult.DamageDealt = Data.Read<int32_t>();
		ActionResult.TargetHealthChange = Data.Read<int32_t>();
		ActionResult.TargetManaChange = Data.Read<int32_t>();
		int TargetFighterHealth = Data.Read<int32_t>();
		int TargetFighterMana = Data.Read<int32_t>();

		// Update target fighter
		ActionResult.TargetFighter = GetObjectByID(TargetBattleID);
		if(ActionResult.TargetFighter) {
			ActionResult.TargetFighter->Health = TargetFighterHealth;
			ActionResult.TargetFighter->Mana = TargetFighterMana;
		}

		ActionResults.push_back(ActionResult);
	}
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
	Fighter->BattleTargets.clear();
	Fighter->BattleAction.Unset();
	Fighter->PotentialAction.Unset();

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

	// Build packet
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::BATTLE_START);

	// Write fighter count
	int FighterCount = Fighters.size();
	Packet.Write<int>(FighterCount);

	// Write fighter information
	for(auto &Fighter : Fighters) {
		Fighter->TurnTimer = GetRandomReal(0, BATTLE_MAX_START_TURNTIMER);
		//Fighter->TurnTimer = 1;

		// Write fighter type
		Packet.Write<uint32_t>(Fighter->DatabaseID);
		Packet.Write<char>(Fighter->BattleSide);
		Packet.Write<double>(Fighter->TurnTimer);

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
			SideStats[Side].TotalExperienceGiven += Fighter->ExperienceGiven;
			SideStats[Side].TotalGoldGiven += Fighter->GoldGiven;
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
		Fighter->CalculateStats();
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
			Fighter->AddItem(Stats->Items[ItemID], 1, -1);
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
		const _Item *Item = Stats->Items[ItemID];
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
	Time = 0.0;
}

// Calculates a screen position for a slot
void _Battle::GetBattleOffset(int SideIndex, _Object *Fighter) {
	_Element *BattleElement = Assets.Elements["element_battle"];
	glm::vec2 Center = (BattleElement->Bounds.End + BattleElement->Bounds.Start) / 2;

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
			if(Time > BATTLE_WAITENDTIME) {
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
					//ChangeTarget(0, -1);
				break;
				case _Actions::RIGHT:
					//ChangeTarget(0, 1);
				break;
			}
		break;
	}

	return false;
}

// Handles input from the client
void _Battle::ServerHandleAction(_Object *Fighter, _Buffer &Data) {

	// Check for needed commands
	if(!Fighter->BattleAction.IsSet()) {

		uint8_t ActionBarSlot = Data.Read<uint8_t>();
		uint8_t TargetCount = Data.Read<uint8_t>();

		// Get targets
		Fighter->BattleTargets.clear();
		for(uint8_t i = 0; i < TargetCount; i++) {
			uint8_t TargetID = Data.Read<uint8_t>();
			_Object *Target = GetObjectByID(TargetID);
			if(Target)
				Fighter->BattleTargets.push_back(Target);
		}

		// Set skill
		uint32_t SkillID = 0;
		uint32_t ItemID = 0;
		if(ActionBarSlot < Fighter->ActionBar.size()) {
			Fighter->BattleAction.Skill = Fighter->ActionBar[ActionBarSlot].Skill;
			Fighter->BattleAction.Item = Fighter->ActionBar[ActionBarSlot].Item;
			if(Fighter->BattleAction.Skill)
				SkillID = Fighter->BattleAction.Skill->ID;
			else if(Fighter->BattleAction.Item)
				ItemID = Fighter->BattleAction.Item->ID;
		}

		// Notify other players of action
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::BATTLE_ACTION);
		Packet.Write<char>(Fighter->BattleID);
		Packet.Write<uint32_t>(SkillID);
		Packet.Write<uint32_t>(ItemID);

		BroadcastPacket(Packet);
	}
}

// Handle action from another player
void _Battle::ClientHandlePlayerAction(_Buffer &Data) {

	int BattleID = Data.Read<char>();
	int SkillID = Data.Read<uint32_t>();
	int ItemID = Data.Read<uint32_t>();

	_Object *Fighter = GetObjectByID(BattleID);
	if(Fighter) {
		Fighter->BattleAction.Skill = Stats->Skills[SkillID];
		Fighter->BattleAction.Item = Stats->Items[ItemID];
	}
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
