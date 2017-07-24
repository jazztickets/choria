/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
#include <hud.h>
#include <graphics.h>
#include <scripting.h>
#include <stats.h>
#include <config.h>
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
	SideCount{0, 0},
	Boss(false),
	Zone(0),
	Cooldown(0.0),
	Time(0),
	WaitTimer(0),
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

			// Update ui
			if(BattleElement && ActionResult.Source.Object && ActionResult.Target.Object) {

				// Find start position
				glm::vec2 StartPosition = ActionResult.Source.Object->ResultPosition - glm::vec2(ActionResult.Source.Object->Portrait->Size.x/2 + ActionResult.Texture->Size.x/2 + 10, 0);
				ActionResult.LastPosition = ActionResult.Position;

				// Interpolate between start and end position of action used
				ActionResult.Position = glm::mix(StartPosition, ActionResult.Target.Object->ResultPosition, std::min(ActionResult.Time * ActionResult.Speed / ActionResult.Timeout, 1.0));
				if(ActionResult.Time == 0.0)
					ActionResult.LastPosition = ActionResult.Position;

			}

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
	if(ActionResult.Target.HasStat(StatType::HEALTH) && ActionResult.Target.Values[StatType::HEALTH].Integer > 0)
		TextColor = COLOR_GREEN;
	else if(ActionResult.Target.HasStat(StatType::CRIT) && ActionResult.Target.Values[StatType::CRIT].Integer)
		TextColor = COLOR_YELLOW;

	TextColor.a = AlphaPercent;

	std::stringstream Buffer;
	if(ActionResult.Target.HasStat(StatType::MISS))
		Buffer << "miss";
	else if(ActionResult.Target.HasStat(StatType::HEALTH))
		Buffer << std::abs(ActionResult.Target.Values[StatType::HEALTH].Integer);
	Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + glm::vec2(0, 7), TextColor, CENTER_BASELINE);
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

		// Clear existing targets
		ClientPlayer->Targets.clear();

		// Check if item can be used
		const _Item *Item = ClientPlayer->ActionBar[ActionBarSlot].Item;
		if(Item) {
			if(!Item->CanUse(Scripting, ActionResult))
				Item = nullptr;

			if(Item && !Item->IsSkill() && ClientPlayer->ActionBar[ActionBarSlot].Count == 0)
				Item = nullptr;
		}

		// Set up initial target
		if(Item) {
			if(Config.ShowTutorial && ClientPlayer->Level == 1 && ClientPlayer->HUD)
				ClientPlayer->HUD->SetMessage("Hit up/down or use mouse to change targets. Press " + Actions.GetInputNameForAction(_Actions::SKILL1 + ActionBarSlot) + " again to confirm.");

			// Get opposite side
			int StartingSide = !ClientPlayer->BattleSide;

			// Pick sides depending on action
			if(Item->TargetID != TargetType::ANY && Item->CanTargetAlly()) {
				StartingSide = ClientPlayer->BattleSide;
				if(!ClientPlayer->LastTarget[StartingSide])
					ClientPlayer->LastTarget[StartingSide] = ClientPlayer;
			}

			if(Item->TargetID == TargetType::SELF) {
				ClientPlayer->Targets.push_back(ClientPlayer);
			}
			else {
				ClientSetTarget(Item, StartingSide, ClientPlayer->LastTarget[StartingSide]);
			}
		}

		// Set potential skill
		if(ClientPlayer->Targets.size()) {
			ClientPlayer->PotentialAction.Item = Item;
			ClientPlayer->PotentialAction.ActionBarSlot = ActionBarSlot;
		}
		else
			ClientPlayer->PotentialAction.Unset();
	}
	// Apply action
	else if(ClientPlayer->Targets.size()) {
		if(Config.ShowTutorial && ClientPlayer->Level == 1 && ClientPlayer->HUD)
			ClientPlayer->HUD->SetMessage("");

		// Check if action can be used
		_ActionResult ActionResult;
		ActionResult.Source.Object = ClientPlayer;
		ActionResult.Scope = ScopeType::BATTLE;
		ActionResult.ActionUsed = Action;
		const _Item *Item = ClientPlayer->ActionBar[ActionBarSlot].Item;
		if(!Item->CanUse(Scripting, ActionResult)) {
			ClientPlayer->PotentialAction.Unset();
			ClientPlayer->Targets.clear();
			return;
		}

		// Remember target
		if(ClientPlayer->Targets.size())
			ClientPlayer->LastTarget[ClientPlayer->Targets.front()->BattleSide] = ClientPlayer->Targets.front();

		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::ACTION_USE);
		Packet.Write<uint8_t>(ActionBarSlot);
		Packet.Write<uint8_t>((uint8_t)ClientPlayer->Targets.size());
		for(const auto &BattleTarget : ClientPlayer->Targets)
			Packet.Write<NetworkIDType>(BattleTarget->NetworkID);

		ClientNetwork->SendPacket(Packet);

		ClientPlayer->Action.Item = Item;
		ClientPlayer->PotentialAction.Unset();
	}
}

