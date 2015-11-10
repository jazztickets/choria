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
#include <game.h>
#include <state.h>
#include <graphics.h>
#include <input.h>
#include <globals.h>
#include <random.h>
#include <objectmanager.h>
#include <config.h>
#include <stats.h>
#include <constants.h>
#include <network/singlenetwork.h>
#include <network/multinetwork.h>
#include <states/mainmenu.h>
#include <states/mapeditor.h>
#include <states/playserver.h>
#include <states/connect.h>
#include <states/account.h>

_Game Game;

using namespace irr;

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <winbase.h>
#else
	#include <unistd.h>
#endif

// Processes parameters and initializes the game
int _Game::Init(int TArgumentCount, char **TArguments) {

	WindowActive = true;
	LocalServerRunning = false;
	State = &MainMenuState;

	bool IsServer = false;
	video::E_DRIVER_TYPE DriverType = video::EDT_OPENGL;

	// Process arguments
	core::stringc Token;
	int TokensRemaining;
	for(int i = 1; i < TArgumentCount; i++) {
		Token = core::stringc(TArguments[i]);
		TokensRemaining = TArgumentCount - i - 1;
		if(Token == "-host") {
			State = &PlayServerState;
			PlayServerState.StartCommandThread();
			IsServer = true;
			DriverType = video::EDT_NULL;
		}
		else if(Token == "-mapeditor") {
			State = &MapEditorState;
		}
		else if(Token == "-connect") {
			State = &ConnectState;
		}
		else if(Token == "-login" && TokensRemaining > 1) {
			AccountState.SetLoginInfo(TArguments[i+1], TArguments[i+2]);
			i += 2;
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
	RandomGenerator.seed(irrTimer->getRealTime());

	// Initialize enet
	if(enet_initialize() == -1)
		return 0;

	// Set up networking
	MultiNetwork = new _MultiNetwork();
	ClientSingleNetwork = new _SingleNetwork();
	ServerSingleNetwork = new _SingleNetwork();
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
	NewState = nullptr;
	ManagerState = STATE_INIT;
	Done = false;

	return 1;
}

// Shuts down the game
void _Game::Close() {

	// Close the state
	State->Close();
	if(LocalServerRunning)
		PlayServerState.Close();

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
void _Game::ChangeState(_State *TState) {

	NewState = TState;
	ManagerState = STATE_FADEOUT;
}

// Updates the current state and manages the state stack
void _Game::Update() {

	// Run irrlicht engine
	if(!irrDevice->run())
		Done = true;

	// Get time difference from last frame
	LastFrameTime = (irrTimer->getTime() - TimeStamp) * 0.001f;
	TimeStamp = irrTimer->getTime();

	// Limit frame rate
	float ExtraTime = GAME_SLEEPRATE - LastFrameTime;
	if(ExtraTime > 0.0f) {
		irrDevice->sleep((uint32_t)(ExtraTime * 1000));
	}

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
			Graphics.Clear();
			ResetTimer();
			Input.ResetInputState();
			if(!State->Init()) {
				Done = true;
				return;
			}
			State->Update(LastFrameTime);

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
				PlayServerState.Update(LastFrameTime);
			else
				MultiNetwork->Update();
			State->Update(LastFrameTime);

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
void _Game::ResetTimer() {

	irrTimer->setTime(0);
	TimeStamp = irrTimer->getTime();
}

// Starts the local server
void _Game::StartLocalServer() {
	if(!LocalServerRunning) {
		LocalServerRunning = true;

		ClientNetwork = ClientSingleNetwork;
		ServerNetwork = ServerSingleNetwork;

		PlayServerState.Init();
	}
}

// Stops the local server
void _Game::StopLocalServer() {
	if(LocalServerRunning) {
		LocalServerRunning = false;

		PlayServerState.Close();

		ClientNetwork = MultiNetwork;
		ServerNetwork = MultiNetwork;
	}
}
