/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2021 Alan Witkowski
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
#include <ae/database.h>
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
#include <ae/input.h>
#include <constants.h>
#include <server.h>
#include <actiontype.h>
#include <scripting.h>
#include <stats.h>
#include <config.h>
#include <packet.h>
#include <glm/gtc/type_ptr.hpp>
#include <SDL_keycode.h>
#include <vector>
#include <algorithm>
#include <map>
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

	Objects.reserve(BATTLE_MAX_OBJECTS_PER_SIDE * 2);
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
				glm::vec2 StartPosition = ActionResult.Source.Object->Fighter->ResultPosition - glm::vec2(UI_PORTRAIT_SIZE.x/2 + UI_SLOT_SIZE.x/2 + 10, 0) * ae::_Element::GetUIScale();
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
		Object->RenderBattle(ClientPlayer, Time, ae::Input.ModKeyDown(KMOD_ALT));

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
	if(ActionResult.ActionUsed.Item && !ActionResult.ActionUsed.Item->IsSkill()) {
		WhiteAlpha = glm::vec4(1.0f, 1.0f, 1.0f, AlphaPercent);
		ae::Graphics.DrawScaledImage(DrawPosition, ae::Assets.Textures["textures/hud/item_back.png"], UI_SLOT_SIZE, WhiteAlpha);
	}
	ae::Graphics.DrawScaledImage(DrawPosition, ActionResult.Texture, UI_SLOT_SIZE, WhiteAlpha);

	// Get damage value and color
	glm::vec4 TextColor = glm::vec4(1.0f);
	std::stringstream Buffer;
	if(ActionResult.Target.HasStat("Health")) {

		if(ActionResult.Target.HasStat("DamageType")) {
			uint32_t DamageTypeID = ActionResult.Target.Values["DamageType"].Int;
			TextColor = Stats->DamageTypes.at(DamageTypeID).Color;
		}

		Buffer << std::abs(ActionResult.Target.Values["Health"].Int);
	}
	else if(ActionResult.Target.HasStat("Miss"))
		Buffer << "miss";

	// Change color
	if(ActionResult.Target.HasStat("Health") && ActionResult.Target.Values["Health"].Int > 0)
		TextColor = ae::Assets.Colors["green"];
	else if(ActionResult.Target.HasStat("Crit") && ActionResult.Target.Values["Crit"].Int)
		TextColor = ae::Assets.Colors["yellow"];

	// Draw damage dealt
	TextColor.a = AlphaPercent;
	std::string DamageText = Buffer.str();
	std::string Font = DamageText.length() >= 4 ? "hud_small" : "hud_medium";
	ae::Assets.Fonts[Font]->DrawText(Buffer.str(), DrawPosition + glm::vec2(0, 7) * ae::_Element::GetUIScale(), ae::CENTER_BASELINE, TextColor);

	// Draw mana damage
	if(ActionResult.Target.HasStat("Mana") && ActionResult.Target.Values["Mana"].Int < 0) {
		Buffer.str("");
		Buffer << std::abs(ActionResult.Target.Values["Mana"].Int);
		TextColor = ae::Assets.Colors["light_blue"];
		TextColor.a = AlphaPercent;
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition + glm::vec2(24, 24) * ae::_Element::GetUIScale(), ae::RIGHT_BASELINE, TextColor);
	}
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

	// Action has been chosen, select target
	if(!ClientPlayer->Fighter->PotentialAction.IsSet() || ChangingAction) {
		_ActionResult ActionResult;
		ActionResult.Source.Object = ClientPlayer;
		ActionResult.Scope = ScopeType::BATTLE;
		ActionResult.ActionUsed = Action;

		// Clear existing targets
		ClientPlayer->Character->Targets.clear();

		// Set up initial target
		const _Item *Item = ClientPlayer->Character->ActionBar[ActionBarSlot].Item;
		if(Item) {
			if(Config.ShowTutorial && ClientPlayer->Character->Level == 1 && ClientPlayer->Character->HUD)
				ClientPlayer->Character->HUD->SetMessage("Hit up/down or use mouse to change targets. Press " + ae::Actions.GetInputNameForAction(Action::GAME_SKILL1 + ActionBarSlot) + " again to confirm.");

			// Get side to start action on
			int StartingSide = ClientPlayer->Fighter->GetStartingSide(Item);

			// Set initial target to self when targetting ally/self
			if(StartingSide == ClientPlayer->Fighter->BattleSide && !ClientPlayer->Fighter->LastTarget[Item->ID])
				ClientPlayer->Fighter->LastTarget[Item->ID] = ClientPlayer;

			// Set target
			ClientSetTarget(Item, StartingSide, ClientPlayer->Fighter->LastTarget[Item->ID]);
			if(ClientPlayer->Character->Targets.size()) {

				// Check if item can be used
				if(!Item->CanUse(Scripting, ActionResult))
					Item = nullptr;

				// Check quantity
				if(Item && !Item->IsSkill() && ClientPlayer->Character->ActionBar[ActionBarSlot].Count == 0)
					Item = nullptr;
			}
		}

		// Set potential skill
		if(ClientPlayer->Character->Targets.size() && Item) {
			ClientPlayer->Fighter->PotentialAction.Item = Item;
			ClientPlayer->Fighter->PotentialAction.ActionBarSlot = ActionBarSlot;
		}
		else
			ClientPlayer->Fighter->PotentialAction.Unset();
	}
	// Confirm action
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
		ClientPlayer->Fighter->LastTarget[Item->ID] = ClientPlayer->Character->Targets.front();

		// Notify server
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::ACTION_USE);
		Packet.WriteBit(1);
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

	// Get side of last target
	if(InitialTarget)
		Side = InitialTarget->Fighter->BattleSide;

	// Get list of objects on each side
	std::vector<_Object *> ObjectList;
	ObjectList.reserve(BATTLE_MAX_OBJECTS_PER_SIDE);
	GetObjectList(Side, ObjectList);
	auto Iterator = ObjectList.begin();

	// Get iterator to last target
	_Object *LastTarget = InitialTarget;
	if(ObjectList.size() && LastTarget && Item->CanTarget(Scripting, ClientPlayer, LastTarget)) {
		Iterator = std::find(ObjectList.begin(), ObjectList.end(), LastTarget);
		if(Iterator == ObjectList.end())
			Iterator = ObjectList.begin();
	}

	// Set up targets
	int TargetCount = Item->GetTargetCount(Scripting, ClientPlayer);
	int TargetIndex = 1;
	for(std::size_t i = 0; i < ObjectList.size(); i++) {

		// Check for valid target
		_Object *Target = *Iterator;
		if(Item->CanTarget(Scripting, ClientPlayer, Target)) {

			// Add object to list of targets
			ClientPlayer->Character->Targets.push_back(Target);
			Target->Fighter->TargetIndex = TargetIndex++;

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
void _Battle::ClientChangeTarget(int Direction, bool ChangeSides) {
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
	std::vector<_Object *> ObjectList;
	ObjectList.reserve(BATTLE_MAX_OBJECTS_PER_SIDE);
	GetObjectList(BattleTargetSide, ObjectList);

	// Get iterator to current target
	auto Iterator = ObjectList.begin();
	if(ObjectList.size())
	Iterator = std::find(ObjectList.begin(), ObjectList.end(), ClientPlayer->Character->Targets.front());

	// Get target count
	std::size_t TargetCount;
	if(Item->TargetID == TargetType::ENEMY_ALL || Item->TargetID == TargetType::ALLY_ALL)
		TargetCount = BATTLE_MAX_OBJECTS_PER_SIDE;
	else
		TargetCount = ClientPlayer->Character->Targets.size();
	ClientPlayer->Character->Targets.clear();

	// Get max available targets
	std::size_t MaxTargets = 0;
	for(auto &Target : ObjectList) {
		if(Item->CanTarget(Scripting, ClientPlayer, Target))
			MaxTargets++;
	}

	// Cap target count
	TargetCount = std::min(TargetCount, MaxTargets);

	// Search for valid target
	_Object *NewTarget = nullptr;
	int TargetIndex = 1;
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
		if(Item->CanTarget(Scripting, ClientPlayer, NewTarget)) {
			ClientPlayer->Character->Targets.push_back(NewTarget);
			NewTarget->Fighter->TargetIndex = TargetIndex++;

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
	Object->Fighter->LastTarget.clear();
	Object->Character->Targets.clear();
	Object->Character->Action.Unset();
	Object->Fighter->PotentialAction.Unset();
	Object->Character->ResetUIState();
	Object->Fighter->JoinedBattle = Join;
	Object->Fighter->GoldStolen = 0;
	Object->Fighter->Corpse = 1;
	if(Server) {
		Object->Character->GenerateNextBattle();
		Object->Fighter->TurnTimer = std::clamp(ae::GetRandomReal(0, BATTLE_MAX_START_TURNTIMER) + Object->Character->Attributes["Initiative"].Mult(), 0.0, 1.0);

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
void _Battle::GetObjectList(int Side, std::vector<_Object *> &SideObjects) {
	for(auto &Object : Objects) {
		if(Object->Fighter->BattleSide == Side)
			SideObjects.push_back(Object);
	}
}

// Starts the battle and notifies the players
void _Battle::Serialize(ae::_Buffer &Data) {

	// Write object count
	Data.Write<uint8_t>((uint8_t)Objects.size());
	Data.Write<uint32_t>(Zone);

	// Write object information
	for(auto &Object : Objects)
		Object->SerializeBattle(Data);
}

// Unserialize for network
void _Battle::Unserialize(ae::_Buffer &Data, _HUD *HUD) {

	// Get object count
	int ObjectCount = Data.Read<uint8_t>();
	Zone = Data.Read<uint32_t>();

	// Get object information
	for(int i = 0; i < ObjectCount; i++) {

		// Get object data
		ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();
		uint32_t DatabaseID = Data.Read<uint32_t>();

		// Get object pointers
		_Object *Object = nullptr;
		if(DatabaseID) {
			Object = Manager->CreateWithID(NetworkID);
			Object->CreateComponents();
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
	std::vector<_Object *> SideObjects[2];
	SideObjects[0].reserve(BATTLE_MAX_OBJECTS_PER_SIDE);
	SideObjects[1].reserve(BATTLE_MAX_OBJECTS_PER_SIDE);
	for(int Side = 0; Side < 2; Side++) {

		// Get a list of objects that are still in the battle
		GetObjectList(Side, SideObjects[Side]);

		// Loop through objects
		for(auto &Object : SideObjects[Side]) {

			// Keep track of players
			if(!Object->IsMonster()) {
				if(Object->Character->IsAlive())
					SideStats[Side].AlivePlayerCount++;
				SideStats[Side].PlayerCount++;
			}
			else {
				if(Object->Character->IsAlive())
					SideStats[Side].AliveMonsterCount++;
				SideStats[Side].MonsterCount++;
			}

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
			SideStats[Side].TotalGoldStolen += Object->Fighter->GoldStolen;
			SideStats[Side].TotalBounty += Object->Character->Attributes["Bounty"].Int64;
			if(Object->IsMonster())
				SideStats[Side].TotalGoldGiven += Object->Monster->GoldGiven;
			else
				SideStats[Side].TotalGoldGiven += Object->Character->Attributes["Bounty"].Int64 + (int64_t)(Object->Character->Attributes["Gold"].Int64 * BountyEarned + 0.5f);
		}

		SideStats[Side].TotalExperienceGiven = std::ceil(SideStats[Side].TotalExperienceGiven);
		SideStats[Side].TotalGoldGiven = std::ceil(SideStats[Side].TotalGoldGiven);
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
			int DivideCount = SideStats[Side].AlivePlayerCount;
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
				SideStats[Side].GoldStolenPerCharacter = SideStats[OtherSide].TotalGoldStolen / DivideCount;
				if(SideStats[Side].GoldPerCharacter <= 0)
					SideStats[Side].GoldPerCharacter = 1;
			}
		}

		// Get list of objects that get rewards
		std::vector<_Object *> RewardObjects;
		RewardObjects.reserve(BATTLE_MAX_OBJECTS_PER_SIDE);
		for(auto &Object : SideObjects[WinningSide]) {
			if(Object->Character->IsAlive() && !Object->IsMonster())
				RewardObjects.push_back(Object);
		}

		// Check for reward recipients
		if(RewardObjects.size() && !PVP) {

			// Boss drops aren't divided up and only come from zonedrop
			if(Boss) {
				std::vector<std::pair<uint32_t, int>> ItemDrops;
				ItemDrops.reserve(10);

				// Get items from zonedrops
				Stats->Database->PrepareQuery("SELECT item_id, count FROM zonedrop WHERE zone_id = @zone_id");
				Stats->Database->BindInt(1, Zone);
				while(Stats->Database->FetchRow()) {
					uint32_t ItemID = Stats->Database->GetInt<uint32_t>("item_id");
					int Count = Stats->Database->GetInt<int>("count");
					ItemDrops.push_back(std::pair(ItemID, Count));
				}
				Stats->Database->CloseQuery();

				// Hand out drops
				for(auto &ItemDrop : ItemDrops) {
					for(auto &Object : RewardObjects) {

						// Give drops to players that don't have the boss on cooldown
						if(!Object->Character->IsZoneOnCooldown(Zone)) {
							int Count = ItemDrop.second * (Object->Character->Attributes["DropRate"].Mult() + 0.001f);
							for(int i = 0; i < Count; i++)
								Object->Fighter->ItemDropsReceived.push_back(ItemDrop.first);
						}
					}
				}
			}
			else {

				// Get shuffled copy of reward objects
				std::vector<_Object *> ShuffledRewardObjects { std::begin(RewardObjects), std::end(RewardObjects) };
				std::shuffle(ShuffledRewardObjects.begin(), ShuffledRewardObjects.end(), ae::RandomGenerator);

				// Iterate through monsters
				std::size_t PlayerIndex = 0;
				for(auto &Object : SideObjects[!WinningSide]) {
					if(Object->IsMonster()) {
						_Object *Player = ShuffledRewardObjects[PlayerIndex++];
						if(PlayerIndex >= ShuffledRewardObjects.size())
							PlayerIndex = 0;

						// Generate item
						std::vector<uint32_t> ItemDrops;
						ItemDrops.reserve(10);
						Stats->GenerateItemDrops(Object->Monster->DatabaseID, 1, ItemDrops, Player->Character->Attributes["DropRate"].Mult());
						for(auto &ItemID : ItemDrops)
							Player->Fighter->ItemDropsReceived.push_back(ItemID);
					}
				}
			}
		}
	}

	// Build results
	typedef std::pair<_Object *, const _Buff *> _SummonKey;
	std::map<_SummonKey, int> PlayerSummons;
	for(auto &Object : Objects) {
		Object->Controller->InputStates.clear();
		Object->Fighter->PotentialAction.Unset();
		Object->Character->Action.Unset();

		// Get rewards
		int64_t ExperienceEarned = 0;
		int64_t GoldEarned = 0;
		if(Object->Character->IsAlive()) {

			// Get rewards if boss isn't on cooldown
			if(!Object->Character->IsZoneOnCooldown(Zone)) {
				ExperienceEarned = SideStats[WinningSide].ExperiencePerCharacter;
				GoldEarned = SideStats[WinningSide].GoldPerCharacter;

				// Boost xp/gold gain
				if(!PVP) {
					ExperienceEarned *= Object->Character->Attributes["ExperienceBonus"].BonusMult();
					GoldEarned *= Object->Character->Attributes["GoldBonus"].BonusMult();
				}

				if(Zone) {

					// Start cooldown timer
					if(Cooldown > 0.0)
						Object->Character->BossCooldowns[Zone] = std::max(Cooldown * Object->Character->Attributes["BossCooldowns"].Mult(), 10.0);

					// Add to kill count
					if(Boss) {
						if(Cooldown >= 1000) {
							Object->Character->BossKills[Zone]++;
							if(Cooldown <= 1000000)
								Server->SendMessage(Object->Peer, std::string("The soul grows stronger"), "yellow");
						}
						else
							Object->Character->BossKills[Zone] = 0;
					}
				}
			}

			// Handle pickpocket
			GoldEarned += SideStats[WinningSide].GoldStolenPerCharacter;

			Object->Character->Attributes["PlayerKills"].Int += SideStats[!WinningSide].PlayerCount;
			Object->Character->Attributes["MonsterKills"].Int += SideStats[!WinningSide].MonsterCount;
			if(PVP && Object->Fighter->BattleSide == BATTLE_PVP_ATTACKER_SIDE) {
				if(BountyEarned) {
					Object->Character->Attributes["Bounty"].Int64 += GoldEarned;
					if(Object->Character->Attributes["Bounty"].Int64) {
						std::string BountyMessage = "Player " + Object->Name + " now has a bounty of " + std::to_string(Object->Character->Attributes["Bounty"].Int64) + " gold!";
						Server->BroadcastMessage(nullptr, BountyMessage, "cyan");
						Server->Log << "[BOUNTY] " << BountyMessage << std::endl;
					}
				}
			}
		}
		else {

			// Give stolen gold back if dead
			Object->Character->UpdateGold(-Object->Fighter->GoldStolen);

			if(PVP) {

				// Fugitive was bounty hunted, apply regular death penalty
				float AttackPenalty = BountyEarned;
				if(BountyEarned == 0.0)
					AttackPenalty = PLAYER_DEATH_GOLD_PENALTY;

				// Both sides died
				if(WinningSide == -1) {

					// Victim applies penalty first, then gets bounty share
					if(Object->Fighter->BattleSide == BATTLE_PVP_VICTIM_SIDE) {
						Object->ApplyDeathPenalty(true, PLAYER_DEATH_GOLD_PENALTY, 0);
						GoldEarned = SideStats[BATTLE_PVP_ATTACKER_SIDE].TotalBounty / SideStats[BATTLE_PVP_VICTIM_SIDE].PlayerCount;
					}
					// Attacker loses contract
					else {
						Object->ApplyDeathPenalty(true, AttackPenalty, Object->Character->Attributes["Bounty"].Int64);
					}
				}
				else {
					Object->ApplyDeathPenalty(true, AttackPenalty, Object->Character->Attributes["Bounty"].Int64);
				}
			}
			else
				Object->ApplyDeathPenalty(true, PLAYER_DEATH_GOLD_PENALTY, 0);
		}

		// Update stats
		int CurrentLevel = Object->Character->Level;
		Object->Fighter->GoldStolen = 0;
		Object->Character->UpdateExperience(ExperienceEarned);
		Object->Character->UpdateGold(GoldEarned);
		Object->Character->CalculateStats();
		int NewLevel = Object->Character->Level;
		if(NewLevel > CurrentLevel) {
			if(Object->Peer)
				Server->SendMessage(Object->Peer, std::string("You are now level " + std::to_string(NewLevel) + "!"), "gold");

			Object->Character->Attributes["Health"].Int = Object->Character->Attributes["MaxHealth"].Int;
			Object->Character->Attributes["Mana"].Int = Object->Character->Attributes["MaxMana"].Int;
		}

		// Build map of player summons
		_Object *Owner = Object->Monster->Owner;
		if(Owner && !Owner->IsMonster() && Object->Monster->SummonBuff) {
			_SummonKey Key(Owner, Object->Monster->SummonBuff);

			// Check if monster died
			if(!Object->Character->IsAlive()) {

				// Add at least one summon if owner also died
				if(!Owner->Character->IsAlive() && PlayerSummons.find(Key) == PlayerSummons.end())
					PlayerSummons[Key] = 0;
			}
			else
				PlayerSummons[Key]++;
		}

		// Get boss cooldown
		double BossCooldown = 0.0;
		if(Boss)
			BossCooldown = Object->Character->BossCooldowns[Zone];

		// Write results
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::BATTLE_END);
		Packet.Write<float>(BossCooldown);
		Packet.Write<int>(Object->Character->Attributes["PlayerKills"].Int);
		Packet.Write<int>(Object->Character->Attributes["MonsterKills"].Int);
		Packet.Write<int64_t>(Object->Character->Attributes["GoldLost"].Int64);
		Packet.Write<int64_t>(Object->Character->Attributes["Bounty"].Int64);
		Packet.Write<int64_t>(ExperienceEarned);
		Packet.Write<int64_t>(GoldEarned);

		// Sort item drops
		std::unordered_map<uint32_t, int> SortedItems;
		for(auto &ItemID : Object->Fighter->ItemDropsReceived) {
			SortedItems[ItemID]++;
		}
		Object->Fighter->ItemDropsReceived.clear();

		// Write item count
		std::size_t ItemCount = SortedItems.size();
		Packet.Write<uint8_t>((uint8_t)ItemCount);

		// Write items
		bool Full = false;
		for(auto &Iterator : SortedItems) {
			Packet.Write<uint32_t>(Iterator.first);
			Packet.Write<uint8_t>(0);
			Packet.Write<uint8_t>((uint8_t)Iterator.second);
			if(!Object->Inventory->AddItem(Stats->Items.at(Iterator.first), 0, Iterator.second))
				Full = true;
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
			if(Full)
				Server->SendInventoryFullMessage(Object->Peer);
			Server->Network->SendPacket(Packet, Object->Peer);
			Server->SendHUD(Object->Peer);
		}
	}

	// Handle summon updates
	for(auto &PlayerSummon : PlayerSummons) {

		// Update summon buff
		_StatChange Summons;
		Summons.Object = PlayerSummon.first.first;
		Summons.Values["Buff"].Pointer = (void *)PlayerSummon.first.second;
		Summons.Values["BuffLevel"].Int = std::max(1, PlayerSummon.second);
		Summons.Values["BuffDuration"].Float = -1;
		Summons.Object->UpdateStats(Summons, Summons.Object);

		// Sync status effects
		ae::_Buffer Packet;
		Packet.Write<PacketType>(PacketType::PLAYER_STATUSEFFECTS);
		Packet.Write<ae::NetworkIDType>(Summons.Object->NetworkID);
		Summons.Object->SerializeStatusEffects(Packet);
		Server->Network->SendPacket(Packet, Summons.Object->Peer);
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
		Object->Fighter->BattleBaseOffset.x = -240 - Column * BATTLE_COLUMN_SPACING;
	else
		Object->Fighter->BattleBaseOffset.x = 100  + Column * BATTLE_COLUMN_SPACING;

	// Get row count for a given column
	float RowCount = (float)SideCount[Object->Fighter->BattleSide] / BATTLE_ROWS_PER_SIDE - Column;
	if(RowCount >= 1)
		RowCount = BATTLE_ROWS_PER_SIDE;
	else
		RowCount *= BATTLE_ROWS_PER_SIDE;

	// Divide space into RowCount parts, then divide that by 2
	int SpacingY = (int)((BattleElement->BaseSize.y / RowCount) / 2);

	// Place slots in between main divisions
	Object->Fighter->BattleBaseOffset.y = SpacingY * (2 * (SideIndex % BATTLE_ROWS_PER_SIDE) + 1) - BattleElement->BaseSize.y/2;
}

// Adjust existing battle elements
void _Battle::AdjustBattleElements(int SideIndex, _Object *Object) {

	// Get position on screen
	GetBattleOffset(SideIndex, Object);

	// Update position
	if(Object->Fighter->BattleElement) {
		Object->Fighter->BattleElement->BaseOffset = Object->Fighter->BattleBaseOffset;
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

	// Remove object from last target array, status effect's source object and monster owners
	for(auto Iterator = Objects.begin(); Iterator != Objects.end(); ++Iterator) {
		_Object *Object = *Iterator;
		if(!Object)
			continue;

		if(Object->Fighter) {
			for(auto &Entry : Object->Fighter->LastTarget) {
				if(Entry.second == RemoveObject)
					Object->Fighter->LastTarget[Entry.first] = nullptr;
			}
		}

		if(Object->Monster && Object->Monster->Owner == RemoveObject)
			Object->Monster->Owner = nullptr;

		if(Object->Character) {

			// Remove source in status effect
			for(auto &StatusEffect : Object->Character->StatusEffects) {
				if(RemoveObject == StatusEffect->Source)
					StatusEffect->Source = nullptr;
			}

			// Remove from targets
			for(auto Iterator = Object->Character->Targets.begin(); Iterator != Object->Character->Targets.end(); ) {
				if(*Iterator == RemoveObject)
					Iterator = Object->Character->Targets.erase(Iterator);
				else
					++Iterator;
			}
		}
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
void _Battle::GetSeparateObjectList(uint8_t Side, std::vector<_Object *> &Allies, std::vector<_Object *> &Enemies) {
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

// Handle player input, return true if mouse combat should be enabled
bool _Battle::ClientHandleInput(std::size_t Action, bool MouseCombat) {

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
			return MouseCombat;
		break;
		case Action::GAME_ITEM1:
		case Action::GAME_ITEM2:
		case Action::GAME_ITEM3:
		case Action::GAME_ITEM4:
			ClientSetAction((uint8_t)(Action - Action::GAME_ITEM1 + ACTIONBAR_BELT_STARTS));
			return MouseCombat;
		break;
		case Action::GAME_UP:
			ClientChangeTarget(-1, false);
			return false;
		break;
		case Action::GAME_DOWN:
			ClientChangeTarget(1, false);
			return false;
		break;
		case Action::GAME_LEFT:
		case Action::GAME_RIGHT:
			ClientChangeTarget(0, true);
			return false;
		break;
	}

	return false;
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

// Broadcast an object's current list of status effects
void _Battle::BroadcastStatusEffects(_Object *UpdatedObject) {
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::PLAYER_STATUSEFFECTS);
	Packet.Write<ae::NetworkIDType>(UpdatedObject->NetworkID);
	UpdatedObject->SerializeStatusEffects(Packet);

	// Send packet to all players
	for(auto &Object : Objects) {
		if(UpdatedObject != Object && !Object->Deleted && Object->Peer) {
			Server->Network->SendPacket(Packet, Object->Peer);
		}
	}
}
