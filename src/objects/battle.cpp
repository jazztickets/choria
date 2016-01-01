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
#include <objects/battle.h>
#include <network/servernetwork.h>
#include <network/clientnetwork.h>
#include <objects/object.h>
#include <objects/buff.h>
#include <objects/inventory.h>
#include <objects/statchange.h>
#include <objects/statuseffect.h>
#include <ui/element.h>
#include <ui/label.h>
#include <ui/image.h>
#include <constants.h>
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
	Manager(nullptr),
	Time(0),
	WaitTimer(0),
	SideCount{0, 0},
	BattleElement(nullptr) {

}

// Destructor
_Battle::~_Battle() {
	if(BattleElement)
		BattleElement->SetVisible(false);

	// Remove fighters
	for(auto &Fighter : Fighters) {
		if(Fighter->DatabaseID)
			Fighter->Deleted = true;
		Fighter->StopBattle();
	}
}

// Update battle
void _Battle::Update(double FrameTime) {

	// Check for end
	if(Server) {

		// Count alive fighters for each side
		int AliveCount[2] = { 0, 0 };
		for(auto &Fighter : Fighters) {
			if(Fighter->IsAlive())
				AliveCount[Fighter->BattleSide]++;
		}

		// Check for end conditions
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
			glm::vec2 StartPosition = ActionResult.Source.Object->ResultPosition - glm::vec2(ActionResult.Source.Object->Portrait->Size.x/2 + ActionResult.Texture->Size.x/2 + 10, 0);
			ActionResult.LastPosition = ActionResult.Position;

			// Interpolate between start and end position of action used
			ActionResult.Position = glm::mix(StartPosition, ActionResult.Target.Object->ResultPosition, std::min(ActionResult.Time * ActionResult.Speed / ActionResult.Timeout, 1.0));
			if(ActionResult.Time == 0.0)
				ActionResult.LastPosition = ActionResult.Position;

			// Update timer
			ActionResult.Time += FrameTime;
			if(ActionResult.Time >= ActionResult.Timeout) {
				Iterator = ActionResults.erase(Iterator);
			}
			else
				++Iterator;
		}

	}

	Time += FrameTime;
}

// Render the battle system
void _Battle::Render(double BlendFactor) {
	RenderBattle(BlendFactor);
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
	if(!ActionResult.Target.Object || !ActionResult.Source.Object)
		return;

	// Get alpha
	double TimeLeft = ActionResult.Timeout - ActionResult.Time;
	float AlphaPercent = 1.0f;
	if(TimeLeft < HUD_ACTIONRESULT_FADETIME)
		AlphaPercent = (float)(TimeLeft / HUD_ACTIONRESULT_FADETIME);

	// Get final draw position
	glm::vec2 DrawPosition = glm::mix(ActionResult.LastPosition, ActionResult.Position, BlendFactor);

	// Draw icon
	glm::vec4 WhiteAlpha = glm::vec4(0.5f, 0.5f, 0.5f, AlphaPercent);
	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	if(ActionResult.ActionUsed.Item)
		Graphics.DrawCenteredImage(DrawPosition, Assets.Textures["hud/item_back.png"], WhiteAlpha);
	Graphics.DrawCenteredImage(DrawPosition, ActionResult.Texture, WhiteAlpha);

	// Draw damage dealt
	glm::vec4 TextColor = COLOR_WHITE;
	if(ActionResult.Target.Values[StatType::HEALTH].Float > 0)
		TextColor = COLOR_GREEN;
	else if(ActionResult.Target.HasStat(StatType::CRIT))
		TextColor = COLOR_YELLOW;

	TextColor.a = AlphaPercent;

	std::stringstream Buffer;
	if(ActionResult.Target.HasStat(StatType::MISS))
		Buffer << "miss";
	else
		Buffer << std::abs(ActionResult.Target.Values[StatType::HEALTH].Float);
	Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + glm::vec2(0, 7), TextColor, CENTER_BASELINE);
}

