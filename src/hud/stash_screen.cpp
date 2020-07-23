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
#include <hud/stash_screen.h>
#include <hud/inventory_screen.h>
#include <hud/hud.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <states/play.h>
#include <ae/clientnetwork.h>
#include <ae/buffer.h>
#include <ae/font.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <ae/ui.h>
#include <packet.h>
#include <stats.h>
#include <sstream>

// Constructor
_StashScreen::_StashScreen(_HUD *MainHUD, ae::_Element *MainElement) :
	_Screen(MainHUD, MainElement) {
}

// Initialize
void _StashScreen::Init() {
	HUD->Cursor.Reset();
	HUD->InventoryScreen->InitInventoryTab(0);
	Element->SetActive(true);
}

// Close screen
bool _StashScreen::Close(bool SendNotify) {
	bool WasOpen = Element->Active;
	HUD->InventoryScreen->Close();
	if(HUD->Player)
		HUD->Player->Character->ViewingStash = false;

	Element->SetActive(false);

	return WasOpen;
}

// Render
void _StashScreen::Render(double BlendFactor) {
	if(!HUD->Player->Character->ViewingStash) {
		Element->Active = false;
		return;
	}

	Element->Render();

	// Draw stash items
	_Bag &Bag = HUD->Player->Inventory->GetBag(BagType::STASH);
	for(size_t i = 0; i < Bag.Slots.size(); i++) {

		// Get inventory slot
		_InventorySlot *Item = &Bag.Slots[i];
		if(!Item->Item)
			continue;

		// Get bag button
		std::stringstream Buffer;
		Buffer << "button_stash_bag_" << i;
		ae::_Element *Button = ae::Assets.Elements[Buffer.str()];

		// Get position of slot
		glm::vec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2.0f;

		// Draw item
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.DrawScaledImage(DrawPosition, Item->Item->Texture);

		// Draw count
		if(Item->Count > 1)
			ae::Assets.Fonts["hud_tiny"]->DrawText(std::to_string(Item->Count), DrawPosition + glm::vec2(28, 28) * ae::_Element::GetUIScale(), ae::RIGHT_BASELINE);
	}
}