// Set target for client
void _Battle::ClientSetTarget(const _Item *Item, int Side, _Object *InitialTarget) {

	// Get list of fighters on each side
	std::list<_Object *> FighterList;
	GetFighterList(Side, FighterList);
	auto Iterator = FighterList.begin();

	// Get iterator to last target
	_Object *LastTarget = InitialTarget;
	if(FighterList.size() && LastTarget && Item->CanTarget(ClientPlayer, LastTarget))
	   Iterator = std::find(FighterList.begin(), FighterList.end(), LastTarget);

	// Set up targets
	int TargetCount = Item->GetTargetCount();
	for(size_t i = 0; i < FighterList.size(); i++) {

		// Check for valid target
		_Object *Target = *Iterator;
		if(Item->CanTarget(ClientPlayer, Target)) {

			// Add fighter to list of targets
			ClientPlayer->Targets.push_back(Target);

			// Update count
			TargetCount--;
			if(TargetCount <= 0)
				break;
		}

		// Update target
		++Iterator;
		if(Iterator == FighterList.end())
			Iterator = FighterList.begin();
	}
}

// Changes targets
void _Battle::ChangeTarget(int Direction, bool ChangeSides) {
	if(!ClientNetwork || !ClientPlayer->PotentialAction.IsSet() || !ClientPlayer->IsAlive() || !ClientPlayer->Targets.size())
		return;

	const _Item *Item = ClientPlayer->PotentialAction.Item;
	if(Item->TargetID == TargetType::SELF)
		return;

	// Get current target side
	int BattleTargetSide = ClientPlayer->Targets.front()->BattleSide;

	// Change sides
	if(Item->TargetID == TargetType::ANY && ChangeSides)
		BattleTargetSide = !BattleTargetSide;

	// Get list of fighters on target side
	std::list<_Object *> FighterList;
	GetFighterList(BattleTargetSide, FighterList);

	// Get iterator to current target
	auto Iterator = FighterList.begin();
	if(FighterList.size())
	   Iterator = std::find(FighterList.begin(), FighterList.end(), ClientPlayer->Targets.front());

	// Get target count
	size_t TargetCount = ClientPlayer->Targets.size();
	ClientPlayer->Targets.clear();

	// Get max available targets
	size_t MaxTargets = 0;
	for(auto &Target : FighterList) {
		if(Item->CanTarget(ClientPlayer, Target))
			MaxTargets++;
	}

	// Cap target count
	TargetCount = std::min(TargetCount, MaxTargets);

	// Search for valid target
	_Object *NewTarget = nullptr;
	while(TargetCount) {

		// Wrap around
		if(Iterator == FighterList.end())
			Iterator = FighterList.begin();

		// Update target
		if(Direction > 0) {
			++Iterator;
			if(Iterator == FighterList.end())
				Iterator = FighterList.begin();

			NewTarget = *Iterator;
		}
		else if(Direction < 0) {
			if(Iterator == FighterList.begin()) {
				Iterator = FighterList.end();
				--Iterator;
			}
			else {
				--Iterator;
			}

			NewTarget = *Iterator;
		}
		else
			NewTarget = *Iterator;

		// Check break condition
		if(Item->CanTarget(ClientPlayer, NewTarget)) {
			ClientPlayer->Targets.push_back(NewTarget);

			// Update count
			TargetCount--;

			// Start moving down after first target found
			Direction = 1;
		}
	}
}

