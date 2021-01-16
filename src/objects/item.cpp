/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020 Alan Witkowski
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
#include <ae/input.h>
#include <scripting.h>
#include <stats.h>
#include <constants.h>
#include <stats.h>
#include <config.h>
#include <scripting.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>
#include <algorithm>
#include <SDL_keycode.h>

// Category names
static const std::string SkillCategories[5] = {
	"Passive Skill",
	"Attack Skill",
	"Spell",
	"Summon Spell",
	"Skill",
};

// Stats to hide on the item tooltip
static const std::unordered_map<std::string, int> HiddenStats = {
	{ "Cursed", 1 },
	{ "MinDamage", 1 },
	{ "MaxDamage", 1 },
	{ "Resist", 1 },
};

// Draw tooltip
void _Item::DrawTooltip(const glm::vec2 &Position, _Object *Player, const _Cursor &Tooltip, const _Slot &CompareSlot) const {
	if(!Player)
		return;

	bool Blacksmith = (Tooltip.Window == _HUD::WINDOW_BLACKSMITH);
	bool ShowAltText = ae::Input.ModKeyDown(KMOD_ALT);
	bool ShowFractions = ae::Input.ModKeyDown(KMOD_ALT) || Blacksmith;

	ae::_Element *TooltipElement = ae::Assets.Elements["element_item_tooltip"];
	ae::_Element *TooltipName = ae::Assets.Elements["label_item_tooltip_name"];
	ae::_Element *TooltipType = ae::Assets.Elements["label_item_tooltip_type"];
	TooltipElement->SetActive(true);

	// Set label values
	TooltipName->Text = Name;
	TooltipType->Text = "";
	TooltipType->Color = glm::vec4(1.0f);
	if(Category && Category <= 5)
		TooltipType->Text = SkillCategories[Category-1];
	else if(Type != ItemType::NONE) {
		TooltipType->Text = Player->Stats->ItemTypes.at((uint32_t)Type);
		if(Type == ItemType::CONSUMABLE && Scope == ScopeType::BATTLE) {
			TooltipType->Text = "Battle " + TooltipType->Text;
		}
		else if(IsCursed()) {
			TooltipType->Text = "Cursed " + TooltipType->Text;
			TooltipType->Color = ae::Assets.Colors["red"];
		}
	}

	// Set up window size
	std::list<std::string> HelpTextList;
	glm::vec2 Size;
	glm::vec2 DrawPosition;
	Size.x = INVENTORY_TOOLTIP_WIDTH * ae::_Element::GetUIScale();
	float SidePadding = 36 * ae::_Element::GetUIScale();
	float SpacingY = 36 * ae::_Element::GetUIScale();
	float ControlSpacingY = 32 * ae::_Element::GetUIScale();
	float LargeSpacingY = 56 * ae::_Element::GetUIScale();
	float RewindSpacingY = -28 * ae::_Element::GetUIScale();
	glm::vec2 Spacing = glm::vec2(10, 0) * ae::_Element::GetUIScale();

	// Set window width
	ae::_TextBounds TextBounds;
	ae::Assets.Fonts["hud_medium"]->GetStringDimensions(TooltipName->Text, TextBounds);
	Size.x = std::max(Size.x, (float)TextBounds.Width / ae::_Element::GetUIScale()) + SidePadding * 2;

	// Set window height
	Size.y = INVENTORY_TOOLTIP_HEIGHT * ae::_Element::GetUIScale();
	if(Player->Character->Vendor)
		Size.y += LargeSpacingY;
	if(IsCursed())
		Size.y += SpacingY;
	if(Player->Character->IsTrading())
		Size.y += SpacingY;
	if(Player->Character->Blacksmith)
		Size.y += SpacingY + LargeSpacingY;
	if(Type == ItemType::MAP)
		Size.y = INVENTORY_TOOLTIP_MAP_HEIGHT;

	// Increase size for description
	int DescriptionWidth = Size.x - SidePadding * 2;
	int DescriptionLines = DrawDescription(false, Player, Script, DrawPosition, false, 0, 0, 0, 0, true, DescriptionWidth, 0);
	if(!Proc.empty()) {
		DescriptionLines += DrawDescription(false, Player, Proc, DrawPosition, false, 0, 0, 0, 0, false, DescriptionWidth, 0);
	}
	if(SetID)
		DescriptionLines += DrawSetDescription(false, Player, DrawPosition, false, DescriptionWidth, 0) + 1;
	float DescriptionSizeY = DescriptionLines * INVENTORY_TOOLTIP_TEXT_SPACING * ae::_Element::GetUIScale();
	Size.y += DescriptionSizeY;

	// Add extra size for two skill levels
	if(IsSkill() && (Tooltip.Window == _HUD::WINDOW_SKILLS || Tooltip.Window == _HUD::WINDOW_ENCHANTER))
		Size.y += DescriptionSizeY + SpacingY;

	// Increase window size for each attribute
	int Upgrades = Tooltip.InventorySlot.Upgrades;
	Size.y += SpacingY * GetAttributeCount(Upgrades);

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
	if(WindowOffset.y < 0)
		WindowOffset.y = 0;

	TooltipElement->Offset = WindowOffset;
	TooltipElement->Size = Size;
	TooltipElement->CalculateBounds(false);

	// Render tooltip
	TooltipElement->Render();
	TooltipElement->SetActive(false);

	// Set draw position to center of window
	DrawPosition = glm::vec2((int)(TooltipElement->Size.x / 2 + WindowOffset.x), (int)TooltipType->Bounds.End.y);
	DrawPosition.y += LargeSpacingY;

	// Draw map
	if(Type == ItemType::MAP) {
		if(AltTexture == nullptr)
			return;

		// Restrict viewing
		if(Tooltip.Window == _HUD::WINDOW_VENDOR || Tooltip.Window == _HUD::WINDOW_TRADER || Tooltip.Window == _HUD::WINDOW_TRADETHEIRS) {
			ae::Assets.Fonts["hud_small"]->DrawText("Purchase to view", glm::ivec2(DrawPosition), ae::CENTER_BASELINE, ae::Assets.Colors["red"]);
			return;
		}

		// Get size
		DrawPosition.y = TooltipElement->Size.y / 2 + WindowOffset.y;
		glm::vec2 TextureSize = glm::vec2(AltTexture->Size) * (TooltipElement->Size.x / AltTexture->Size.x / ae::_Element::GetUIScale()) * 0.45f * ae::_Element::GetUIScale();
		ae::_Bounds Bounds(DrawPosition - TextureSize, DrawPosition + TextureSize);

		// Draw image
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.SetColor(glm::vec4(1.0f));
		ae::Graphics.DrawImage(Bounds, AltTexture, true);

		return;
	}

	// Draw target text
	if(TargetID != TargetType::NONE) {
		DrawPosition.y += RewindSpacingY;
		std::string DeadText = TargetAlive ? "" : "Dead ";
		std::string InfoText = "Target " + DeadText + Player->Stats->TargetTypes.at((uint32_t)TargetID);
		if(TargetID == TargetType::ENEMY_CORPSE_AOE)
			InfoText = "Target Dead Enemy";
		ae::Assets.Fonts["hud_small"]->DrawText(InfoText, DrawPosition, ae::CENTER_BASELINE, glm::vec4(1.0f));
		DrawPosition.y += LargeSpacingY;
	}

	// Draw cooldown
	if(!IsEquippable() && Cooldown > 0.0) {
		DrawPosition.y += RewindSpacingY;
		std::stringstream Buffer;
		double Duration = Cooldown * Player->Character->Attributes["Cooldowns"].Mult();
		if(!ae::Input.ModKeyDown(KMOD_ALT) && Duration > 60.0)
			Buffer << std::fixed << std::setprecision(1) << (int)(Duration / 60.0) << " minute cooldown";
		else
			Buffer << std::fixed << std::setprecision(1) << Duration << " second cooldown";
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["red"]);
		DrawPosition.y += LargeSpacingY;
	}

	// Get level of item or skill
	int DrawLevel = Level;
	int PlayerMaxSkillLevel = 0;
	int EnchanterMaxLevel = 0;
	int SkillLevel = 0;
	bool IsLocked = false;
	bool ShowLevel = false;
	bool ShowingNextUpgradeLevel = false;
	if(IsSkill()) {

		// Get max skill level
		if(Tooltip.Window == _HUD::WINDOW_ENCHANTER) {
			auto MaxSkillLevelIterator = Player->Character->MaxSkillLevels.find(ID);
			if(MaxSkillLevelIterator != Player->Character->MaxSkillLevels.end()) {
				DrawLevel = MaxSkillLevelIterator->second + 1;
				if(Player->Character->Enchanter)
					EnchanterMaxLevel = Player->Character->Enchanter->Level;
			}
		}
		else {

			// Get skill level
			auto SkillIterator = Player->Character->Skills.find(ID);
			if(SkillIterator != Player->Character->Skills.end()) {
				PlayerMaxSkillLevel = Player->Character->MaxSkillLevels[ID];
				SkillLevel = DrawLevel = SkillIterator->second;
				if(!(Tooltip.Window == _HUD::WINDOW_SKILLS && ae::Input.ModKeyDown(KMOD_ALT)) && SkillIterator->second > 0) {
					DrawLevel += Player->Character->Attributes["AllSkills"].Int;
					DrawLevel = std::min(DrawLevel, PlayerMaxSkillLevel);
				}
			}
			else
				IsLocked = true;
		}

		// For skills set minimum of level 1
		DrawLevel = std::max(DrawLevel, 1);

		// Show vendor skills at level 1
		if(Tooltip.Window == _HUD::WINDOW_EQUIPMENT || Tooltip.Window == _HUD::WINDOW_INVENTORY || Tooltip.Window == _HUD::WINDOW_VENDOR || Tooltip.Window == _HUD::WINDOW_TRADER)
			DrawLevel = 1;

		// Determine whether to show level
		if(Tooltip.Window == _HUD::WINDOW_SKILLS || Tooltip.Window == _HUD::WINDOW_ENCHANTER || Tooltip.Window == _HUD::WINDOW_SKILLBAR)
			ShowLevel = true;
	}
	else {

		// Draw upgrade level for items
		if(IsEquippable()) {

			// Show next upgrade stats if holding ctrl and item is not in the blacksmith slot
			if(Player->Character->Blacksmith && ae::Input.ModKeyDown(KMOD_CTRL) && CompareSlot.Index != NOSLOT && Tooltip.Window != _HUD::WINDOW_BLACKSMITH) {
				if(Upgrades + 1 <= MaxLevel)
					ShowingNextUpgradeLevel = true;

				Upgrades = std::min(Upgrades + 1, MaxLevel);
			}

			// Get level text
			std::stringstream Buffer;
			if((MaxLevel == 0 || Upgrades == MaxLevel))
				Buffer << "Max Level";
			else
				Buffer << "Level " << Upgrades << "/" << MaxLevel;
			DrawPosition.y += RewindSpacingY;
			ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["gray"]);
			DrawPosition.y += LargeSpacingY;
		}
	}

	// Draw proc info
	DrawDescription(true, Player, Proc, DrawPosition, Blacksmith, DrawLevel, PlayerMaxSkillLevel, EnchanterMaxLevel, Upgrades, ShowLevel, DescriptionWidth, SpacingY);

	// Draw set info
	DrawSetDescription(true, Player, DrawPosition, Blacksmith, DescriptionWidth, SpacingY);

	// Draw item info
	DrawDescription(true, Player, Script, DrawPosition, Blacksmith, DrawLevel, PlayerMaxSkillLevel, EnchanterMaxLevel, Upgrades, ShowLevel, DescriptionWidth, SpacingY);

	// Draw next level description
	if(IsSkill() && Tooltip.Window == _HUD::WINDOW_SKILLS)
		DrawDescription(true, Player, Script, DrawPosition, false, DrawLevel+1, PlayerMaxSkillLevel, EnchanterMaxLevel, 0, true, Size.x - SidePadding * 2, SpacingY);

	// Get item to compare
	_InventorySlot CompareInventory;
	if(Player->Inventory->IsValidSlot(CompareSlot))
		CompareInventory = Player->Inventory->GetSlot(CompareSlot);

	bool StatDrawn = false;

	// Damage
	int DrawMinDamage = std::floor(GetAttribute("MinDamage", Upgrades));
	int DrawMaxDamage = std::floor(GetAttribute("MaxDamage", Upgrades));
	if(DrawMinDamage != 0 || DrawMaxDamage != 0) {
		std::stringstream Buffer;
		if(ShowAltText)
			Buffer << ae::Round((DrawMinDamage + DrawMaxDamage) * 0.5f);
		else if(DrawMinDamage != DrawMaxDamage)
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
	if(!IsSkill() && DamageTypeID > 1) {
		std::stringstream Buffer;
		Buffer << Stats->DamageTypes.at(DamageTypeID).Name;
		ae::Assets.Fonts["hud_medium"]->DrawText("Damage Type", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Display attributes
	for(const auto &AttributeName : Stats->AttributeRank) {
		const _Attribute &Attribute = Stats->Attributes.at(AttributeName);
		if(HiddenStats.find(AttributeName) != HiddenStats.end())
			continue;

		if(Attributes.find(AttributeName) == Attributes.end())
			continue;

		// Get upgraded stat
		float UpgradedValue = GetAttribute(AttributeName, Upgrades);
		if(UpgradedValue == 0.0f)
			continue;

		// Show fractions
		if(!ShowFractions)
			UpgradedValue = std::floor(UpgradedValue);

		// Get value string
		std::stringstream Buffer;
		Buffer << (UpgradedValue < 0 ? "" : "+") << UpgradedValue;
		if(Attribute.Type == StatValueType::PERCENT)
			Buffer << "%";

		// Get compare color
		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetAttribute(AttributeName, Upgrades), CompareInventory.Item->GetAttribute(AttributeName, CompareInventory.Upgrades));

		// Draw label and stat
		ae::Assets.Fonts["hud_medium"]->DrawText(Attribute.Label, glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Resistance
	if(ResistanceTypeID) {
		float DrawResistance = GetAttribute("Resist", Upgrades);
		if(!ShowFractions)
			DrawResistance = std::floor(DrawResistance);

		std::stringstream Buffer;
		Buffer << (DrawResistance < 0 ? "" : "+") << DrawResistance << "%";

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetAttribute("Resist", Upgrades), CompareInventory.Item->GetAttribute("Resist", CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText(Player->Stats->DamageTypes.at(ResistanceTypeID).Name + " Resist", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Cooldown reduction
	float DrawCooldownReduction = GetCooldownReduction(Upgrades);
	if(IsEquippable() && DrawCooldownReduction != 0.0f) {
		if(!ShowFractions)
			DrawCooldownReduction = std::floor(DrawCooldownReduction);

		std::stringstream Buffer;
		Buffer << (DrawCooldownReduction < 0 ? "" : "+") << DrawCooldownReduction << "%";

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(-GetCooldownReduction(Upgrades), -CompareInventory.Item->GetCooldownReduction(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Cooldowns", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	if(StatDrawn)
		DrawPosition.y += SpacingY;

	// Vendors
	if(Player->Character->Vendor) {
		std::stringstream Buffer;
		if(Tooltip.Window == _HUD::WINDOW_VENDOR) {
			Buffer << "Buy " << Tooltip.InventorySlot.Count << "x for " << Tooltip.Cost << " gold";
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["gold"]);
			DrawPosition.y += SpacingY;
			HelpTextList.push_back("Right-click to buy");
		}
		else if(Tooltip.Window == _HUD::WINDOW_EQUIPMENT || Tooltip.Window == _HUD::WINDOW_INVENTORY) {
			Buffer << "Sell for " << Tooltip.Cost << " gold";
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["gold"]);
			DrawPosition.y += SpacingY;
			if(Config.RightClickSell)
				HelpTextList.push_back("Right-click to sell");
			else
				HelpTextList.push_back("Shift+Right-click to sell");
		}
	}
	else if(Player->Character->Blacksmith) {
		std::stringstream Buffer;
		if(Player->Character->Blacksmith && Player->Character->Blacksmith->CanUpgrade(this, Upgrades - ShowingNextUpgradeLevel) && (Tooltip.Window == _HUD::WINDOW_EQUIPMENT || Tooltip.Window == _HUD::WINDOW_INVENTORY)) {
			glm::vec4 Color = ae::Assets.Colors["gold"];
			if(Tooltip.Cost > Player->Character->Attributes["Gold"].Int64)
				Color = ae::Assets.Colors["red"];

			Buffer << "Upgrade for " << Tooltip.Cost << " gold";
			ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
			DrawPosition.y += SpacingY;
			HelpTextList.push_back("Ctrl+click to buy upgrade");
		}
	}

	if(Tooltip.Window == _HUD::WINDOW_ENCHANTER) {
		std::stringstream Buffer;
		Buffer << "Upgrade for " << Tooltip.Cost << " gold";
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["gold"]);
		DrawPosition.y += SpacingY;
	}

	// Tradable
	if(!Tradable && (Tooltip.Window == _HUD::WINDOW_INVENTORY || Tooltip.Window == _HUD::WINDOW_VENDOR || Tooltip.Window == _HUD::WINDOW_TRADER)) {
		ae::Assets.Fonts["hud_small"]->DrawText("Cannot trade with other players", DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["red"]);
		DrawPosition.y += ControlSpacingY;
	}

	// Draw help text
	std::string InfoText;
	glm::vec4 InfoColor = ae::Assets.Colors["gray"];
	switch(Type) {
		case ItemType::HELMET:
		case ItemType::ARMOR:
		case ItemType::BOOTS:
		case ItemType::ONEHANDED_WEAPON:
		case ItemType::TWOHANDED_WEAPON:
		case ItemType::SHIELD:
		case ItemType::RING:
		case ItemType::AMULET:
		case ItemType::OFFHAND:
			if(IsCursed()) {
				InfoText = "Cursed items cannot be unequipped";
				InfoColor = ae::Assets.Colors["red"];
			}
			else if((Tooltip.Slot.Type == BagType::INVENTORY || Tooltip.Slot.Type == BagType::TRADE) && !(Player->Character->Vendor && Config.RightClickSell))
				HelpTextList.push_back("Right-click to equip");
			else if(Tooltip.Slot.Type == BagType::EQUIPMENT)
				HelpTextList.push_back("Right-click to unequip");
		break;
		case ItemType::CONSUMABLE:
			if(Tooltip.Window == _HUD::WINDOW_INVENTORY && CheckScope(ScopeType::WORLD) && !(Player->Character->Vendor && Config.RightClickSell))
				HelpTextList.push_back("Right-click to use");
			else if(Tooltip.Window == _HUD::WINDOW_SKILLBAR && CheckScope(ScopeType::WORLD))
				HelpTextList.push_back("Left-click to use");
		break;
		case ItemType::SKILL:
			if(Tooltip.Window == _HUD::WINDOW_SKILLBAR) {
				if(CheckScope(ScopeType::WORLD) && TargetID != TargetType::NONE)
					HelpTextList.push_back("Left-click to use");
			}
			else if(Tooltip.Window == _HUD::WINDOW_SKILLS) {
				if(Scope == ScopeType::NONE)
					HelpTextList.push_back("Passive skills must be equipped");
				if(SkillLevel > 0 && Player->Character->CanEquipSkill(this))
					HelpTextList.push_back("Right-click to equip");
			}
			else if(Tooltip.Window == _HUD::WINDOW_ENCHANTER) {
			}
			else {
				if(IsLocked) {
					if(Tooltip.Window == _HUD::WINDOW_INVENTORY && !(Player->Character->Vendor && Config.RightClickSell))
						HelpTextList.push_back("Right-click to learn");
				}
				else {
					InfoText = "Already learned";
					InfoColor = ae::Assets.Colors["red"];
				}
			}
		break;
		case ItemType::UNLOCKABLE: {
			bool Unlocked = Player->Character->HasUnlocked(this);
			if(!Unlocked && Tooltip.Window == _HUD::WINDOW_INVENTORY && !(Player->Character->Vendor && Config.RightClickSell))
				HelpTextList.push_back("Right-click to unlock");
			else if(Unlocked) {
				InfoText = "Already unlocked";
				InfoColor = ae::Assets.Colors["red"];
			}
		} break;
		case ItemType::KEY: {
			if(Player->Inventory->GetBag(BagType::KEYS).HasItemID(ID)) {
				InfoText = "Already in keychain";
				InfoColor = ae::Assets.Colors["red"];
			}
			else if(Tooltip.Window == _HUD::WINDOW_INVENTORY && !(Player->Character->Vendor && Config.RightClickSell)) {
				HelpTextList.push_back("Right-click to add to keychain");
			}
		} break;
		default:
		break;
	}

	if(InfoText.length()) {
		ae::Assets.Fonts["hud_small"]->DrawText(InfoText, glm::ivec2(DrawPosition), ae::CENTER_BASELINE, InfoColor);
		DrawPosition.y += ControlSpacingY;
	}

	// Move hint
	if(Player->Character->IsTrading() && Tradable && !(IsCursed() && Tooltip.Window == _HUD::WINDOW_EQUIPMENT))
		HelpTextList.push_back("Shift+click to move");

	// Split hint
	if(Tooltip.InventorySlot.Count > 1 && Tooltip.Window != _HUD::WINDOW_VENDOR)
		HelpTextList.push_back("Ctrl+click to split");

	// Set hint
	if(SetID || Script.find("SetBonus") == 0)
		HelpTextList.push_back("Hold Alt for more info");

	// Draw ui hints
	for(const auto &Text : HelpTextList) {
		ae::Assets.Fonts["hud_small"]->DrawText(Text, glm::ivec2(DrawPosition), ae::CENTER_BASELINE, ae::Assets.Colors["gray"]);
		DrawPosition.y += ControlSpacingY;
	}
}

// Draw item description
int _Item::DrawDescription(bool Render, _Object *Object, const std::string &Function, glm::vec2 &DrawPosition, bool Blacksmith, int DrawLevel, int PlayerMaxSkillLevel, int EnchanterMaxLevel, int Upgrades, bool ShowLevel, float Width, float SpacingY) const {
	_Scripting *Scripting = Object->Scripting;
	int LineCount = 0;

	// Check for scripting function
	std::string Info = "";
	if(Scripting->StartMethodCall(Function, "GetInfo")) {
		int SetLevel = 0;
		int MaxSetLevel = 0;
		if(SetID) {
			SetLevel = std::min(Object->Character->Sets[SetID].Level + Blacksmith, MaxLevel);
			MaxSetLevel = Object->Character->Sets[SetID].MaxLevel;
		}

		// Get description from script
		Scripting->PushObject(Object);
		Scripting->PushItemParameters(Chance, DrawLevel, Duration, Upgrades, SetLevel, MaxSetLevel, ae::Input.ModKeyDown(KMOD_ALT));
		Scripting->MethodCall(2, 1);
		Info = Scripting->GetString(1);
		Scripting->FinishMethodCall();

		// Draw level text
		if(ShowLevel) {
			std::string Text = "Level " + std::to_string(DrawLevel);
			glm::vec4 Color = ae::Assets.Colors["gray"];
			if(EnchanterMaxLevel && (DrawLevel > EnchanterMaxLevel || DrawLevel > MaxLevel)) {
				Text = "I can't upgrade this";
				Color = ae::Assets.Colors["red"];
			}
			else if(DrawLevel == PlayerMaxSkillLevel) {
				Text = "Max " + Text;
				Color = ae::Assets.Colors["red"];
			}
			else if(!EnchanterMaxLevel && DrawLevel >= PlayerMaxSkillLevel) {
				if(DrawLevel > MaxLevel)
					return LineCount;

				Text = "Enchanter required for level " + std::to_string(DrawLevel);
				Color = ae::Assets.Colors["red"];
			}
			if(Render) {
				ae::Assets.Fonts["hud_small"]->DrawText(Text, DrawPosition, ae::CENTER_BASELINE, Color);
				DrawPosition.y += SpacingY;
			}
		}

		std::stringstream Buffer(Info);
		std::string Token;

		// Draw description
		float TextSpacingY = INVENTORY_TOOLTIP_TEXT_SPACING * ae::_Element::GetUIScale();
		while(std::getline(Buffer, Token, '\n')) {
			std::list<std::string> Strings;
			ae::Assets.Fonts["hud_small"]->BreakupString(Token, Width, Strings, true);
			for(const auto &LineToken : Strings) {
				if(Render) {
					ae::Assets.Fonts["hud_small"]->DrawTextFormatted(LineToken, DrawPosition, ae::CENTER_BASELINE);
					DrawPosition.y += TextSpacingY;
				}
				LineCount++;
			}
		}

		if(Render)
			DrawPosition.y += SpacingY;

		LineCount++;
	}

	return LineCount;
}

// Draw set description
int _Item::DrawSetDescription(bool Render, _Object *Object, glm::vec2 &DrawPosition, bool Blacksmith, float Width, float SpacingY) const {
	if(!SetID)
		return 0;

	// Don't draw full set description for added bonus items
	if(Script.find("SetBonus") == 0)
		return 0;

	_Scripting *Scripting = Object->Scripting;
	const _Set &Set = Object->Stats->Sets.at(SetID);
	int LineCount = 0;

	// Check for scripting function
	std::string Info = "";
	if(Scripting->StartMethodCall(Set.Script, "GetSetInfo")) {
		int EquippedCount = Object->Character->Sets[SetID].EquippedCount;
		int Upgrades = std::min(Object->Character->Sets[SetID].Level + Blacksmith, MaxLevel);
		int MaxSetLevel = Object->Character->Sets[SetID].MaxLevel;
		bool MoreInfo = ae::Input.ModKeyDown(KMOD_ALT);

		// Get description from script
		Scripting->PushObject(Object);
		Scripting->PushInt(Upgrades);
		Scripting->PushInt(MaxSetLevel);
		Scripting->PushBoolean(MoreInfo);
		Scripting->MethodCall(4, 1);
		Info = Scripting->GetString(1);
		Scripting->FinishMethodCall();

		std::stringstream Buffer(Info);
		std::string Token;
		float TextSpacingY = INVENTORY_TOOLTIP_TEXT_SPACING * ae::_Element::GetUIScale();

		// Draw header
		if(Render) {
			std::string LevelText;
			if(MoreInfo)
				LevelText = " Level " + std::to_string(Upgrades);
			else
				LevelText = " (" + std::to_string(EquippedCount) + "/" + std::to_string(Set.Count) + ")";

			ae::Assets.Fonts["hud_small"]->DrawTextFormatted("[c light_green]" + Set.Name + " Set Bonus" + LevelText, DrawPosition, ae::CENTER_BASELINE);
			DrawPosition.y += TextSpacingY;
		}
		LineCount++;

		// Draw description
		while(std::getline(Buffer, Token, '\n')) {
			std::list<std::string> Strings;
			ae::Assets.Fonts["hud_small"]->BreakupString(Token, Width, Strings, true);
			for(const auto &LineToken : Strings) {
				if(Render) {
					ae::Assets.Fonts["hud_small"]->DrawTextFormatted(LineToken, DrawPosition, ae::CENTER_BASELINE);
					DrawPosition.y += TextSpacingY;
				}
				LineCount++;
			}
		}

		DrawPosition.y += SpacingY;
	}

	return LineCount;
}

// Get target count based on target type
int _Item::GetTargetCount(_Scripting *Scripting, _Object *Object, bool InitialTarget) const {

	switch(TargetID) {
		case TargetType::SELF:
		case TargetType::ALLY:
		case TargetType::ANY:
			return 1;
		break;
		case TargetType::ENEMY_MULTI:
		case TargetType::ALLY_MULTI:
		case TargetType::ENEMY_CORPSE_AOE: {

			if(InitialTarget && TargetID == TargetType::ENEMY_CORPSE_AOE)
				return 1;

			int TargetCount = 0;
			if(Scripting->StartMethodCall(Script, "GetTargetCount")) {
				int SkillLevel = 1;
				auto SkillIterator = Object->Character->Skills.find(ID);
				if(SkillIterator != Object->Character->Skills.end()) {
					SkillLevel = SkillIterator->second + Object->Character->Attributes["AllSkills"].Int;
					if(Object->Character->MaxSkillLevels.find(ID) != Object->Character->MaxSkillLevels.end())
						SkillLevel = std::min(SkillLevel, Object->Character->MaxSkillLevels.at(ID));
				}

				Scripting->PushInt(SkillLevel);
				Scripting->MethodCall(1, 1);
				TargetCount = Scripting->GetInt(1);
				Scripting->FinishMethodCall();
			}
			else
				TargetCount = BATTLE_MULTI_TARGET_COUNT;

			return TargetCount;
		} break;
		case TargetType::ENEMY_ALL:
		case TargetType::ALLY_ALL:
			return BATTLE_MAX_OBJECTS_PER_SIDE;
		break;
		default:
		break;
	}

	return 0;
}

// Return a valid equipment slot for an item
void _Item::GetEquipmentSlot(_Slot &Slot) const {

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
		case ItemType::OFFHAND:
			Slot.Index = EquipmentType::HAND2;
		break;
		default:
			Slot.Type = BagType::NONE;
		break;
	}
}

// Returns the item's price to/from a vendor
int64_t _Item::GetPrice(_Scripting *Scripting, _Object *Source, const _Vendor *Vendor, int QueryCount, bool Buy, int Level) const {

	// Calculate
	float Percent = 1.0f;
	if(Vendor) {
		if(Buy) {
			Percent = Vendor->BuyPercent;
		}
		else {
			Percent = Vendor->SellPercent;
			if(Category == 100)
				Percent = 0.0;
		}
	}

	// Check for GetCost function in script
	int64_t ItemCost = Cost;
	if(Scripting->StartMethodCall(Script, "GetCost")) {
		Scripting->PushObject(Source);
		Scripting->MethodCall(1, 1);
		ItemCost = Scripting->GetInt64(1);
		Scripting->FinishMethodCall();
	}

	// Get vendor's price
	int64_t Price = (int64_t)(ItemCost * Percent) * QueryCount;

	// Add some value for upgrades
	if(Level) {
		for(int i = 1; i <= Level; i++)
			Price += GetUpgradeCost(i) * Percent;
	}

	// Cap
	if(Price < 0)
		Price = 0;
	else if(Price > PLAYER_MAX_GOLD)
		Price = PLAYER_MAX_GOLD;

	return Price;
}

// Get upgrade cost
int64_t _Item::GetUpgradeCost(int Level) const {
	if(MaxLevel <= 0)
		return 0;

	return (int64_t)(std::floor(GAME_UPGRADE_COST_MULTIPLIER * Level * Cost + GAME_UPGRADE_BASE_COST));
}

// Get enchant cost
int64_t _Item::GetEnchantCost(int Level) {
	int64_t Index = Level - GAME_DEFAULT_MAX_SKILL_LEVEL;
	return std::floor(std::pow(Index, GAME_ENCHANT_COST_POWER) + Index * GAME_ENCHANT_COST_RATE + GAME_ENCHANT_COST_BASE);
}

// Get count of drawable attributes
int _Item::GetAttributeCount(int Upgrades) const {
	int Count = 0;

	if(std::floor(GetAttribute("MinDamage", Upgrades)) != 0 || std::floor(GetAttribute("MaxDamage", Upgrades)) != 0)
		Count++;

	if(!IsSkill() && DamageTypeID > 1)
		Count++;

	for(const auto &AttributeName : Stats->AttributeRank) {
		if(HiddenStats.find(AttributeName) != HiddenStats.end())
			continue;

		if(Attributes.find(AttributeName) == Attributes.end())
			continue;

		float UpgradedValue = GetAttribute(AttributeName, Upgrades);
		if(UpgradedValue == 0.0f)
			continue;

		Count++;
	}

	if(IsEquippable() && GetCooldownReduction(Upgrades) != 0.0f)
		Count++;

	if(ResistanceTypeID)
		Count++;

	return Count;
}

// Return true if the item can be used
bool _Item::CanUse(_Scripting *Scripting, _ActionResult &ActionResult) const {
	_Object *Object = ActionResult.Source.Object;
	if(!Object)
		return false;

	// Check cooldown
	if(Object->Character->Cooldowns.find(ActionResult.ActionUsed.Item->ID) != Object->Character->Cooldowns.end())
		return false;

	// Unlocking skill for the first time
	if(IsSkill() && ActionResult.ActionUsed.InventorySlot != -1)
		return !Object->Character->HasLearned(this);

	// Check for item in key bag
	if(IsKey())
		return !Object->Inventory->GetBag(BagType::KEYS).HasItemID(ID);

	// Unlocking item
	if(IsUnlockable())
		return !Object->Character->HasUnlocked(this);

	// Check for item count
	if(!ActionResult.ActionUsed.Item->IsSkill()) {
		std::size_t Index;
		if(!Object->Inventory->FindItem(ActionResult.ActionUsed.Item, Index, (std::size_t)ActionResult.ActionUsed.InventorySlot))
			return false;
	}

	// Check scope
	if(!CheckScope(ActionResult.Scope))
		return false;

	// Check targets
	_Object *FirstTarget = nullptr;
	if(Object->Character->Targets.size()) {

		// Check each target and see if action can be used
		bool IsFirstTarget = true;
		for(auto Iterator = Object->Character->Targets.begin(); Iterator != Object->Character->Targets.end(); ) {
			_Object *Target = *Iterator;

			// Determine whether to force target alive based on target type
			bool ForceTargetAlive = false;
			if(ActionResult.ActionUsed.Item->TargetID == TargetType::ENEMY_CORPSE_AOE && !IsFirstTarget)
				ForceTargetAlive = true;

			// Remove bad targets
			if(!CanTarget(Scripting, Object, Target, ForceTargetAlive))
				Iterator = Object->Character->Targets.erase(Iterator);
			else
				++Iterator;

			IsFirstTarget = false;
		}

		// No targets left
		if(Object->Character->Targets.empty())
			return false;

		// Get first target
		FirstTarget = *Object->Character->Targets.begin();
	}

	// Check script's function
	if(Scripting->StartMethodCall(Script, "CanUse")) {
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->PushObject(FirstTarget);
		Scripting->MethodCall(3, 1);
		int Value = Scripting->GetBoolean(1);
		Scripting->FinishMethodCall();

		return Value;
	}

	return true;
}

// Check if an item can target an object
bool _Item::CanTarget(_Scripting *Scripting, _Object *Source, _Object *Target, bool ForceTargetAlive) const {
	bool CurrentTargetAlive = TargetAlive;
	if(ForceTargetAlive)
		CurrentTargetAlive = true;

	if(CurrentTargetAlive && !Target->Character->IsAlive())
		return false;

	if(!CurrentTargetAlive && Target->Character->IsAlive())
		return false;

	if(Source->Character->Battle) {
		if(Source->Fighter->BattleSide == Target->Fighter->BattleSide && !CanTargetAlly())
			return false;

		if(Source->Fighter->BattleSide != Target->Fighter->BattleSide && !CanTargetEnemy())
			return false;
	}

	// Check script's function
	if(Scripting->StartMethodCall(Script, "CanTarget")) {
		Scripting->PushObject(Source);
		Scripting->PushObject(Target);
		Scripting->PushBoolean(CurrentTargetAlive);
		Scripting->MethodCall(3, 1);
		int Value = Scripting->GetBoolean(1);
		Scripting->FinishMethodCall();

		return Value;
	}

	return true;
}

// Check if the item can be used in the given scope
bool _Item::CheckScope(ScopeType CheckScope) const {
	if(Scope == ScopeType::NONE || (Scope != ScopeType::ALL && Scope != CheckScope))
		return false;

	return true;
}

// Apply the cost
void _Item::ApplyCost(_Scripting *Scripting, _ActionResult &ActionResult) const {
	if(Scripting->StartMethodCall(Script, "ApplyCost")) {
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushActionResult(&ActionResult);
		Scripting->MethodCall(3, 1);
		Scripting->GetActionResult(1, ActionResult);
		Scripting->FinishMethodCall();
	}
}

// Use an item
void _Item::Use(_Scripting *Scripting, _ActionResult &ActionResult) const {
	if(Scripting->StartMethodCall(Script, "Use")) {
		Scripting->PushInt(ActionResult.ActionUsed.Level);
		Scripting->PushInt(ActionResult.ActionUsed.Duration);
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->PushObject(ActionResult.Target.Object);
		Scripting->PushActionResult(&ActionResult);
		Scripting->MethodCall(5, 1);
		Scripting->GetActionResult(1, ActionResult);
		Scripting->FinishMethodCall();
	}
}

// Get passive stats
void _Item::GetStats(_Scripting *Scripting, _ActionResult &ActionResult, int SetLevel, int MaxSetLevel) const {
	if(Scripting->StartMethodCall(Script, "Stats")) {
		if(IsSkill())
			Scripting->PushInt(ActionResult.ActionUsed.Level);
		else
			Scripting->PushItemParameters(Chance, Level, Duration, ActionResult.ActionUsed.Level, SetLevel, MaxSetLevel, 0);
		Scripting->PushObject(ActionResult.Source.Object);
		Scripting->PushStatChange(&ActionResult.Source);
		Scripting->MethodCall(3, 1);
		Scripting->GetStatChange(1, ActionResult.Source.Object->Stats, ActionResult.Source);
		Scripting->FinishMethodCall();
	}
}

// Play audio through scripting
void _Item::PlaySound(_Scripting *Scripting) const {
	if(Scripting->StartMethodCall(Script, "PlaySound")) {
		Scripting->MethodCall(0, 0);
		Scripting->FinishMethodCall();
	}
}

// Get upgraded attribute
float _Item::GetAttribute(const std::string &Name, int Upgrades) const {
	auto AttributeIterator = Attributes.find(Name);
	if(AttributeIterator == Attributes.end())
		return 0.0f;

	return GetUpgradedValue<float>(Name, Upgrades, AttributeIterator->second.Int);
}

// Get average damage
float _Item::GetAverageDamage(int Upgrades) const {
	return (GetAttribute("MinDamage", Upgrades) + GetAttribute("MaxDamage", Upgrades)) * 0.5f;
}

// Get cooldown reduction
float _Item::GetCooldownReduction(int Upgrades) const {
	return -GetUpgradedValue<float>("Cooldowns", Upgrades, -Cooldown);
}

// Get appropriate text color when comparing items
glm::vec4 _Item::GetCompareColor(float ItemValue, float EquippedValue) const {
	if(ItemValue > EquippedValue)
		return ae::Assets.Colors["green"];
	else if(ItemValue < EquippedValue)
		return ae::Assets.Colors["red"];

	return glm::vec4(1.0f);
}

// Return value of a stat after upgrades
template<typename T> T _Item::GetUpgradedValue(const std::string &AttributeName, int Upgrades, T Value) const {
	if(MaxLevel <= 0)
		return Value;

	float UpgradedValue = Stats->Attributes.at(AttributeName).UpgradeScale * GAME_UPGRADE_AMOUNT * Upgrades * std::abs(Value);
	if(Value < 0)
		return std::min(0.0f, Value + (T)(GAME_NEGATIVE_UPGRADE_SCALE * UpgradedValue));
	else
		return Value + (T)(UpgradedValue);
}
