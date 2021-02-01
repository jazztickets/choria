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
#include <objects/statuseffect.h>
#include <objects/buff.h>
#include <ae/ui.h>
#include <ae/buffer.h>
#include <ae/assets.h>
#include <ae/font.h>
#include <ae/input.h>
#include <ae/graphics.h>
#include <ae/util.h>
#include <constants.h>
#include <stats.h>
#include <SDL_keycode.h>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>

// Constructor
_StatusEffect::_StatusEffect() :
	Buff(nullptr),
	BattleElement(nullptr),
	HUDElement(nullptr),
	Source(nullptr),
	Time(0.0),
	Duration(0.0),
	MaxDuration(0.0),
	Level(0),
	Infinite(false),
	Deleted(false) {

}

// Destructor
_StatusEffect::~_StatusEffect() {
	if(BattleElement) {
		if(BattleElement->Parent)
			BattleElement->Parent->RemoveChild(BattleElement);

		delete BattleElement;
	}

	if(HUDElement) {
		if(HUDElement->Parent)
			HUDElement->Parent->RemoveChild(HUDElement);

		delete HUDElement;
	}
}

// Serialize for network
void _StatusEffect::Serialize(ae::_Buffer &Data) {
	Data.Write<uint32_t>(Buff->ID);
	Data.WriteBit(Deleted);
	Data.WriteBit(Infinite);
	Data.Write<int>(Level);
	if(!Infinite) {
		Data.Write<float>((float)Duration);
		Data.Write<float>((float)MaxDuration);
	}
}

// Unserialize from network
void _StatusEffect::Unserialize(ae::_Buffer &Data, const _Stats *Stats) {
	uint32_t BuffID = Data.Read<uint32_t>();
	Buff = Stats->Buffs.at(BuffID);
	Deleted = Data.ReadBit();
	Infinite = Data.ReadBit();
	Level = Data.Read<int>();
	if(!Infinite) {
		Duration = Data.Read<float>();
		MaxDuration = Data.Read<float>();
	}
}

// Create element for hud
ae::_Element *_StatusEffect::CreateUIElement(ae::_Element *Parent) {
	ae::_Element *Element = new ae::_Element();
	Element->BaseSize = UI_BUFF_SIZE;
	Element->Alignment = ae::LEFT_TOP;
	Element->Active = true;
	Element->Index = 0;
	Element->UserData = (void *)this;
	Element->Parent = Parent;
	Element->Clickable = true;
	Parent->Children.push_back(Element);

	return Element;
}

// Render the status effect
void _StatusEffect::Render(ae::_Element *Element, double Timer) {

	// Warning flash
	glm::vec4 Color(glm::vec4(1.0f));
	if(Buff->WarningTime > 0 && Duration <= Buff->WarningTime) {
		double FastTime = Timer * 4;
		if(FastTime - (int)FastTime < 0.5)
			Color *= 0.5f;
	}

	// Draw buff icon
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
	ae::Graphics.SetColor(Color);
	ae::Graphics.DrawImage(Element->Bounds, Buff->Texture);

	// Set up graphics
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
	ae::Graphics.SetColor(glm::vec4(0, 0, 0, 0.7f));

	// Draw dark percentage bg
	if(MaxDuration) {
		float OverlayHeight = (Duration / MaxDuration) * (Element->Bounds.End.y - Element->Bounds.Start.y);
		ae::Graphics.DrawRectangle(Element->Bounds.Start + glm::vec2(0, OverlayHeight), Element->Bounds.End, true);
	}

	// Draw level or timer
	std::stringstream Buffer;
	if(Level && Buff->ShowLevel == 1)
		Buffer << Level;
	else if(Buff->ShowLevel == 2)
		Buffer << std::fixed << std::setprecision(1) << ae::Round((float)Duration);

	// Draw small text
	if(Buffer.str().length())
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::ivec2(Element->Bounds.Start.x + 2 * ae::_Element::GetUIScale(), Element->Bounds.End.y - 3 * ae::_Element::GetUIScale()), ae::LEFT_BASELINE, ae::Assets.Colors["white"]);
}
