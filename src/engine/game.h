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
#ifndef GAME_H
#define GAME_H

// Libraries
#include <irrlicht.h>
#include "singleton.h"

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Forward Declarations
class StateClass;
class SingleNetworkClass;
class MultiNetworkClass;

class GameClass {

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
		void ChangeState(StateClass *TState);
		StateClass *GetState() { return State; }

		bool IsDone() { return Done; }
		void SetDone(bool TValue) { Done = TValue; }

		int GetManagerState() const { return ManagerState; }

		// Networking
		bool IsLocalServerRunning() const { return LocalServerRunning; }
		void StartLocalServer();
		void StopLocalServer();

	private:

		void Delay(int TTime);
		void ResetTimer();
		void ResetGraphics();

		// States
		ManagerStateType ManagerState;
		StateClass *State, *NewState;
		bool PreviousWindowActive, WindowActive;

		// Flags
		bool Done, MouseWasLocked, MouseWasShown;

		// Time
		u32 TimeStamp, DeltaTime;

		// Networking
		bool LocalServerRunning;
		SingleNetworkClass *ClientSingleNetwork, *ServerSingleNetwork;
		MultiNetworkClass *MultiNetwork;

};

// Singletons
typedef SingletonClass<GameClass> Game;

#endif
