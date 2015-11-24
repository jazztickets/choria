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
#include <instances.h>
#include <instances/map.h>
#include <instances/serverbattle.h>

// Constructor
_Instance::_Instance() {
}

// Destructor
_Instance::~_Instance() {

	// Delete maps
	for(auto &Map : Maps)
		delete Map;

	Maps.clear();

	// Delete battles
	for(auto &Battle : Battles)
		delete Battle;

	Battles.clear();
}

// Updates all the instances
void _Instance::Update(double FrameTime) {

	// Update maps
	for(auto &Map : Maps)
		Map->Update(FrameTime);

	// Update battles
	for(auto &Battle : Battles)
		Battle->Update(FrameTime);
}

// Gets a map from the manager. Loads the level if it doesn't exist
_Map *_Instance::GetMap(int MapID) {

	// Loop through loaded maps
	for(auto &Map : Maps) {

		// Check id
		if(Map->ID == MapID)
			return Map;
	}

	// Not found, so create it
	_Map *NewMap = new _Map(MapID);
	Maps.push_back(NewMap);

	return NewMap;
}

// Create a server battle instance
_ServerBattle *_Instance::CreateServerBattle() {
	_ServerBattle *NewBattle = new _ServerBattle();
	Battles.push_back(NewBattle);

	return NewBattle;
}

// Battle has finished and can be removed
void _Instance::DeleteBattle(_Battle *Battle) {

	// Loop through loaded battles
	for(auto BattleIterator = Battles.begin(); BattleIterator != Battles.end(); ++BattleIterator) {
		if(*BattleIterator == Battle) {
			Battles.erase(BattleIterator);
			delete Battle;
			return;
		}
	}
}
