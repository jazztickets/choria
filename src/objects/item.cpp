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

const std::string SkillCategories[4] = {
	"Passive Skill",
	"Attack Skill",
	"Spell",
	"Skill",
};

// Draw tooltip
void _Item::DrawTooltip(const glm::vec2 &Position, _Scripting *Scripting, _Object *Player, const _Cursor &Tooltip, const _Slot &CompareSlot) const {
	if(!Player)
		return;

	ae::_Element *TooltipElement = ae::Assets.Elements["element_item_tooltip"];
	ae::_Element *TooltipName = ae::Assets.Elements["label_item_tooltip_name"];
	ae::_Element *TooltipType = ae::Assets.Elements["label_item_tooltip_type"];
	TooltipElement->SetActive(true);

	// Set label values
	TooltipName->Text = Name;
	TooltipType->Text = "";
	TooltipType->Color = glm::vec4(1.0f);
	if(Category && Category <= 4)
		TooltipType->Text = SkillCategories[Category-1];
	else if(Type != ItemType::NONE) {
		TooltipType->Text = Player->Stats->ItemTypes.at((uint32_t)Type);
		if(Type == ItemType::CONSUMABLE && Scope == ScopeType::BATTLE) {
			TooltipType->Text = "Battle " + TooltipType->Text;
		}
		else if(Cursed) {
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
	if(ResistanceTypeID)
		Size.x += 36 * ae::_Element::GetUIScale();
	else if(IsSkill())
		Size.x += 42 * ae::_Element::GetUIScale();

	// Set window height
	Size.y = INVENTORY_TOOLTIP_HEIGHT * ae::_Element::GetUIScale();
	if(Player->Character->Vendor)
		Size.y += LargeSpacingY;
	if(Cursed)
		Size.y += SpacingY;
	if(Player->Character->IsTrading())
		Size.y += SpacingY;
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
		Buffer << std::fixed << std::setprecision(1) << Cooldown * Player->Character->CooldownMultiplier << " second cooldown";
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["red"]);
		DrawPosition.y += LargeSpacingY;
	}

	// Get level of item or skill
	int DrawLevel = Level;
	int PlayerMaxSkillLevel = 0;
	int EnchanterMaxLevel = 0;
	bool IsLocked = false;
	bool ShowLevel = false;
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
				DrawLevel = SkillIterator->second;
				if(!ae::Input.ModKeyDown(KMOD_ALT) && SkillIterator->second > 0) {
					DrawLevel += Player->Character->AllSkills;
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
	bool Blacksmith = (Tooltip.Window == _HUD::WINDOW_BLACKSMITH);
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
	int DrawMinDamage = (int)GetMinDamage(Upgrades);
	int DrawMaxDamage = (int)GetMaxDamage(Upgrades);
	if(DrawMinDamage != 0 || DrawMaxDamage != 0) {
		std::stringstream Buffer;
		if(ae::Input.ModKeyDown(KMOD_ALT))
			Buffer << ae::Round((DrawMinDamage + DrawMaxDamage) / 2.0f);
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
	if(DrawMaxHealth != 0) {
		std::stringstream Buffer;
		Buffer << (DrawMaxHealth < 0 ? "" : "+") << DrawMaxHealth;

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetMaxHealth(Upgrades), CompareInventory.Item->GetMaxHealth(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Max Health", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Max mana
	int DrawMaxMana = (int)GetMaxMana(Upgrades);
	if(DrawMaxMana != 0) {
		std::stringstream Buffer;
		Buffer << (DrawMaxMana < 0 ? "" : "+") << DrawMaxMana;

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetMaxMana(Upgrades), CompareInventory.Item->GetMaxMana(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Max Mana", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
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

		ae::Assets.Fonts["hud_medium"]->DrawText(Player->Stats->DamageTypes.at(ResistanceTypeID).Name + " Resist", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Move speed
	int DrawMoveSpeed = (int)GetMoveSpeed(Upgrades);
	if(DrawMoveSpeed != 0) {
		std::stringstream Buffer;
		Buffer << (DrawMoveSpeed < 0 ? "" : "+") << DrawMoveSpeed << "%";

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetMoveSpeed(Upgrades), CompareInventory.Item->GetMoveSpeed(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Move Speed", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Battle speed
	int DrawBattleSpeed = (int)GetBattleSpeed(Upgrades);
	if(DrawBattleSpeed != 0) {
		std::stringstream Buffer;
		Buffer << (DrawBattleSpeed < 0 ? "" : "+") << DrawBattleSpeed << "%";

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetBattleSpeed(Upgrades), CompareInventory.Item->GetBattleSpeed(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Battle Speed", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Evasion
	int DrawEvasion = (int)GetEvasion(Upgrades);
	if(DrawEvasion != 0) {
		std::stringstream Buffer;
		Buffer << (DrawEvasion < 0 ? "" : "+") << DrawEvasion << "%";

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetEvasion(Upgrades), CompareInventory.Item->GetEvasion(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Evasion", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
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

	// Gold bonus
	int DrawGoldBonus = (int)GetGoldBonus(Upgrades);
	if(DrawGoldBonus != 0) {
		std::stringstream Buffer;
		Buffer << (DrawGoldBonus < 0 ? "" : "+") << DrawGoldBonus << "%";

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetGoldBonus(Upgrades), CompareInventory.Item->GetGoldBonus(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Gold Bonus", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Experience bonus
	int DrawExpBonus = (int)GetExpBonus(Upgrades);
	if(DrawExpBonus != 0) {
		std::stringstream Buffer;
		Buffer << (DrawExpBonus < 0 ? "" : "+") << DrawExpBonus << "%";

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetExpBonus(Upgrades), CompareInventory.Item->GetExpBonus(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("XP Bonus", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// Cooldown reduction
	int DrawCooldownReduction = (int)GetCooldownReduction(Upgrades);
	if(IsEquippable() && DrawCooldownReduction != 0) {
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

	// Spell Damage
	int DrawSpellDamage = (int)GetSpellDamage(Upgrades);
	if(DrawSpellDamage != 0) {
		std::stringstream Buffer;
		Buffer << (DrawSpellDamage < 0 ? "" : "+") << DrawSpellDamage << "%";

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetSpellDamage(Upgrades), CompareInventory.Item->GetSpellDamage(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("Spell Damage", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
		ae::Assets.Fonts["hud_medium"]->DrawText(Buffer.str(), glm::ivec2(DrawPosition + Spacing), ae::LEFT_BASELINE, Color);
		DrawPosition.y += SpacingY;
		StatDrawn = true;
	}

	// + all skills
	int DrawAllSkills = (int)GetAllSkills(Upgrades);
	if(DrawAllSkills != 0) {
		std::stringstream Buffer;
		Buffer << (DrawAllSkills < 0 ? "" : "+") << DrawAllSkills;

		glm::vec4 Color(1.0f);
		if(CompareInventory.Item)
			Color = GetCompareColor(GetAllSkills(Upgrades), CompareInventory.Item->GetAllSkills(CompareInventory.Upgrades));

		ae::Assets.Fonts["hud_medium"]->DrawText("All Skills", glm::ivec2(DrawPosition + -Spacing), ae::RIGHT_BASELINE);
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
			if(Cursed) {
				InfoText = "Cursed items cannot be unequipped";
				InfoColor = ae::Assets.Colors["red"];
			}
			else if(Tooltip.Window == _HUD::WINDOW_INVENTORY && Tooltip.Slot.Type == BagType::INVENTORY && !(Player->Character->Vendor && Config.RightClickSell))
				HelpTextList.push_back("Right-click to equip");
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
	if(Player->Character->IsTrading() && Tradable)
		HelpTextList.push_back("Shift+click to move");

	// Split hint
	if(Tooltip.InventorySlot.Count > 1)
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

		// Get description from script
		Scripting->PushObject(Object);
		Scripting->PushInt(Upgrades);
		Scripting->PushInt(MaxSetLevel);
		Scripting->PushBoolean(ae::Input.ModKeyDown(KMOD_ALT));
		Scripting->MethodCall(4, 1);
		Info = Scripting->GetString(1);
		Scripting->FinishMethodCall();

		std::stringstream Buffer(Info);
		std::string Token;
		float TextSpacingY = INVENTORY_TOOLTIP_TEXT_SPACING * ae::_Element::GetUIScale();

		// Draw header
		if(Render) {
			ae::Assets.Fonts["hud_small"]->DrawTextFormatted("[c light_green]" + Set.Name + " Set Bonus" + " (" + std::to_string(EquippedCount) + "/" + std::to_string(Set.Count) + ")", DrawPosition, ae::CENTER_BASELINE);
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
					SkillLevel = SkillIterator->second + Object->Character->AllSkills;
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
int _Item::GetPrice(_Scripting *Scripting, _Object *Source, const _Vendor *Vendor, int QueryCount, bool Buy, int Level) const {
	if(!Vendor)
		return 0;

	// Calculate
	float Percent;
	if(Buy)
		Percent = Vendor->BuyPercent;
	else {
		Percent = Vendor->SellPercent;
		if(Category == 5)
			Percent = 0.0;
	}

	// Check for GetCost function in script
	int ItemCost = Cost;
	if(Scripting->StartMethodCall(Script, "GetCost")) {
		Scripting->PushObject(Source);
		Scripting->MethodCall(1, 1);
		ItemCost = Scripting->GetInt(1);
		Scripting->FinishMethodCall();
	}

	// Get vendor's price
	int Price = (int)(ItemCost * Percent) * QueryCount;

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
int _Item::GetUpgradeCost(int Level) const {
	if(MaxLevel <= 0)
		return 0;

	return (int)(std::floor(GAME_UPGRADE_COST_MULTIPLIER * Level * Cost + GAME_UPGRADE_BASE_COST));
}

// Get enchant cost
int _Item::GetEnchantCost(int Level) {
	int Index = Level - GAME_DEFAULT_MAX_SKILL_LEVEL;
	return std::floor(std::pow(Index, GAME_ENCHANT_COST_POWER) + Index * GAME_ENCHANT_COST_RATE + GAME_ENCHANT_COST_BASE);
}

// Get count of drawable attributes
int _Item::GetAttributeCount(int Upgrades) const {
	int Count = 0;

	if((int)GetMinDamage(Upgrades) != 0 || (int)GetMaxDamage(Upgrades))
		Count++;
	if(!IsSkill() && DamageTypeID > 1)
		Count++;
	if((int)GetPierce(Upgrades) != 0)
		Count++;
	if((int)GetArmor(Upgrades) != 0)
		Count++;
	if((int)GetDamageBlock(Upgrades) != 0)
		Count++;
	if((int)GetMaxHealth(Upgrades) != 0)
		Count++;
	if((int)GetMaxMana(Upgrades) != 0)
		Count++;
	if(ResistanceTypeID)
		Count++;
	if((int)GetMoveSpeed(Upgrades) != 0)
		Count++;
	if((int)GetBattleSpeed(Upgrades) != 0)
		Count++;
	if((int)GetEvasion(Upgrades) != 0)
		Count++;
	if((int)GetHealthRegen(Upgrades) != 0)
		Count++;
	if((int)GetManaRegen(Upgrades) != 0)
		Count++;
	if((int)GetGoldBonus(Upgrades) != 0)
		Count++;
	if((int)GetExpBonus(Upgrades) != 0)
		Count++;
	if(IsEquippable() && (int)GetCooldownReduction(Upgrades) != 0)
		Count++;
	if((int)GetSpellDamage(Upgrades) != 0)
		Count++;
	if((int)GetAllSkills(Upgrades) != 0)
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
		size_t Index;
		if(!Object->Inventory->FindItem(ActionResult.ActionUsed.Item, Index, (size_t)ActionResult.ActionUsed.InventorySlot))
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
		Scripting->GetStatChange(1, ActionResult.Source);
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

// Get average damage
float _Item::GetAverageDamage(int Upgrades) const {
	return (GetUpgradedValue<float>(StatType::MINDAMAGE, Upgrades, MinDamage) + GetUpgradedValue<float>(StatType::MAXDAMAGE, Upgrades, MaxDamage)) / 2.0f;
}

// Get min damage
float _Item::GetMinDamage(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::MINDAMAGE, Upgrades, MinDamage);
}

// Get max damage
float _Item::GetMaxDamage(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::MAXDAMAGE, Upgrades, MaxDamage);
}

// Get armor
float _Item::GetArmor(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::ARMOR, Upgrades, Armor);
}

// Get damage block
float _Item::GetDamageBlock(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::DAMAGEBLOCK, Upgrades, DamageBlock);
}

// Get pierce
float _Item::GetPierce(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::PIERCE, Upgrades, Pierce);
}

// Get max health
float _Item::GetMaxHealth(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::MAXHEALTH, Upgrades, MaxHealth);
}

// Get max mana
float _Item::GetMaxMana(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::MAXMANA, Upgrades, MaxMana);
}

// Get health regen
float _Item::GetHealthRegen(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::HEALTHREGEN, Upgrades, HealthRegen);
}

// Get mana regen
float _Item::GetManaRegen(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::MANAREGEN, Upgrades, ManaRegen);
}

// Get battle speed
float _Item::GetBattleSpeed(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::BATTLESPEED, Upgrades, BattleSpeed);
}

// Get move speed
float _Item::GetMoveSpeed(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::MOVESPEED, Upgrades, MoveSpeed);
}

// Get evasion
float _Item::GetEvasion(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::EVASION, Upgrades, Evasion);
}

// Get spell damage
float _Item::GetSpellDamage(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::SPELL_DAMAGE, Upgrades, SpellDamage);
}

// Get resistance
float _Item::GetResistance(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::RESIST, Upgrades, Resistance);
}

// Get gold bonus
float _Item::GetGoldBonus(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::GOLD_BONUS, Upgrades, GoldBonus);
}

// Get experience bonus
float _Item::GetExpBonus(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::EXP_BONUS, Upgrades, ExpBonus);
}

// Get cooldown reduction
float _Item::GetCooldownReduction(int Upgrades) const {
	return -GetUpgradedValue<float>(StatType::COOLDOWN, Upgrades, -Cooldown);
}

// Get + all skills
float _Item::GetAllSkills(int Upgrades) const {
	return GetUpgradedValue<float>(StatType::ALLSKILLS, Upgrades, AllSkills);
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
template<typename T> T _Item::GetUpgradedValue(StatType Type, int Upgrades, T Value) const {
	if(MaxLevel <= 0)
		return Value;

	if(Value < 0)
		return std::min(0.0f, Value + (T)(GAME_NEGATIVE_UPGRADE_SCALE * GAME_UPGRADE_AMOUNT * Stats->UpgradeScale.at(Type) * Upgrades * std::abs(Value)));
	else
		return Value + (T)(GAME_UPGRADE_AMOUNT * Stats->UpgradeScale.at(Type) * Upgrades * std::abs(Value));
}
