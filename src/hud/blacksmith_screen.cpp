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
#include <hud/blacksmith_screen.h>
#include <hud/character_screen.h>
#include <hud/inventory_screen.h>
#include <hud/hud.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <config.h>
#include <ae/graphics.h>
#include <ae/ui.h>
#include <ae/assets.h>
#include <stats.h>
#include <sstream>

// Constructor
_BlacksmithScreen::_BlacksmithScreen(_HUD *HUD, ae::_Element *Element) :
	_Screen(HUD, Element) {
}

// Initialize
void _BlacksmithScreen::Init() {
	HUD->Cursor.Reset();
	HUD->InventoryScreen->InitInventoryTab(0);
	UpgradeSlot.Type = BagType::NONE;

	Element->SetActive(true);
	HUD->CharacterScreen->Element->SetActive(true);
	ae::Assets.Elements["label_blacksmith_cost"]->SetActive(true);
	ae::Assets.Elements["button_blacksmith_upgrade"]->SetEnabled(false);
}

// Close
bool _BlacksmithScreen::Close(bool SendNotify) {
	bool WasOpen = Element->Active;
	HUD->InventoryScreen->Close();

	Element->SetActive(false);
	HUD->Cursor.Reset();

	if(HUD->Player)
		HUD->Player->Character->Blacksmith = nullptr;

	UpgradeSlot.Type = BagType::NONE;

	return WasOpen;
}

// Render
void _BlacksmithScreen::Render(double BlendFactor) {
	if(!HUD->Player->Character->Blacksmith) {
		Element->Active = false;
		return;
	}

	// Get UI elements
	ae::_Element *BlacksmithTitle = ae::Assets.Elements["label_blacksmith_title"];
	ae::_Element *BlacksmithCost = ae::Assets.Elements["label_blacksmith_cost"];
	ae::_Element *BlacksmithLevel = ae::Assets.Elements["label_blacksmith_level"];
	ae::_Element *UpgradeButton = ae::Assets.Elements["button_blacksmith_upgrade"];

	// Set title
	BlacksmithTitle->Text = HUD->Player->Character->Blacksmith->Name;
	BlacksmithLevel->Text = "Level " + std::to_string(HUD->Player->Character->Blacksmith->Level);

	// Draw element
	Element->Render();

	// Draw item
	if(HUD->Player->Inventory->IsValidSlot(UpgradeSlot)) {

		// Get upgrade bag button
		ae::_Element *BagButton = ae::Assets.Elements["button_blacksmith_bag"];
		glm::vec2 DrawPosition = (BagButton->Bounds.Start + BagButton->Bounds.End) / 2.0f;

		const _InventorySlot &InventorySlot = HUD->Player->Inventory->GetSlot(UpgradeSlot);
		const _Item *Item = InventorySlot.Item;
		if(Item) {
			ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
			ae::Graphics.DrawScaledImage(DrawPosition, Item->Texture, UI_SLOT_SIZE);

			BlacksmithCost->SetActive(true);
			UpgradeButton->SetEnabled(true);

			// Get cost
			int64_t Cost = Item->GetUpgradeCost(HUD->Player, InventorySlot.Upgrades+1);

			// Update cost label
			std::stringstream Buffer;
			Buffer.imbue(std::locale(Config.Locale));
			Buffer << Cost << " gold";
			BlacksmithCost->Font = ae::Assets.Fonts["hud_medium"];
			BlacksmithCost->Color = ae::Assets.Colors["gold"];
			BlacksmithCost->Text = Buffer.str();

			// Check upgrade conditions
			bool Disabled = false;
			if(HUD->Player->Character->Attributes["Gold"].Int64 < Cost)
				Disabled = true;

			// Check blacksmith level
			if(InventorySlot.Upgrades >= HUD->Player->Character->Blacksmith->Level) {
				Disabled = true;
				BlacksmithCost->Text = "I can't upgrade this";
			}

			// Check item level
			if(InventorySlot.Upgrades >= Item->MaxLevel) {
				Disabled = true;
				BlacksmithCost->Text = "Max Level";
			}

			// Disable button
			if(Disabled) {
				BlacksmithCost->Color = ae::Assets.Colors["red"];
				UpgradeButton->SetEnabled(false);
			}
		}
		else
			UpgradeButton->SetEnabled(false);
	}
	else {
		BlacksmithCost->Font = ae::Assets.Fonts["hud_small"];
		BlacksmithCost->Color = ae::Assets.Colors["gray"];
		BlacksmithCost->Text = "Drag an item to the slot";
		UpgradeButton->SetEnabled(false);
	}
}
