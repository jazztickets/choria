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
#include <objects/object.h>
#include <instances/map.h>
#include <ui/element.h>
#include <ui/image.h>
#include <assets.h>
#include <graphics.h>
#include <random.h>
#include <stats.h>
#include <database.h>
#include <font.h>
#include <program.h>
#include <constants.h>
#include <instances/battle.h>
#include <states/oldserver.h>
#include <objects/skill.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>

// Constructor
_Object::_Object()
:	Map(nullptr),
	OldPeer(nullptr),
	Peer(nullptr),
	Type(0),
	Moved(false),
	Deleted(false),
	Position(-1, -1),
	NetworkID(0),

	Name(""),
	Level(0),
	Health(0),
	MaxHealth(0),
	Mana(0),
	MaxMana(0),
	MinDamage(0),
	MaxDamage(0),
	MinDefense(0),
	MaxDefense(0),
	HealthRegen(0.0f),
	ManaRegen(0.0f),
	HealthAccumulator(0.0f),
	ManaAccumulator(0.0f),
	Battle(nullptr),
	Command(-1),
	Target(0),
	SkillUsing(nullptr),
	SkillUsed(nullptr),
	Portrait(nullptr),

	CharacterID(0),
	State(STATE_WALK),
	MoveTime(0),
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
	TradeGold(0),
	TradeAccepted(false),
	TradePlayer(nullptr),

	Dummy(0) {

	for(int i = 0; i < ACTIONBAR_SIZE; i++)
		ActionBar[i] = nullptr;

	Offset.x = Offset.y = 0;
	Position.x = 0;
	Position.y = 0;

	for(int i = 0; i < 2; i++)
		MaxPotions[i] = PotionsLeft[i] = 0;

	for(int i = 0; i < _Object::INVENTORY_COUNT; i++) {
		Inventory[i].Item = nullptr;
		Inventory[i].Count = 0;
	}

	InputState = 0;
	Database = 0;
}

// Destructor
_Object::~_Object() {
}

