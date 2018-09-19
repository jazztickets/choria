/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2018  Alan Witkowski
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
_SkillScreen::_SkillScreen(_HUD *HUD, ae::_Element *Element) :
	_Screen(HUD, Element) {
}

// Initialize
void _SkillScreen::Init() {

	// Clear old children
	ClearSkills();

	glm::vec2 Start(10, 25);
	glm::vec2 Offset(Start);
	glm::vec2 LevelOffset(0, -4);
	glm::vec2 Spacing(10, 50);
	glm::vec2 PlusOffset(-12, 37);
	glm::vec2 MinusOffset(12, 37);
	glm::vec2 LabelOffset(0, 2);
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
		Button->Name = "button_skills_skill";
		Button->Parent = Element;
		Button->Offset = Offset;
		Button->Size = Skill->Texture->Size;
		Button->Alignment = ae::LEFT_TOP;
		Button->Texture = Skill->Texture;
		Button->Index = (int)Skill->ID;
		Element->Children.push_back(Button);

		// Add level label
		ae::_Element *LevelLabel = new ae::_Element();
		LevelLabel->Name = "label_skills_level";
		LevelLabel->Parent = Button;
		LevelLabel->Offset = LevelOffset;
		LevelLabel->Alignment = ae::CENTER_BASELINE;
		LevelLabel->Font = ae::Assets.Fonts["hud_small"];
		LevelLabel->Index = (int)Skill->ID;
		Element->Children.push_back(LevelLabel);

		// Add plus button
		ae::_Element *PlusButton = new ae::_Element();
		PlusButton->Name = "button_skills_plus";
		PlusButton->Parent = Button;
		PlusButton->Size = glm::vec2(16, 16);
		PlusButton->Offset = PlusOffset;
		PlusButton->Alignment = ae::CENTER_MIDDLE;
		PlusButton->Style = ae::Assets.Styles["style_menu_button"];
		PlusButton->HoverStyle = ae::Assets.Styles["style_menu_button_hover"];
		Element->Children.push_back(PlusButton);

		// Add minus button
		ae::_Element *MinusButton = new ae::_Element();
		MinusButton->Name = "button_skills_minus";
		MinusButton->Parent = Button;
		MinusButton->Size = glm::vec2(16, 16);
		MinusButton->Offset = MinusOffset;
		MinusButton->Alignment = ae::CENTER_MIDDLE;
		MinusButton->Style = ae::Assets.Styles["style_menu_button"];
		MinusButton->HoverStyle = ae::Assets.Styles["style_menu_button_hover"];
		Element->Children.push_back(MinusButton);

		// Add plus label
		ae::_Element *PlusLabel = new ae::_Element();
		PlusLabel->Parent = PlusButton;
		PlusLabel->Text = "+";
		PlusLabel->Offset = LabelOffset;
		PlusLabel->Alignment = ae::CENTER_MIDDLE;
		PlusLabel->Font = ae::Assets.Fonts["hud_medium"];
		PlusButton->Children.push_back(PlusLabel);

		// Add minus label
		ae::_Element *MinusLabel = new ae::_Element();
		MinusLabel->Parent = MinusButton;
		MinusLabel->Text = "-";
		MinusLabel->Offset = LabelOffset;
		MinusLabel->Alignment = ae::CENTER_MIDDLE;
		MinusLabel->Font = ae::Assets.Fonts["hud_medium"];
		MinusButton->Children.push_back(MinusLabel);

		// Update position
		Offset.x += Skill->Texture->Size.x + Spacing.x;
		if(Offset.x > Element->Size.x - Skill->Texture->Size.x) {
			Offset.y += Skill->Texture->Size.y + Spacing.y;
			Offset.x = Start.x;
		}

		i++;
	}
	PlayState.Stats->Database->CloseQuery();

	Element->CalculateBounds();
	Element->SetActive(true);
	HUD->CharacterScreen->Element->SetActive(true);

	RefreshSkillButtons();
	HUD->Cursor.Reset();

	PlayState.SendStatus(_Character::STATUS_SKILLS);
}

// Close screen
bool _SkillScreen::Close() {
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

	// Show remaining skill points
	std::string Text = std::to_string(HUD->Player->Character->GetSkillPointsAvailable()) + " skill point";
	if(HUD->Player->Character->GetSkillPointsAvailable() != 1)
		Text += "s";

	glm::vec2 DrawPosition = glm::vec2((Element->Bounds.End.x + Element->Bounds.Start.x) / 2, Element->Bounds.End.y - 30);
	ae::Assets.Fonts["hud_medium"]->DrawText(Text, DrawPosition, ae::CENTER_BASELINE);

	// Show skill points unused
	int SkillPointsUnused = HUD->Player->Character->SkillPointsUsed - HUD->Player->Character->SkillPointsOnActionBar;
	if(SkillPointsUnused > 0) {
		DrawPosition.y += 21;

		Text = std::to_string(SkillPointsUnused) + " skill point";
		if(SkillPointsUnused != 1)
			Text += "s";

		Text += " unused";

		ae::Assets.Fonts["hud_small"]->DrawText(Text, DrawPosition, ae::CENTER_BASELINE, ae::Assets.Colors["red"]);
	}
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

	// Get remaining points
	int SkillPointsRemaining = HUD->Player->Character->GetSkillPointsAvailable();

	// Loop through buttons
	for(auto &ChildElement : Element->Children) {
		if(ChildElement->Name == "label_skills_level") {
			uint32_t SkillID = (uint32_t)ChildElement->Index;
			ChildElement->Text = std::to_string(HUD->Player->Character->Skills[SkillID]);
		}
		else if(ChildElement->Name == "button_skills_plus") {

			// Get skill
			uint32_t SkillID = (uint32_t)ChildElement->Parent->Index;
			if(SkillPointsRemaining <= 0 || HUD->Player->Character->Skills[SkillID] >= HUD->Player->Stats->Items.at(SkillID)->MaxLevel)
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


// Adjust skill level
void _SkillScreen::AdjustSkillLevel(uint32_t SkillID, int Amount) {
	if(SkillID == 0)
		return;

	if(Amount < 0 && !HUD->Player->CanRespec()) {
		HUD->SetMessage("You can only respec on spawn points");
		return;
	}

	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::SKILLS_SKILLADJUST);

	// Sell skill
	Packet.Write<uint32_t>(SkillID);
	Packet.Write<int>(Amount);

	int OldSkillLevel = HUD->Player->Character->Skills[SkillID];
	HUD->Player->Character->AdjustSkillLevel(SkillID, Amount);

	// Equip new skills
	if(Amount > 0 && OldSkillLevel == 0) {
		EquipSkill(SkillID);
	}

	PlayState.Network->SendPacket(Packet);

	// Update player
	HUD->Player->Character->CalculateStats();
	RefreshSkillButtons();
}

// Equip a skill
void _SkillScreen::EquipSkill(uint32_t SkillID) {
	const _Item *Skill = PlayState.Stats->Items.at(SkillID);
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
			if(!HUD->Player->Character->ActionBar[i].IsSet()) {
				HUD->SetActionBar(i, HUD->Player->Character->ActionBar.size(), Skill);
				return;
			}
		}
	}
}