// Sends an action selection to the server
void _Battle::ClientSetAction(uint8_t ActionBarSlot) {
	if(ActionBarSlot >= ClientPlayer->ActionBar.size())
		return;

	// Check for dead
	if(!ClientPlayer->IsAlive())
		return;

	// Player already locked in
	if(ClientPlayer->Action.IsSet())
		return;

	// Get skillbar action
	_Action Action = ClientPlayer->ActionBar[ActionBarSlot];
	ClientPlayer->GetActionFromSkillbar(Action, ActionBarSlot);

	// Check for changing an action
	bool ChangingAction = false;
	if(ClientPlayer->PotentialAction.IsSet() && Action != ClientPlayer->PotentialAction)
		ChangingAction = true;

	// Choose an action to use
	if(!ClientPlayer->PotentialAction.IsSet() || ChangingAction) {

		_ActionResult ActionResult;
		ActionResult.Source.Object = ClientPlayer;
		ActionResult.Scope = ScopeType::BATTLE;
		ActionResult.ActionUsed = Action;

		const _Item *Item = ClientPlayer->ActionBar[ActionBarSlot].Item;
		if(Item) {
			if(!Item->CanUse(Scripting, ActionResult))
				Item = nullptr;

			if(Item && !Item->IsSkill() && ClientPlayer->ActionBar[ActionBarSlot].Count == 0)
				Item = nullptr;
		}
		else
			Item = nullptr;

		// Set up initial target
		if(Item) {

			// Get opposite side
			int StartingSide = !ClientPlayer->BattleSide;

			// Pick sides depending on action
			bool Multiple = false;
			bool Self = false;
			switch(Item->TargetID) {
				case TargetType::SELF:
					Self = true;
				break;
				case TargetType::ALLY_ALL:
					Multiple = true;
				case TargetType::ALLY:
					StartingSide = ClientPlayer->BattleSide;
				break;
				case TargetType::ENEMY_ALL:
					Multiple = true;
				break;
				default:
				break;
			}

			bool TargetAlive = Item->TargetAlive;

			// Clear existing targets
			ClientPlayer->Targets.clear();

			if(Self) {
				ClientPlayer->Targets.push_back(ClientPlayer);
			}
			else {

				// Get list of fighters on each side
				std::list<_Object *> FighterList;
				GetFighterList(StartingSide, FighterList);

				// Find last target
				if(ClientPlayer->LastTarget && !Multiple) {
					for(auto &Fighter : FighterList) {
						if((TargetAlive && Fighter->IsAlive()) || !TargetAlive) {
							if(ClientPlayer->LastTarget == Fighter) {
								ClientPlayer->Targets.push_back(Fighter);
								break;
							}
						}
					}
				}

				// Find first alive target
				if(ClientPlayer->Targets.size() == 0) {
					for(auto &Fighter : FighterList) {
						if((TargetAlive && Fighter->IsAlive()) || !TargetAlive) {

							ClientPlayer->Targets.push_back(Fighter);
							if(!Multiple)
								break;
						}
					}
				}
			}
		}

		// Set potential skill
		ClientPlayer->PotentialAction.Item = Item;
	}
	// Apply action
	else if(ClientPlayer->Targets.size()) {

		// Remember target
		if(ClientPlayer->Targets.size() == 1)
			ClientPlayer->LastTarget = ClientPlayer->Targets.front();

		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::ACTION_USE);
		Packet.Write<uint8_t>(ActionBarSlot);
		Packet.Write<uint8_t>((uint8_t)ClientPlayer->Targets.size());
		for(const auto &BattleTarget : ClientPlayer->Targets)
			Packet.Write<NetworkIDType>(BattleTarget->NetworkID);

		ClientNetwork->SendPacket(Packet);

		ClientPlayer->Action.Item = ClientPlayer->ActionBar[ActionBarSlot].Item;
		ClientPlayer->PotentialAction.Unset();
	}
}

