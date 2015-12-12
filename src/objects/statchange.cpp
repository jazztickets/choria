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
#include <stats.h>
#include <buffer.h>

// Serialize change
void _StatChange::Serialize(_Buffer &Data) {

	//Data.Write<uint32_t>(SkillID);
	//Data.Write<uint32_t>(ItemID);
}

// Unserialize change
void _StatChange::Unserialize(_Buffer &Data, _Stats *Stats) {

	//uint32_t SkillID = Data.Read<uint32_t>();
	//uint32_t ItemID = Data.Read<uint32_t>();

	//Skill = Stats->Skills[SkillID];
	//Item = Stats->Items[ItemID];
}
