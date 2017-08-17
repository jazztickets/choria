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
#include <objects/statchange.h>
#include <objects/object.h>
#include <objects/buff.h>
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
_StatChange::_StatChange() {
	Reset();
}

// Reset stats
void _StatChange::Reset() {
	Object = nullptr;
	Values.clear();
}

// Get bit field of fields changed
uint64_t _StatChange::GetChangedFlag() {
	uint64_t Flag = 0;

	for(auto Iterator : Values) {
		Flag |= ((uint64_t)1 << (uint64_t)Iterator.first);
	}

	return Flag;
}

// Return true if a stat was changed
bool _StatChange::HasStat(StatType Type) {

	return Values.find(Type) != Values.end();
}

// Serialize network
void _StatChange::Serialize(_Buffer &Data) {
	if(!Object)
		throw std::runtime_error("_StatChange::Serialize: Object is null!");

	uint64_t ChangedFlag = GetChangedFlag();
	Data.Write<NetworkIDType>(Object->NetworkID);
	Data.Write<uint64_t>(ChangedFlag);

	for(auto Iterator : Values) {
		if(Iterator.first == StatType::BUFF) {
			_Buff *Buff = (_Buff *)(Iterator.second.Pointer);
			Data.Write<uint32_t>(Buff->ID);
		}
		else
			Data.Write<int>(Iterator.second.Integer);
	}
}

// Unserialize network
void _StatChange::Unserialize(_Buffer &Data, _Manager<_Object> *Manager) {
	Reset();

	NetworkIDType NetworkID = Data.Read<NetworkIDType>();
	Object = Manager->GetObject(NetworkID);
	if(!Object) {
		std::cout << "_StatChange::Unserialize BadObject NetworkID=" << NetworkID << std::endl;
		return;
	}

	// Get changes
	uint64_t ChangedFlag = Data.Read<uint64_t>();
	if(!ChangedFlag)
		return;

	// Update values
	for(uint64_t i = 0; i < (uint64_t)StatType::COUNT; i++) {
		if(ChangedFlag & ((uint64_t)1 << i)) {
			if(i == (uint64_t)StatType::BUFF) {
				uint32_t BuffID = Data.Read<uint32_t>();
				Values[(StatType)i].Pointer = (void *)Object->Stats->Buffs[BuffID];
			}
			else
				Values[(StatType)i].Integer = Data.Read<int>();
		}
	}
}

// Constructor
_StatChangeUI::_StatChangeUI() :
	Object(nullptr),
	Font(nullptr),
	Color(1.0f),
	LastPosition(0, 0),
	Position(0, 0),
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
	Font->DrawText(Text, DrawPosition + glm::vec2(0, 7), CENTER_BASELINE, Color);
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
		Color = COLOR_WHITE;
		Buffer << " ";
	}

	// Set text
	Buffer << (int64_t)(std::abs(Change));
	Text = Buffer.str();
}