// Changes targets
void _Battle::ChangeTarget(int Direction, int SideDirection) {
	if(!ClientNetwork || !ClientPlayer->PotentialAction.IsSet() || !ClientPlayer->IsAlive() || ClientPlayer->Targets.size() != 1)
		return;

	if(ClientPlayer->PotentialAction.Item->TargetID == TargetType::SELF)
		return;

	// Get current target side
	int BattleTargetSide = ClientPlayer->Targets.front()->BattleSide;

	// Change sides
	if(SideDirection != 0)
		BattleTargetSide = !BattleTargetSide;

	// Get list of fighters on target side
	std::list<_Object *> FighterList;
	GetFighterList(BattleTargetSide, FighterList);

	// Get iterator to current target
	auto Iterator = FighterList.begin();
	if(FighterList.size())
	   Iterator = std::find(FighterList.begin(), FighterList.end(), ClientPlayer->Targets.front());

	// Search for valid target
	_Object *NewTarget = nullptr;
	for(size_t i = 0; i < FighterList.size(); i++) {

		// Wrap around
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
			if(Iterator == FighterList.begin()) {
				Iterator = FighterList.end();
				Iterator--;
			}
			else {
				Iterator--;
			}

			NewTarget = *Iterator;
		}
		else
			NewTarget = *Iterator;

		// Check break condition
		if(NewTarget->IsAlive())
			break;
	}

	ClientPlayer->Targets.clear();
	ClientPlayer->Targets.push_back(NewTarget);
}

// Add a fighter to the battle
void _Battle::AddFighter(_Object *Fighter, uint8_t Side) {
	Fighter->Battle = this;
	Fighter->BattleSide = Side;
	Fighter->LastTarget = nullptr;
	Fighter->Targets.clear();
	Fighter->Action.Unset();
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
		if(Fighter->BattleSide == Side && Fighter->IsAlive()) {
			AliveFighters.push_back(Fighter);
		}
	}
}

// Starts the battle and notifies the players
void _Battle::Serialize(_Buffer &Data) {

	// Write fighter count
	size_t FighterCount = Fighters.size();
	Data.Write<uint8_t>((uint8_t)FighterCount);

	// Write fighter information
	for(auto &Fighter : Fighters) {
		Fighter->TurnTimer = GetRandomReal(0, BATTLE_MAX_START_TURNTIMER);
		//Fighter->TurnTimer = 1;

		Fighter->SerializeBattle(Data);
	}
}

// Unserialize for network
void _Battle::Unserialize(_Buffer &Data, _HUD *HUD) {

	// Get fighter count
	int FighterCount = Data.Read<uint8_t>();

	// Get fighter information
	for(int i = 0; i < FighterCount; i++) {

		// Get object data
		NetworkIDType NetworkID = Data.Read<NetworkIDType>();
		uint32_t DatabaseID = Data.Read<uint32_t>();

		// Get object pointers
		_Object *Fighter = nullptr;
		if(DatabaseID) {
			Fighter = Manager->CreateWithID(NetworkID);
			Stats->GetMonsterStats(DatabaseID, Fighter);
		}
		else
			Fighter = Manager->IDMap[NetworkID];

		// Get battle stats
		Fighter->UnserializeBattle(Data);
		Fighter->HUD = HUD;
		Fighter->Scripting = Scripting;
		Fighter->Stats = Stats;
		Fighter->CalculateStats();

		// Add fighter
		AddFighter(Fighter, Fighter->BattleSide);
	}

	// Set up ui
	BattleElement = Assets.Elements["element_battle"];
	BattleElement->SetVisible(true);

	// Set fighter position offsets and create ui elements
	int SideCount[2] = { 0, 0 };
	for(auto &Fighter : Fighters) {

		// Get position on screen
		GetBattleOffset(SideCount[Fighter->BattleSide], Fighter);
		SideCount[Fighter->BattleSide]++;

		// Create ui element
		Fighter->CreateBattleElement(BattleElement);

		// Create ui elements for status effects
		for(auto &StatusEffect : Fighter->StatusEffects) {
			StatusEffect->BattleElement = StatusEffect->CreateUIElement(Fighter->BattleElement);
			if(ClientPlayer == Fighter)
				StatusEffect->HUDElement = StatusEffect->CreateUIElement(Assets.Elements["element_hud_statuseffects"]);
		}
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
			if(Fighter->IsAlive()) {
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
			if(!SideStats[Side].FighterCount)
				break;

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
		Fighter->InputStates.clear();
		Fighter->PotentialAction.Unset();
		Fighter->Action.Unset();

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
			if(Fighter->Peer)
				Server->SendMessage(Fighter->Peer, std::string("You are now level " + std::to_string(NewLevel) + "!"), COLOR_GOLD);
		}

		// Write results
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::BATTLE_END);
		Packet.WriteBit(SideStats[0].Dead);
		Packet.WriteBit(SideStats[1].Dead);
		Packet.Write<uint8_t>(SideStats[!Fighter->BattleSide].PlayerCount);
		Packet.Write<uint8_t>(SideStats[!Fighter->BattleSide].MonsterCount);
		Packet.Write<int32_t>(ExperienceEarned);
		Packet.Write<int32_t>(GoldEarned);

		// Sort item drops
		std::unordered_map<uint32_t, int> SortedItems;
		for(auto &ItemID : Fighter->ItemDropsReceived) {
			SortedItems[ItemID]++;
		}
		Fighter->ItemDropsReceived.clear();

		// Write item count
		size_t ItemCount = SortedItems.size();
		Packet.Write<uint8_t>((uint8_t)ItemCount);

		// Write items
		for(auto &Iterator : SortedItems) {
			Packet.Write<uint32_t>(Iterator.first);
			Packet.Write<uint8_t>((uint8_t)Iterator.second);
			Fighter->Inventory->AddItem(Stats->Items[Iterator.first], Iterator.second);
		}

		// Send info
		if(Fighter->Peer) {
			Server->Network->SendPacket(Packet, Fighter->Peer);
			Server->SendHUD(Fighter->Peer);
		}
	}

	Deleted = true;
}

