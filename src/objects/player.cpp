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
#include "player.h"
#include <enet/enet.h>
#include "../engine/globals.h"
#include "../engine/graphics.h"
#include "../engine/instances.h"
#include "../engine/random.h"
#include "../engine/stats.h"
#include "../engine/database.h"
#include "../instances/map.h"
#include "../instances/battle.h"
#include "../server.h"
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
	ActionBarLength(0),
	TradeRequestTime(0),
	TradePlayer(NULL),
	TradeAccepted(false),
	TradeGold(0),
	TownPortalTime(0),
	State(STATE_WALK) {

	WorldImage = irrDriver->getTexture("textures/players/basic.png");

	Position.X = 0;
	Position.Y = 0;

	// Reset items
	for(int i = 0; i < PlayerClass::INVENTORY_COUNT; i++) {
		Inventory[i].Item = NULL;
		Inventory[i].Count = 0;
	}

	GenerateNextBattle();
}

// Destructor
PlayerClass::~PlayerClass() {

	if(Map)
		Map->RemoveObject(this);
}

// Updates the player
void PlayerClass::Update(u32 FrameTime) {

	MoveTime += FrameTime;
	AutoSaveTime += FrameTime;
	AttackPlayerTime += FrameTime;
	TradeRequestTime += FrameTime;
	PlayTimeAccumulator += FrameTime;
	TownPortalTime += FrameTime;
	if(PlayTimeAccumulator > 1000) {
		PlayTimeAccumulator -= 1000;
		PlayTime++;
	}

	// Run server commands
	if(Database) {
		if(AutoSaveTime >= 60000)
			Save();

		if(State == STATE_TOWNPORTAL && TownPortalTime > 5000) {
			ServerState.PlayerTownPortal(this);
			State = STATE_WALK;
		}
	}
}

// Renders the player while walking around the world
void PlayerClass::RenderWorld() {
	if(Map) {

		position2di ScreenPosition;
		bool OnScreen = Map->GridToScreen(Position, ScreenPosition);

		if(OnScreen) {
			Graphics.DrawCenteredImage(WorldImage, ScreenPosition.X, ScreenPosition.Y, SColor(255, 255, 255, 255));
			if(StateImage) {
				Graphics.DrawCenteredImage(StateImage, ScreenPosition.X, ScreenPosition.Y, SColor(255, 255, 255, 255));
			}
		}
	}
}

