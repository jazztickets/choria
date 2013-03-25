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
#include "player.h"
#include <enet/enet.h>
#include "../engine/globals.h"
#include "../engine/graphics.h"
#include "../engine/instances.h"
#include "../engine/random.h"
#include "../engine/stats.h"
#include "../engine/database.h"
#include "../engine/constants.h"
#include "../instances/map.h"
#include "../instances/battle.h"
#include "../playserver.h"
#include "skill.h"

// Constructor
PlayerClass::PlayerClass()
:	ObjectClass(PLAYER),
	FighterClass(TYPE_PLAYER),
	AccountID(0),
	CharacterID(0),
	MoveTime(0),
	PortraitID(0),
	Gold(0),
	Experience(0),
	SpawnMapID(1),
	SpawnPoint(0),
	SkillPoints(0),
	SkillPointsUsed(0),
	WeaponDamageModifier(0.0f),
	WeaponMinDamage(0),
	WeaponMaxDamage(0),
	ArmorMinDefense(0),
	ArmorMaxDefense(0),
	MinDamageBonus(0),
	MaxDamageBonus(0),
	MinDefenseBonus(0),
	MaxDefenseBonus(0),
	PlayTime(0),
	PlayTimeAccumulator(0),
	Deaths(0),
	MonsterKills(0),
	PlayerKills(0),
	Bounty(0),
	AttackPlayerTime(0),
	Vendor(NULL),
	StateImage(NULL),
	Database(NULL),
	TradeRequestTime(0),
	TradePlayer(NULL),
	TradeAccepted(false),
	TradeGold(0),
	TownPortalTime(0),
	InvisPower(0),
	State(STATE_WALK) {

	WorldImage = irrDriver->getTexture(WorkingDirectory + "textures/players/basic.png");

	Position.X = 0;
	Position.Y = 0;

	// Reset number of potions used in battle
	for(int i = 0; i < 2; i++)
		MaxPotions[i] = PotionsLeft[i] = 0;

	// Reset items
	for(int i = 0; i < PlayerClass::INVENTORY_COUNT; i++) {
		Inventory[i].Item = NULL;
		Inventory[i].Count = 0;
	}

	// Reset skills
	for(int i = 0; i < PlayerClass::SKILL_COUNT; i++) {
		SkillLevels[i] = 0;
	}

	GenerateNextBattle();
}

// Destructor
PlayerClass::~PlayerClass() {

	if(Map)
		Map->RemoveObject(this);
}

// Updates the player
void PlayerClass::Update(u32 TDeltaTime) {

	MoveTime += TDeltaTime;
	AutoSaveTime += TDeltaTime;
	AttackPlayerTime += TDeltaTime;
	TradeRequestTime += TDeltaTime;
	PlayTimeAccumulator += TDeltaTime;
	TownPortalTime += TDeltaTime;
	if(PlayTimeAccumulator > 1000) {
		PlayTimeAccumulator -= 1000;
		PlayTime++;
	}

	// Run server commands
	if(Database) {
		if(AutoSaveTime >= 60000)
			Save();

		if(State == STATE_TOWNPORTAL && TownPortalTime > GAME_PORTALTIME) {
			PlayServerState::Instance()->PlayerTownPortal(this);
			State = STATE_WALK;
		}
	}
}

// Renders the player while walking around the world
void PlayerClass::RenderWorld(const MapClass *TMap) {
	if(TMap) {

		position2di ScreenPosition;
		bool OnScreen = TMap->GridToScreen(Position, ScreenPosition);

		if(OnScreen) {
			int Alpha = 255;
			if(IsInvisible())
				Alpha = 70;
				
			Graphics::Instance().DrawCenteredImage(WorldImage, ScreenPosition.X, ScreenPosition.Y, SColor(Alpha, 255, 255, 255));
			if(StateImage) {
				Graphics::Instance().DrawCenteredImage(StateImage, ScreenPosition.X, ScreenPosition.Y, SColor(Alpha, 255, 255, 255));
			}
		}
	}
}