// Calculates a screen position for a slot
void _Battle::GetBattleOffset(int SideIndex, _Object *Fighter) {
	int Column = SideIndex / BATTLE_ROWS_PER_SIDE;

	// Check sides
	if(Fighter->BattleSide == 0)
		Fighter->BattleOffset.x = -170 - Column * BATTLE_COLUMN_SPACING;
	else
		Fighter->BattleOffset.x = 70 + Column * BATTLE_COLUMN_SPACING;

	// Get row count for a given column
	float RowCount = (float)SideCount[Fighter->BattleSide] / BATTLE_ROWS_PER_SIDE - Column;
	if(RowCount >= 1)
		RowCount = BATTLE_ROWS_PER_SIDE;
	else
		RowCount *= BATTLE_ROWS_PER_SIDE;

	// Divide space into RowCount parts, then divide that by 2
	int SpacingY = (int)((BattleElement->Size.y / RowCount) / 2);

	// Place slots in between main divisions
	Fighter->BattleOffset.y = SpacingY * (2 * (SideIndex % BATTLE_ROWS_PER_SIDE) + 1) - BattleElement->Size.y/2;
}

// Removes a player from the battle
void _Battle::RemoveFighter(_Object *RemoveFighter) {

	// Remove action results
	for(auto Iterator = ActionResults.begin(); Iterator != ActionResults.end(); ) {
		_ActionResult &ActionResult = *Iterator;
		if(ActionResult.Source.Object == RemoveFighter || ActionResult.Target.Object == RemoveFighter) {
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

	switch(Action) {
		case _Actions::SKILL1:
		case _Actions::SKILL2:
		case _Actions::SKILL3:
		case _Actions::SKILL4:
		case _Actions::SKILL5:
		case _Actions::SKILL6:
		case _Actions::SKILL7:
		case _Actions::SKILL8:
			ClientSetAction((uint8_t)(Action - _Actions::SKILL1));
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

	return false;
}

// Handle action from another player
void _Battle::ClientHandlePlayerAction(_Buffer &Data) {

	NetworkIDType NetworkID = Data.Read<NetworkIDType>();
	uint32_t ItemID = Data.Read<uint32_t>();

	_Object *Fighter = Manager->IDMap[NetworkID];
	if(Fighter) {
		Fighter->Action.Item = Stats->Items[ItemID];
	}
}

// Send a packet to all players
void _Battle::BroadcastPacket(_Buffer &Data) {

	// Send packet to all players
	for(auto &Fighter : Fighters) {
		if(Fighter->Peer) {
			Server->Network->SendPacket(Data, Fighter->Peer);
		}
	}
}
