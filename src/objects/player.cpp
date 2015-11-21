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
#include <objects/player.h>
#include <enet/enet.h>
#include <globals.h>
#include <graphics.h>
#include <instances.h>
#include <random.h>
#include <stats.h>
#include <database.h>
#include <assets.h>
#include <program.h>
#include <constants.h>
#include <instances/map.h>
#include <instances/battle.h>
#include <states/server.h>
#include <objects/skill.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Constructor
_Player::_Player()
:	_Object(PLAYER),
	_Fighter(TYPE_PLAYER),
	AccountID(0),
	CharacterID(0),
	Database(nullptr),
	State(STATE_WALK),
	MoveTime(0),
	AutoSaveTime(0),
	PortraitID(0),
	StateImage(nullptr),
	SpawnMapID(1),
	SpawnPoint(0),
	TeleportTime(0),
	PlayTime(0),
	PlayTimeAccumulator(0),
	Deaths(0),
	MonsterKills(0),
	PlayerKills(0),
	Bounty(0),
	Gold(0),
	Experience(0),
	MinDamageBonus(0),
	MaxDamageBonus(0),
	MinDefenseBonus(0),
	MaxDefenseBonus(0),
	WeaponDamageModifier(0.0f),
	WeaponMinDamage(0),
	WeaponMaxDamage(0),
	ArmorMinDefense(0),
	ArmorMaxDefense(0),
	AttackPlayerTime(0),
	InvisPower(0),
	Vendor(nullptr),
	SkillPoints(0),
	SkillPointsUsed(0),
	TradeRequestTime(0),
	TradeGold(0),
	TradeAccepted(false),
	TradePlayer(nullptr) {

	WorldImage = Assets.Textures["players/basic.png"];

	Position.x = 0;
	Position.y = 0;

	// Reset number of potions used in battle
	for(int i = 0; i < 2; i++)
		MaxPotions[i] = PotionsLeft[i] = 0;

	// Reset items
	for(int i = 0; i < _Player::INVENTORY_COUNT; i++) {
		Inventory[i].Item = nullptr;
		Inventory[i].Count = 0;
	}

	// Reset skills
	for(int i = 0; i < _Player::SKILL_COUNT; i++) {
		SkillLevels[i] = 0;
	}

	GenerateNextBattle();
}

// Destructor
_Player::~_Player() {

	if(Map)
		Map->RemoveObject(this);
}

// Updates the player
void _Player::Update(double FrameTime) {

	MoveTime += FrameTime;
	AutoSaveTime += FrameTime;
	AttackPlayerTime += FrameTime;
	TradeRequestTime += FrameTime;
	PlayTimeAccumulator += FrameTime;
	TeleportTime += FrameTime;
	if(PlayTimeAccumulator > 1.0) {
		PlayTimeAccumulator -= 1.0;
		PlayTime++;
	}

	// Run server commands
	if(Database) {
		if(AutoSaveTime >= GAME_AUTOSAVEPERIOD)
			Save();

		if(State == STATE_TELEPORT && TeleportTime > GAME_TELEPORT_TIME) {
			ServerState.PlayerTeleport(this);
			State = STATE_WALK;
		}
	}
}

// Renders the player while walking around the world
void _Player::RenderWorld(const _Object *ClientPlayer) {
	if(Map) {

		float Alpha = 1.0f;
		if(IsInvisible())
			Alpha = PLAYER_INVIS_ALPHA;

		Graphics.SetProgram(Assets.Programs["pos_uv"]);
		glUniformMatrix4fv(Assets.Programs["pos_uv"]->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));

		Graphics.SetColor(glm::vec4(Alpha, 1.0f, 1.0f, 1.0f));
		Graphics.SetVBO(VBO_QUAD);

		Graphics.SetDepthTest(false);
		Graphics.SetDepthMask(false);

		glm::vec3 DrawPosition = glm::vec3(Position, 0.0f) + glm::vec3(0.5f, 0.5f, 0);
		Graphics.DrawSprite(DrawPosition, WorldImage);
		if(StateImage) {
			Graphics.DrawSprite(DrawPosition, StateImage);
		}

		if(ClientPlayer != this) {
			//Graphics.SetFont(_Graphics::FONT_8);
			//Graphics.RenderText(Name.c_str(), ScreenPosition.x, ScreenPosition.y - 28, _Graphics::ALIGN_CENTER);
		}
	}
}

