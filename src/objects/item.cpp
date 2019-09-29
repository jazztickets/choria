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
#include <iostream>

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
	Tradable(true) {
}

// Draw tooltip
void _BaseItem::DrawTooltip(const glm::vec2 &Position, const _Object *Player, const _Cursor &Tooltip, const _Slot &CompareSlot) const {
	if(!Player)
		return;

	ae::_Element *TooltipElement = ae::Assets.Elements["element_item_tooltip"];
	ae::_Element *TooltipName = ae::Assets.Elements["label_item_tooltip_name"];
	ae::_Element *TooltipType = ae::Assets.Elements["label_item_tooltip_type"];
	TooltipElement->SetActive(true);

	// Set label values
	TooltipName->Text = Name;
	TooltipType->Text = "";
	if(Type != ItemType::NONE)
		TooltipType->Text = Player->Stats->ItemTypes.at(Type).second;

	// Set up window size
	glm::vec2 Size;
	Size.x = INVENTORY_TOOLTIP_WIDTH * ae::_Element::GetUIScale();
	float SidePadding = 36 * ae::_Element::GetUIScale();
	float SpacingY = 36 * ae::_Element::GetUIScale();
	float ControlSpacingY = 28 * ae::_Element::GetUIScale();
	float LargeSpacingY = 56 * ae::_Element::GetUIScale();
	glm::vec2 Spacing = glm::vec2(10, 0) * ae::_Element::GetUIScale();

	// Set window width
	ae::_TextBounds TextBounds;
	ae::Assets.Fonts["hud_medium"]->GetStringDimensions(TooltipName->Text, TextBounds);
	Size.x = std::max(Size.x, (float)TextBounds.Width / ae::_Element::GetUIScale()) + SidePadding * 2;
	if(ResistanceTypeID)
		Size.x += 36 * ae::_Element::GetUIScale();

	// Set window height
	Size.y = INVENTORY_TOOLTIP_HEIGHT * ae::_Element::GetUIScale();
	if(Player->Character->Vendor)
		Size.y += LargeSpacingY;

	// Position window
	glm::vec2 WindowOffset = Position;

	// Center vertically
	if(Position.y < 0) {
		WindowOffset.y = (ae::Graphics.CurrentSize.y - Size.y) / 2;
	}
	else {
		WindowOffset.x += INVENTORY_TOOLTIP_OFFSET * ae::_Element::GetUIScale();
		WindowOffset.y += -(TooltipElement->Bounds.End.y - TooltipElement->Bounds.Start.y) / 2;
	}

	// Reposition window if out of bounds
	if(WindowOffset.x + Size.x > ae::Graphics.Element->Bounds.End.x - INVENTORY_TOOLTIP_PADDING)
		WindowOffset.x -= Size.x + INVENTORY_TOOLTIP_OFFSET + INVENTORY_TOOLTIP_PADDING;
	if(WindowOffset.y + Size.y > ae::Graphics.Element->Bounds.End.y - INVENTORY_TOOLTIP_PADDING)
		WindowOffset.y -= Size.y + INVENTORY_TOOLTIP_OFFSET - (TooltipElement->Bounds.End.y - TooltipElement->Bounds.Start.y) / 2;

	TooltipElement->Offset = WindowOffset;
	TooltipElement->Size = Size;
	TooltipElement->CalculateBounds(false);

	// Render tooltip
	TooltipElement->Render();
	TooltipElement->SetActive(false);

	// Set draw position to center of window
	glm::vec2 DrawPosition((int)(TooltipElement->Size.x / 2 + WindowOffset.x), (int)TooltipType->Bounds.End.y);
	DrawPosition.y += LargeSpacingY;

	// Draw target text
	if(Target != TargetType::NONE) {
		DrawPosition.y -= 28 * ae::_Element::GetUIScale();
		std::string InfoText = "Target " + Player->Stats->TargetTypes.at(Target).second;
		ae::Assets.Fonts["hud_small"]->DrawText(InfoText, DrawPosition, ae::CENTER_BASELINE, glm::vec4(1.0f));
		DrawPosition.y += LargeSpacingY;
	}

	// Get level of item
	int DrawLevel = Level;
	bool ShowLevel = false;

	// Draw upgrade level for items
	if(Tooltip.ItemUpgrades) {
		ae::Assets.Fonts["hud_small"]->DrawText("Level " + std::to_string(Tooltip.ItemUpgrades), DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["gray"]);
		DrawPosition.y += SpacingY;
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

		ae::Assets.Fonts["hud_medium"]->DrawText("Damage", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + Spacing, ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Damage type
	if(DamageTypeID > 1) {
		std::stringstream Buffer;
		Buffer << Player->Stats->DamageTypes.at((DamageType)DamageTypeID).second;
		ae::Assets.Fonts["hud_medium"]->DrawText("Damage Type", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + Spacing, ae::LEFT_BASELINE);
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

		ae::Assets.Fonts["hud_medium"]->DrawText("Pierce", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + Spacing, ae::LEFT_BASELINE, Color);
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

		ae::Assets.Fonts["hud_medium"]->DrawText("Armor", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + Spacing, ae::LEFT_BASELINE, Color);
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

		ae::Assets.Fonts["hud_medium"]->DrawText("Damage Block", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + Spacing, ae::LEFT_BASELINE, Color);
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

		ae::Assets.Fonts["hud_medium"]->DrawText("Health", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + Spacing, ae::LEFT_BASELINE, Color);
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

		ae::Assets.Fonts["hud_medium"]->DrawText("Mana", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + Spacing, ae::LEFT_BASELINE, Color);
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

		ae::Assets.Fonts["hud_medium"]->DrawText(Player->Stats->DamageTypes.at((DamageType)ResistanceTypeID).second + " Resist", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + Spacing, ae::LEFT_BASELINE, Color);
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

		ae::Assets.Fonts["hud_medium"]->DrawText("Move Speed", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + Spacing, ae::LEFT_BASELINE, Color);
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

		ae::Assets.Fonts["hud_medium"]->DrawText("Health Regen", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + Spacing, ae::LEFT_BASELINE, Color);
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

		ae::Assets.Fonts["hud_medium"]->DrawText("Mana Regen", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + Spacing, ae::LEFT_BASELINE, Color);
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

		ae::Assets.Fonts["hud_medium"]->DrawText("Drop Rate", DrawPosition + -Spacing, ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition + Spacing, ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	if(StatDrawn)
		DrawPosition.y += SpacingY;

	// Vendors
	if(Player->Character->Vendor) {
		std::stringstream Buffer;
		if(Tooltip.Window == _HUD::WINDOW_VENDOR) {
			Buffer << "Buy " << Tooltip.ItemCount << "x for " << Tooltip.Cost << " gold";
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["gold"]);
			DrawPosition.y += SpacingY;
			ae::Assets.Fonts["hud_small"]->DrawText("Right-click to buy", DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["gray"]);
			DrawPosition.y += SpacingY;
		}
		else if(Tooltip.Window == _HUD::WINDOW_EQUIPMENT || Tooltip.Window == _HUD::WINDOW_INVENTORY) {
			Buffer << "Sell for " << Tooltip.Cost << " gold";
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["gold"]);
			DrawPosition.y += SpacingY;
			ae::Assets.Fonts["hud_small"]->DrawText("Shift+Right-click to sell", DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["gray"]);
			DrawPosition.y += SpacingY;
		}
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
			if(Tooltip.Window == _HUD::WINDOW_INVENTORY && Tooltip.Slot.Type == BagType::INVENTORY)
				InfoText = "Right-click to equip";
		break;
		case ItemType::CONSUMABLE:
			if(Tooltip.Window == _HUD::WINDOW_INVENTORY && CheckScope(ScopeType::WORLD))
				InfoText = "Right-click to use";
			else if(Tooltip.Window == _HUD::WINDOW_ACTIONBAR && CheckScope(ScopeType::WORLD))
				InfoText = "Left-click to use";
		break;
		case ItemType::UNLOCKABLE: {
			if(!Player->Character->HasUnlocked(this))
				InfoText = "Right-click to unlock";
			else {
				InfoText = "Already unlocked";
				InfoColor = ae::Assets.Colors["red"];
			}
		} break;
		case ItemType::KEY: {
			if(!Player->Inventory->GetBag(BagType::KEYS).HasItem(ID))
				InfoText = "Right-click to add to keychain";
			else {
				InfoText = "Already in keychain";
				InfoColor = ae::Assets.Colors["red"];
			}
		} break;
		default:
		break;
	}

	if(InfoText.length()) {
		ae::Assets.Fonts["hud_small"]->DrawText(InfoText, DrawPosition, ae::CENTER_BASELINE, InfoColor);
		DrawPosition.y += ControlSpacingY;
	}

	if(Tooltip.Window == _HUD::WINDOW_INVENTORY && Tooltip.ItemCount > 1) {
		ae::Assets.Fonts["hud_small"]->DrawText("Ctrl+click to split", DrawPosition, ae::CENTER_BASELINE, InfoColor);
		DrawPosition.y += SpacingY;
	}

	if(!Tradable && (Tooltip.Window == _HUD::WINDOW_INVENTORY || Tooltip.Window == _HUD::WINDOW_VENDOR || Tooltip.Window == _HUD::WINDOW_TRADER)) {
		ae::Assets.Fonts["hud_small"]->DrawText("Untradable", DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["red"]);
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
		return !ActionResult.Source.Object->Character->HasUnlocked(AsItem());

	// Check for item count
	size_t Index;
	if(!ActionResult.Source.Object->Inventory->FindItem(ActionResult.ActionUsed.Usable->AsItem(), Index, (size_t)ActionResult.ActionUsed.InventorySlot))
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