// Moves the player
bool PlayerClass::MovePlayer(int TDirection) {
	if(State != STATE_WALK)
		return false;

	// Get new position
	position2di NewPosition = Position;
	switch(TDirection) {
		case MOVE_LEFT:
			NewPosition.X--;
		break;
		case MOVE_UP:
			NewPosition.Y--;
		break;
		case MOVE_RIGHT:
			NewPosition.X++;
		break;
		case MOVE_DOWN:
			NewPosition.Y++;
		break;
	}

	// Move player
	bool Moved = false;
	if(Map->CanMoveTo(NewPosition)) {
		Moved = true;
		MoveTime = 0;
		Position = NewPosition;
		if(InvisPower > 0)
			InvisPower--;
		else
			NextBattle--;
			
		// Update regen
		int HealthUpdate, ManaUpdate;
		UpdateRegen(HealthUpdate, ManaUpdate);
		UpdateHealth(HealthUpdate);
		UpdateMana(ManaUpdate);
	}

	return Moved;
}

// Saves the player
void PlayerClass::Save() {
	if(!Database || CharacterID == 0)
		return;

	//printf("Saving Player\n");

	char Query[512];
	Database->RunQuery("BEGIN TRANSACTION");

	// Save character stats
	sprintf(Query, "UPDATE Characters SET "
					"MapID = %d"
					", SpawnPoint = %d"
					", Experience = %d"
					", Gold = %d"
					", SkillBar0 = %d"
					", SkillBar1 = %d"
					", SkillBar2 = %d"
					", SkillBar3 = %d"
					", SkillBar4 = %d"
					", SkillBar5 = %d"
					", SkillBar6 = %d"
					", SkillBar7 = %d"
					", PlayTime = %u"
					", Deaths = %d"
					", MonsterKills = %d"
					", PlayerKills = %d"
					", Bounty = %d"
					" WHERE ID = %d",
					SpawnMapID,
					SpawnPoint,
					Experience,
					Gold,
					GetSkillBarID(0),
					GetSkillBarID(1),
					GetSkillBarID(2),
					GetSkillBarID(3),
					GetSkillBarID(4),
					GetSkillBarID(5),
					GetSkillBarID(6),
					GetSkillBarID(7),
					PlayTime,
					Deaths,
					MonsterKills,
					PlayerKills,
					Bounty,
					CharacterID
					);
	Database->RunQuery(Query);

	// Save items
	sprintf(Query, "DELETE FROM Inventory WHERE CharactersID = %d", CharacterID);
	Database->RunQuery(Query);

	const InventoryStruct *Item;
	for(int i = 0; i < INVENTORY_COUNT; i++) {
		Item = GetInventory(i);
		if(Item->Item) {
 			sprintf(Query, "INSERT INTO Inventory VALUES(%d, %d, %d, %d)", CharacterID, i, Item->Item->GetID(), Item->Count);
			Database->RunQuery(Query);
		}
	}

	// Save skill points
	sprintf(Query, "DELETE FROM SkillLevel WHERE CharactersID = %d", CharacterID);
	Database->RunQuery(Query);

	for(int i = 0; i < PlayerClass::SKILL_COUNT; i++) {
		int Points = GetSkillLevel(i);
		if(Points > 0) {
 			sprintf(Query, "INSERT INTO SkillLevel VALUES(%d, %d, %d)", CharacterID, i, Points);
			Database->RunQuery(Query);
		}
	}

	Database->RunQuery("END TRANSACTION");

	AutoSaveTime = 0;
}

// Get the zone that the player is standing in
int PlayerClass::GetCurrentZone() {

	return GetTile()->Zone;
}

// Gets the tile that the player is currently standing on
const TileStruct *PlayerClass::GetTile() {

	return Map->GetTile(Position.X, Position.Y);
}

// Generates the number of moves until the next battle
void PlayerClass::GenerateNextBattle() {

	NextBattle = Random::Instance().GenerateRange(4, 14);
	//NextBattle = Random::Instance().GenerateRange(1, 2);
}

