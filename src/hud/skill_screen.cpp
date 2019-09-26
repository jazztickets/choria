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
#include <hud/skill_screen.h>
#include <hud/character_screen.h>
#include <hud/hud.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <objects/components/controller.h>
#include <states/play.h>
#include <ae/clientnetwork.h>
#include <ae/ui.h>
#include <ae/assets.h>
#include <ae/font.h>
#include <ae/database.h>
#include <ae/buffer.h>
#include <packet.h>
#include <stats.h>
#include <glm/vec2.hpp>
#include <sstream>

// Constructor
_SkillScreen::_SkillScreen(_HUD *MainHUD, ae::_Element *MainElement) :
	_Screen(MainHUD, MainElement) {
}

// Initialize
void _SkillScreen::Init() {

	// Clear old children
	ClearSkills();

	glm::vec2 Start(14, 36);
	glm::vec2 Offset(Start);
	glm::vec2 LevelOffset(0, -6);
	glm::vec2 Spacing(14, 70);
	glm::vec2 PlusOffset(-16, 52);
	glm::vec2 MinusOffset(16, 52);
	glm::vec2 LabelOffset(0, 2);
	glm::vec2 ButtonSize(23, 23);
	size_t i = 0;

	// Get all player skills
	std::list<const _BaseItem *> SortedSkills;
	for(auto &SkillID : HUD->Player->Character->Skills) {
		const _BaseItem *Skill = PlayState.Stats->ItemsIndex.at(SkillID.first);
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
		Button->Name = "button_skills_skill";
		Button->Parent = Element;
		Button->BaseOffset = Offset;
		Button->BaseSize = Skill->Texture->Size;
		Button->Alignment = ae::LEFT_TOP;
		Button->Texture = Skill->Texture;
		Button->Index = (int)Skill->NetworkID;
		Button->Clickable = true;
		Element->Children.push_back(Button);

		// Add level label
		ae::_Element *LevelLabel = new ae::_Element();
		LevelLabel->Name = "label_skills_level";
		LevelLabel->Parent = Button;
		LevelLabel->BaseOffset = LevelOffset;
		LevelLabel->Alignment = ae::CENTER_BASELINE;
		LevelLabel->Font = ae::Assets.Fonts["hud_small"];
		LevelLabel->Index = (int)Skill->NetworkID;
		Element->Children.push_back(LevelLabel);

		// Update position
		Offset.x += Skill->Texture->Size.x + Spacing.x;
		if(Offset.x > Element->BaseSize.x - Skill->Texture->Size.x) {
			Offset.y += Skill->Texture->Size.y + Spacing.y;
			Offset.x = Start.x;
		}

		i++;
	}

	Element->CalculateBounds();
	Element->SetActive(true);
	HUD->CharacterScreen->Element->SetActive(true);

	RefreshSkillButtons();
	HUD->Cursor.Reset();

	PlayState.SendStatus(_Character::STATUS_SKILLS);
}

// Close screen
bool _SkillScreen::Close(bool SendNotify) {
	bool WasOpen = Element->Active;

	Element->SetActive(false);
	HUD->Cursor.Reset();

	return WasOpen;
}

// Toggle display
void _SkillScreen::Toggle() {
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
void _SkillScreen::Render(double BlendFactor) {
	if(!Element->Active)
		return;

	Element->Render();
}

// Delete memory used by skill page
void _SkillScreen::ClearSkills() {

	// Delete children
	for(auto &Child : Element->Children)
		delete Child;

	Element->Children.clear();
}

// Shows or hides the plus/minus buttons
void _SkillScreen::RefreshSkillButtons() {

	// Loop through buttons
	for(auto &ChildElement : Element->Children) {
		if(ChildElement->Name == "label_skills_level") {
			uint32_t SkillID = (uint32_t)ChildElement->Index;
			ChildElement->Text = std::to_string(HUD->Player->Character->Skills[SkillID]);
		}
		else if(ChildElement->Name == "button_skills_plus") {

			// Get skill
			uint32_t SkillID = (uint32_t)ChildElement->Parent->Index;
			if(HUD->Player->Character->Skills[SkillID] >= HUD->Player->Stats->ItemsIndex.at(SkillID)->MaxLevel)
				ChildElement->SetActive(false);
			else
				ChildElement->SetActive(true);
		}
		else if(ChildElement->Name == "button_skills_minus") {

			// Get skill
			uint32_t SkillID = (uint32_t)ChildElement->Parent->Index;
			if(HUD->Player->Character->Skills[SkillID] == 0)
				ChildElement->SetActive(false);
			else
				ChildElement->SetActive(true);
		}
	}
}

// Equip a skill
void _SkillScreen::EquipSkill(uint32_t SkillID) {
	const _BaseItem *Skill = PlayState.Stats->ItemsIndex.at(SkillID);
	if(Skill) {

		// Check skill
		if(!HUD->Player->Character->HasLearned(Skill))
			return;

		if(!HUD->Player->Character->Skills[SkillID])
			return;

		// Find existing action
		for(size_t i = 0; i < HUD->Player->Character->ActionBar.size(); i++) {
			if(HUD->Player->Character->ActionBar[i].Item == Skill)
				return;
		}

		// Find an empty slot
		for(size_t i = 0; i < HUD->Player->Character->ActionBar.size(); i++) {
			if(!HUD->Player->Character->ActionBar[i].Item) {
				HUD->SetActionBar(i, HUD->Player->Character->ActionBar.size(), Skill);
				return;
			}
		}
	}
}
