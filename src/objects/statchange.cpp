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
#include <objects/statchange.h>
#include <objects/object.h>
#include <stats.h>
#include <buffer.h>
#include <constants.h>
#include <font.h>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <sstream>

// Constructor
_StatChange::_StatChange() :
	Object(nullptr),
	Health(0),
	Mana(0),
	Experience(0),
	Gold(0) {

}

// Get bit field of fields changed
int _StatChange::GetChangedFlag() {
	int Flag = 0;

	if(Health != 0)
		Flag |= StatType::HEALTH;
	if(Mana != 0)
		Flag |= StatType::MANA;
	if(Experience != 0)
		Flag |= StatType::EXPERIENCE;
	if(Gold != 0)
		Flag |= StatType::GOLD;

	return Flag;
}

// Serialize network
void _StatChange::Serialize(_Buffer &Data) {
	if(!Object)
		throw std::runtime_error("_StatChange::Serialize: Object is null!");

	int ChangedFlag = GetChangedFlag();
	Data.Write<NetworkIDType>(Object->NetworkID);
	Data.Write<int>(ChangedFlag);
	if(ChangedFlag & StatType::HEALTH)
		Data.Write<int>(Health);
	if(ChangedFlag & StatType::MANA)
		Data.Write<int>(Mana);
	if(ChangedFlag & StatType::EXPERIENCE)
		Data.Write<int>(Experience);
	if(ChangedFlag & StatType::GOLD)
		Data.Write<int>(Gold);
}

// Unserialize network
void _StatChange::Unserialize(_Buffer &Data, _Manager<_Object> *Manager) {
	NetworkIDType NetworkID = Data.Read<NetworkIDType>();
	Object = Manager->IDMap[NetworkID];

	int ChangedFlag = Data.Read<int>();
	if(ChangedFlag & StatType::HEALTH)
		Health = Data.Read<int>();
	if(ChangedFlag & StatType::MANA)
		Mana = Data.Read<int>();
	if(ChangedFlag & StatType::EXPERIENCE)
		Experience = Data.Read<int>();
	if(ChangedFlag & StatType::GOLD)
		Gold = Data.Read<int>();
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
	TimeOut(STATCHANGE_TIMEOUT),
	Change(0) {

}

// Render stat change
void _StatChangeUI::Render(double BlendFactor) {
	if(!Object || Change == 0)
		return;

	// Get alpha
	double TimeLeft = TimeOut - Time;
	Color.a = 1.0f;
	if(TimeLeft < ACTIONRESULT_FADETIME)
		Color.a = (float)(TimeLeft / ACTIONRESULT_FADETIME);

	// Get final draw position
	glm::vec2 DrawPosition = glm::mix(LastPosition, Position, BlendFactor);

	// Draw text
	Font->DrawText(Text.c_str(), DrawPosition + glm::vec2(0, 7), Color, CENTER_BASELINE);
}

// Set text and color
void _StatChangeUI::SetText(const glm::vec4 &NegativeColor, const glm::vec4 &PositiveColor) {

	// Get text color
	std::stringstream Buffer;
	if(Change > 0) {
		Color = PositiveColor;
		Buffer << "+";
	}
	else if(Change < 0) {
		Color = NegativeColor;
		Buffer << "-";
	}
	else {
		Color = COLOR_WHITE;
		Buffer << " ";
	}

	// Set text
	Buffer << std::abs(Change);
	Text = Buffer.str();
}