// Starts a battle
void PlayerClass::StartBattle(BattleClass *TBattle) {

	State = STATE_BATTLE;
	Battle = TBattle;
	Command = -1;
	for(int i = 0; i < 2; i++)
		PotionsLeft[i] = MaxPotions[i];
}

// Stop a battle
void PlayerClass::StopBattle() {

	State = STATE_WALK;
	Battle = NULL;
	GenerateNextBattle();
}

// Determines if a player can attack
bool PlayerClass::CanAttackPlayer() {

	return State == PlayerClass::STATE_WALK && AttackPlayerTime > PLAYER_ATTACKTIME;
}

// Update gold amount
void PlayerClass::UpdateGold(int TValue) {

	Gold += TValue;
	if(Gold < 0)
		Gold = 0;
	else if(Gold > STATS_MAXGOLD)
		Gold = STATS_MAXGOLD;
}

// Get the percentage to the next level
float PlayerClass::GetNextLevelPercent() const {
	float Percent = 0;

	if(ExperienceNextLevel > 0)
		Percent = 1.0f - (f32)ExperienceNeeded / ExperienceNextLevel;

	return Percent;
}

// Sets the player's portrait
void PlayerClass::SetPortraitID(int TID) {
	PortraitID = TID;
	Portrait = Stats::Instance().GetPortrait(PortraitID)->Image;
}

// Sets the player's vendor
void PlayerClass::SetVendor(const VendorStruct *TVendor) {

	Vendor = TVendor;
}

// Gets the player's vendor
const VendorStruct *PlayerClass::GetVendor() {

	return Vendor;
}

// Sets the player's trader
void PlayerClass::SetTrader(const TraderStruct *TTrader) {

	Trader = TTrader;
}

// Gets the player's trader
const TraderStruct *PlayerClass::GetTrader() {

	return Trader;
}

// Fills an array with inventory indices correlating to a trader's required items
int PlayerClass::GetRequiredItemSlots(const TraderStruct *TTrader, int *TSlots) {
	int RewardItemSlot = -1;

	// Check for an open reward slot
	for(int i = INVENTORY_BACKPACK; i < INVENTORY_TRADE; i++) {
		InventoryStruct *InventoryItem = &Inventory[i];
		if(InventoryItem->Item == NULL || InventoryItem->Item == TTrader->RewardItem && InventoryItem->Count + TTrader->Count <= 255) {
			RewardItemSlot = i;
			break;
		}		
	}

	// Go through required items
	for(u32 i = 0; i < TTrader->TraderItems.size(); i++) {
		const ItemClass *RequiredItem = TTrader->TraderItems[i].Item;
		int RequiredCount = TTrader->TraderItems[i].Count;
		TSlots[i] = -1;

		// Search for the required item
		for(int j = INVENTORY_HEAD; j < INVENTORY_TRADE; j++) {
			InventoryStruct *InventoryItem = &Inventory[j];
			if(InventoryItem->Item == RequiredItem && InventoryItem->Count >= RequiredCount) {
				TSlots[i] = j;
				break;
			}
		}

		// Didn't find an item
		if(TSlots[i] == -1)
			RewardItemSlot = -1;
	}

	return RewardItemSlot;
}

// Accept a trade from a trader
void PlayerClass::AcceptTrader(const TraderStruct *TTrader, int *TSlots, int TRewardSlot) {
	if(TTrader == NULL || !IsSlotInventory(TRewardSlot))
		return;

	// Trade in required items
	for(u32 i = 0; i < TTrader->TraderItems.size(); i++) {
		UpdateInventory(TSlots[i], -TTrader->TraderItems[i].Count);
	}

	// Give player reward
	AddItem(TTrader->RewardItem, TTrader->Count, TRewardSlot);
}

// Finds a potion in the player's inventory for use in battle
int PlayerClass::GetPotionBattle(int TType) {

	// Check for skill uses
	if(PotionsLeft[TType] <= 0)
		return -1;

	// Search
	for(int i = INVENTORY_BACKPACK; i < INVENTORY_TRADE; i++) {
		if(Inventory[i].Item && Inventory[i].Item->IsPotionType(TType))
			return i;
	}

	return -1;
}