// Moves the player
bool _Player::MovePlayer(int Direction) {
	if(State != STATE_WALK)
		return false;

	// Get new position
	glm::ivec2 NewPosition = Position;
	switch(Direction) {
		case MOVE_LEFT:
			NewPosition.x--;
		break;
		case MOVE_UP:
			NewPosition.y--;
		break;
		case MOVE_RIGHT:
			NewPosition.x++;
		break;
		case MOVE_DOWN:
			NewPosition.y++;
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
void _Player::Save() {
	if(!Database || CharacterID == 0)
		return;

	printf("Saving Character %s\n", Name.c_str());

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
					", PlayTime = %d"
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

	const _InventorySlot *Item;
	for(int i = 0; i < INVENTORY_COUNT; i++) {
		Item = &Inventory[i];
		if(Item->Item) {
			sprintf(Query, "INSERT INTO Inventory VALUES(%d, %d, %d, %d)", CharacterID, i, Item->Item->GetID(), Item->Count);
			Database->RunQuery(Query);
		}
	}

	// Save skill points
	sprintf(Query, "DELETE FROM SkillLevel WHERE CharactersID = %d", CharacterID);
	Database->RunQuery(Query);

	for(int i = 0; i < _Player::SKILL_COUNT; i++) {
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
int _Player::GetCurrentZone() {

	return GetTile()->Zone;
}

// Gets the tile that the player is currently standing on
const _Tile *_Player::GetTile() {

	return Map->GetTile(Position);
}

// Generates the number of moves until the next battle
void _Player::GenerateNextBattle() {
	std::uniform_int_distribution<int> Distribution(4, 14);
	NextBattle = Distribution(RandomGenerator);
}

// Starts a battle
void _Player::StartBattle(_Battle *Battle) {
	State = STATE_BATTLE;
	this->Battle = Battle;
	Command = -1;
	for(int i = 0; i < 2; i++)
		PotionsLeft[i] = MaxPotions[i];
}

// Stop a battle
void _Player::StopBattle() {
	State = STATE_WALK;
	Battle = nullptr;
	GenerateNextBattle();
}

// Determines if a player can attack
bool _Player::CanAttackPlayer() {

	return State == _Player::STATE_WALK && AttackPlayerTime > PLAYER_ATTACKTIME;
}

// Update gold amount
void _Player::UpdateGold(int Value) {

	Gold += Value;
	if(Gold < 0)
		Gold = 0;
	else if(Gold > STATS_MAXGOLD)
		Gold = STATS_MAXGOLD;
}

// Get the percentage to the next level
float _Player::GetNextLevelPercent() const {
	float Percent = 0;

	if(ExperienceNextLevel > 0)
		Percent = 1.0f - (float)ExperienceNeeded / ExperienceNextLevel;

	return Percent;
}

// Sets the player's portrait
void _Player::SetPortraitID(int Value) {
	PortraitID = Value;
	Portrait = Stats.GetPortrait(PortraitID)->Image;
}

// Fills an array with inventory indices correlating to a trader's required items
int _Player::GetRequiredItemSlots(int *Slots) {
	int RewardItemSlot = -1;

	// Check for an open reward slot
	for(int i = INVENTORY_BACKPACK; i < INVENTORY_TRADE; i++) {
		_InventorySlot *InventoryItem = &Inventory[i];
		if(InventoryItem->Item == nullptr || (InventoryItem->Item == Trader->RewardItem && InventoryItem->Count + Trader->Count <= 255)) {
			RewardItemSlot = i;
			break;
		}
	}

	// Go through required items
	for(size_t i = 0; i < Trader->TraderItems.size(); i++) {
		const _Item *RequiredItem = Trader->TraderItems[i].Item;
		int RequiredCount = Trader->TraderItems[i].Count;
		Slots[i] = -1;

		// Search for the required item
		for(int j = INVENTORY_HEAD; j < INVENTORY_TRADE; j++) {
			_InventorySlot *InventoryItem = &Inventory[j];
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
void _Player::AcceptTrader(int *Slots, int RewardSlot) {
	if(Trader == nullptr || !IsSlotInventory(RewardSlot))
		return;

	// Trade in required items
	for(uint32_t i = 0; i < Trader->TraderItems.size(); i++) {
		UpdateInventory(Slots[i], -Trader->TraderItems[i].Count);
	}

	// Give player reward
	AddItem(Trader->RewardItem, Trader->Count, RewardSlot);
}

// Finds a potion in the player's inventory for use in battle
int _Player::GetPotionBattle(int PotionType) {

	// Check for skill uses
	if(PotionsLeft[PotionType] <= 0)
		return -1;

	// Search
	for(int i = INVENTORY_BACKPACK; i < INVENTORY_TRADE; i++) {
		if(Inventory[i].Item && Inventory[i].Item->IsPotionType(PotionType))
			return i;
	}

	return -1;
}

// Uses a potion in battle
bool _Player::UsePotionBattle(int Slot, int SkillType, int &HealthChange, int &ManaChange) {
	const _Item *Item = GetInventoryItem(Slot);
	if(Item == nullptr || Item->GetType() != _Item::TYPE_POTION)
		return false;

	HealthChange = Item->GetHealthRestore();
	ManaChange = Item->GetManaRestore();
	UpdatePotionsLeft(SkillType);
	UpdateInventory(Slot, -1);

	return true;
}

// Uses a potion in the world
bool _Player::UsePotionWorld(int Slot) {
	const _Item *Item = GetInventoryItem(Slot);
	if(Item == nullptr)
		return false;

	// Get potion stats
	int HealthRestore = Item->GetHealthRestore();
	int ManaRestore = Item->GetManaRestore();
	int ItemInvisPower = Item->GetInvisPower();

	// Use only if needed
	if((Item->IsHealthPotion() && Health < MaxHealth) || (Item->IsManaPotion() && Mana < MaxMana) || Item->IsInvisPotion()) {
		UpdateHealth(HealthRestore);
		UpdateMana(ManaRestore);
		InvisPower = ItemInvisPower;
		UpdateInventory(Slot, -1);
		return true;
	}

	return false;
}

// Uses an item from the inventory
bool _Player::UseInventory(int Slot) {
	const _Item *Item = GetInventoryItem(Slot);
	if(Item == nullptr)
		return false;

	// Handle item types
	switch(Item->GetType()) {
		case _Item::TYPE_POTION:
			return UsePotionWorld(Slot);
		break;
	}

	return true;
}

// Sets an item in the inventory
void _Player::SetInventory(int Slot, int ItemID, int Count) {

	if(ItemID == 0) {
		Inventory[Slot].Item = nullptr;
		Inventory[Slot].Count = 0;
	}
	else {
		Inventory[Slot].Item = Stats.GetItem(ItemID);
		Inventory[Slot].Count = Count;
	}
}

// Sets an item in the inventory
void _Player::SetInventory(int Slot, _InventorySlot *Item) {

	if(Item->Item == nullptr) {
		Inventory[Slot].Item = nullptr;
		Inventory[Slot].Count = 0;
	}
	else {
		Inventory[Slot].Item = Item->Item;
		Inventory[Slot].Count = Item->Count;
	}
}

// Gets an inventory item
const _Item *_Player::GetInventoryItem(int Slot) {

	// Check for bad slots
	if(Slot < INVENTORY_BACKPACK || Slot >= INVENTORY_TRADE || !Inventory[Slot].Item)
		return nullptr;

	return Inventory[Slot].Item;
}

// Moves an item from one slot to another
bool _Player::MoveInventory(int OldSlot, int NewSlot) {
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
				Inventory[OldSlot].Item = nullptr;
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
void _Player::SwapItem(int Slot, int OldSlot) {
	_InventorySlot TempItem;

	// Swap items
	TempItem = Inventory[Slot];
	Inventory[Slot] = Inventory[OldSlot];
	Inventory[OldSlot] = TempItem;
}

// Updates an item's count, deleting if necessary
bool _Player::UpdateInventory(int Slot, int Amount) {

	Inventory[Slot].Count += Amount;
	if(Inventory[Slot].Count <= 0) {
		Inventory[Slot].Item = nullptr;
		Inventory[Slot].Count = 0;
		return true;
	}

	return false;
}

// Attempts to add an item to the inventory
bool _Player::AddItem(const _Item *Item, int Count, int Slot) {

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
			if(Inventory[i].Item == nullptr && EmptySlot == -1)
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
	else if(Inventory[Slot].Item == nullptr) {
		Inventory[Slot].Item = Item;
		Inventory[Slot].Count = Count;
		return true;
	}

	return false;
}

// Moves the player's trade items to their backpack
void _Player::MoveTradeToInventory() {

	for(int i = INVENTORY_TRADE; i < INVENTORY_COUNT; i++) {
		if(Inventory[i].Item && AddItem(Inventory[i].Item, Inventory[i].Count, -1))
			Inventory[i].Item = nullptr;
	}
}

// Splits a stack
void _Player::SplitStack(int Slot, int Count) {
	if(Slot < 0 || Slot >= INVENTORY_COUNT)
		return;

	// Make sure stack is large enough
	_InventorySlot *SplitItem = &Inventory[Slot];
	if(SplitItem->Item && SplitItem->Count > Count) {

		// Find an empty slot or existing item
		int EmptySlot = Slot;
		_InventorySlot *Item;
		do {
			EmptySlot++;
			if(EmptySlot >= INVENTORY_TRADE)
				EmptySlot = INVENTORY_BACKPACK;

			Item = &Inventory[EmptySlot];
		} while(!(EmptySlot == Slot || Item->Item == nullptr || (Item->Item == SplitItem->Item && Item->Count <= 255 - Count)));

		// Split item
		if(EmptySlot != Slot) {
			SplitItem->Count -= Count;
			AddItem(SplitItem->Item, Count, EmptySlot);
		}
	}
}

// Determines if the player's backpack is full
bool _Player::IsBackpackFull() {

	// Search backpack
	for(int i = INVENTORY_BACKPACK; i < INVENTORY_TRADE; i++) {
		if(Inventory[i].Item == nullptr)
			return false;
	}

	return true;
}

// Checks if an item can be equipped
bool _Player::CanEquipItem(int Slot, const _Item *Item) {

	// Already equipped
	if(Inventory[Slot].Item)
		return false;

	// Check type
	switch(Slot) {
		case INVENTORY_HEAD:
			if(Item->GetType() == _Item::TYPE_HEAD)
				return true;
		break;
		case INVENTORY_BODY:
			if(Item->GetType() == _Item::TYPE_BODY)
				return true;
		break;
		case INVENTORY_LEGS:
			if(Item->GetType() == _Item::TYPE_LEGS)
				return true;
		break;
		case INVENTORY_HAND1:
			if(Item->GetType() == _Item::TYPE_WEAPON1HAND)
				return true;
		break;
		case INVENTORY_HAND2:
			if(Item->GetType() == _Item::TYPE_SHIELD)
				return true;
		break;
		case INVENTORY_RING1:
		case INVENTORY_RING2:
			if(Item->GetType() == _Item::TYPE_RING)
				return true;
		break;
		default:
		break;
	}

	return false;
}

// Updates a skill level
void _Player::AdjustSkillLevel(int SkillID, int Adjust) {
	const _Skill *Skill = Stats.GetSkill(SkillID);
	if(Skill == nullptr)
		return;

	// Buying
	if(Adjust > 0) {
		if(Skill->SkillCost > GetSkillPointsRemaining() || SkillLevels[SkillID] >= 255)
			return;

		// Update level
		SkillLevels[SkillID] += Adjust;
		if(SkillLevels[SkillID] > 255)
			SkillLevels[SkillID] = 255;
	}
	else if(Adjust < 0) {
		if(SkillLevels[SkillID] == 0)
			return;

		// Update level
		SkillLevels[SkillID] += Adjust;
		if(SkillLevels[SkillID] < 0)
			SkillLevels[SkillID] = 0;

		// Update skill bar
		if(SkillLevels[SkillID] == 0) {
			for(int i = 0; i < 8; i++) {
				if(SkillBar[i] == Skill) {
					SkillBar[i] = nullptr;
					break;
				}
			}
		}
	}
}

// Calculates the number of skill points used
void _Player::CalculateSkillPoints() {

	SkillPointsUsed = 0;
	for(uint32_t i = 0; i < Stats.Skills.size(); i++) {
		SkillPointsUsed += Stats.Skills[i].SkillCost * SkillLevels[i];
	}
}

// Toggles the player's busy state
void _Player::ToggleBusy(bool Value) {

	if(Value && State == STATE_WALK) {
		State = STATE_BUSY;
	}
	else if(!Value && State == STATE_BUSY) {
		State = STATE_WALK;
	}
}

// Starts the teleport process
void _Player::StartTeleport() {
	if(State == STATE_TELEPORT)
		State = STATE_WALK;
	else if(State == STATE_WALK)
		State = STATE_TELEPORT;

	TeleportTime = 0;
}

// Calculates all of the player stats
void _Player::CalculatePlayerStats() {
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
void _Player::CalculateLevelStats() {

	// Cap min experience
	if(Experience < 0)
		Experience = 0;

	// Cap max experience
	const _Level *MaxLevelStat = Stats.GetLevel(Stats.GetMaxLevel());
	if(Experience > MaxLevelStat->Experience)
		Experience = MaxLevelStat->Experience;

	// Find current level
	const _Level *LevelStat = Stats.FindLevel(Experience);
	Level = LevelStat->Level;
	MaxHealth = LevelStat->Health;
	MaxMana = LevelStat->Mana;
	SkillPoints = LevelStat->SkillPoints;
	ExperienceNextLevel = LevelStat->NextLevel;
	if(Level == Stats.GetMaxLevel())
		ExperienceNeeded = 0;
	else
		ExperienceNeeded = LevelStat->NextLevel - (Experience - LevelStat->Experience);

}

// Calculates stats from equipped items
void _Player::CalculateGearStats() {

	// Get stats
	if(!Inventory[INVENTORY_HAND1].Item)
		WeaponMinDamage = WeaponMaxDamage = 1;

	// Check each item
	for(int i = 0; i < INVENTORY_BACKPACK; i++) {
		const _Item *Item = Inventory[i].Item;
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
			MaxHealth += Item->MaxHealth;
			MaxMana += Item->MaxMana;
			HealthRegen += Item->GetHealthRegen();
			ManaRegen += Item->GetManaRegen();
		}
	}
}

// Calculates skill bonuses
void _Player::CalculateSkillStats() {

	// Go through each skill bar
	for(int i = 0; i < 8; i++) {
		const _Skill *Skill = SkillBar[i];
		if(Skill) {
			int Min, Max, MinRound, MaxRound;
			float MinFloat, MaxFloat;
			Skill->GetPowerRange(SkillLevels[Skill->ID], Min, Max);
			Skill->GetPowerRangeRound(SkillLevels[Skill->ID], MinRound, MaxRound);
			Skill->GetPowerRange(SkillLevels[Skill->ID], MinFloat, MaxFloat);

			switch(Skill->ID) {
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
void _Player::CalculateFinalStats() {
	MinDamage = MinDamageBonus + (int)std::roundf(WeaponMinDamage * WeaponDamageModifier);
	MaxDamage = MaxDamageBonus + (int)std::roundf(WeaponMaxDamage * WeaponDamageModifier);
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
