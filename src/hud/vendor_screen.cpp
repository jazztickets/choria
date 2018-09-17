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
#include <hud/vendor_screen.h>
#include <hud/inventory_screen.h>
#include <hud/hud.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <ae/ui.h>
#include <stats.h>
#include <sstream>

// Constructor
_VendorScreen::_VendorScreen(_HUD *HUD, ae::_Element *Element) :
	_Screen(HUD, Element) {
}

// Initialize
void _VendorScreen::Init() {
	HUD->Cursor.Reset();

	// Open inventory
	HUD->InventoryScreen->InitInventoryTab(0);
	Element->SetActive(true);
}

// Close screen
bool _VendorScreen::Close() {
	bool WasOpen = Element->Active;
	HUD->InventoryScreen->Close();
	if(HUD->Player)
		HUD->Player->Character->Vendor = nullptr;

	Element->SetActive(false);
	HUD->Cursor.Reset();

	return WasOpen;
}

// Render
void _VendorScreen::Render(double BlendFactor) {
	if(!HUD->Player->Character->Vendor) {
		Element->Active = false;
		return;
	}

	Element->Render();

	// Draw vendor items
	for(size_t i = 0; i < HUD->Player->Character->Vendor->Items.size(); i++) {
		const _Item *Item = HUD->Player->Character->Vendor->Items[i];
		if(Item && !HUD->Cursor.IsEqual(i, _HUD::WINDOW_VENDOR)) {

			// Get bag button
			std::stringstream Buffer;
			Buffer << "button_vendor_bag_" << i;
			ae::_Element *Button = ae::Assets.Elements[Buffer.str()];

			// Get position of slot
			glm::vec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2.0f;

			// Draw item
			ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
			if(Item->Texture)
				ae::Graphics.DrawCenteredImage(DrawPosition, Item->Texture);

			// Draw price
			HUD->DrawItemPrice(Item, 1, DrawPosition, true);
		}
	}
}