// Add a fighter to the battle
void _Battle::AddFighter(_Object *Fighter, uint8_t Side, bool Join) {
	Fighter->Battle = this;
	Fighter->BattleSide = Side;
	Fighter->LastTarget[0] = nullptr;
	Fighter->LastTarget[1] = nullptr;
	Fighter->Targets.clear();
	Fighter->Action.Unset();
	Fighter->PotentialAction.Unset();
	Fighter->InventoryOpen = false;
	Fighter->SkillsOpen = false;
	Fighter->Paused = false;
	Fighter->Vendor = nullptr;
	Fighter->Trader = nullptr;
	Fighter->TeleportTime = -1.0;
	Fighter->JoinedBattle = Join;
	if(Fighter->Server) {
		Fighter->GenerateNextBattle();
		Fighter->TurnTimer = GetRandomReal(0, BATTLE_MAX_START_TURNTIMER);

		// Send player join packet to current fighters
		if(Join) {
			_Buffer Packet;
			Packet.Write<PacketType>(PacketType::BATTLE_JOIN);
			Fighter->SerializeBattle(Packet);
			BroadcastPacket(Packet);
		}
	}

	// Count fighters and set slots
	SideCount[Side]++;
	Fighters.push_back(Fighter);

	// Fighter joining on the client
	if(!Fighter->Server && Join) {

		// Adjust existing battle elements and create new one
		int SideIndex = 0;
		for(auto &AdjustFighter : Fighters) {
			if(AdjustFighter->BattleSide == Side) {
				if(AdjustFighter == Fighter)
					CreateBattleElements(SideIndex, AdjustFighter);
				else
					AdjustBattleElements(SideIndex, AdjustFighter);

				SideIndex++;
			}
		}
	}
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
	for(auto &Fighter : Fighters)
		Fighter->SerializeBattle(Data);
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
			Fighter = Manager->GetObject(NetworkID);

		// Get battle stats
		Fighter->Stats = Stats;
		Fighter->HUD = HUD;
		Fighter->Scripting = Scripting;
		Fighter->UnserializeBattle(Data);
		Fighter->CalculateStats();

		// Add fighter
		AddFighter(Fighter, Fighter->BattleSide);
	}

	// Set fighter position offsets and create ui elements
	int SideIndex[2] = { 0, 0 };
	for(auto &Fighter : Fighters) {
		CreateBattleElements(SideIndex[Fighter->BattleSide], Fighter);
		SideIndex[Fighter->BattleSide]++;
	}
}

