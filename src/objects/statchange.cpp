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
#include <assets.h>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <sstream>

// Constructor
_StatChange::_StatChange() :
	Object(nullptr),
	LastPosition(0, 0),
	Position(0, 0),
	Time(0.0),
	HealthChange(0),
	ManaChange(0) {

}

// Return true if there are stat changes
bool _StatChange::IsChanged() {

	return HealthChange != 0 || ManaChange != 0;
}

// Serialize change
void _StatChange::Serialize(_Buffer &Data) {
	if(!Object)
		throw std::runtime_error("_StatChange::Serialize: Object is null!");

	Data.Write<NetworkIDType>(Object->NetworkID);
	Data.Write<int>(HealthChange);
	Data.Write<int>(ManaChange);
}

// Unserialize change
void _StatChange::Unserialize(_Buffer &Data, _Manager<_Object> *Manager) {

	NetworkIDType NetworkID = Data.Read<NetworkIDType>();
	Object = Manager->IDMap[NetworkID];
	HealthChange = Data.Read<int>();
	ManaChange = Data.Read<int>();
}


// Render recent stat changes
void _StatChange::Render(double BlendFactor) {
	if(!Object)
		return;

	// Get text color
	glm::vec4 TextColor = COLOR_WHITE;
	char Sign = ' ';
	if(HealthChange > 0) {
		TextColor = COLOR_GREEN;
		Sign = '+';
	}
	else if(HealthChange < 0) {
		TextColor = COLOR_RED;
		Sign = '-';
	}

	// Get alpha
	double TimeLeft = STATCHANGE_TIMEOUT - Time;
	TextColor.a = 1.0f;
	if(TimeLeft < ACTIONRESULT_FADETIME)
		TextColor.a = (float)(TimeLeft / ACTIONRESULT_FADETIME);

	// Get final draw position
	glm::vec2 DrawPosition = glm::mix(LastPosition, Position, BlendFactor);

	// Draw stat
	std::stringstream Buffer;
	Buffer << Sign << std::abs(HealthChange);
	Assets.Fonts["hud_medium"]->DrawText(Buffer.str().c_str(), DrawPosition + glm::vec2(0, 7), TextColor, CENTER_BASELINE);
}
