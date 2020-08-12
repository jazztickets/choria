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
#include <hud/trade_screen.h>
#include <hud/inventory_screen.h>
#include <hud/hud.h>
#include <objects/object.h>
#include <objects/components/character.h>
#include <objects/components/controller.h>
#include <states/play.h>
#include <ae/clientnetwork.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <ae/buffer.h>
#include <ae/util.h>
#include <ae/font.h>
#include <ae/ui.h>
#include <packet.h>
#include <stats.h>
#include <sstream>

// Constructor
_TradeScreen::_TradeScreen(_HUD *HUD, ae::_Element *Element) :
	_Screen(HUD, Element) {
}

// Initialize
void _TradeScreen::Init() {
	if(HUD->Player->Character->WaitingForTrade)
		return;

	HUD->Player->Character->WaitingForTrade = true;
	HUD->InventoryScreen->InitInventoryTab(0);
	Element->SetActive(true);

	// Send request to server
	SendTradeRequest();

	// Reset UI
	ResetAcceptButton();

	// Reset their trade UI
	ResetTradeTheirsWindow();
}

// Close screen
bool _TradeScreen::Close(bool SendNotify) {
	bool WasOpen = Element->Active;

	// Close inventory
	HUD->InventoryScreen->Close();
	Element->SetActive(false);
	ae::FocusedElement = nullptr;

	// Notify server
	if(SendNotify)
		SendTradeCancel();

	if(HUD->Player) {
		HUD->Player->Character->WaitingForTrade = false;
		HUD->Player->Character->TradePlayer = nullptr;
	}

	return WasOpen;
}

// Toggle screen
void _TradeScreen::Toggle() {
	if(HUD->Player->Controller->WaitForServer || !HUD->Player->Character->CanOpenTrade())
		return;

	// Restrict trading for new characters
	if(!HUD->Player->Character->CanTrade()) {
		HUD->SetMessage("Trading unlocks at level " + std::to_string(GAME_TRADING_LEVEL));
		return;
	}

	if(!Element->Active) {
		HUD->CloseWindows(true);
		Init();
	}
	else {
		HUD->CloseWindows(true);
	}
}

// Render
void _TradeScreen::Render(double BlendFactor) {
	if(!Element->Active)
		return;

	Element->Render();

	// Draw items
	DrawTradeItems(HUD->Player, "button_trade_yourbag_", _HUD::WINDOW_TRADEYOURS);
	DrawTradeItems(HUD->Player->Character->TradePlayer, "button_trade_theirbag_", _HUD::WINDOW_TRADETHEIRS);
}

// Draws trading items
void _TradeScreen::DrawTradeItems(_Object *Player, const std::string &ElementPrefix, int Window) {
	if(!Player)
		return;

	// Draw offered items
	int BagIndex = 0;
	_Bag &Bag = Player->Inventory->GetBag(BagType::TRADE);
	for(size_t i = 0; i < Bag.Slots.size(); i++) {

		// Get inventory slot
		_InventorySlot *Item = &Bag.Slots[i];
		if(Item->Item) {

			// Get bag button
			std::stringstream Buffer;
			Buffer << ElementPrefix << BagIndex;
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

		BagIndex++;
	}
}

// Update accept button label text
void _TradeScreen::UpdateAcceptButton() {
	ae::_Element *AcceptButton = ae::Assets.Elements["button_trade_accept_yours"];
	ae::_Element *LabelTradeStatusYours = ae::Assets.Elements["label_trade_status_yours"];
	if(AcceptButton->Checked) {
		LabelTradeStatusYours->Text = "Accepted";
		LabelTradeStatusYours->Color = ae::Assets.Colors["green"];
	}
	else {
		LabelTradeStatusYours->Text = "Accept";
		LabelTradeStatusYours->Color = glm::vec4(1.0f);
	}
}

// Resets the trade agreement
void _TradeScreen::ResetAcceptButton() {
	ae::Assets.Elements["button_trade_accept_yours"]->Checked = false;
	UpdateAcceptButton();
	UpdateTradeStatus(false);
}

// Resets upper trade window status
void _TradeScreen::ResetTradeTheirsWindow() {
	ae::Assets.Elements["element_trade_theirs"]->SetActive(false);
	ae::Assets.Elements["label_trade_status"]->SetActive(true);
	ae::Assets.Elements["textbox_trade_gold_theirs"]->Enabled = false;
	ae::Assets.Elements["textbox_trade_gold_theirs"]->SetText("0");
	ae::Assets.Elements["textbox_trade_gold_yours"]->SetText("0");
	ae::Assets.Elements["label_trade_name_yours"]->Text = HUD->Player->Name;
	ae::Assets.Elements["image_trade_portrait_yours"]->Texture = HUD->Player->Character->Portrait;

	ae::_Element *RestrictLabel = ae::Assets.Elements["label_trade_status_restrict"];
	if(GAME_TRADING_LEVEL_RANGE) {
		RestrictLabel->SetActive(true);
		RestrictLabel->Text = "Other player must be within " + std::to_string(GAME_TRADING_LEVEL_RANGE) + " levels";
	}
	else
		RestrictLabel->SetActive(false);
}

// Trade with another player
void _TradeScreen::SendTradeRequest() {
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::TRADE_REQUEST);
	PlayState.Network->SendPacket(Packet);
}

// Cancel a trade
void _TradeScreen::SendTradeCancel() {
	if(!HUD->Player)
		return;

	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::TRADE_CANCEL);
	PlayState.Network->SendPacket(Packet);

	HUD->Player->Character->TradePlayer = nullptr;
}

// Update their status label
void _TradeScreen::UpdateTradeStatus(bool Accepted) {
	ae::_Element *LabelTradeStatusTheirs = ae::Assets.Elements["label_trade_status_theirs"];
	if(Accepted) {
		LabelTradeStatusTheirs->Text = "Accepted";
		LabelTradeStatusTheirs->Color = ae::Assets.Colors["green"];
	}
	else {
		LabelTradeStatusTheirs->Text = "Unaccepted";
		LabelTradeStatusTheirs->Color = ae::Assets.Colors["red"];
	}
}

// Make sure the trade gold box is valid and send gold to player
void _TradeScreen::ValidateTradeGold() {
	if(!HUD->Player || !Element->Active)
		return;

	ae::_Element *GoldTextBox = ae::Assets.Elements["textbox_trade_gold_yours"];

	// Get gold amount
	int Gold = ae::ToNumber<int>(GoldTextBox->Text);
	if(Gold < 0)
		Gold = 0;
	else if(Gold > HUD->Player->Character->Attributes["Gold"].Int)
		Gold = std::max(0, HUD->Player->Character->Attributes["Gold"].Int);

	// Set text
	GoldTextBox->SetText(std::to_string(Gold));

	// Send amount
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::TRADE_GOLD);
	Packet.Write<int>(Gold);
	PlayState.Network->SendPacket(Packet);

	// Reset agreement
	ResetAcceptButton();
}
