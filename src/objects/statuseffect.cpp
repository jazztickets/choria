/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2015  Alan Witkowski
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
#include <ui/element.h>
#include <buffer.h>
#include <stats.h>
#include <stdexcept>

// Constructor
_StatusEffect::_StatusEffect() :
	Buff(nullptr),
	BattleElement(nullptr),
	HUDElement(nullptr),
	Time(0.0),
	Level(0),
	Count(0) {

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
	Data.Write<int>(Count);
}

// Unserialize from network
void _StatusEffect::Unserialize(_Buffer &Data, _Stats *Stats) {
	uint32_t BuffID = Data.Read<uint32_t>();
	Buff = Stats->Buffs[BuffID];
	Level = Data.Read<int>();
	Count = Data.Read<int>();
}

// Create element for hud
_Element *_StatusEffect::CreateUIElement(_Element *Parent) {

	_Element *Element = new _Element();
	Element->Size = glm::vec2(Buff->Texture->Size);
	Element->Alignment = LEFT_TOP;
	Element->UserCreated = true;
	Element->Visible = true;
	Element->UserData = (void *)this;
	Element->Parent = Parent;
	Parent->Children.push_back(Element);

	return Element;
}
