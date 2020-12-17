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
#include <hud/trader_screen.h>
#include <hud/hud.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <ae/font.h>
#include <ae/ui.h>
#include <stats.h>
#include <sstream>

// Constructor
_TraderScreen::_TraderScreen(_HUD *HUD, ae::_Element *Element) :
	_Screen(HUD, Element) {
}

// Initialize
void _TraderScreen::Init() {

	// Check for required items
	RequiredItemSlots.resize(HUD->Player->Character->Trader->Items.size());
	RewardItemSlot = HUD->Player->Inventory->GetRequiredItemSlots(HUD->Player->Character->Trader, RequiredItemSlots);

	// Disable accept button if requirements not met
	if(!HUD->Player->Inventory->IsValidSlot(RewardItemSlot))
		ae::Assets.Elements["button_trader_accept"]->SetEnabled(false);
	else
		ae::Assets.Elements["button_trader_accept"]->SetEnabled(true);

	Element->SetActive(true);
}

// Close screen
bool _TraderScreen::Close(bool SendNotify) {
	bool WasOpen = Element->Active;
	Element->SetActive(false);
	HUD->Cursor.Reset();

	if(HUD->Player)
		HUD->Player->Character->Trader = nullptr;

	return WasOpen;
}

// Render
void _TraderScreen::Render(double BlendFactor) {
	if(!HUD->Player->Character->Trader) {
		Element->Active = false;
		return;
	}

	// Render ui elements
	Element->Render();

	// Get item count offset
	glm::vec2 CountOffset = glm::vec2(28, 26) * ae::_Element::GetUIScale();

	// Draw trader items
	for(std::size_t i = 0; i < HUD->Player->Character->Trader->Items.size(); i++) {

		// Get button position
		std::stringstream Buffer;
		Buffer << "button_trader_bag_" << i;
		ae::_Element *Button = ae::Assets.Elements[Buffer.str()];
		glm::vec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2.0f;

		// Draw item
		const _Item *Item = HUD->Player->Character->Trader->Items[i].Item;
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawScaledImage(DrawPosition, Item->Texture, UI_SLOT_SIZE);

		glm::vec4 Color;
		if(!HUD->Player->Inventory->IsValidSlot(RequiredItemSlots[i]))
			Color = ae::Assets.Colors["red"];
		else
			Color = glm::vec4(1.0f);

		ae::Assets.Fonts["hud_tiny"]->DrawText(std::to_string(HUD->Player->Character->Trader->Items[i].Count), DrawPosition + CountOffset, ae::RIGHT_BASELINE, Color);
	}

	// Get reward button
	ae::_Element *RewardButton = ae::Assets.Elements["button_trader_bag_reward"];
	glm::vec2 DrawPosition = (RewardButton->Bounds.Start + RewardButton->Bounds.End) / 2.0f;

	// Draw item
	if(HUD->Player->Character->Trader->RewardItem) {
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawScaledImage(DrawPosition, HUD->Player->Character->Trader->RewardItem->Texture, UI_SLOT_SIZE);

		if(HUD->Player->Character->Trader->Count > 1)
			ae::Assets.Fonts["hud_tiny"]->DrawText(std::to_string(HUD->Player->Character->Trader->Count), DrawPosition + CountOffset, ae::RIGHT_BASELINE);
	}
}