// Uses a potion in battle
bool PlayerClass::UsePotionBattle(int TSlot, int TSkillType, int &THealthChange, int &TManaChange) {
	const ItemClass *Item = GetInventoryItem(TSlot);
	if(Item == NULL || Item->GetType() != ItemClass::TYPE_POTION)
		return false;

	THealthChange = Item->GetHealthRestore();
	TManaChange = Item->GetManaRestore();
	UpdatePotionsLeft(TSkillType);
	UpdateInventory(TSlot, -1);

	return true;
}

// Uses a potion in the world
bool PlayerClass::UsePotionWorld(int TSlot) {
	const ItemClass *Item = GetInventoryItem(TSlot);
	if(Item == NULL)
		return false;

	// Get potion stats
	int HealthRestore = Item->GetHealthRestore();
	int ManaRestore = Item->GetManaRestore();
	int ItemInvisPower = Item->GetInvisPower();

	// Use only if needed
	if(Item->IsHealthPotion() && Health < MaxHealth || Item->IsManaPotion() && Mana < MaxMana || Item->IsInvisPotion()) {
		UpdateHealth(HealthRestore);
		UpdateMana(ManaRestore);
		SetInvisPower(ItemInvisPower);
		UpdateInventory(TSlot, -1);
		return true;
	}

	return false;
}

// Uses an item from the inventory
bool PlayerClass::UseInventory(int TSlot) {
	const ItemClass *Item = GetInventoryItem(TSlot);
	if(Item == NULL)
		return false;

	// Handle item types
	switch(Item->GetType()) {
		case ItemClass::TYPE_POTION:
			return UsePotionWorld(TSlot);
		break;
	}

	return true;
}

// Sets an item in the inventory
void PlayerClass::SetInventory(int TSlot, int TItemID, int TCount) {

	if(TItemID == 0) {
		Inventory[TSlot].Item = NULL;
		Inventory[TSlot].Count = 0;
	}
	else {
		Inventory[TSlot].Item = Stats::Instance().GetItem(TItemID);
		Inventory[TSlot].Count = TCount;
	}
}

// Sets an item in the inventory
void PlayerClass::SetInventory(int TSlot, InventoryStruct *TItem) {

	if(TItem->Item == NULL) {
		Inventory[TSlot].Item = NULL;
		Inventory[TSlot].Count = 0;
	}
	else {
		Inventory[TSlot].Item = TItem->Item;
		Inventory[TSlot].Count = TItem->Count;
	}
}

// Returns an item from the inventory
InventoryStruct *PlayerClass::GetInventory(int TSlot) {

	return &Inventory[TSlot];
}

// Gets an inventory item
const ItemClass *PlayerClass::GetInventoryItem(int TSlot) {

	// Check for bad slots
	if(TSlot < INVENTORY_BACKPACK || TSlot >= INVENTORY_TRADE || !Inventory[TSlot].Item)
		return NULL;

	return Inventory[TSlot].Item;
}

// Moves an item from one slot to another
bool PlayerClass::MoveInventory(int TOldSlot, int TNewSlot) {
	if(TOldSlot == TNewSlot)
		return false;

	// Equippable items
	if(TNewSlot < INVENTORY_BACKPACK) {

		// Check if the item is even equippable
		if(!CanEquipItem(TNewSlot, Inventory[TOldSlot].Item))
			return false;

		// Split stacks
		if(Inventory[TOldSlot].Count > 1) {
			Inventory[TNewSlot].Item = Inventory[TOldSlot].Item;
			Inventory[TNewSlot].Count = 1;
			Inventory[TOldSlot].Count--;
		}
		else
			SwapItem(TNewSlot, TOldSlot);

		return true;
	}
	// Back pack
	else {

		// Add to stack
		if(Inventory[TNewSlot].Item == Inventory[TOldSlot].Item) {
			Inventory[TNewSlot].Count += Inventory[TOldSlot].Count;

			// Group stacks
			if(Inventory[TNewSlot].Count > 255) {
				Inventory[TOldSlot].Count = Inventory[TNewSlot].Count - 255;
				Inventory[TNewSlot].Count = 255;
			}
			else
				Inventory[TOldSlot].Item = NULL;
		}
		else {

			// Disable reverse equip for now
			if(TOldSlot < INVENTORY_BACKPACK && Inventory[TNewSlot].Item)
				return false;

			SwapItem(TNewSlot, TOldSlot);
		}

		return true;
	}
}

