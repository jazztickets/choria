/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#include "game.h"
#include "state.h"
#include "graphics.h"
#include "input.h"
#include "globals.h"
#include "random.h"
#include "objectmanager.h"
#include "config.h"
#include "stats.h"
#include "../network/singlenetwork.h"
#include "../network/multinetwork.h"
#include "../mainmenu.h"
#include "../mapeditor.h"
#include "../server.h"
#include "../connect.h"
#include "../account.h"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <winbase.h>
#else
	#include <unistd.h>
#endif

GameClass Game;
RandomClass Random;

// Processes parameters and initializes the game
int GameClass::Init(int ArgumentCount, char **Arguments) {

	WindowActive = true;
	LocalServerRunning = false;
	State = &MainMenuState;

	bool IsServer = false;
	E_DRIVER_TYPE DriverType = EDT_OPENGL;

	// Process arguments
	stringc Token;
	int TokensRemaining;
	for(int i = 1; i < ArgumentCount; i++) {
		Token = stringc(Arguments[i]);
		TokensRemaining = ArgumentCount - i - 1;
		if(Token == "-host") {
			State = &ServerState;
			IsServer = true;
			DriverType = EDT_NULL;
		}
		else if(Token == "-mapeditor") {
			State = &MapEditorState;
		}
		else if(Token == "-connect") {
			State = &ConnectState;
		}
		else if(Token == "-login" && TokensRemaining > 1) {
			AccountState.SetLoginInfo(Arguments[++i], Arguments[++i]);
		}
	}

	// Initialize config system
	if(!Config.Init())
		return 0;
	Config.LoadSettings();

	// Initialize graphics system
	if(!Graphics.Init(800, 600, false, DriverType, &Input))
		return 0;

	// Initialize input system
	if(!Input.Init())
		return 0;

	// Set up the stats system
	if(!Stats.Init())
		return 0;

	// Set random seed
	Random.SetSeed(irrTimer->getRealTime());

	// Initialize enet
	if(enet_initialize() == -1)
		return 0;

	// Set up networking
	MultiNetwork = new MultiNetworkClass();
	ClientSingleNetwork = new SingleNetworkClass();
	ServerSingleNetwork = new SingleNetworkClass();
	MultiNetwork->Init(IsServer);
	ClientSingleNetwork->Init(false);
	ServerSingleNetwork->Init(false);
	if(IsServer)
		ServerNetwork = MultiNetwork;
	else
		ClientNetwork = MultiNetwork;

	// Start the state timer
	ResetTimer();

	// Set the first state
	NewState = NULL;
	ManagerState = STATE_INIT;
	Done = false;

	return 1;
}

// Shuts down the game
void GameClass::Close() {

	// Close the state
	State->Close();
	if(LocalServerRunning)
		ServerState.Close();

	MultiNetwork->WaitForDisconnect();
	Config.SaveSettings();

	// Shut down the system
	enet_deinitialize();
	MultiNetwork->Close();
	ClientSingleNetwork->Close();
	ServerSingleNetwork->Close();
	delete MultiNetwork;
	delete ClientSingleNetwork;
	delete ServerSingleNetwork;
	Stats.Close();
	Input.Close();
	Graphics.Close();
	Config.Close();
}

// Requests a state change
void GameClass::ChangeState(StateClass *State) {

	NewState = State;
	ManagerState = STATE_FADEOUT;
}

// Updates the current state and manages the state stack
void GameClass::Update() {

	// Run irrlicht engine
	if(!irrDevice->run())
		Done = true;

	// Get time difference from last frame
	FrameTime = irrTimer->getTime() - TimeStamp;
	TimeStamp = irrTimer->getTime();

	// Limit frame rate
	if(FrameTime < 15)
		Delay(15 - FrameTime);

	// Check for window activity
	PreviousWindowActive = WindowActive;
	WindowActive = irrDevice->isWindowActive();

	// Check for window focus/blur events
	if(PreviousWindowActive != WindowActive) {
		Input.ResetInputState();
	}

	// Update the current state
	switch(ManagerState) {
		case STATE_INIT:
			ResetGraphics();
			ResetTimer();
			Input.ResetInputState();
			if(!State->Init()) {
				Done = true;
				return;
			}
			State->Update(FrameTime);

			// Draw
			Graphics.BeginFrame();
			State->Draw();
			Graphics.EndFrame();

			ManagerState = STATE_FADEIN;
		break;
		case STATE_FADEIN:
			ManagerState = STATE_UPDATE;
		break;
		case STATE_UPDATE:
			if(LocalServerRunning)
				ServerState.Update(FrameTime);
			else
				MultiNetwork->Update();
			State->Update(FrameTime);

			// Draw
			Graphics.BeginFrame();
			State->Draw();
			Graphics.EndFrame();
		break;
		case STATE_FADEOUT:
			ManagerState = STATE_CLOSE;
		break;
		case STATE_CLOSE:
			State->Close();
			State = NewState;
			ManagerState = STATE_INIT;
		break;
	}

}

// Resets the game timer
void GameClass::ResetTimer() {

	irrTimer->setTime(0);
	TimeStamp = irrTimer->getTime();
}

// Resets the graphics for a state
void GameClass::ResetGraphics() {
	Graphics.Clear();
}

// Delays execution of the program
void GameClass::Delay(int Time) {
	#ifdef _WIN32
		Sleep(Time);
	#else
		usleep(Time * 1000);
	#endif
}

// Starts the local server
void GameClass::StartLocalServer() {
	if(!LocalServerRunning) {
		LocalServerRunning = true;

		ClientNetwork = ClientSingleNetwork;
		ServerNetwork = ServerSingleNetwork;

		ServerState.Init();
	}
}

// Stops the local server
void GameClass::StopLocalServer() {
	if(LocalServerRunning) {
		LocalServerRunning = false;

		ServerState.Close();

		ClientNetwork = MultiNetwork;
		ServerNetwork = MultiNetwork;
	}
}
