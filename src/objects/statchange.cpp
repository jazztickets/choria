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
#include <objects/statchange.h>
#include <objects/object.h>
#include <objects/buff.h>
#include <ae/manager.h>
#include <ae/buffer.h>
#include <ae/util.h>
#include <ae/font.h>
#include <stats.h>
#include <constants.h>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <sstream>
#include <iostream>

// Constructor
_StatChange::_StatChange() :
	Object(nullptr) {
}

// Serialize network
void _StatChange::Serialize(ae::_Buffer &Data) {
	if(!Object)
		throw std::runtime_error("_StatChange::Serialize: Object is null!");

	// Write header
	Data.Write<ae::NetworkIDType>(Object->NetworkID);
	Data.Write<uint8_t>(Values.size());

	// Iterate over statchanges
	for(auto Iterator : Values) {

		// Write type
		Data.Write<uint8_t>((uint8_t)Iterator.first);

		// Write data
		if(Iterator.first == StatType::BUFF) {
			_Buff *Buff = (_Buff *)(Iterator.second.Pointer);
			Data.Write<uint32_t>(Buff->ID);
		}
		else
			Data.Write<int>(Iterator.second.Integer);
	}
}

// Unserialize network
void _StatChange::Unserialize(ae::_Buffer &Data, ae::_Manager<_Object> *Manager) {
	Reset();

	// Get object id
	ae::NetworkIDType NetworkID = Data.Read<ae::NetworkIDType>();
	Object = Manager->GetObject(NetworkID);
	if(!Object) {
		std::cout << "_StatChange::Unserialize BadObject NetworkID=" << NetworkID << std::endl;
		return;
	}

	// Get change count
	int Count = Data.Read<uint8_t>();
	if(!Count)
		return;

	// Get values
	for(int i = 0; i < Count; i++) {

		// Get type
		StatType Type = (StatType)Data.Read<uint8_t>();

		// Get data
		if(Type == StatType::BUFF) {
			uint32_t BuffID = Data.Read<uint32_t>();
			Values[Type].Pointer = (void *)Object->Stats->Buffs.at(BuffID);
		}
		else
			Values[Type].Integer = Data.Read<int>();
	}
}

// Constructor
_StatChangeUI::_StatChangeUI() :
	Object(nullptr),
	Font(nullptr),
	Color(1.0f),
	StartPosition(0.0f, 0.0f),
	LastPosition(0.0f, 0.0f),
	Position(0.0f, 0.0f),
	Direction(-1.0f),
	Time(0.0),
	Timeout(HUD_STATCHANGE_TIMEOUT),
	Change(0.0f),
	Battle(false) {

}

// Render stat change
void _StatChangeUI::Render(double BlendFactor) {
	if(!Object || Change == 0.0f)
		return;

	// Get alpha
	double TimeLeft = Timeout - Time;
	Color.a = 1.0f;
	if(TimeLeft < HUD_STATCHANGE_FADETIME)
		Color.a = (float)(TimeLeft / HUD_STATCHANGE_FADETIME);

	// Get final draw position
	glm::vec2 DrawPosition = glm::mix(LastPosition, Position, BlendFactor);

	// Draw text
	Font->DrawText(Text, DrawPosition + glm::vec2(0, 7), ae::CENTER_BASELINE, Color);
}

// Set text and color
void _StatChangeUI::SetText(const glm::vec4 &NegativeColor, const glm::vec4 &PositiveColor) {

	// Get text color and sign
	std::stringstream Buffer;
	if(Change > 0.0f) {
		Color = PositiveColor;
		Buffer << "+";
	}
	else if(Change < 0.0f) {
		Color = NegativeColor;
		Buffer << "-";
	}
	else {
		Color = glm::vec4(1.0f);
		Buffer << " ";
	}

	// Set text
	Buffer << (int64_t)(std::abs(Change));
	Text = Buffer.str();
}
