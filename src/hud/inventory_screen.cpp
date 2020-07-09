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
#include <hud/inventory_screen.h>
#include <hud/character_screen.h>
#include <hud/hud.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <objects/components/controller.h>
#include <objects/item.h>
#include <states/play.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <ae/font.h>
#include <ae/ui.h>
#include <ae/input.h>
#include <stats.h>
#include <sstream>
#include <SDL_keycode.h>

// Constructor
_InventoryScreen::_InventoryScreen(_HUD *HUD, ae::_Element *Element) :
	_Screen(HUD, Element) {

	EquipmentElement = ae::Assets.Elements["element_equipment"];
	InventoryElement = ae::Assets.Elements["element_inventory"];
	KeysElement = ae::Assets.Elements["element_keys"];

	EquipmentElement->SetActive(false);
	InventoryElement->SetActive(false);
	KeysElement->SetActive(false);
}

// Close screen
bool _InventoryScreen::Close() {
	bool WasOpen = Element->Active;
	HUD->Cursor.Reset();

	Element->SetActive(false);
	EquipmentElement->SetActive(false);
	InventoryElement->SetActive(false);
	KeysElement->SetActive(false);
	HUD->CharacterScreen->Element->SetActive(false);

	return WasOpen;
}

// Toggle display
void _InventoryScreen::Toggle() {
	if(HUD->Player->Controller->WaitForServer || !HUD->Player->Character->CanOpenInventory())
		return;

	if(HUD->Minigame) {
		HUD->CharacterScreen->Toggle();
		return;
	}

	if(!Element->Active) {
		HUD->CloseWindows(true);

		Element->SetActive(true);
		InitInventoryTab(0);
		HUD->CharacterScreen->Element->SetActive(true);
		PlayState.SendStatus(_Character::STATUS_INVENTORY);
	}
	else {
		HUD->CloseWindows(true);
	}
}

// Render
void _InventoryScreen::Render(double BlendFactor) {
	if(!Element->Active)
		return;

	Element->Render();
	if(InventoryElement->Active) {
		EquipmentElement->Render();
		InventoryElement->Render();
		DrawBag(BagType::EQUIPMENT);
		DrawBag(BagType::INVENTORY);
	}
	else if(KeysElement->Active) {
		KeysElement->Render();
		DrawKeys();
	}
}

// Initialize an inventory tab
void _InventoryScreen::InitInventoryTab(int Index) {
	for(auto &Child : Element->Children)
		Child->Checked = Child->Index == Index ? true : false;

	Element->SetActive(true);
	EquipmentElement->SetActive(false);
	InventoryElement->SetActive(false);
	KeysElement->SetActive(false);
	if(Index == 0) {
		EquipmentElement->SetActive(true);
		InventoryElement->SetActive(true);
	}
	else if(Index == 1) {
		KeysElement->SetActive(true);
	}
}

// Draw an inventory bag
void _InventoryScreen::DrawBag(BagType Type) {
	_Bag &Bag = HUD->Player->Inventory->GetBag(Type);
	for(size_t i = 0; i < Bag.Slots.size(); i++) {

		// Get inventory slot
		_InventorySlot *Slot = &Bag.Slots[i];
		if(Slot->Item) {

			// Get bag button
			std::stringstream Buffer;
			Buffer << "button_" << Bag.Name << "_bag_" << i;
			ae::_Element *Button = ae::Assets.Elements[Buffer.str()];
			Buffer.str("");

			// Get position of slot
			glm::vec2 DrawPosition = (Button->Bounds.Start + Button->Bounds.End) / 2.0f;

			// Draw item
			ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
			ae::Graphics.DrawScaledImage(DrawPosition, Slot->Item->Texture);

			// Draw two handed weapon twice in equipment bag
			if(Type == BagType::EQUIPMENT && i == EquipmentType::HAND1 && Slot->Item->Type == ItemType::TWOHANDED_WEAPON) {
				Buffer << "button_" << Bag.Name << "_bag_" << EquipmentType::HAND2;
				ae::_Element *Button = ae::Assets.Elements[Buffer.str()];
				ae::Graphics.DrawScaledImage((Button->Bounds.Start + Button->Bounds.End) / 2.0f, Slot->Item->Texture, ae::Assets.Colors["itemfade"]);
			}

			// Draw price if using vendor
			HUD->DrawItemPrice(Slot->Item, Slot->Count, DrawPosition, false, Slot->Upgrades);

			// Draw upgrade count if using blacksmith or holding alt
			if(!Slot->Item->IsSkill() && (Slot->Item->MaxLevel && (Type == BagType::EQUIPMENT || ae::Input.ModKeyDown(KMOD_ALT) || HUD->Player->Character->Blacksmith))) {
				glm::vec4 Color = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);
				if(HUD->Player->Character->Blacksmith) {
					if(Slot->Upgrades >= Slot->Item->MaxLevel || Slot->Upgrades >= HUD->Player->Character->Blacksmith->Level)
						Color = ae::Assets.Colors["red"];
					else
						Color = ae::Assets.Colors["green"];
				}

				ae::Assets.Fonts["hud_tiny"]->DrawText(std::to_string(Slot->Upgrades), DrawPosition + glm::vec2(-30, 28) * ae::_Element::GetUIScale(), ae::LEFT_BASELINE, Color);
			}

			// Draw count
			if(Slot->Count > 1)
				ae::Assets.Fonts["hud_tiny"]->DrawText(std::to_string(Slot->Count), DrawPosition + glm::vec2(28, 28) * ae::_Element::GetUIScale(), ae::RIGHT_BASELINE);
		}
	}
}

// Draw keychain
void _InventoryScreen::DrawKeys() {

	glm::vec2 StartOffset = glm::vec2(14, 26) * ae::_Element::GetUIScale();
	glm::vec2 Spacing = glm::vec2(168, 20) * ae::_Element::GetUIScale();
	int Column = 0;
	int Row = 0;

	_Bag &Bag = HUD->Player->Inventory->GetBag(BagType::KEYS);

	// No keys
	if(!Bag.Slots.size()) {
		glm::vec2 DrawPosition = KeysElement->Bounds.Start + StartOffset;
		ae::Assets.Fonts["hud_tiny"]->DrawText("No keys", DrawPosition, ae::LEFT_BASELINE, ae::Assets.Colors["gray"]);

		return;
	}

	// Draw key names
	for(size_t i = 0; i < Bag.Slots.size(); i++) {

		// Get inventory slot
		_InventorySlot *Slot = &Bag.Slots[i];
		if(Slot->Item) {

			// Get position
			glm::vec2 DrawPosition = KeysElement->Bounds.Start + StartOffset + glm::vec2(Spacing.x * Column, Spacing.y * Row);
			ae::Assets.Fonts["hud_tiny"]->DrawText(Slot->Item->Name, DrawPosition);

			// Update position
			Row++;
			if(Row >= 14) {
				Column++;
				Row = 0;
			}
		}
	}
}