// Renders the fighter during a battle
void _Object::RenderBattle(bool ShowResults, float TimerPercent, _ActionResult *Result, bool IsTarget) {

	// Get slot ui element depending on side
	_Element *Slot;
	if(GetSide() == 0)
		Slot = Assets.Elements["element_side_left"];
	else
		Slot = Assets.Elements["element_side_right"];

	// Draw slot
	Slot->Offset = Offset;
	Slot->CalculateBounds();
	Slot->SetVisible(true);
	Slot->Render();
	Slot->SetVisible(false);

	// Get slot center
	glm::ivec2 SlotPosition = (Slot->Bounds.Start + Slot->Bounds.End) / 2;

	// Name
	Assets.Fonts["hud_medium"]->DrawText(Name.c_str(), Slot->Bounds.Start + glm::ivec2(32, -10), COLOR_WHITE, CENTER_BASELINE);

	// Portrait
	if(Portrait) {
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(SlotPosition, Portrait);
	}

	// Health/mana bars
	glm::ivec2 BarSize(90, 22);
	glm::ivec2 BarOffset(Portrait->Size.x/2 + 10, -15);
	if(MaxMana == 0)
		BarOffset.y = 0;

	// Get health percent
	float HealthPercent = MaxHealth > 0 ? Health / (float)MaxHealth : 0;

	// Get ui size
	_Bounds BarBounds;
	BarBounds.Start = SlotPosition + glm::ivec2(0, -BarSize.y/2) + BarOffset;
	BarBounds.End = SlotPosition + glm::ivec2(BarSize.x, BarSize.y/2) + BarOffset;
	glm::ivec2 BarCenter = (BarBounds.Start + BarBounds.End) / 2;
	glm::ivec2 HealthBarCenter = BarCenter;
	int BarEndX = BarBounds.End.x;

	// Draw empty bar
	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	Graphics.SetVBO(VBO_NONE);
	Graphics.DrawImage(BarBounds, Assets.Images["image_hud_health_bar_empty"]->Texture, true);

	// Draw full bar
	BarBounds.End = SlotPosition + glm::ivec2(BarSize.x * HealthPercent, BarSize.y/2) + BarOffset;
	Graphics.DrawImage(BarBounds, Assets.Images["image_hud_health_bar_full"]->Texture, true);

	// Draw health text
	std::stringstream Buffer;
	Buffer << Health << " / " << MaxHealth;
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), BarCenter + glm::ivec2(0, 5), COLOR_WHITE, CENTER_BASELINE);
	Buffer.str("");

	// Draw mana
	if(MaxMana > 0) {
		float ManaPercent = MaxMana > 0 ? Mana / (float)MaxMana : 0;

		// Get ui size
		BarOffset.y = -BarOffset.y;
		BarBounds.Start = SlotPosition + glm::ivec2(0, -BarSize.y/2) + BarOffset;
		BarBounds.End = SlotPosition + glm::ivec2(BarSize.x, BarSize.y/2) + BarOffset;
		BarCenter = (BarBounds.Start + BarBounds.End) / 2;

		// Draw empty bar
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.SetVBO(VBO_NONE);
		Graphics.DrawImage(BarBounds, Assets.Images["image_hud_mana_bar_empty"]->Texture, true);

		// Draw full bar
		BarBounds.End = SlotPosition + glm::ivec2(BarSize.x * ManaPercent, BarSize.y/2) + BarOffset;
		Graphics.DrawImage(BarBounds, Assets.Images["image_hud_mana_bar_full"]->Texture, true);

		// Draw mana text
		Buffer << Mana << " / " << MaxMana;
		Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), BarCenter + glm::ivec2(0, 5), COLOR_WHITE, CENTER_BASELINE);
		Buffer.str("");
	}

	// Show results of last turn
	if(ShowResults) {

		float AlphaPercent;
		if(TimerPercent > 0.75f)
			AlphaPercent = 1;
		else
			AlphaPercent = TimerPercent / 0.75f;

		char Sign = ' ';
		glm::vec4 Color = COLOR_GRAY;
		if(Result->HealthChange > 0) {
			Sign = '+';
			Color = COLOR_GREEN;
		}
		else if(Result->HealthChange < 0) {
			Color = COLOR_RED;
		}
		Color.a = AlphaPercent;

		Buffer << Sign << Result->HealthChange;
		Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), HealthBarCenter + glm::ivec2(0, -20), Color, CENTER_BASELINE);

		const _Texture *SkillTexture;
		if(SkillUsed)
			SkillTexture = SkillUsed->Image;
		else
			SkillTexture = Assets.Textures["skills/attack.png"];

		// Draw skill icon
		glm::vec4 WhiteAlpha = glm::vec4(1.0f, 1.0f, 1.0f, AlphaPercent);
		glm::ivec2 SkillUsedPosition = SlotPosition - glm::ivec2(Portrait->Size.x/2 + SkillTexture->Size.x/2 + 10, 0);
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(SkillUsedPosition, SkillTexture, WhiteAlpha);

		// Draw damage dealt
		Assets.Fonts["hud_medium"]->DrawText(std::to_string(Result->DamageDealt).c_str(), SkillUsedPosition, WhiteAlpha, CENTER_MIDDLE);
	}
	// Draw the skill used
	if(SkillUsing) {
		glm::ivec2 SkillUsingPosition = SlotPosition - glm::ivec2(Portrait->Size.x/2 + SkillUsing->Image->Size.x/2 + 10, 0);
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(SkillUsingPosition, SkillUsing->Image);
	}

	// Draw target
	if(IsTarget) {
		const _Texture *Texture = Assets.Textures["battle/target.png"];
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.DrawCenteredImage(glm::ivec2(BarEndX + Texture->Size.x/2 + 10, SlotPosition.y), Texture);
	}
}

// Update health
void _Object::UpdateHealth(int Value) {
	Health += Value;

	if(Health < 0)
		Health = 0;
	else if(Health > MaxHealth)
		Health = MaxHealth;
}

// Update mana
void _Object::UpdateMana(int Value) {
	Mana += Value;

	if(Mana < 0)
		Mana = 0;
	else if(Mana > MaxMana)
		Mana = MaxMana;
}

