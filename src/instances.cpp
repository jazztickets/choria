/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#include <instances.h>
#include <globals.h>
#include <instances/map.h>
#include <instances/clientbattle.h>
#include <instances/serverbattle.h>

// Constructor
InstanceClass::InstanceClass() {

}

// Destructor
InstanceClass::~InstanceClass() {

	// Delete maps
	for(MapIterator = Maps.begin(); MapIterator != Maps.end(); ++MapIterator) {
		delete *MapIterator;
	}
	Maps.clear();

	// Delete battles
	for(BattleIterator = Battles.begin(); BattleIterator != Battles.end(); ++BattleIterator) {
		delete *BattleIterator;
	}
	Battles.clear();
}

// Updates all the instances
void InstanceClass::Update(uint32_t TDeltaTime) {

	// Update maps
	for(MapIterator = Maps.begin(); MapIterator != Maps.end(); ++MapIterator) {
		(*MapIterator)->Update(TDeltaTime);
	}

	// Update battles
	for(BattleIterator = Battles.begin(); BattleIterator != Battles.end(); ++BattleIterator) {
		(*BattleIterator)->Update(TDeltaTime);
	}
}

// Gets a map from the manager. Loads the level if it doesn't exist
_Map *InstanceClass::GetMap(int TMapID) {

	// Loop through loaded maps
	for(MapIterator = Maps.begin(); MapIterator != Maps.end(); ++MapIterator) {

		// Check id
		if((*MapIterator)->GetID() == TMapID)
			return (*MapIterator);
	}

	// Not found, so create it
	_Map *NewMap = new _Map(TMapID);
	Maps.push_back(NewMap);

	return NewMap;
}

// Create a client battle instance
_ClientBattle *InstanceClass::CreateClientBattle() {
	_ClientBattle *NewBattle = new _ClientBattle();
	Battles.push_back(NewBattle);

	return NewBattle;
}

// Create a server battle instance
ServerBattleClass *InstanceClass::CreateServerBattle() {
	ServerBattleClass *NewBattle = new ServerBattleClass();
	Battles.push_back(NewBattle);

	return NewBattle;
}

// Battle has finished and can be removed
void InstanceClass::DeleteBattle(_Battle *TBattle) {

	// Loop through loaded battles
	for(BattleIterator = Battles.begin(); BattleIterator != Battles.end(); ++BattleIterator) {
		if(*BattleIterator == TBattle) {
			Battles.erase(BattleIterator);
			delete TBattle;
			break;
		}
	}
}