// Swaps two items
void PlayerClass::SwapItem(int TSlot, int TOldSlot) {
	InventoryStruct TempItem;

	// Swap items
	TempItem = Inventory[TSlot];
	Inventory[TSlot] = Inventory[TOldSlot];
	Inventory[TOldSlot] = TempItem;
}

// Updates an item's count, deleting if necessary
bool PlayerClass::UpdateInventory(int TSlot, int TAmount) {

	Inventory[TSlot].Count += TAmount;
	if(Inventory[TSlot].Count <= 0) {
		Inventory[TSlot].Item = NULL;
		Inventory[TSlot].Count = 0;
		return true;
	}

	return false;
}

// Attempts to add an item to the inventory
bool PlayerClass::AddItem(const ItemClass *TItem, int TCount, int TSlot) {

	// Place somewhere in backpack
	if(TSlot == -1) {

		// Find existing item
		int EmptySlot = -1;
		for(int i = INVENTORY_BACKPACK; i < INVENTORY_TRADE; i++) {
			if(Inventory[i].Item == TItem && Inventory[i].Count + TCount <= 255) {
				Inventory[i].Count += TCount;
				return true;
			}

			// Keep track of the first empty slot
			if(Inventory[i].Item == NULL && EmptySlot == -1)
				EmptySlot = i;
		}

		// Found an empty slot
		if(EmptySlot != -1) {
			Inventory[EmptySlot].Item = TItem;
			Inventory[EmptySlot].Count = TCount;
			return true;
		}

		return false;
	}
	// Trying to equip an item
	else if(TSlot < INVENTORY_BACKPACK) {

		// Make sure it can be equipped
		if(!CanEquipItem(TSlot, TItem))
			return false;

		// Set item
		Inventory[TSlot].Item = TItem;
		Inventory[TSlot].Count = TCount;

		return true;
	}

	// Add item
	if(Inventory[TSlot].Item == TItem && Inventory[TSlot].Count + TCount <= 255) {
		Inventory[TSlot].Count += TCount;
		return true;
	}
	else if(Inventory[TSlot].Item == NULL) {
		Inventory[TSlot].Item = TItem;
		Inventory[TSlot].Count = TCount;
		return true;
	}

	return false;
}

// Moves the player's trade items to their backpack
void PlayerClass::MoveTradeToInventory() {

	for(int i = INVENTORY_TRADE; i < INVENTORY_COUNT; i++) {
		if(Inventory[i].Item && AddItem(Inventory[i].Item, Inventory[i].Count, -1))
			Inventory[i].Item = NULL;
	}
}

// Splits a stack
void PlayerClass::SplitStack(int TSlot, int TCount) {
	if(TSlot < 0 || TSlot >= INVENTORY_COUNT)
		return;

	// Make sure stack is large enough
	InventoryStruct *SplitItem = &Inventory[TSlot];
	if(SplitItem->Item && SplitItem->Count > TCount) {

		// Find an empty slot or existing item
		int EmptySlot = TSlot;
		InventoryStruct *Item;
		do {
			EmptySlot++;
			if(EmptySlot >= INVENTORY_TRADE)
				EmptySlot = INVENTORY_BACKPACK;

			Item = &Inventory[EmptySlot];
		} while(!(EmptySlot == TSlot || Item->Item == NULL || (Item->Item == SplitItem->Item && Item->Count <= 255 - TCount)));

		// Split item
		if(EmptySlot != TSlot) {
			SplitItem->Count -= TCount;
			AddItem(SplitItem->Item, TCount, EmptySlot);
		}
	}	
}

// Determines if the player's backpack is full
bool PlayerClass::IsBackpackFull() {

	// Search backpack
	for(int i = INVENTORY_BACKPACK; i < INVENTORY_TRADE; i++) {
		if(Inventory[i].Item == NULL)
			return false;
	}

	return true;
}

