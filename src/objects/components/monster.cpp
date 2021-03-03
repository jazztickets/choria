/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2021 Alan Witkowski
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
#include <objects/components/monster.h>

// Constructor
_Monster::_Monster(_Object *Object) :
	Object(Object),
	Owner(nullptr),
	SummonBuff(nullptr),
	DatabaseID(0),
	SpellID(0),
	Duration(0.0),
	Difficulty(0),
	ExperienceGiven(0),
	GoldGiven(0),
	AI("") {

}
