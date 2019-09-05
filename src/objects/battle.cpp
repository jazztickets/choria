/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2018  Alan Witkowski
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
#include <objects/object.h>
#include <objects/buff.h>
#include <objects/components/character.h>
#include <objects/components/inventory.h>
#include <objects/components/fighter.h>
#include <objects/components/controller.h>
#include <objects/components/monster.h>
#include <objects/statchange.h>
#include <objects/statuseffect.h>
#include <hud/hud.h>
#include <ae/manager.h>
#include <ae/servernetwork.h>
#include <ae/clientnetwork.h>
#include <ae/peer.h>
#include <ae/ui.h>
#include <ae/actions.h>
#include <ae/buffer.h>
#include <ae/graphics.h>
#include <ae/program.h>
#include <ae/assets.h>
#include <ae/font.h>
#include <ae/random.h>
#include <constants.h>
#include <server.h>
#include <actiontype.h>
#include <scripting.h>
#include <stats.h>
#include <config.h>
#include <packet.h>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>

// Constructor
_Battle::_Battle() :
	Stats(nullptr),
	Server(nullptr),
	Scripting(nullptr),
	ClientNetwork(nullptr),
	ClientPlayer(nullptr),
	Manager(nullptr),

	Difficulty{0.0, 1.0},
	Cooldown(0.0),
	Zone(0),
	SideCount{0, 0},
	PVP(false),
	BountyEarned(0.0f),
	BountyClaimed(0.0f),
	Boss(false),

	Time(0),
	WaitTimer(0),
	BattleElement(nullptr) {

}

// Destructor
_Battle::~_Battle() {
	if(BattleElement)
		BattleElement->SetActive(false);

	// Remove objects
	for(auto &Object : Objects) {
		if(Object->IsMonster())
			Object->Deleted = true;
		Object->StopBattle();
	}

	// Remove entry from battle table
	if(Server)
		Scripting->DeleteBattle(this);
}