// Checks if an item can be equipped
bool PlayerClass::CanEquipItem(int TSlot, const ItemClass *TItem) {

	// Already equipped
	if(Inventory[TSlot].Item)
		return false;

	// Check type
	switch(TSlot) {
		case INVENTORY_HEAD:
			if(TItem->GetType() == ItemClass::TYPE_HEAD)
				return true;
		break;
		case INVENTORY_BODY:
			if(TItem->GetType() == ItemClass::TYPE_BODY)
				return true;
		break;
		case INVENTORY_LEGS:
			if(TItem->GetType() == ItemClass::TYPE_LEGS)
				return true;
		break;
		case INVENTORY_HAND1:
			if(TItem->GetType() == ItemClass::TYPE_WEAPON1HAND)
				return true;
		break;
		case INVENTORY_HAND2:
			if(TItem->GetType() == ItemClass::TYPE_SHIELD)
				return true;
		break;
		case INVENTORY_RING1:
		case INVENTORY_RING2:
			if(TItem->GetType() == ItemClass::TYPE_RING)
				return true;
		break;
		default:
		break;
	}

	return false;
}

// Updates a skill level
void PlayerClass::AdjustSkillLevel(int TSkillID, int TAdjust) {
	const SkillClass *Skill = Stats::Instance().GetSkill(TSkillID);
	if(Skill == NULL)
		return;

	// Buying
	if(TAdjust > 0) {
		if(Skill->GetSkillCost() > GetSkillPointsRemaining() || SkillLevels[TSkillID] >= 255)
			return;

		// Update level
		SkillLevels[TSkillID] += TAdjust;
		if(SkillLevels[TSkillID] > 255)
			SkillLevels[TSkillID] = 255;
	}
	else if(TAdjust < 0) {
		int SellCost = Skill->GetSellCost(Level);
		if(SellCost > Gold || SkillLevels[TSkillID] == 0)
			return;

		// Update level
		SkillLevels[TSkillID] += TAdjust;
		if(SkillLevels[TSkillID] < 0)
			SkillLevels[TSkillID] = 0;

		UpdateGold(-SellCost);

		// Update skill bar
		if(SkillLevels[TSkillID] == 0) {
			for(int i = 0; i < 8; i++) {
				if(SkillBar[i] == Skill) {
					SkillBar[i] = NULL;
					break;
				}
			}
		}
	}
}

// Calculates the number of skill points used
void PlayerClass::CalculateSkillPoints() {

	SkillPointsUsed = 0;
	const array<SkillClass> &Skills = Stats::Instance().GetSkillList();
	for(u32 i = 0; i < Skills.size(); i++) {
		SkillPointsUsed += Skills[i].GetSkillCost() * SkillLevels[i];
	}
}

// Toggles the player's busy state
void PlayerClass::ToggleBusy(bool TValue) {

	if(TValue && State == STATE_WALK) {
		State = STATE_BUSY;
	}
	else if(!TValue && State == STATE_BUSY) {
		State = STATE_WALK;
	}
}

// Starts the town portal process
void PlayerClass::StartTownPortal() {
	if(State == STATE_TOWNPORTAL)
		State = STATE_WALK;
	else if(State == STATE_WALK)
		State = STATE_TOWNPORTAL;

	TownPortalTime = 0;
}

// Calculates all of the player stats
void PlayerClass::CalculatePlayerStats() {
	HealthRegen = ManaRegen = 0.0f;
	MinDamage = MaxDamage = MinDefense = MaxDefense = 0;
	MinDamageBonus = MaxDamageBonus = MinDefenseBonus = MaxDefenseBonus = 0;
	WeaponMinDamage = WeaponMaxDamage = 0;
	ArmorMinDefense = ArmorMaxDefense = 0;
	WeaponDamageModifier = 1.0f;

	// Get base stats
	CalculateLevelStats();

	// Get gear stats
	CalculateGearStats();

	// Get skill bonus
	CalculateSkillStats();

	// Combine all stats
	CalculateFinalStats();

	// Cap stats
	UpdateHealth(0);
	UpdateMana(0);
}

