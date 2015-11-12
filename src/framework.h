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
#pragma once

// Libraries
#include <cstdint>

// Forward Declarations
class _State;
class _SingleNetwork;
class _MultiNetwork;
class _FrameLimit;

class _Framework {

	public:

		enum StateType {
			INIT,
			FADEIN,
			UPDATE,
			FADEOUT,
			CLOSE
		};

		void Init(int ArgumentCount, char **Arguments);
		void Update();
		void Close();

		// States
		void ChangeState(_State *TState);
		_State *GetState() { return State; }

		bool IsDone() { return Done; }
		void SetDone(bool TValue) { Done = TValue; }

		int GetManagerState() const { return FrameworkState; }

		// Networking
		bool IsLocalServerRunning() const { return LocalServerRunning; }
		void StartLocalServer();
		void StopLocalServer();

	private:

		void ResetTimer();

		// States
		StateType FrameworkState;
		_State *State;
		_State *RequestedState;
		bool Done;
		bool PreviousWindowActive;
		bool WindowActive;

		// Flags
		bool MouseWasLocked, MouseWasShown;

		// Time
		_FrameLimit *FrameLimit;
		uint64_t Timer;

		// Networking
		bool LocalServerRunning;
		_SingleNetwork *ClientSingleNetwork;
		_SingleNetwork *ServerSingleNetwork;
		_MultiNetwork *MultiNetwork;

};

extern _Framework Framework;
