/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#include <ae/util.h>
#include <ae/font.h>
#include <ae/graphics.h>
#include <constants.h>
#include <stats.h>
#include <stdexcept>
#include <sstream>
#include <iomanip>

// Constructor
_StatusEffect::_StatusEffect() :
	Buff(nullptr),
	BattleElement(nullptr),
	HUDElement(nullptr),
	Time(0.0),
	Level(0),
	Duration(0.0) {

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
void _StatusEffect::Serialize(_Buffer &Data) {
	Data.Write<uint32_t>(Buff->ID);
	Data.Write<int>(Level);
	Data.Write<float>((float)Duration);
}

// Unserialize from network
void _StatusEffect::Unserialize(_Buffer &Data, _Stats *Stats) {
	uint32_t BuffID = Data.Read<uint32_t>();
	Buff = Stats->Buffs[BuffID];
	Level = Data.Read<int>();
	Duration = Data.Read<float>();
}

// Create element for hud
_Element *_StatusEffect::CreateUIElement(_Element *Parent) {

	_Element *Element = new _Element();
	Element->Size = glm::vec2(Buff->Texture->Size);
	Element->Alignment = LEFT_TOP;
	Element->Active = true;
	Element->Index = 0;
	Element->UserData = (void *)this;
	Element->Parent = Parent;
	Parent->Children.push_back(Element);

	return Element;
}

// Render the status effect
void _StatusEffect::Render(_Element *Element, const glm::vec4 &Color) {
	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	Graphics.SetVBO(VBO_NONE);
	Graphics.SetColor(Color);
	Graphics.DrawImage(Element->Bounds, Buff->Texture);

	glm::vec4 TextColor = COLOR_WHITE;
	TextColor.a = Color.a;

	// Build string
	std::stringstream Buffer;
	Buffer << std::fixed << std::setprecision(1) << Round((float)Duration);

	// Get text dimensions
	_TextBounds TextBounds;
	Assets.Fonts["hud_tiny"]->GetStringDimensions(Buffer.str(), TextBounds, false);

	glm::vec2 StartPosition = glm::vec2(Element->Bounds.End.x-3, Element->Bounds.End.y-2);

	// Draw text bg
	Graphics.SetProgram(Assets.Programs["ortho_pos"]);
	Graphics.SetVBO(VBO_NONE);
	Graphics.SetColor(glm::vec4(0, 0, 0, 0.5f));
	Graphics.DrawRectangle(glm::vec2(StartPosition.x - TextBounds.Width, StartPosition.y - TextBounds.AboveBase), glm::vec2(StartPosition.x, StartPosition.y + TextBounds.BelowBase), true);

	// Draw text
	Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), StartPosition, RIGHT_BASELINE, TextColor);
}