// Calculates the base level stats
void PlayerClass::CalculateLevelStats() {

	// Cap min experience
	if(Experience < 0)
		Experience = 0;

	// Cap max experience
	const LevelStruct *MaxLevelStat = Stats::Instance().GetLevel(Stats::Instance().GetMaxLevel());
	if(Experience > MaxLevelStat->Experience)
		Experience = MaxLevelStat->Experience;

	// Find current level
	const LevelStruct *LevelStat = Stats::Instance().FindLevel(Experience);
	Level = LevelStat->Level;
	MaxHealth = LevelStat->Health;
	MaxMana = LevelStat->Mana;
	SkillPoints = LevelStat->SkillPoints;
	ExperienceNextLevel = LevelStat->NextLevel;
	if(Level == Stats::Instance().GetMaxLevel())
		ExperienceNeeded = 0;
	else
		ExperienceNeeded = LevelStat->NextLevel - (Experience - LevelStat->Experience);

}

// Calculates stats from equipped items
void PlayerClass::CalculateGearStats() {

	// Get stats
	if(!Inventory[INVENTORY_HAND1].Item)
		WeaponMinDamage = WeaponMaxDamage = 1;

	// Check each item
	for(int i = 0; i < INVENTORY_BACKPACK; i++) {
		const ItemClass *Item = Inventory[i].Item;
		if(Item) {
			int Min, Max;

			// Add damage
			Item->GetDamageRange(Min, Max);
			WeaponMinDamage += Min;
			WeaponMaxDamage += Max;

			// Add defense
			Item->GetDefenseRange(Min, Max);
			ArmorMinDefense += Min;
			ArmorMaxDefense += Max;

			// Boosts
			MaxHealth += Item->GetMaxHealth();
			MaxMana += Item->GetMaxMana();
			HealthRegen += Item->GetHealthRegen();
			ManaRegen += Item->GetManaRegen();
		}
	}
}

// Calculates skill bonuses
void PlayerClass::CalculateSkillStats() {

	// Go through each skill bar
	for(int i = 0; i < 8; i++) {
		const SkillClass *Skill = SkillBar[i];
		if(Skill) {
			int Min, Max, MinRound, MaxRound;
			float MinFloat, MaxFloat;
			Skill->GetPowerRange(SkillLevels[Skill->GetID()], Min, Max);
			Skill->GetPowerRangeRound(SkillLevels[Skill->GetID()], MinRound, MaxRound);
			Skill->GetPowerRange(SkillLevels[Skill->GetID()], MinFloat, MaxFloat);

			switch(Skill->GetID()) {
				case 0:
					WeaponDamageModifier = MaxFloat;
				break;
				case 1:
					MaxPotions[0] = Min;
				break;
				case 2:
					MaxPotions[1] = Min;
				break;
				case 4:
					MaxHealth += MaxRound;
				break;
				case 5:
					MaxMana += MaxRound;
				break;
				case 7:
					HealthRegen += MaxFloat;
				break;
				case 8:
					ManaRegen += MaxFloat;
				break;
				case 9:
					MinDamageBonus += Max;
					MaxDamageBonus += Max;
				break;
				case 10:
					MaxDefenseBonus += Max;
				break;
			}
		}
	}
}

// Combine all stats
void PlayerClass::CalculateFinalStats() {
	MinDamage = MinDamageBonus + (int)round_(WeaponMinDamage * WeaponDamageModifier);
	MaxDamage = MaxDamageBonus + (int)round_(WeaponMaxDamage * WeaponDamageModifier);
	if(MinDamage < 0)
		MinDamage = 0;
	if(MaxDamage < 0)
		MaxDamage = 0;

	MinDefense = ArmorMinDefense + MinDefenseBonus;
	MaxDefense = ArmorMaxDefense + MaxDefenseBonus;
	if(MinDefense < 0)
		MinDefense = 0;
	if(MaxDefense < 0)
		MaxDefense = 0;
}
