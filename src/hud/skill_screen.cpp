/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2021 Alan Witkowski
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
#include <ae/input.h>
#include <ae/database.h>
#include <ae/buffer.h>
#include <packet.h>
#include <stats.h>
#include <glm/vec2.hpp>
#include <SDL_keycode.h>
#include <sstream>

// Constructor
_SkillScreen::_SkillScreen(_HUD *HUD, ae::_Element *Element) :
	_Screen(HUD, Element) {
}

// Initialize
void _SkillScreen::Init() {

	// Clear old children
	ClearSkills();

	glm::vec2 Start(14, 36);
	glm::vec2 Offset(Start);
	glm::vec2 LevelOffset(0, -6);
	glm::vec2 Spacing(14, 70);
	glm::vec2 PlusOffset(6, 64 + 9);
	glm::vec2 MinusOffset(36, 64 + 9);
	glm::vec2 LabelOffset(0, 2);
	glm::vec2 ButtonSize(23, 23);
	std::size_t i = 0;

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
		Button->BaseOffset = Offset;
		Button->BaseSize = UI_SLOT_SIZE;
		Button->Alignment = ae::LEFT_TOP;
		Button->Texture = Skill->Texture;
		Button->Index = (int)Skill->ID;
		Button->Clickable = true;
		Element->Children.push_back(Button);

		// Add level label
		ae::_Element *LevelLabel = new ae::_Element();
		LevelLabel->Name = "label_skills_level";
		LevelLabel->Parent = Button;
		LevelLabel->BaseOffset = LevelOffset;
		LevelLabel->Alignment = ae::CENTER_BASELINE;
		LevelLabel->Font = ae::Assets.Fonts["hud_small"];
		LevelLabel->Index = (int)Skill->ID;
		Element->Children.push_back(LevelLabel);

		// Add plus button
		ae::_Element *PlusButton = new ae::_Element();
		PlusButton->Name = "button_skills_plus";
		PlusButton->Parent = Element;
		PlusButton->BaseSize = ButtonSize;
		PlusButton->BaseOffset = Offset + PlusOffset;
		PlusButton->Alignment = ae::LEFT_TOP;
		PlusButton->Style = ae::Assets.Styles["style_menu_button"];
		PlusButton->HoverStyle = ae::Assets.Styles["style_menu_button_hover"];
		PlusButton->Index = (int)Skill->ID;
		PlusButton->Clickable = true;
		Element->Children.push_back(PlusButton);

		// Add minus button
		ae::_Element *MinusButton = new ae::_Element();
		MinusButton->Name = "button_skills_minus";
		MinusButton->Parent = Element;
		MinusButton->BaseSize = ButtonSize;
		MinusButton->BaseOffset = Offset + MinusOffset;
		MinusButton->Alignment = ae::LEFT_TOP;
		MinusButton->Style = ae::Assets.Styles["style_menu_button"];
		MinusButton->HoverStyle = ae::Assets.Styles["style_menu_button_hover"];
		MinusButton->Index = (int)Skill->ID;
		MinusButton->Clickable = true;
		Element->Children.push_back(MinusButton);

		// Add plus label
		ae::_Element *PlusLabel = new ae::_Element();
		PlusLabel->Parent = PlusButton;
		PlusLabel->Text = "+";
		PlusLabel->BaseOffset = LabelOffset;
		PlusLabel->Alignment = ae::CENTER_MIDDLE;
		PlusLabel->Font = ae::Assets.Fonts["hud_medium"];
		PlusButton->Children.push_back(PlusLabel);

		// Add minus label
		ae::_Element *MinusLabel = new ae::_Element();
		MinusLabel->Parent = MinusButton;
		MinusLabel->Text = "-";
		MinusLabel->BaseOffset = LabelOffset + glm::vec2(0, 2);
		MinusLabel->Alignment = ae::CENTER_MIDDLE;
		MinusLabel->Font = ae::Assets.Fonts["hud_medium"];
		MinusButton->Children.push_back(MinusLabel);

		// Update position
		Offset.x += UI_SLOT_SIZE.x + Spacing.x;
		if(Offset.x > Element->BaseSize.x - UI_SLOT_SIZE.x) {
			Offset.y += UI_SLOT_SIZE.y + Spacing.y;
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
	glm::vec2 DrawPosition = glm::vec2((Element->Bounds.End.x + Element->Bounds.Start.x) / 2, Element->Bounds.End.y - 42 * ae::_Element::GetUIScale());

	// Show remaining skill points
	std::string Text = std::to_string(HUD->Player->Character->GetSkillPointsAvailable()) + " skill point";
	if(HUD->Player->Character->GetSkillPointsAvailable() != 1)
		Text += "s";

	// Show more info when holding alt
	int SkillPointsUnlocked = HUD->Player->Character->SkillPointsUnlocked;
	int EternalKnowledge = HUD->Player->Character->Attributes["EternalKnowledge"].Int;
	if(ae::Input.ModKeyDown(KMOD_ALT)) {
		std::string SubText;
		if(SkillPointsUnlocked)
			SubText = std::to_string(SkillPointsUnlocked) + " unlocked";

		if(EternalKnowledge) {
			if(SubText != "")
				SubText += ", ";
			SubText += std::to_string(EternalKnowledge) + " eternal knowledge";
		}

		if(SubText != "")
			Text += " (" + SubText + ")";
	}

	ae::Assets.Fonts["hud_tiny"]->DrawText("Hold Alt for more info", DrawPosition + glm::vec2(-384, 28) * ae::_Element::GetUIScale(), ae::LEFT_BASELINE, ae::Assets.Colors["gray"]);
	ae::Assets.Fonts["hud_medium"]->DrawText(Text, DrawPosition, ae::CENTER_BASELINE);

	// Show skill points unused
	int SkillPointsUnused = HUD->Player->Character->SkillPointsUsed - HUD->Player->Character->SkillPointsOnActionBar;
	if(SkillPointsUnused > 0) {
		Text = std::to_string(SkillPointsUnused) + " skill point";
		if(SkillPointsUnused != 1)
			Text += "s";

		Text += " unused";
		ae::Assets.Fonts["hud_small"]->DrawText(Text, DrawPosition + glm::vec2(0, 30) * ae::_Element::GetUIScale(), ae::CENTER_BASELINE, ae::Assets.Colors["red"]);
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
void _SkillScreen::RefreshSkillButtons(bool ShowBonusPoints) {

	// Get remaining points
	int SkillPointsRemaining = HUD->Player->Character->GetSkillPointsAvailable();

	// Loop through buttons
	for(auto &ChildElement : Element->Children) {
		if(ChildElement->Name == "label_skills_level") {
			uint32_t SkillID = (uint32_t)ChildElement->Index;
			int DrawLevel = HUD->Player->Character->Skills[SkillID];
			int MaxSkillLevel = HUD->Player->Character->MaxSkillLevels[SkillID];
			glm::vec4 LevelColor = glm::vec4(1.0f);
			if(ShowBonusPoints && DrawLevel > 0 && HUD->Player->Character->Attributes["AllSkills"].Int) {
				DrawLevel += HUD->Player->Character->Attributes["AllSkills"].Int;
				if(DrawLevel > MaxSkillLevel)
					LevelColor = ae::Assets.Colors["red"];
				else if(DrawLevel == MaxSkillLevel)
					LevelColor = ae::Assets.Colors["yellow"];
				else
					LevelColor = ae::Assets.Colors["light_blue"];
				DrawLevel = std::min(DrawLevel, MaxSkillLevel);
			}

			if(!HUD->Player->Character->Attributes["AllSkills"].Int) {
				if(DrawLevel == MaxSkillLevel)
					LevelColor = ae::Assets.Colors["yellow"];
			}

			if(ShowBonusPoints)
				ChildElement->Text = std::to_string(DrawLevel);
			else
				ChildElement->Text = std::to_string(DrawLevel) + "/" + std::to_string(HUD->Player->Character->MaxSkillLevels[SkillID]);

			ChildElement->Color = DrawLevel > 0 ? LevelColor : ae::Assets.Colors["gray"];
		}
		else if(ChildElement->Name == "button_skills_plus") {

			// Get skill
			uint32_t SkillID = (uint32_t)ChildElement->Index;
			if(SkillPointsRemaining <= 0 || HUD->Player->Character->Skills[SkillID] >= HUD->Player->Character->MaxSkillLevels[SkillID])
				ChildElement->SetActive(false);
			else
				ChildElement->SetActive(true);
		}
		else if(ChildElement->Name == "button_skills_minus") {

			// Get skill
			uint32_t SkillID = (uint32_t)ChildElement->Index;
			if(HUD->Player->Character->Skills[SkillID] == 0)
				ChildElement->SetActive(false);
			else
				ChildElement->SetActive(true);
		}
	}
}

// Adjust skill level
void _SkillScreen::AdjustSkillLevel(uint32_t SkillID, int Amount, bool SoftMax) {
	if(SkillID == 0)
		return;

	// Write respec message
	if(Amount < 0 && !HUD->Player->CanRespec()) {
		HUD->SetMessage("You can only respec on spawn points");
		return;
	}

	// Notify server
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::SKILLS_SKILLADJUST);
	Packet.Write<uint32_t>(SkillID);
	Packet.Write<int>(Amount);
	Packet.WriteBit(SoftMax);
	PlayState.Network->SendPacket(Packet);

	// Update skill
	int OldSkillLevel = HUD->Player->Character->Skills[SkillID];
	HUD->Player->Character->AdjustSkillLevel(SkillID, Amount, SoftMax);

	// Equip new skills
	if(Amount > 0 && OldSkillLevel == 0)
		EquipSkill(SkillID);

	// Update player
	HUD->Player->Character->CalculateStats();
	RefreshSkillButtons(!ae::Input.ModKeyDown(KMOD_ALT));
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
		for(int i = 0; i < HUD->Player->Character->SkillBarSize; i++) {
			if(HUD->Player->Character->ActionBar[i].Item == Skill)
				return;
		}

		// Find an empty slot
		for(int i = 0; i < HUD->Player->Character->SkillBarSize; i++) {
			if(!HUD->Player->Character->ActionBar[i].IsSet()) {
				HUD->SetActionBar(i, NOSLOT, Skill);
				return;
			}
		}
	}
}
