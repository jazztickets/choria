/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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
#include <objects/item.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <objects/components/inventory.h>
#include <objects/components/fighter.h>
#include <hud/hud.h>
#include <ae/ui.h>
#include <ae/font.h>
#include <ae/util.h>
#include <ae/graphics.h>
#include <ae/input.h>
#include <ae/assets.h>
#include <scripting.h>
#include <stats.h>
#include <constants.h>
#include <stats.h>
#include <scripting.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

// Constants
const int TOOLTIP_ATTRIBUTE_SPACING = 10;

// Constructor
_BaseItem::_BaseItem() :
	Type(ItemType::NONE),
	WeaponType(nullptr),
	Cost(0),
	DamageTypeID(0),
	MinDamage(0),
	MaxDamage(0),
	Armor(0),
	DamageBlock(0),
	Pierce(0),
	MaxHealth(0),
	MaxMana(0),
	HealthRegen(0),
	ManaRegen(0),
	BattleSpeed(0),
	MoveSpeed(0),
	DropRate(0),
	ResistanceTypeID(0),
	Resistance(0),
	Tradable(true),
	Destroyable(true) {
}

// Draw item tooltip
void _BaseItem::DrawTooltip(const glm::vec2 &Position, const _Object *Player, const _Cursor &Tooltip, const _Slot &CompareSlot) const {
	if(!Player)
		return;

	// Set up spacing
	std::list<std::string> HelpTextList;
	glm::vec2 Spacing = glm::vec2(TOOLTIP_ATTRIBUTE_SPACING, 0) * ae::_Element::GetUIScale();
	float SpacingY = TOOLTIP_SPACING * ae::_Element::GetUIScale();
	float ControlSpacingY = TOOLTIP_HELP_SPACING * ae::_Element::GetUIScale();
	glm::vec2 Size(INVENTORY_TOOLTIP_WIDTH, INVENTORY_TOOLTIP_HEIGHT);
	float SidePadding = TOOLTIP_SIDE_PADDING * ae::_Element::GetUIScale();
	if(ResistanceTypeID)
		Size.x += TOOLTIP_SIDE_PADDING * ae::_Element::GetUIScale();

	// Get item type
	std::string TypeText;
	if(Type != ItemType::NONE)
		TypeText = Player->Stats->ItemTypes.at(Type).second;

	// Draw base
	glm::vec2 DrawPosition;
	DrawTooltipBase(Position, Player, TypeText, DrawPosition, Size);

	// Get level of item
	int DrawLevel = Level;
	bool ShowLevel = false;

	// Draw upgrade level for items
	if(Tooltip.ItemUpgrades) {
		DrawLevel = Tooltip.ItemUpgrades;
		ShowLevel = true;
	}

	// Draw description
	DrawDescription(Player->Scripting, DrawPosition, DrawLevel, ShowLevel, Size.x - SidePadding * 2, SpacingY);

	// Get item to compare
	_InventorySlot CompareInventory;
	if(Player->Inventory->IsValidSlot(CompareSlot))
		CompareInventory = Player->Inventory->GetSlot(CompareSlot);

	bool StatDrawn = false;
	int Upgrades = Tooltip.ItemUpgrades;

	// Damage
	int DrawMinDamage = (int)GetMinDamage(Upgrades);
	int DrawMaxDamage = (int)GetMaxDamage(Upgrades);
	if(DrawMinDamage != 0 || DrawMaxDamage != 0) {
		std::stringstream Buffer;
		if(DrawMinDamage != DrawMaxDamage)
			Buffer << DrawMinDamage << " - " << DrawMaxDamage;
		else
			Buffer << DrawMinDamage;

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetAverageDamage(Upgrades), CompareInventory.Item->GetAverageDamage(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Damage", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Damage type
	if(DamageTypeID > 1) {
		std::stringstream Buffer;
		Buffer << Player->Stats->DamageTypes.at((DamageType)DamageTypeID).second;
		ae::Assets.Fonts["hud_medium"]->DrawText("Damage Type", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Pierce
	int DrawPierce = (int)GetPierce(Upgrades);
	if(DrawPierce != 0) {
		std::stringstream Buffer;
		Buffer << DrawPierce;

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetPierce(Upgrades), CompareInventory.Item->GetPierce(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Pierce", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Armor
	int DrawArmor = (int)GetArmor(Upgrades);
	if(DrawArmor != 0) {
		std::stringstream Buffer;
		Buffer << (DrawArmor < 0 ? "" : "+") << DrawArmor;

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetArmor(Upgrades), CompareInventory.Item->GetArmor(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Armor", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Damage block
	int DrawDamageBlock = (int)GetDamageBlock(Upgrades);
	if(DrawDamageBlock != 0) {
		std::stringstream Buffer;
		Buffer << (DrawDamageBlock < 0 ? "" : "+") << DrawDamageBlock;

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetDamageBlock(Upgrades), CompareInventory.Item->GetDamageBlock(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Damage Block", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Max health
	int DrawMaxHealth = (int)GetMaxHealth(Upgrades);
	if(DrawMaxHealth > 0) {
		std::stringstream Buffer;
		Buffer << (DrawMaxHealth < 0 ? "" : "+") << DrawMaxHealth;

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetMaxHealth(Upgrades), CompareInventory.Item->GetMaxHealth(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Health", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Max mana
	int DrawMaxMana = (int)GetMaxMana(Upgrades);
	if(DrawMaxMana > 0) {
		std::stringstream Buffer;
		Buffer << (DrawMaxMana < 0 ? "" : "+") << DrawMaxMana;

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetMaxMana(Upgrades), CompareInventory.Item->GetMaxMana(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Mana", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Resistance
	if(ResistanceTypeID) {
		int DrawResistance = (int)GetResistance(Upgrades);
		std::stringstream Buffer;
		Buffer << (DrawResistance < 0 ? "" : "+") << DrawResistance << "%";

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetResistance(Upgrades), CompareInventory.Item->GetResistance(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText(Player->Stats->DamageTypes.at((DamageType)ResistanceTypeID).second + " Resist", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Move speed
	int DrawMoveSpeed = (int)GetMoveSpeed(Upgrades);
	if(DrawMoveSpeed != 0) {
		std::stringstream Buffer;
		Buffer << (MoveSpeed < 0 ? "" : "+") << DrawMoveSpeed << "%";

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetMoveSpeed(Upgrades), CompareInventory.Item->GetMoveSpeed(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Move Speed", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Health regen
	int DrawHealthRegen = (int)GetHealthRegen(Upgrades);
	if(DrawHealthRegen != 0) {
		std::stringstream Buffer;
		Buffer << (DrawHealthRegen < 0 ? "" : "+") << DrawHealthRegen;

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetHealthRegen(Upgrades), CompareInventory.Item->GetHealthRegen(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Health Regen", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Mana regen
	int DrawManaRegen = (int)GetManaRegen(Upgrades);
	if(DrawManaRegen != 0) {
		std::stringstream Buffer;
		Buffer << (DrawManaRegen < 0 ? "" : "+") << DrawManaRegen;

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetManaRegen(Upgrades), CompareInventory.Item->GetManaRegen(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Mana Regen", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Drop rate
	int DrawDropRate = (int)GetDropRate(Upgrades);
	if(DrawDropRate != 0) {
		std::stringstream Buffer;
		Buffer << (DrawDropRate < 0 ? "" : "+") << DrawDropRate;

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetDropRate(Upgrades), CompareInventory.Item->GetDropRate(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Drop Rate", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	if(StatDrawn)
		DrawPosition.y += ControlSpacingY;

	// Vendors
	if(Player->Character->Vendor) {
		std::stringstream Buffer;
		if(Tooltip.Window == _HUD::WINDOW_VENDOR) {
			Buffer << "Buy " << Tooltip.ItemCount << "x for " << Tooltip.Cost << " gold";
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE, ae::Assets.Colors["gold"]);
			DrawPosition.y += SpacingY;
			HelpTextList.push_back("Right-click to buy");
		}
		else if(Tooltip.Window == _HUD::WINDOW_EQUIPMENT || Tooltip.Window == _HUD::WINDOW_INVENTORY) {
			Buffer << "Sell for " << Tooltip.Cost << " gold";
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition), ae::CENTER_BASELINE, ae::Assets.Colors["gold"]);
			DrawPosition.y += SpacingY;
			HelpTextList.push_back("Shift+Right-click to sell");
		}
	}

	// Tradable
	if(!Tradable && (Tooltip.Window == _HUD::WINDOW_INVENTORY || Tooltip.Window == _HUD::WINDOW_STASH || Tooltip.Window == _HUD::WINDOW_VENDOR || Tooltip.Window == _HUD::WINDOW_TRADER)) {
		ae::Assets.Fonts["hud_small"]->DrawText("Untradable", glm::ivec2(DrawPosition), ae::CENTER_BASELINE, ae::Assets.Colors["red"]);
		DrawPosition.y += ControlSpacingY;
	}

	// Draw help text
	std::string InfoText;
	glm::vec4 InfoColor = ae::Assets.Colors["gray"];
	switch(Type) {
		case ItemType::ONEHANDED_WEAPON:
		case ItemType::TWOHANDED_WEAPON:
		case ItemType::HELMET:
		case ItemType::ARMOR:
		case ItemType::BOOTS:
		case ItemType::SHIELD:
			if(Tooltip.Window == _HUD::WINDOW_INVENTORY || Tooltip.Window == _HUD::WINDOW_STASH)
				HelpTextList.push_back("Right-click to equip");
		break;
		case ItemType::CONSUMABLE:
			if((Tooltip.Window == _HUD::WINDOW_INVENTORY || Tooltip.Window == _HUD::WINDOW_STASH) && CheckScope(ScopeType::WORLD))
				HelpTextList.push_back("Right-click to use");
			else if(Tooltip.Window == _HUD::WINDOW_ACTIONBAR && CheckScope(ScopeType::WORLD))
				HelpTextList.push_back("Left-click to use");
		break;
		case ItemType::UNLOCKABLE: {
			if(!Player->Character->HasUnlocked(this))
				HelpTextList.push_back("Right-click to unlock");
			else {
				InfoText = "Already unlocked";
				InfoColor = ae::Assets.Colors["red"];
			}
		} break;
		case ItemType::KEY: {
			if(Player->Inventory->GetBag(BagType::KEYS).HasItem(ID)) {
				InfoText = "Already in keychain";
				InfoColor = ae::Assets.Colors["red"];
			}
			else if(Tooltip.Window == _HUD::WINDOW_INVENTORY || Tooltip.Window == _HUD::WINDOW_STASH) {
				HelpTextList.push_back("Right-click to add to keychain");
			}
		} break;
		default:
		break;
	}

	// Draw help text
	if(InfoText.length()) {
		ae::Assets.Fonts["hud_small"]->DrawText(InfoText, glm::ivec2(DrawPosition), ae::CENTER_BASELINE, InfoColor);
		DrawPosition.y += ControlSpacingY;
	}

	// Move hint
	if(Player->Character->ViewingStash || Player->Character->IsTrading())
		HelpTextList.push_back("Shift+click to move");

	// Split hint
	if((Tooltip.Window == _HUD::WINDOW_INVENTORY || Tooltip.Window == _HUD::WINDOW_STASH) && Tooltip.ItemCount > 1)
		HelpTextList.push_back("Ctrl+click to split");

	// Draw ui hints
	for(const auto &Text : HelpTextList) {
		ae::Assets.Fonts["hud_small"]->DrawText(Text, glm::ivec2(DrawPosition), ae::CENTER_BASELINE, ae::Assets.Colors["gray"]);
		DrawPosition.y += ControlSpacingY;
	}
}

// Return a valid equipment slot for an item
void _BaseItem::GetEquipmentSlot(_Slot &Slot) const {

	Slot.Type = BagType::EQUIPMENT;
	switch(Type) {
		case ItemType::HELMET:
			Slot.Index = EquipmentType::HEAD;
		break;
		case ItemType::ARMOR:
			Slot.Index = EquipmentType::BODY;
		break;
		case ItemType::BOOTS:
			Slot.Index = EquipmentType::LEGS;
		break;
		case ItemType::ONEHANDED_WEAPON:
		case ItemType::TWOHANDED_WEAPON:
			Slot.Index = EquipmentType::HAND1;
		break;
		case ItemType::SHIELD:
			Slot.Index = EquipmentType::HAND2;
		break;
		case ItemType::RING:
			Slot.Index = EquipmentType::RING1;
		break;
		case ItemType::AMULET:
			Slot.Index = EquipmentType::AMULET;
		break;
		default:
			Slot.Type = BagType::NONE;
		break;
	}
}

// Returns the item's price to/from a vendor
int _BaseItem::GetPrice(const _Vendor *Vendor, int QueryCount, bool Buy, int Upgrades) const {
	if(!Vendor)
		return 0;

	// Calculate
	float Percent;
	if(Buy)
		Percent = Vendor->BuyPercent;
	else
		Percent = Vendor->SellPercent;

	int Price = (int)(Cost * Percent) * QueryCount;

	// Add some value for upgrades
	if(Upgrades) {
		for(int i = 1; i <= Upgrades; i++)
			Price += GetUpgradePrice(i) * Percent;
	}

	// Cap
	if(Price < 0)
		Price = 0;
	else if(Price > PLAYER_MAX_GOLD)
		Price = PLAYER_MAX_GOLD;

	return Price;
}

// Get upgrade price
int _BaseItem::GetUpgradePrice(int Upgrades) const {
	if(MaxLevel <= 0)
		return 0;

	return (int)(std::ceil(GAME_UPGRADE_COST_MULTIPLIER * Upgrades * Cost + GAME_BASE_UPGRADE_COST));
}

// Apply cost and return non zero flags to continue with action result
bool _BaseItem::ApplyCost(_ActionResult &ActionResult, ActionResultFlag &ResultFlags) const {

	// Get slot where item is
	_Slot FoundSlot;
	if(!ActionResult.Source.Object->Inventory->FindItem(this, FoundSlot, ActionResult.Source.Object->Character->Action.Slot))
		return false;

	// Spend item
	ActionResult.Source.Object->Inventory->UpdateItemCount(FoundSlot, -1);
	ResultFlags |= ActionResultFlag::DECREMENT;

	// Unlock item
	if(IsUnlockable()) {
		ActionResult.Source.Object->Character->Unlocks[ID].Level = 1;
		ResultFlags |= ActionResultFlag::UNLOCK;
	}
	// Add key to keychain
	else if(IsKey()) {
		ActionResult.Source.Object->Inventory->GetBag(BagType::KEYS).Slots.push_back(_InventorySlot(this, 1));
		ResultFlags |= ActionResultFlag::KEY;
	}

	return true;
}

float _BaseItem::GetAverageDamage(int Upgrades) const {
	return (GetUpgradedValue<float>(StatType::MINDAMAGE, Upgrades, MinDamage) + GetUpgradedValue<float>(StatType::MAXDAMAGE, Upgrades, MaxDamage)) / 2.0f;
}

// Get min damage
float _BaseItem::GetMinDamage(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::MINDAMAGE, Upgrades, MinDamage);
}

// Get max damage
float _BaseItem::GetMaxDamage(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::MAXDAMAGE, Upgrades, MaxDamage);
}

// Get armor
float _BaseItem::GetArmor(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::ARMOR, Upgrades, Armor);
}

// Get damage block
float _BaseItem::GetDamageBlock(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::DAMAGEBLOCK, Upgrades, DamageBlock);
}

// Get pierce
float _BaseItem::GetPierce(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::PIERCE, Upgrades, Pierce);
}

// Get max health
float _BaseItem::GetMaxHealth(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::MAXHEALTH, Upgrades, MaxHealth);
}

// Get max mana
float _BaseItem::GetMaxMana(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::MAXMANA, Upgrades, MaxMana);
}

// Get health regen
float _BaseItem::GetHealthRegen(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::HEALTHREGEN, Upgrades, HealthRegen);
}

// Get mana regen
float _BaseItem::GetManaRegen(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::MANAREGEN, Upgrades, ManaRegen);
}

// Get move speed
float _BaseItem::GetMoveSpeed(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::MOVESPEED, Upgrades, MoveSpeed);
}

// Get resistance
float _BaseItem::GetResistance(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::RESIST, Upgrades, Resistance);
}

// Get drop rate
float _BaseItem::GetDropRate(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::DROPRATE, Upgrades, DropRate);
}

// Check if an item can be used
bool _BaseItem::CheckRequirements(_Scripting *Scripting, _ActionResult &ActionResult) const {

	// Check for item in key bag
	if(IsKey())
		return !ActionResult.Source.Object->Inventory->GetBag(BagType::KEYS).HasItem(ID);

	// Unlocking item
	if(IsUnlockable())
		return !ActionResult.Source.Object->Character->HasUnlocked(this);

	// Check for item count
	_Slot FoundSlot;
	if(!ActionResult.Source.Object->Inventory->FindItem(this, FoundSlot, ActionResult.ActionUsed.Slot))
		return false;

	return true;
}

// Get appropriate text color when comparing items
glm::vec4 _BaseItem::GetCompareColor(float ItemValue, float EquippedValue) const {
	if(ItemValue > EquippedValue)
		return ae::Assets.Colors["green"];
	else if(ItemValue < EquippedValue)
		return ae::Assets.Colors["red"];

	return glm::vec4(1.0f);
}

// Return value of a stat after upgrades
template<typename T> T _BaseItem::GetUpgradedValue(StatType Type, int Upgrades, T Value) const {
	if(MaxLevel <= 0)
		return Value;

	//return Value + (T)(GAME_UPGRADE_AMOUNT * Stats->UpgradeScale.at(Type) * Upgrades * std::abs(Value));
	return Value + (T)(GAME_UPGRADE_AMOUNT * 1 * Upgrades * std::abs(Value));
}