// Set health and mana to max
void _Object::RestoreHealthMana() {
	Health = MaxHealth;
	Mana = MaxMana;
}

// Updates the fighter's regen
void _Object::UpdateRegen(int &HealthUpdate, int &ManaUpdate) {

	HealthUpdate = 0;
	ManaUpdate = 0;
	HealthAccumulator += HealthRegen * 0.01f * MaxHealth;
	ManaAccumulator += ManaRegen * 0.01f * MaxMana;

	if(HealthAccumulator >= 1.0f) {
		int IntegerAccumulator = (int)HealthAccumulator;

		HealthUpdate = IntegerAccumulator;
		HealthAccumulator -= IntegerAccumulator;
	}
	else if(HealthAccumulator < 0.0f) {
		HealthAccumulator = 0;
	}

	if(ManaAccumulator >= 1.0f) {
		int IntegerAccumulator = (int)ManaAccumulator;

		ManaUpdate = IntegerAccumulator;
		ManaAccumulator -= IntegerAccumulator;
	}
	else if(ManaAccumulator < 0.0f) {
		ManaAccumulator = 0;
	}
}

// Command input
int _Object::GetCommand() {
	 if(Type == MONSTER)
		 return 0;
	 else
		 return Command;
}

// Generate damage
int _Object::GenerateDamage() {
	std::uniform_int_distribution<int> Distribution(MinDamage, MaxDamage);
	return Distribution(RandomGenerator);
}

// Generate defense
int _Object::GenerateDefense() {
	std::uniform_int_distribution<int> Distribution(MinDefense, MaxDefense);
	return Distribution(RandomGenerator);
}

// Gets a skill id from the skill bar
uint32_t _Object::GetActionBarID(int Slot) {
	if(ActionBar[Slot] == nullptr)
		return 0;

	return ActionBar[Slot]->ID;
}

// Get a skill from the skill bar
const _Skill *_Object::GetActionBar(int Slot) {
	if(Slot < 0 || Slot >= ACTIONBAR_SIZE)
		return nullptr;

	return ActionBar[Slot];
}

// Updates the monster's target based on AI
void _Object::UpdateTarget(const std::vector<_Object *> &Fighters) {

	// Get count of fighters
	int Count = Fighters.size();

	// Get a random index
	std::uniform_int_distribution<int> Distribution(0, Count-1);
	int RandomIndex = Distribution(RandomGenerator);

	Target = Fighters[RandomIndex]->BattleSlot;
}

// Updates the player
void _Object::Update(double FrameTime) {

	// Update player position
	Moved = Move();
	if(Moved)
		InputState = 0;

	// Update timers
	MoveTime += FrameTime;
	AttackPlayerTime += FrameTime;
	PlayTimeAccumulator += FrameTime;
	TeleportTime += FrameTime;
	if(PlayTimeAccumulator >= 1.0) {
		PlayTimeAccumulator -= 1.0;
		PlayTime++;
	}
}

// Renders the player while walking around the world
void _Object::Render(const _Object *ClientPlayer) {
	if(Map && WorldImage) {

		float Alpha = 1.0f;
		if(IsInvisible())
			Alpha = PLAYER_INVIS_ALPHA;

		Graphics.SetProgram(Assets.Programs["pos_uv"]);
		glUniformMatrix4fv(Assets.Programs["pos_uv"]->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));

		glm::vec4 Color(1.0f, 1.0f, 1.0f, Alpha);
		Graphics.SetColor(Color);
		Graphics.SetVBO(VBO_QUAD);

		Graphics.SetDepthTest(false);
		Graphics.SetDepthMask(false);

		glm::vec3 DrawPosition = glm::vec3(Position, 0.0f) + glm::vec3(0.5f, 0.5f, 0);
		Graphics.DrawSprite(DrawPosition, WorldImage);
		if(StateImage) {
			Graphics.DrawSprite(DrawPosition, StateImage);
		}

		if(ClientPlayer != this) {
			Assets.Fonts["hud_small"]->DrawText(Name.c_str(), glm::vec2(DrawPosition) + glm::vec2(0, -0.5f), Color, CENTER_BASELINE, 1.0f / WorldImage->Size.x);
		}
	}
}