// Moves the player
bool PlayerClass::MovePlayer(int Direction) {
	if(State != STATE_WALK)
		return false;

	// Get new position
	position2di NewPosition = Position;
	switch(Direction) {
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
					", ActionBarLength = %d"
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
					ActionBarLength,
					PlayTime,
					Deaths,
					MonsterKills,
					PlayerKills,
					Bounty,
					CharacterID
					);
	Database->RunQuery(Query);

	// Delete old items
	sprintf(Query, "DELETE FROM Inventory WHERE CharactersID = %d", CharacterID);
	Database->RunQuery(Query);

	// Add new items
	const InventoryStruct *Item;
	for(int i = 0; i < INVENTORY_COUNT; i++) {
		Item = GetInventory(i);
		if(Item->Item) {
 			sprintf(Query, "INSERT INTO Inventory VALUES(%d, %d, %d, %d)", CharacterID, i, Item->Item->GetID(), Item->Count);
			Database->RunQuery(Query);
		}
	}

	// Delete old skills
	sprintf(Query, "DELETE FROM Skills WHERE CharactersID = %d", CharacterID);
	Database->RunQuery(Query);

	// Add new skills
	for(u32 i = 0; i < SkillsUnlocked.size(); i++) {
		sprintf(Query, "INSERT INTO Skills VALUES(%d, %d)", CharacterID, SkillsUnlocked[i]->GetID());
		Database->RunQuery(Query);
	}

	// Delete old action bar
	sprintf(Query, "DELETE FROM Actionbar WHERE CharactersID = %d", CharacterID);
	Database->RunQuery(Query);

	// Add new action bar
	for(int i = 0; i < ActionBarLength; i++) {
		const ActionClass *Action = ActionBar[i];
		if(Action) {
 			sprintf(Query, "INSERT INTO ActionBar VALUES(%d, %d, %d, %d)", CharacterID, Action->GetType(), Action->GetID(), i);
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

	//NextBattle = Random.GenerateRange(4, 14);
	NextBattle = Random.GenerateRange(1, 1);
}

// Starts a battle
void PlayerClass::StartBattle(BattleClass *Battle) {

	State = STATE_BATTLE;
	this->Battle = Battle;
	BattleAction = NULL;
}

// Stop a battle
void PlayerClass::ExitBattle() {

	State = STATE_WALK;
	Battle = NULL;
	GenerateNextBattle();
}

// Determines if a player can attack
bool PlayerClass::CanAttackPlayer() {

	return State == PlayerClass::STATE_WALK && AttackPlayerTime > PLAYER_ATTACKTIME;
}

// Update gold amount
void PlayerClass::UpdateGold(int Value) {

	Gold += Value;
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
void PlayerClass::SetPortraitID(int ID) {
	PortraitID = ID;
	Portrait = Stats.GetPortrait(PortraitID)->Image;
}

// Sets the player's vendor
void PlayerClass::SetVendor(const VendorStruct *Vendor) {

	this->Vendor = Vendor;
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
int PlayerClass::GetRequiredItemSlots(const TraderStruct *TTrader, int *Slots) {
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
		Slots[i] = -1;

		// Search for the required item
		for(int j = INVENTORY_BACKPACK; j < INVENTORY_TRADE; j++) {
			InventoryStruct *InventoryItem = &Inventory[j];
			if(InventoryItem->Item == RequiredItem && InventoryItem->Count >= RequiredCount) {
				Slots[i] = j;
				break;
			}
		}

		// Didn't find an item
		if(Slots[i] == -1)
			RewardItemSlot = -1;
	}

	return RewardItemSlot;
}

// Accept a trade from a trader
void PlayerClass::AcceptTrader(const TraderStruct *TTrader, int *Slots, int TRewardSlot) {
	if(TTrader == NULL || !IsSlotInventory(TRewardSlot))
		return;

	// Trade in required items
	for(u32 i = 0; i < TTrader->TraderItems.size(); i++) {
		UpdateInventory(Slots[i], -TTrader->TraderItems[i].Count);
	}

	// Give player reward
	AddItem(TTrader->RewardItem, TTrader->Count, TRewardSlot);
}

// Finds a potion in the player's inventory for use in battle
int PlayerClass::GetPotionBattle(int Type) {
/*
	// Search
	for(int i = INVENTORY_BACKPACK; i < INVENTORY_TRADE; i++) {
		if(Inventory[i].Item && Inventory[i].Item->IsPotionType(Type))
			return i;
	}
*/
	return -1;
}

// Uses a potion in battle
bool PlayerClass::UsePotionBattle(int Slot, int SkillType, int &HealthChange, int &ManaChange) {
	const ItemClass *Item = GetInventoryItem(Slot);
	if(Item == NULL)
		return false;

	HealthChange = Item->GetHealthRestore();
	ManaChange = Item->GetManaRestore();
	UpdateInventory(Slot, -1);

	return true;
}

// Uses a potion in the world
bool PlayerClass::UsePotionWorld(int Slot) {
	const ItemClass *Item = GetInventoryItem(Slot);
	if(Item == NULL)
		return false;
	/*

	// Get potion stats
	int HealthRestore = Item->GetHealthRestore();
	int ManaRestore = Item->GetManaRestore();

	// Use only if needed
	if(Item->IsHealthPotion() && Health < MaxHealth || Item->IsManaPotion() && Mana < MaxMana) {
		UpdateHealth(HealthRestore);
		UpdateMana(ManaRestore);
		UpdateInventory(Slot, -1);
		return true;
	}*/

	return false;
}

// Uses an item from the inventory
bool PlayerClass::UseInventory(int Slot) {
	const ItemClass *Item = GetInventoryItem(Slot);
	if(Item == NULL)
		return false;

	// Handle item types
	switch(Item->GetType()) {
		case ItemClass::YPE_USEABLE:
			return UsePotionWorld(Slot);
		break;
	}

	return true;
}

// Sets an item in the inventory
void PlayerClass::SetInventory(int Slot, int ItemID, int Count) {

	if(ItemID == 0) {
		Inventory[Slot].Item = NULL;
		Inventory[Slot].Count = 0;
	}
	else {
		Inventory[Slot].Item = Stats.GetItem(ItemID);
		Inventory[Slot].Count = Count;
	}
}

// Sets an item in the inventory
void PlayerClass::SetInventory(int Slot, InventoryStruct *Item) {

	if(Item->Item == NULL) {
		Inventory[Slot].Item = NULL;
		Inventory[Slot].Count = 0;
	}
	else {
		Inventory[Slot].Item = Item->Item;
		Inventory[Slot].Count = Item->Count;
	}
}

// Returns an item from the inventory
InventoryStruct *PlayerClass::GetInventory(int Slot) {

	return &Inventory[Slot];
}

// Gets an inventory item
const ItemClass *PlayerClass::GetInventoryItem(int Slot) {

	// Check for bad slots
	if(Slot < INVENTORY_BACKPACK || Slot >= INVENTORY_TRADE || !Inventory[Slot].Item)
		return NULL;

	return Inventory[Slot].Item;
}

// Moves an item from one slot to another
bool PlayerClass::MoveInventory(int OldSlot, int NewSlot) {
	if(OldSlot == NewSlot)
		return false;

	// Equippable items
	if(NewSlot < INVENTORY_BACKPACK) {

		// Check if the item is even equippable
		if(!CanEquipItem(NewSlot, Inventory[OldSlot].Item))
			return false;

		// Split stacks
		if(Inventory[OldSlot].Count > 1) {
			Inventory[NewSlot].Item = Inventory[OldSlot].Item;
			Inventory[NewSlot].Count = 1;
			Inventory[OldSlot].Count--;
		}
		else
			SwapItem(NewSlot, OldSlot);

		return true;
	}
	// Back pack
	else {

		// Add to stack
		if(Inventory[NewSlot].Item == Inventory[OldSlot].Item) {
			Inventory[NewSlot].Count += Inventory[OldSlot].Count;

			// Group stacks
			if(Inventory[NewSlot].Count > 255) {
				Inventory[OldSlot].Count = Inventory[NewSlot].Count - 255;
				Inventory[NewSlot].Count = 255;
			}
			else
				Inventory[OldSlot].Item = NULL;
		}
		else {

			// Disable reverse equip for now
			if(OldSlot < INVENTORY_BACKPACK && Inventory[NewSlot].Item)
				return false;

			SwapItem(NewSlot, OldSlot);
		}

		return true;
	}
}

// Swaps two items
void PlayerClass::SwapItem(int Slot, int OldSlot) {
	InventoryStruct TempItem;

	// Swap items
	TempItem = Inventory[Slot];
	Inventory[Slot] = Inventory[OldSlot];
	Inventory[OldSlot] = TempItem;
}

// Updates an item's count, deleting if necessary
bool PlayerClass::UpdateInventory(int Slot, int Amount) {

	Inventory[Slot].Count += Amount;
	if(Inventory[Slot].Count <= 0) {
		Inventory[Slot].Item = NULL;
		Inventory[Slot].Count = 0;
		return true;
	}

	return false;
}

// Attempts to add an item to the inventory
bool PlayerClass::AddItem(const ItemClass *Item, int Count, int Slot) {

	// Place somewhere in backpack
	if(Slot == -1) {

		// Find existing item
		int EmptySlot = -1;
		for(int i = INVENTORY_BACKPACK; i < INVENTORY_TRADE; i++) {
			if(Inventory[i].Item == Item && Inventory[i].Count + Count <= 255) {
				Inventory[i].Count += Count;
				return true;
			}

			// Keep track of the first empty slot
			if(Inventory[i].Item == NULL && EmptySlot == -1)
				EmptySlot = i;
		}

		// Found an empty slot
		if(EmptySlot != -1) {
			Inventory[EmptySlot].Item = Item;
			Inventory[EmptySlot].Count = Count;
			return true;
		}

		return false;
	}
	// Trying to equip an item
	else if(Slot < INVENTORY_BACKPACK) {

		// Make sure it can be equipped
		if(!CanEquipItem(Slot, Item))
			return false;

		// Set item
		Inventory[Slot].Item = Item;
		Inventory[Slot].Count = Count;

		return true;
	}

	// Add item
	if(Inventory[Slot].Item == Item && Inventory[Slot].Count + Count <= 255) {
		Inventory[Slot].Count += Count;
		return true;
	}
	else if(Inventory[Slot].Item == NULL) {
		Inventory[Slot].Item = Item;
		Inventory[Slot].Count = Count;
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
void PlayerClass::SplitStack(int Slot, int Count) {
	if(Slot < 0 || Slot >= INVENTORY_COUNT)
		return;

	// Make sure stack is large enough
	InventoryStruct *SplitItem = &Inventory[Slot];
	if(SplitItem->Item && SplitItem->Count > Count) {

		// Find an empty slot or existing item
		int EmptySlot = Slot;
		InventoryStruct *Item;
		do {
			EmptySlot++;
			if(EmptySlot >= INVENTORY_TRADE)
				EmptySlot = INVENTORY_BACKPACK;

			Item = &Inventory[EmptySlot];
		} while(!(EmptySlot == Slot || Item->Item == NULL || (Item->Item == SplitItem->Item && Item->Count <= 255 - Count)));

		// Split item
		if(EmptySlot != Slot) {
			SplitItem->Count -= Count;
			AddItem(SplitItem->Item, Count, EmptySlot);
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
bool PlayerClass::CanEquipItem(int Slot, const ItemClass *Item) {

	// Already equipped
	if(Inventory[Slot].Item)
		return false;

	// Check type
	switch(Slot) {
		case INVENTORY_HEAD:
			if(Item->GetType() == ItemClass::YPE_HEAD)
				return true;
		break;
		case INVENTORY_BODY:
			if(Item->GetType() == ItemClass::YPE_BODY)
				return true;
		break;
		case INVENTORY_LEGS:
			if(Item->GetType() == ItemClass::YPE_LEGS)
				return true;
		break;
		case INVENTORY_HAND1:
			if(Item->GetType() == ItemClass::YPE_WEAPON1HAND)
				return true;
		break;
		case INVENTORY_HAND2:
			if(Item->GetType() == ItemClass::YPE_SHIELD)
				return true;
		break;
		case INVENTORY_RING1:
		case INVENTORY_RING2:
			if(Item->GetType() == ItemClass::YPE_RING)
				return true;
		break;
		default:
		break;
	}

	return false;
}

// Toggles the player's busy state
void PlayerClass::ToggleBusy(bool Value) {

	if(Value && State == STATE_WALK) {
		State = STATE_BUSY;
	}
	else if(!Value && State == STATE_BUSY) {
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
	const LevelStruct *MaxLevelStat = Stats.GetLevel(Stats.GetMaxLevel());
	if(Experience > MaxLevelStat->Experience)
		Experience = MaxLevelStat->Experience;

	// Find current level
	const LevelStruct *LevelStat = Stats.FindLevel(Experience);
	Level = LevelStat->Level;
	MaxHealth = LevelStat->Health;
	MaxMana = LevelStat->Mana;
	ExperienceNextLevel = LevelStat->NextLevel;
	if(Level == Stats.GetMaxLevel())
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

// Use an action outside of battle
void PlayerClass::UseActionWorld(int Slot) {
	const ActionClass *Action = ActionBar[Slot];

}

// Clears unlocked skills
void PlayerClass::ClearSkillsUnlocked() {

	SkillsUnlocked.clear();
}

// Unlocks a skill
void PlayerClass::AddSkillUnlocked(const SkillClass *Skill) {

	SkillsUnlocked.push_back(Skill);
}

// Returns a list of skills
const array<const SkillClass *> &PlayerClass::GetSkillsUnlocked() const {

	return SkillsUnlocked;
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