// End battle and give rewards
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
			if(Fighter->DatabaseID == 0)
				SideStats[Side].PlayerCount++;
			else
				SideStats[Side].MonsterCount++;

			if(Fighter->JoinedBattle)
				SideStats[Side].JoinedCount++;

			// Tally alive fighters
			if(Fighter->IsAlive()) {
				SideStats[Side].AliveCount++;
				SideStats[Side].Dead = false;
			}

			// Sum experience and gold
			SideStats[Side].TotalExperienceGiven += Fighter->ExperienceGiven;
			SideStats[Side].TotalGoldGiven += Fighter->GoldGiven;
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
			int DivideCount = SideStats[Side].AliveCount - SideStats[Side].JoinedCount;
			if(DivideCount <= 0)
				break;

			// Divide experience up
			if(SideStats[OtherSide].TotalExperienceGiven > 0) {
				SideStats[Side].ExperiencePerFighter = SideStats[OtherSide].TotalExperienceGiven / DivideCount;
				if(SideStats[Side].ExperiencePerFighter <= 0)
					SideStats[Side].ExperiencePerFighter = 1;
			}

			// Divide gold up
			if(SideStats[OtherSide].TotalGoldGiven > 0) {
				SideStats[Side].GoldPerFighter = SideStats[OtherSide].TotalGoldGiven / DivideCount;
				if(SideStats[Side].GoldPerFighter <= 0)
					SideStats[Side].GoldPerFighter = 1;
			}
		}

		// Get list of fighters that get rewards
		std::list<_Object *> RewardFighters;
		for(auto &Fighter : SideFighters[WinningSide]) {
			if(Fighter->IsAlive() && !Fighter->JoinedBattle)
				RewardFighters.push_back(Fighter);
		}

		// Check for reward recipients
		if(RewardFighters.size()) {

			// Convert winning side list to array
			std::vector<_Object *> FighterArray { std::begin(RewardFighters), std::end(RewardFighters) };

			// Generate items drops
			std::list<uint32_t> ItemDrops;
			for(auto &Fighter : SideFighters[!WinningSide]) {
				if(Fighter->DatabaseID)
					Stats->GenerateItemDrops(Fighter->DatabaseID, 1, ItemDrops);
			}

			// Boss drops aren't divided up
			if(Boss) {
				for(auto &ItemID : ItemDrops) {
					for(auto &Fighter : RewardFighters) {
						Fighter->ItemDropsReceived.push_back(ItemID);
					}
				}
			}
			// Give out drops randomly
			else {
				for(auto &ItemID : ItemDrops) {
					std::shuffle(FighterArray.begin(), FighterArray.end(), RandomGenerator);
					_Object *Fighter = FighterArray[0];
					Fighter->ItemDropsReceived.push_back(ItemID);
				}
			}
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
		if(!Fighter->IsAlive()) {
			Fighter->ApplyDeathPenalty();
		}
		else if(!Fighter->JoinedBattle) {
			ExperienceEarned = SideStats[WinningSide].ExperiencePerFighter;
			GoldEarned = SideStats[WinningSide].GoldPerFighter;
			Fighter->PlayerKills += SideStats[!WinningSide].PlayerCount;
			Fighter->MonsterKills += SideStats[!WinningSide].MonsterCount;
		}

		// Start cooldown timer
		if(Fighter->IsAlive() && Cooldown > 0.0 && Zone)
			Fighter->BattleCooldown[Zone] = Cooldown;

		// Update stats
		int CurrentLevel = Fighter->Level;
		Fighter->UpdateExperience(ExperienceEarned);
		Fighter->UpdateGold(GoldEarned);
		Fighter->CalculateStats();
		int NewLevel = Fighter->Level;
		if(NewLevel > CurrentLevel) {
			if(Fighter->Peer)
				Server->SendMessage(Fighter->Peer, std::string("You are now level " + std::to_string(NewLevel) + "!"), COLOR_GOLD);

			Fighter->Health = Fighter->MaxHealth;
			Fighter->Mana = Fighter->MaxMana;
		}

		// Write results
		_Buffer Packet;
		Packet.Write<PacketType>(PacketType::BATTLE_END);
		Packet.Write<int>(Fighter->PlayerKills);
		Packet.Write<int>(Fighter->MonsterKills);
		Packet.Write<int>(Fighter->GoldLost);
		Packet.Write<int>(ExperienceEarned);
		Packet.Write<int>(GoldEarned);

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
			Packet.Write<uint8_t>(0);
			Packet.Write<uint8_t>((uint8_t)Iterator.second);
			Fighter->Inventory->AddItem(Stats->Items[Iterator.first], 0, Iterator.second);
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
	if(!BattleElement)
		return;

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

// Adjust existing battle elements
void _Battle::AdjustBattleElements(int SideIndex, _Object *Fighter) {

	// Get position on screen
	GetBattleOffset(SideIndex, Fighter);

	// Update position
	if(Fighter->BattleElement) {
		Fighter->BattleElement->Offset = Fighter->BattleOffset;
		Fighter->BattleElement->CalculateBounds();
	}
}

// Create battle element for a fighter
void _Battle::CreateBattleElements(int SideIndex, _Object *Fighter) {

	// Set up ui
	BattleElement = Assets.Elements["element_battle"];
	if(BattleElement)
		BattleElement->SetVisible(true);

	// Get position on screen
	GetBattleOffset(SideIndex, Fighter);

	// Create ui element
	if(BattleElement) {
		Fighter->CreateBattleElement(BattleElement);

		// Create ui elements for status effects
		for(auto &StatusEffect : Fighter->StatusEffects) {
			StatusEffect->BattleElement = StatusEffect->CreateUIElement(Fighter->BattleElement);
			if(ClientPlayer == Fighter)
				StatusEffect->HUDElement = StatusEffect->CreateUIElement(Assets.Elements["element_hud_statuseffects"]);
		}
	}
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

			// Broadcast fighter leaving
			if(Server) {
				_Buffer Packet;
				Packet.Write<PacketType>(PacketType::BATTLE_LEAVE);
				Packet.Write<NetworkIDType>(Fighter->NetworkID);
				BroadcastPacket(Packet);
			}

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
bool _Battle::ClientHandleInput(size_t Action) {

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
		case _Actions::RIGHT:
			ChangeTarget(0, true);
		break;
	}

	return false;
}

// Handle action from another player
void _Battle::ClientHandlePlayerAction(_Buffer &Data) {

	NetworkIDType NetworkID = Data.Read<NetworkIDType>();
	uint32_t ItemID = Data.Read<uint32_t>();

	_Object *Fighter = Manager->GetObject(NetworkID);
	if(Fighter) {
		Fighter->Action.Item = Stats->Items[ItemID];
	}
}

// Send a packet to all players
void _Battle::BroadcastPacket(_Buffer &Data) {

	// Send packet to all players
	for(auto &Fighter : Fighters) {
		if(!Fighter->Deleted && Fighter->Peer) {
			Server->Network->SendPacket(Data, Fighter->Peer);
		}
	}
}