// Moves the player
int _Object::Move() {
	if(State != STATE_WALK || InputState == 0)
		return 0;

	// Get new position
	glm::ivec2 Direction(0, 0);
	if(InputState & MOVE_UP)
		Direction.y += -1;
	if(InputState & MOVE_DOWN)
		Direction.y += 1;
	if(InputState & MOVE_LEFT)
		Direction.x += -1;
	if(InputState & MOVE_RIGHT)
		Direction.x += 1;

	// Remove diagonols
	if(Direction.x != 0 && Direction.y != 0)
		Direction.x = 0;

	// Check timer
	if(MoveTime < PLAYER_MOVETIME)
		return 0;

	// Move player
	if(Map->CanMoveTo(Position + Direction)) {
		Position += Direction;
		if(InvisPower > 0)
			InvisPower--;
		else
			NextBattle--;

		// Update regen
		int HealthUpdate, ManaUpdate;
		UpdateRegen(HealthUpdate, ManaUpdate);
		UpdateHealth(HealthUpdate);
		UpdateMana(ManaUpdate);

		MoveTime = 0;

		return InputState;
	}

	return 0;
}

// Saves the player
void _Object::Save() {
	if(!Database || CharacterID == 0)
		return;

	char Query[512];
	Database->RunQuery("BEGIN TRANSACTION");

	// Save character stats
	sprintf(Query, "UPDATE Characters SET "
					"MapID = %d"
					", SpawnPoint = %d"
					", Experience = %d"
					", Gold = %d"
					", ActionBar0 = %d"
					", ActionBar1 = %d"
					", ActionBar2 = %d"
					", ActionBar3 = %d"
					", ActionBar4 = %d"
					", ActionBar5 = %d"
					", ActionBar6 = %d"
					", ActionBar7 = %d"
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
					GetActionBarID(0),
					GetActionBarID(1),
					GetActionBarID(2),
					GetActionBarID(3),
					GetActionBarID(4),
					GetActionBarID(5),
					GetActionBarID(6),
					GetActionBarID(7),
					PlayTime,
					Deaths,
					MonsterKills,
					PlayerKills,
					Bounty,
					CharacterID
					);
	Database->RunQuery(Query);

	// Save items
	sprintf(Query, "DELETE FROM Inventory WHERE character_id = %d", CharacterID);
	Database->RunQuery(Query);

	const _InventorySlot *Item;
	for(int i = 0; i < INVENTORY_COUNT; i++) {
		Item = &Inventory[i];
		if(Item->Item) {
			sprintf(Query, "INSERT INTO Inventory VALUES(%d, %d, %d, %d)", CharacterID, i, Item->Item->ID, Item->Count);
			Database->RunQuery(Query);
		}
	}

	// Save skill points
	sprintf(Query, "DELETE FROM SkillLevel WHERE character_id = %d", CharacterID);
	Database->RunQuery(Query);

	for(auto &SkillLevel : SkillLevels) {
		if(SkillLevel.second > 0) {
			sprintf(Query, "INSERT INTO SkillLevel VALUES(%d, %d, %d)", CharacterID, SkillLevel.first, SkillLevel.second);
			Database->RunQuery(Query);
		}
	}

	Database->RunQuery("END TRANSACTION");
}

// Get the zone that the player is standing in
int _Object::GetCurrentZone() {

	return GetTile()->Zone;
}

// Gets the tile that the player is currently standing on
const _Tile *_Object::GetTile() {

	return Map->GetTile(Position);
}

// Generates the number of moves until the next battle
void _Object::GenerateNextBattle() {
	std::uniform_int_distribution<int> Distribution(4, 14);
	NextBattle = Distribution(RandomGenerator);
}

// Starts a battle
void _Object::StartBattle(_Battle *Battle) {
	State = STATE_BATTLE;
	this->Battle = Battle;
	Command = -1;
	for(int i = 0; i < 2; i++)
		PotionsLeft[i] = MaxPotions[i];
}

