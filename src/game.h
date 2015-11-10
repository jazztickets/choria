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
#pragma once

// Libraries
#include <cstdint>

// Forward Declarations
class _State;
class _SingleNetwork;
class _MultiNetwork;

class _Game {

	public:

		enum ManagerStateType {
			STATE_INIT,
			STATE_FADEIN,
			STATE_UPDATE,
			STATE_FADEOUT,
			STATE_CLOSE
		};

		int Init(int TArgumentCount, char **TArguments);
		void Update();
		void Close();

		// States
		void ChangeState(_State *TState);
		_State *GetState() { return State; }

		bool IsDone() { return Done; }
		void SetDone(bool TValue) { Done = TValue; }

		int GetManagerState() const { return ManagerState; }

		// Networking
		bool IsLocalServerRunning() const { return LocalServerRunning; }
		void StartLocalServer();
		void StopLocalServer();

	private:

		void ResetTimer();

		// States
		ManagerStateType ManagerState;
		_State *State, *NewState;
		bool PreviousWindowActive, WindowActive;

		// Flags
		bool Done, MouseWasLocked, MouseWasShown;

		// Time
		double TimeStamp, LastFrameTime;

		// Networking
		bool LocalServerRunning;
		_SingleNetwork *ClientSingleNetwork, *ServerSingleNetwork;
		_MultiNetwork *MultiNetwork;

};

extern _Game Game;