// Update battle
void _Battle::Update(double FrameTime) {

	// Check for end
	if(Server) {

		// Count alive objects for each side
		int AliveCount[2] = { 0, 0 };
		for(auto &Object : Objects) {
			if(Object->Character->IsAlive())
				AliveCount[Object->Fighter->BattleSide]++;
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
				glm::vec2 StartPosition = ActionResult.Source.Object->Fighter->ResultPosition - glm::vec2(ActionResult.Source.Object->Character->Portrait->Size.x/2 + ActionResult.Texture->Size.x/2 + 10, 0);
				ActionResult.LastPosition = ActionResult.Position;

				// Interpolate between start and end position of action used
				ActionResult.Position = glm::mix(StartPosition, ActionResult.Target.Object->Fighter->ResultPosition, std::min(ActionResult.Time * ActionResult.Speed / ActionResult.Timeout, 1.0));
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

// Render the battle screen
void _Battle::Render(double BlendFactor) {
	BattleElement->Render();

	// Draw battle elements
	for(auto &Object : Objects)
		Object->RenderBattle(ClientPlayer, Time);

	// Draw action results
	for(auto &ActionResult : ActionResults)
		RenderActionResults(ActionResult, BlendFactor);
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
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
	if(ActionResult.ActionUsed.Item && !ActionResult.ActionUsed.Item->IsSkill())
		ae::Graphics.DrawScaledImage(DrawPosition, ae::Assets.Textures["textures/hud/item_back.png"], WhiteAlpha);
	ae::Graphics.DrawScaledImage(DrawPosition, ActionResult.Texture, WhiteAlpha);

	// Draw damage dealt
	glm::vec4 TextColor = glm::vec4(1.0f);
	if(ActionResult.Target.HasStat(StatType::HEALTH) && ActionResult.Target.Values[StatType::HEALTH].Integer > 0)
		TextColor = ae::Assets.Colors["green"];
	else if(ActionResult.Target.HasStat(StatType::CRIT) && ActionResult.Target.Values[StatType::CRIT].Integer)
		TextColor = ae::Assets.Colors["yellow"];

	TextColor.a = AlphaPercent;

	std::stringstream Buffer;
	if(ActionResult.Target.HasStat(StatType::MISS))
		Buffer << "miss";
	else if(ActionResult.Target.HasStat(StatType::HEALTH))
		Buffer << std::abs(ActionResult.Target.Values[StatType::HEALTH].Integer);
	ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + glm::vec2(0, 7), ae::CENTER_BASELINE, TextColor);
}

// Sends an action selection to the server
void _Battle::ClientSetAction(uint8_t ActionBarSlot) {
	if(ActionBarSlot >= ClientPlayer->Character->ActionBar.size())
		return;

	// Check for dead
	if(!ClientPlayer->Character->IsAlive())
		return;

	// Player already locked in
	if(ClientPlayer->Character->Action.IsSet())
		return;

	// Get skillbar action
	_Action Action = ClientPlayer->Character->ActionBar[ActionBarSlot];
	ClientPlayer->Character->GetActionFromActionBar(Action, ActionBarSlot);

	// Check for changing an action
	bool ChangingAction = false;
	if(ClientPlayer->Fighter->PotentialAction.IsSet() && Action != ClientPlayer->Fighter->PotentialAction)
		ChangingAction = true;

	// Choose an action to use
	if(!ClientPlayer->Fighter->PotentialAction.IsSet() || ChangingAction) {
		_ActionResult ActionResult;
		ActionResult.Source.Object = ClientPlayer;
		ActionResult.Scope = ScopeType::BATTLE;
		ActionResult.ActionUsed = Action;

		// Clear existing targets
		ClientPlayer->Character->Targets.clear();

		// Check if item can be used
		const _Item *Item = ClientPlayer->Character->ActionBar[ActionBarSlot].Item;
		if(Item) {
			if(!Item->CanUse(Scripting, ActionResult))
				Item = nullptr;

			if(Item && !Item->IsSkill() && ClientPlayer->Character->ActionBar[ActionBarSlot].Count == 0)
				Item = nullptr;
		}

		// Set up initial target
		if(Item) {
			if(Config.ShowTutorial && ClientPlayer->Character->Level == 1 && ClientPlayer->Character->HUD)
				ClientPlayer->Character->HUD->SetMessage("Hit up/down or use mouse to change targets. Press " + ae::Actions.GetInputNameForAction(Action::GAME_SKILL1 + ActionBarSlot) + " again to confirm.");

			// Get opposite side
			int StartingSide = !ClientPlayer->Fighter->BattleSide;

			// Pick sides depending on action
			if(Item->TargetID != TargetType::ANY && Item->CanTargetAlly()) {
				StartingSide = ClientPlayer->Fighter->BattleSide;
				if(!ClientPlayer->Fighter->LastTarget[StartingSide])
					ClientPlayer->Fighter->LastTarget[StartingSide] = ClientPlayer;
			}

			// Set target
			ClientSetTarget(Item, StartingSide, ClientPlayer->Fighter->LastTarget[StartingSide]);
		}

		// Set potential skill
		if(ClientPlayer->Character->Targets.size()) {
			ClientPlayer->Fighter->PotentialAction.Item = Item;
			ClientPlayer->Fighter->PotentialAction.ActionBarSlot = ActionBarSlot;
		}
		else
			ClientPlayer->Fighter->PotentialAction.Unset();
	}
	// Apply action
	else if(ClientPlayer->Character->Targets.size()) {

		// Update HUD
		if(ClientPlayer->Character->HUD) {
			if(Config.ShowTutorial && ClientPlayer->Character->Level == 1)
				ClientPlayer->Character->HUD->SetMessage("");
		}

		// Check if action can be used
		_ActionResult ActionResult;
		ActionResult.Source.Object = ClientPlayer;
		ActionResult.Scope = ScopeType::BATTLE;
		ActionResult.ActionUsed = Action;
		const _Item *Item = ClientPlayer->Character->ActionBar[ActionBarSlot].Item;
		if(!Item->CanUse(Scripting, ActionResult)) {
			ClientPlayer->Fighter->PotentialAction.Unset();
			ClientPlayer->Character->Targets.clear();
			return;
		}

		// Remember target
		if(ClientPlayer->Character->Targets.size())
			ClientPlayer->Fighter->LastTarget[ClientPlayer->Character->Targets.front()->Fighter->BattleSide] = ClientPlayer->Character->Targets.front();

		// Notify server
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::ACTION_USE);
		Packet.Write<uint8_t>(ActionBarSlot);
		Packet.Write<uint8_t>((uint8_t)ClientPlayer->Character->Targets.size());
		for(const auto &BattleTarget : ClientPlayer->Character->Targets)
			Packet.Write<ae::NetworkIDType>(BattleTarget->NetworkID);

		ClientNetwork->SendPacket(Packet);

		ClientPlayer->Character->Action.Item = Item;
		ClientPlayer->Fighter->PotentialAction.Unset();
	}
}

// Set target for client
void _Battle::ClientSetTarget(const _Item *Item, int Side, _Object *InitialTarget) {
	ClientPlayer->Character->Targets.clear();

	// Can't change self targets
	if(Item->TargetID == TargetType::SELF) {
		ClientPlayer->Character->Targets.push_back(ClientPlayer);
		return;
	}

	// Get list of objects on each side
	std::list<_Object *> ObjectList;
	GetObjectList(Side, ObjectList);
	auto Iterator = ObjectList.begin();

	// Get iterator to last target
	_Object *LastTarget = InitialTarget;
	if(ObjectList.size() && LastTarget && Item->CanTarget(ClientPlayer, LastTarget))
	   Iterator = std::find(ObjectList.begin(), ObjectList.end(), LastTarget);

	// Set up targets
	int TargetCount = Item->GetTargetCount();
	for(size_t i = 0; i < ObjectList.size(); i++) {

		// Check for valid target
		_Object *Target = *Iterator;
		if(Item->CanTarget(ClientPlayer, Target)) {

			// Add object to list of targets
			ClientPlayer->Character->Targets.push_back(Target);

			// Update count
			TargetCount--;
			if(TargetCount <= 0)
				break;
		}

		// Update target
		++Iterator;
		if(Iterator == ObjectList.end())
			Iterator = ObjectList.begin();
	}
}

// Changes targets
void _Battle::ChangeTarget(int Direction, bool ChangeSides) {
	if(!ClientNetwork || !ClientPlayer->Fighter->PotentialAction.IsSet() || !ClientPlayer->Character->IsAlive() || !ClientPlayer->Character->Targets.size())
		return;

	// Can't change self targetting actions
	const _Item *Item = ClientPlayer->Fighter->PotentialAction.Item;
	if(Item->TargetID == TargetType::SELF)
		return;

	// Get current target side
	int BattleTargetSide = ClientPlayer->Character->Targets.front()->Fighter->BattleSide;

	// Change sides
	if(Item->TargetID == TargetType::ANY && ChangeSides)
		BattleTargetSide = !BattleTargetSide;

	// Get list of objects on target side
	std::list<_Object *> ObjectList;
	GetObjectList(BattleTargetSide, ObjectList);

	// Get iterator to current target
	auto Iterator = ObjectList.begin();
	if(ObjectList.size())
	   Iterator = std::find(ObjectList.begin(), ObjectList.end(), ClientPlayer->Character->Targets.front());

	// Get target count
	size_t TargetCount = ClientPlayer->Character->Targets.size();
	ClientPlayer->Character->Targets.clear();

	// Get max available targets
	size_t MaxTargets = 0;
	for(auto &Target : ObjectList) {
		if(Item->CanTarget(ClientPlayer, Target))
			MaxTargets++;
	}

	// Cap target count
	TargetCount = std::min(TargetCount, MaxTargets);

	// Search for valid target
	_Object *NewTarget = nullptr;
	while(TargetCount) {

		// Wrap around
		if(Iterator == ObjectList.end())
			Iterator = ObjectList.begin();

		// Update target
		if(Direction > 0) {
			++Iterator;
			if(Iterator == ObjectList.end())
				Iterator = ObjectList.begin();

			NewTarget = *Iterator;
		}
		else if(Direction < 0) {
			if(Iterator == ObjectList.begin()) {
				Iterator = ObjectList.end();
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
			ClientPlayer->Character->Targets.push_back(NewTarget);

			// Update count
			TargetCount--;

			// Start moving down after first target found
			Direction = 1;
		}
		else if(ChangeSides)
			Direction = 1;
	}
}

// Add an object to the battle
void _Battle::AddObject(_Object *Object, uint8_t Side, bool Join) {
	Object->Character->Battle = this;
	Object->Fighter->BattleSide = Side;
	Object->Fighter->LastTarget[0] = nullptr;
	Object->Fighter->LastTarget[1] = nullptr;
	Object->Character->Targets.clear();
	Object->Character->Action.Unset();
	Object->Fighter->PotentialAction.Unset();
	Object->Character->ResetUIState();
	Object->Fighter->JoinedBattle = Join;
	Object->Fighter->GoldStolen = 0;
	if(Server) {
		Object->Character->GenerateNextBattle();
		Object->Fighter->TurnTimer = ae::GetRandomReal(0, BATTLE_MAX_START_TURNTIMER);

		// Send player join packet to current objects
		if(Join) {
			ae::_Buffer Packet;
			Packet.Write<PacketType>(PacketType::BATTLE_JOIN);
			Object->SerializeBattle(Packet);
			BroadcastPacket(Packet);
		}
	}

	// Count objects and set slots
	SideCount[Side]++;
	Objects.push_back(Object);

	// Object joining on the client
	if(!Server && Join) {

		// Adjust existing battle elements and create new one
		int SideIndex = 0;
		for(auto &AdjustObject : Objects) {
			if(AdjustObject->Fighter->BattleSide == Side) {
				if(AdjustObject == Object)
					CreateBattleElements(SideIndex, AdjustObject);
				else
					AdjustBattleElements(SideIndex, AdjustObject);

				SideIndex++;
			}
		}
	}
}

// Get a list of objects from a side
void _Battle::GetObjectList(int Side, std::list<_Object *> &SideObjects) {

	for(auto &Object : Objects) {
		if(Object->Fighter->BattleSide == Side) {
			SideObjects.push_back(Object);
		}
	}
}

// Get a list of alive objects from a side
void _Battle::GetAliveObjectList(int Side, std::list<_Object *> &AliveObjects) {

	for(auto &Object : Objects) {
		if(Object->Fighter->BattleSide == Side && Object->Character->IsAlive()) {
			AliveObjects.push_back(Object);
		}
	}
}

// Starts the battle and notifies the players
void _Battle::Serialize(ae::_Buffer &Data) {

	// Write object count
	Data.Write<uint8_t>((uint8_t)Objects.size());

	// Write object information
	for(auto &Object : Objects)
		Object->SerializeBattle(Data);
}

// Unserialize for network
void _Battle::Unserialize(ae::_Buffer &Data, _HUD *HUD) {

	// Get object count
	int ObjectCount = Data.Read<uint8_t>();

	// Get object information
	for(int i = 0; i < ObjectCount; i++) {

		// Get object data
		ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();
		uint32_t DatabaseID = Data.Read<uint32_t>();

		// Get object pointers
		_Object *Object = nullptr;
		if(DatabaseID) {
			Object = Manager->CreateWithID(NetworkID);
			Stats->GetMonsterStats(DatabaseID, Object);
		}
		else
			Object = Manager->GetObject(NetworkID);

		// Get battle stats
		Object->Stats = Stats;
		Object->Character->HUD = HUD;
		Object->Scripting = Scripting;
		Object->UnserializeBattle(Data, ClientPlayer == Object);
		Object->Character->CalculateStats();

		// Add object
		AddObject(Object, Object->Fighter->BattleSide);
	}

	// Set object position offsets and create ui elements
	int SideIndex[2] = { 0, 0 };
	for(auto &Object : Objects) {
		CreateBattleElements(SideIndex[Object->Fighter->BattleSide], Object);
		SideIndex[Object->Fighter->BattleSide]++;
	}
}

// End battle and give rewards
void _Battle::ServerEndBattle() {

	// Get statistics for each side
	_BattleResult SideStats[2];
	std::list<_Object *> SideObjects[2];
	for(int Side = 0; Side < 2; Side++) {

		// Get a list of objects that are still in the battle
		GetObjectList(Side, SideObjects[Side]);

		// Loop through objects
		for(auto &Object : SideObjects[Side]) {

			// Keep track of players
			if(!Object->IsMonster())
				SideStats[Side].PlayerCount++;
			else
				SideStats[Side].MonsterCount++;

			if(Object->Fighter->JoinedBattle)
				SideStats[Side].JoinedCount++;

			// Tally alive objects
			if(Object->Character->IsAlive()) {
				SideStats[Side].AliveCount++;
				SideStats[Side].Dead = false;
			}

			// Sum experience and gold
			SideStats[Side].TotalExperienceGiven += Object->Monster->ExperienceGiven;

			// Calculate gold based on monster or player
			if(Object->IsMonster())
				SideStats[Side].TotalGoldGiven += Object->Monster->GoldGiven + Object->Fighter->GoldStolen;
			else
				SideStats[Side].TotalGoldGiven += Object->Character->Bounty + Object->Fighter->GoldStolen + (int)(Object->Character->Gold * BountyEarned + 0.5f);
		}

		SideStats[Side].TotalExperienceGiven = (int)std::ceil(SideStats[Side].TotalExperienceGiven * Difficulty[Side]);
		SideStats[Side].TotalGoldGiven = (int)std::ceil(SideStats[Side].TotalGoldGiven * Difficulty[Side]);
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
	if(WinningSide != -1 && SideObjects[WinningSide].size()) {

		// Divide up rewards
		for(int Side = 0; Side < 2; Side++) {
			int OtherSide = !Side;
			int DivideCount = SideStats[Side].AliveCount;
			if(DivideCount <= 0)
				continue;

			// Divide experience up
			if(SideStats[OtherSide].TotalExperienceGiven > 0) {
				SideStats[Side].ExperiencePerCharacter = SideStats[OtherSide].TotalExperienceGiven / DivideCount;
				if(SideStats[Side].ExperiencePerCharacter <= 0)
					SideStats[Side].ExperiencePerCharacter = 1;
			}

			// Divide gold up
			if(SideStats[OtherSide].TotalGoldGiven > 0) {
				SideStats[Side].GoldPerCharacter = SideStats[OtherSide].TotalGoldGiven / DivideCount;
				if(SideStats[Side].GoldPerCharacter <= 0)
					SideStats[Side].GoldPerCharacter = 1;
			}
		}

		// Get list of objects that get rewards
		std::list<_Object *> RewardObjects;
		int DropRate = 0;
		for(auto &Object : SideObjects[WinningSide]) {
			if(Object->Character->IsAlive()) {
				DropRate += Object->Character->DropRate;
				RewardObjects.push_back(Object);
			}
		}

		// Check for reward recipients
		if(RewardObjects.size() && !PVP) {

			// Convert winning side list to array
			std::vector<_Object *> ObjectArray { std::begin(RewardObjects), std::end(RewardObjects) };

			// Generate items drops
			std::list<uint32_t> ItemDrops;
			for(auto &Object : SideObjects[!WinningSide]) {
				if(Object->IsMonster())
					Stats->GenerateItemDrops(Object->Monster->DatabaseID, 1, DropRate, ItemDrops);
			}

			// Boss drops aren't divided up
			if(Boss) {
				for(auto &ItemID : ItemDrops) {
					for(auto &Object : RewardObjects) {
						Object->Fighter->ItemDropsReceived.push_back(ItemID);
					}
				}
			}
			// Give out drops randomly
			else {
				for(auto &ItemID : ItemDrops) {
					std::shuffle(ObjectArray.begin(), ObjectArray.end(), ae::RandomGenerator);
					_Object *Object = ObjectArray[0];
					Object->Fighter->ItemDropsReceived.push_back(ItemID);
				}
			}
		}
	}

	// Send data
	for(auto &Object : Objects) {
		Object->Controller->InputStates.clear();
		Object->Fighter->PotentialAction.Unset();
		Object->Character->Action.Unset();

		// Get rewards
		int ExperienceEarned = 0;
		int GoldEarned = 0;
		if(!Object->Character->IsAlive()) {
			if(PVP)
				Object->ApplyDeathPenalty(BountyEarned, Object->Character->Bounty);
			else
				Object->ApplyDeathPenalty(PLAYER_DEATH_GOLD_PENALTY, 0);
		}
		else {
			ExperienceEarned = SideStats[WinningSide].ExperiencePerCharacter;
			GoldEarned = SideStats[WinningSide].GoldPerCharacter;
			Object->Character->PlayerKills += SideStats[!WinningSide].PlayerCount;
			Object->Character->MonsterKills += SideStats[!WinningSide].MonsterCount;
			if(PVP && Object->Fighter->BattleSide == BATTLE_PVP_ATTACKER_SIDE) {
				if(BountyEarned) {
					Object->Character->Bounty += GoldEarned;
					if(Object->Character->Bounty) {
						std::string BountyMessage = "Player " + Object->Name + " now has a bounty of " + std::to_string(Object->Character->Bounty) + " gold!";
						Server->BroadcastMessage(nullptr, BountyMessage, "cyan");
						Server->Log << "[BOUNTY] " << BountyMessage << std::endl;
					}
				}
			}
		}

		// Start cooldown timer
		if(Object->Character->IsAlive() && Cooldown > 0.0 && Zone)
			Object->Character->BattleCooldown[Zone] = Cooldown;

		// Update stats
		int CurrentLevel = Object->Character->Level;
		Object->Character->UpdateExperience(ExperienceEarned);
		Object->Character->UpdateGold(GoldEarned);
		Object->Character->CalculateStats();
		int NewLevel = Object->Character->Level;
		if(NewLevel > CurrentLevel) {
			if(Object->Peer)
				Server->SendMessage(Object->Peer, std::string("You are now level " + std::to_string(NewLevel) + "!"), "gold");

			Object->Character->Health = Object->Character->MaxHealth;
			Object->Character->Mana = Object->Character->MaxMana;
		}

		// Write results
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::BATTLE_END);
		Packet.Write<int>(Object->Character->PlayerKills);
		Packet.Write<int>(Object->Character->MonsterKills);
		Packet.Write<int>(Object->Character->GoldLost);
		Packet.Write<int>(Object->Character->Bounty);
		Packet.Write<int>(ExperienceEarned);
		Packet.Write<int>(GoldEarned);

		// Sort item drops
		std::unordered_map<uint32_t, int> SortedItems;
		for(auto &ItemID : Object->Fighter->ItemDropsReceived) {
			SortedItems[ItemID]++;
		}
		Object->Fighter->ItemDropsReceived.clear();

		// Write item count
		size_t ItemCount = SortedItems.size();
		Packet.Write<uint8_t>((uint8_t)ItemCount);

		// Write items
		for(auto &Iterator : SortedItems) {
			Packet.Write<uint32_t>(Iterator.first);
			Packet.Write<uint8_t>(0);
			Packet.Write<uint8_t>((uint8_t)Iterator.second);
			Object->Inventory->AddItem(Stats->Items.at(Iterator.first), 0, Iterator.second);
		}

		// Update bot goal
		if(Object->Character->Bot) {
			if(Scripting->StartMethodCall("Bot_Server", "DetermineNextGoal")) {
				Scripting->PushObject(Object);
				Scripting->MethodCall(1, 0);
				Scripting->FinishMethodCall();
			}
		}
		// Send info
		else if(Object->Peer) {
			Server->Network->SendPacket(Packet, Object->Peer);
			Server->SendHUD(Object->Peer);
		}
	}

	Deleted = true;
}

// Calculates a screen position for a slot
void _Battle::GetBattleOffset(int SideIndex, _Object *Object) {
	if(!BattleElement)
		return;

	// Get column index
	int Column = SideIndex / BATTLE_ROWS_PER_SIDE;

	// Check sides
	if(Object->Fighter->BattleSide == 0)
		Object->Fighter->BattleOffset.x = -170 - Column * BATTLE_COLUMN_SPACING;
	else
		Object->Fighter->BattleOffset.x = 70 + Column * BATTLE_COLUMN_SPACING;

	// Get row count for a given column
	float RowCount = (float)SideCount[Object->Fighter->BattleSide] / BATTLE_ROWS_PER_SIDE - Column;
	if(RowCount >= 1)
		RowCount = BATTLE_ROWS_PER_SIDE;
	else
		RowCount *= BATTLE_ROWS_PER_SIDE;

	// Divide space into RowCount parts, then divide that by 2
	int SpacingY = (int)((BattleElement->BaseSize.y / RowCount) / 2);

	// Place slots in between main divisions
	Object->Fighter->BattleOffset.y = SpacingY * (2 * (SideIndex % BATTLE_ROWS_PER_SIDE) + 1) - BattleElement->BaseSize.y/2;
}

// Adjust existing battle elements
void _Battle::AdjustBattleElements(int SideIndex, _Object *Object) {

	// Get position on screen
	GetBattleOffset(SideIndex, Object);

	// Update position
	if(Object->Fighter->BattleElement) {
		Object->Fighter->BattleElement->BaseOffset = Object->Fighter->BattleOffset;
		Object->Fighter->BattleElement->CalculateBounds();
	}
}

// Create battle element for an object
void _Battle::CreateBattleElements(int SideIndex, _Object *Object) {

	// Set up ui
	BattleElement = ae::Assets.Elements["element_battle"];
	if(BattleElement)
		BattleElement->SetActive(true);

	// Get position on screen
	GetBattleOffset(SideIndex, Object);

	// Create ui element
	if(BattleElement) {
		Object->Fighter->CreateBattleElement(BattleElement);

		// Create ui elements for status effects
		for(auto &StatusEffect : Object->Character->StatusEffects) {
			StatusEffect->BattleElement = StatusEffect->CreateUIElement(Object->Fighter->BattleElement);
			if(ClientPlayer == Object)
				StatusEffect->HUDElement = StatusEffect->CreateUIElement(ae::Assets.Elements["element_hud_statuseffects"]);
		}
	}
}

// Removes a player from the battle
void _Battle::RemoveObject(_Object *RemoveObject) {

	// Remove action results
	for(auto Iterator = ActionResults.begin(); Iterator != ActionResults.end(); ) {
		_ActionResult &ActionResult = *Iterator;
		if(ActionResult.Source.Object == RemoveObject || ActionResult.Target.Object == RemoveObject) {
			Iterator = ActionResults.erase(Iterator);
		}
		else
			++Iterator;
	}

	// Remove objects
	for(auto Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		_Object *Object = *Iterator;
		if(Object == RemoveObject) {

			// Broadcast object leaving
			if(Server) {
				ae::_Buffer Packet;
				Packet.Write<PacketType>(PacketType::BATTLE_LEAVE);
				Packet.Write<ae::NetworkIDType>(Object->NetworkID);
				BroadcastPacket(Packet);
			}

			SideCount[Object->Fighter->BattleSide]--;
			Object->StopBattle();
			Objects.erase(Iterator);
			return;
		}
	}
}

// Get list of allies and enemies from object list
void _Battle::GetSeparateObjectList(uint8_t Side, std::list<_Object *> &Allies, std::list<_Object *> &Enemies) {
	for(const auto &Object : Objects) {
		if(Object->Deleted)
			continue;

		if(Object->Fighter->BattleSide == Side)
			Allies.push_back(Object);
		else if(Object->Character->IsAlive())
			Enemies.push_back(Object);
	}
}

// Get number of peers in battle
int _Battle::GetPeerCount() {
	int PeerCount = 0;
	for(auto &Object : Objects) {
		if(Object->Peer)
			PeerCount++;
	}

	return PeerCount;
}

// Handle player input
bool _Battle::ClientHandleInput(size_t Action) {

	switch(Action) {
		case Action::GAME_SKILL1:
		case Action::GAME_SKILL2:
		case Action::GAME_SKILL3:
		case Action::GAME_SKILL4:
		case Action::GAME_SKILL5:
		case Action::GAME_SKILL6:
		case Action::GAME_SKILL7:
		case Action::GAME_SKILL8:
			ClientSetAction((uint8_t)(Action - Action::GAME_SKILL1));
		break;
		case Action::GAME_UP:
			ChangeTarget(-1, 0);
		break;
		case Action::GAME_DOWN:
			ChangeTarget(1, 0);
		break;
		case Action::GAME_LEFT:
		case Action::GAME_RIGHT:
			ChangeTarget(0, true);
		break;
		default:
			return false;
		break;
	}

	return true;
}

// Handle action from another player
void _Battle::ClientHandlePlayerAction(ae::_Buffer &Data) {

	ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();
	uint32_t ItemID = Data.Read<uint32_t>();

	_Object *Object = Manager->GetObject(NetworkID);
	if(Object)
		Object->Character->Action.Item = Stats->Items.at(ItemID);
}

// Send a packet to all players
void _Battle::BroadcastPacket(ae::_Buffer &Data) {

	// Send packet to all players
	for(auto &Object : Objects) {
		if(!Object->Deleted && Object->Peer) {
			Server->Network->SendPacket(Data, Object->Peer);
		}
	}
}