// Stop a battle
void _Object::StopBattle() {
	State = STATE_WALK;
	Battle = nullptr;
	GenerateNextBattle();
}

// Determines if a player can attack
bool _Object::CanAttackPlayer() {

	return State == _Object::STATE_WALK && AttackPlayerTime > PLAYER_ATTACKTIME;
}

// Update gold amount
void _Object::UpdateGold(int Value) {

	Gold += Value;
	if(Gold < 0)
		Gold = 0;
	else if(Gold > STATS_MAXGOLD)
		Gold = STATS_MAXGOLD;
}

// Get the percentage to the next level
float _Object::GetNextLevelPercent() const {
	float Percent = 0;

	if(ExperienceNextLevel > 0)
		Percent = 1.0f - (float)ExperienceNeeded / ExperienceNextLevel;

	return Percent;
}

// Fills an array with inventory indices correlating to a trader's required items
int _Object::GetRequiredItemSlots(int *Slots) {
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
void _Object::AcceptTrader(int *Slots, int RewardSlot) {
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
int _Object::GetPotionBattle(int PotionType) {

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
bool _Object::UsePotionBattle(int Slot, int SkillType, int &HealthChange, int &ManaChange) {
	const _Item *Item = GetInventoryItem(Slot);
	if(Item == nullptr || Item->Type != _Item::TYPE_POTION)
		return false;

	HealthChange = Item->HealthRestore;
	ManaChange = Item->ManaRestore;
	UpdatePotionsLeft(SkillType);
	UpdateInventory(Slot, -1);

	return true;
}

// Uses a potion in the world
bool _Object::UsePotionWorld(int Slot) {
	const _Item *Item = GetInventoryItem(Slot);
	if(Item == nullptr)
		return false;

	// Get potion stats
	int HealthRestore = Item->HealthRestore;
	int ManaRestore = Item->ManaRestore;
	int ItemInvisPower = Item->InvisPower;

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
bool _Object::UseInventory(int Slot) {
	const _Item *Item = GetInventoryItem(Slot);
	if(Item == nullptr)
		return false;

	// Handle item types
	switch(Item->Type) {
		case _Item::TYPE_POTION:
			return UsePotionWorld(Slot);
		break;
	}

	return true;
}

// Sets an item in the inventory
void _Object::SetInventory(int Slot, int ItemID, int Count) {

	if(ItemID == 0) {
		Inventory[Slot].Item = nullptr;
		Inventory[Slot].Count = 0;
	}
	else {
		Inventory[Slot].Item = Stats->GetItem(ItemID);
		Inventory[Slot].Count = Count;
	}
}

// Sets an item in the inventory
void _Object::SetInventory(int Slot, _InventorySlot *Item) {

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
const _Item *_Object::GetInventoryItem(int Slot) {

	// Check for bad slots
	if(Slot < INVENTORY_BACKPACK || Slot >= INVENTORY_TRADE || !Inventory[Slot].Item)
		return nullptr;

	return Inventory[Slot].Item;
}

// Moves an item from one slot to another
bool _Object::MoveInventory(int OldSlot, int NewSlot) {
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
void _Object::SwapItem(int Slot, int OldSlot) {
	_InventorySlot TempItem;

	// Swap items
	TempItem = Inventory[Slot];
	Inventory[Slot] = Inventory[OldSlot];
	Inventory[OldSlot] = TempItem;
}

// Updates an item's count, deleting if necessary
bool _Object::UpdateInventory(int Slot, int Amount) {

	Inventory[Slot].Count += Amount;
	if(Inventory[Slot].Count <= 0) {
		Inventory[Slot].Item = nullptr;
		Inventory[Slot].Count = 0;
		return true;
	}

	return false;
}

// Attempts to add an item to the inventory
bool _Object::AddItem(const _Item *Item, int Count, int Slot) {

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
void _Object::MoveTradeToInventory() {

	for(int i = INVENTORY_TRADE; i < INVENTORY_COUNT; i++) {
		if(Inventory[i].Item && AddItem(Inventory[i].Item, Inventory[i].Count, -1))
			Inventory[i].Item = nullptr;
	}
}

// Splits a stack
void _Object::SplitStack(int Slot, int Count) {
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
bool _Object::IsBackpackFull() {

	// Search backpack
	for(int i = INVENTORY_BACKPACK; i < INVENTORY_TRADE; i++) {
		if(Inventory[i].Item == nullptr)
			return false;
	}

	return true;
}

// Checks if an item can be equipped
bool _Object::CanEquipItem(int Slot, const _Item *Item) {

	// Already equipped
	if(Inventory[Slot].Item)
		return false;

	// Check type
	switch(Slot) {
		case INVENTORY_HEAD:
			if(Item->Type == _Item::TYPE_HEAD)
				return true;
		break;
		case INVENTORY_BODY:
			if(Item->Type == _Item::TYPE_BODY)
				return true;
		break;
		case INVENTORY_LEGS:
			if(Item->Type == _Item::TYPE_LEGS)
				return true;
		break;
		case INVENTORY_HAND1:
			if(Item->Type == _Item::TYPE_WEAPON1HAND)
				return true;
		break;
		case INVENTORY_HAND2:
			if(Item->Type == _Item::TYPE_SHIELD)
				return true;
		break;
		case INVENTORY_RING1:
		case INVENTORY_RING2:
			if(Item->Type == _Item::TYPE_RING)
				return true;
		break;
		default:
		break;
	}

	return false;
}

// Updates a skill level
void _Object::AdjustSkillLevel(uint32_t SkillID, int Adjust) {
	if(SkillID == 0)
		return;

	const _Skill *Skill = Stats->Skills[SkillID];
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
				if(ActionBar[i] == Skill) {
					ActionBar[i] = nullptr;
					break;
				}
			}
		}
	}
}

// Calculates the number of skill points used
void _Object::CalculateSkillPoints() {

	SkillPointsUsed = 0;
	for(const auto &SkillLevel : SkillLevels) {
		const _Skill *Skill = Stats->Skills[SkillLevel.first];
		if(Skill)
			SkillPointsUsed += Skill->SkillCost * SkillLevel.second;
	}
}

// Toggles the player's busy state
void _Object::SetBusy(bool Value) {

	if(Value && State == STATE_WALK) {
		State = STATE_BUSY;
	}
	else if(!Value && State == STATE_BUSY) {
		State = STATE_WALK;
	}
}

// Starts the teleport process
void _Object::StartTeleport() {
	if(State == STATE_TELEPORT)
		State = STATE_WALK;
	else if(State == STATE_WALK)
		State = STATE_TELEPORT;

	TeleportTime = 0;
}

// Calculates all of the player stats
void _Object::CalculatePlayerStats() {
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
void _Object::CalculateLevelStats() {

	// Cap min experience
	if(Experience < 0)
		Experience = 0;

	// Cap max experience
	const _Level *MaxLevelStat = Stats->GetLevel(Stats->GetMaxLevel());
	if(Experience > MaxLevelStat->Experience)
		Experience = MaxLevelStat->Experience;

	// Find current level
	const _Level *LevelStat = Stats->FindLevel(Experience);
	Level = LevelStat->Level;
	MaxHealth = LevelStat->Health;
	MaxMana = LevelStat->Mana;
	SkillPoints = LevelStat->SkillPoints;
	ExperienceNextLevel = LevelStat->NextLevel;
	if(Level == Stats->GetMaxLevel())
		ExperienceNeeded = 0;
	else
		ExperienceNeeded = LevelStat->NextLevel - (Experience - LevelStat->Experience);

}

// Calculates stats from equipped items
void _Object::CalculateGearStats() {

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
			HealthRegen += Item->HealthRegen;
			ManaRegen += Item->ManaRegen;
		}
	}
}

// Calculates skill bonuses
void _Object::CalculateSkillStats() {

	// Go through each skill bar
	for(int i = 0; i < 8; i++) {
		const _Skill *Skill = ActionBar[i];
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
void _Object::CalculateFinalStats() {
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
