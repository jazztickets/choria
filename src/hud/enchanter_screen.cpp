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
#include <hud/enchanter_screen.h>
#include <hud/hud.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <objects/components/controller.h>
#include <states/play.h>
#include <ae/clientnetwork.h>
#include <ae/ui.h>
#include <ae/assets.h>
#include <ae/font.h>
#include <ae/input.h>
#include <ae/database.h>
#include <ae/buffer.h>
#include <packet.h>
#include <stats.h>
#include <glm/vec2.hpp>
#include <sstream>

// Constructor
_EnchanterScreen::_EnchanterScreen(_HUD *HUD, ae::_Element *Element) :
	_Screen(HUD, Element) {
}

// Initialize
void _EnchanterScreen::Init() {

	// Clear old children
	ClearSkills();

	glm::vec2 Start(14, 36);
	glm::vec2 Offset(Start);
	glm::vec2 LevelOffset(0, -6);
	glm::vec2 Spacing(14, 70);
	glm::vec2 PlusOffset(0, 64 + 6);
	glm::vec2 LabelOffset(0, 2);
	glm::vec2 ButtonSize(64, 30);
	size_t i = 0;

	// Get all player skills
	std::list<const _Item *> SortedSkills;
	for(auto &SkillID : HUD->Player->Character->Skills) {
		const _Item *Skill = PlayState.Stats->Items.at(SkillID.first);
		if(!Skill)
			continue;

		SortedSkills.push_back(Skill);
	}

	// Sort skills
	SortedSkills.sort(CompareItems);

	// Iterate over skills
	for(auto &Skill : SortedSkills) {

		// Add skill icon
		ae::_Element *Button = new ae::_Element();
		Button->Name = "button_max_skill_levels_skill";
		Button->Parent = Element;
		Button->BaseOffset = Offset;
		Button->BaseSize = Skill->Texture->Size;
		Button->Alignment = ae::LEFT_TOP;
		Button->Texture = Skill->Texture;
		Button->Index = (int)Skill->ID;
		Button->Clickable = true;
		Element->Children.push_back(Button);

		// Add level label
		ae::_Element *LevelLabel = new ae::_Element();
		LevelLabel->Name = "label_max_skill_levels_level";
		LevelLabel->Parent = Button;
		LevelLabel->BaseOffset = LevelOffset;
		LevelLabel->Alignment = ae::CENTER_BASELINE;
		LevelLabel->Font = ae::Assets.Fonts["hud_small"];
		LevelLabel->Index = (int)Skill->ID;
		Element->Children.push_back(LevelLabel);

		// Add buy button
		ae::_Element *BuyButton = new ae::_Element();
		BuyButton->Name = "button_max_skill_levels_buy";
		BuyButton->Parent = Element;
		BuyButton->BaseSize = ButtonSize;
		BuyButton->BaseOffset = Offset + PlusOffset;
		BuyButton->Alignment = ae::LEFT_TOP;
		BuyButton->Style = ae::Assets.Styles["style_menu_button"];
		BuyButton->HoverStyle = ae::Assets.Styles["style_menu_button_hover"];
		BuyButton->DisabledStyle = ae::Assets.Styles["style_menu_button_disabled"];
		BuyButton->Index = (int)Skill->ID;
		BuyButton->Clickable = true;
		Element->Children.push_back(BuyButton);

		// Add buy label
		ae::_Element *PlusLabel = new ae::_Element();
		PlusLabel->Parent = BuyButton;
		PlusLabel->Text = "Buy";
		PlusLabel->BaseOffset = LabelOffset;
		PlusLabel->Alignment = ae::CENTER_MIDDLE;
		PlusLabel->Font = ae::Assets.Fonts["hud_tiny"];
		PlusLabel->Color = ae::Assets.Colors["gold"];
		BuyButton->Children.push_back(PlusLabel);

		// Update position
		Offset.x += Skill->Texture->Size.x + Spacing.x;
		if(Offset.x > Element->BaseSize.x - Skill->Texture->Size.x) {
			Offset.y += Skill->Texture->Size.y + Spacing.y;
			Offset.x = Start.x;
		}

		i++;
	}
	PlayState.Stats->Database->CloseQuery();

	Element->CalculateBounds();
	Element->SetActive(true);

	RefreshBuyButtons();
	HUD->Cursor.Reset();

	PlayState.SendStatus(_Character::STATUS_SKILLS);
}

// Close screen
bool _EnchanterScreen::Close(bool SendNotify) {
	bool WasOpen = Element->Active;

	Element->SetActive(false);
	HUD->Cursor.Reset();

	if(HUD->Player)
		HUD->Player->Character->Enchanter = nullptr;

	return WasOpen;
}

// Toggle display
void _EnchanterScreen::Toggle() {
	if(HUD->Player->Controller->WaitForServer || !HUD->Player->Character->CanOpenInventory())
		return;

	if(!Element->Active) {
		HUD->CloseWindows(true);
		Init();
	}
	else {
		HUD->CloseWindows(true);
	}
}

// Render
void _EnchanterScreen::Render(double BlendFactor) {
	if(!Element->Active)
		return;

	Element->Render();

	const _Enchanter *Enchanter = HUD->Player->Character->Enchanter;

	// Show help text
	std::string Text = Enchanter->Name + " increases max skill levels to " + std::to_string(Enchanter->Level);
	glm::vec2 DrawPosition = glm::vec2((Element->Bounds.End.x + Element->Bounds.Start.x) / 2, Element->Bounds.End.y - 38 * ae::_Element::GetUIScale());
	ae::Assets.Fonts["hud_medium"]->DrawText(Text, DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["gray"]);
}

// Delete memory used by skill page
void _EnchanterScreen::ClearSkills() {

	// Delete children
	for(auto &Child : Element->Children)
		delete Child;

	Element->Children.clear();
}

// Enable or disable buy button
void _EnchanterScreen::RefreshBuyButtons() {
	if(!HUD->Player)
		return;

	const _Enchanter *Enchanter = HUD->Player->Character->Enchanter;
	if(!Enchanter)
		return;

	// Loop through buttons
	for(auto &ChildElement : Element->Children) {
		uint32_t SkillID = (uint32_t)ChildElement->Index;
		const _Item *Skill = HUD->Player->Stats->Items.at(SkillID);
		if(ChildElement->Name == "label_max_skill_levels_level") {
			int DrawLevel = HUD->Player->Character->MaxSkillLevels[SkillID];
			glm::vec4 LevelColor = (DrawLevel >= Enchanter->Level || DrawLevel == Skill->MaxLevel) ? ae::Assets.Colors["red"] : glm::vec4(1.0f);

			ChildElement->Text = std::to_string(DrawLevel);
			ChildElement->Color = LevelColor;
		}
		else if(ChildElement->Name == "button_max_skill_levels_buy") {
			int CurrentLevel = HUD->Player->Character->MaxSkillLevels[SkillID];
			int Cost = _Item::GetEnchantCost(CurrentLevel);

			// Set button state
			if(CurrentLevel >= Skill->MaxLevel || CurrentLevel >= Enchanter->Level || Cost > HUD->Player->Character->Attributes["Gold"].Integer) {
				ChildElement->SetEnabled(false);
				ChildElement->Children.front()->Color = ae::Assets.Colors["gray"];
			}
			else {
				ChildElement->SetEnabled(true);
				ChildElement->Children.front()->Color = ae::Assets.Colors["gold"];
			}
		}
	}
}
