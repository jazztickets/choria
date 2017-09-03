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
#include <objects/components/fighter.h>

// Constructor
_Fighter::_Fighter(_Object *Object) :
	Object(Object),
	BattleElement(nullptr),
	BattleOffset(0, 0),
	ResultPosition(0, 0),
	StatPosition(0, 0),
	LastTarget{nullptr, nullptr},
	TurnTimer(0.0),
	GoldStolen(0),
	JoinedBattle(false),
	BattleSide(0) {

}
