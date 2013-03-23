/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2010  Alan Witkowski
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
**************************************************************************************/
#include "engine/game.h"

int main(int TArgumentCount, char **TArguments) {
	
	// Initialize the game
	if(!Game::Instance().Init(TArgumentCount, TArguments))
		return 1;

	// Main game loop
	while(!Game::Instance().IsDone()) {

		Game::Instance().Update();
	}

	// Shut down the system
	Game::Instance().Close();

	return 0;
}
