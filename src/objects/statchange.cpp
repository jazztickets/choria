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
#include <instances/battle.h>
#include <stats.h>
#include <buffer.h>
#include <stdexcept>

// Constructor
_StatChange::_StatChange() :
	HealthChange(0),
	ManaChange(0) {

}

// Serialize change
void _StatChange::SerializeBattle(_Buffer &Data) {
	if(!Object)
		throw std::runtime_error("_StatChange::Serialize: Object is null!");

	Data.Write<uint8_t>(Object->BattleID);
	Data.Write<int>(HealthChange);
	Data.Write<int>(ManaChange);
}

// Unserialize change
void _StatChange::UnserializeBattle(_Buffer &Data, _Battle *Battle) {

	uint8_t BattleID = Data.Read<uint8_t>();
	Object = Battle->GetObjectByID(BattleID);
	HealthChange = Data.Read<int>();
	ManaChange = Data.Read<int>();
}
