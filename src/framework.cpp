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
#include <framework.h>
#include <states/editor.h>
#include <states/play.h>
#include <states/dedicated.h>
#include <states/bots.h>
#include <ae/network.h>
#include <ae/graphics.h>
#include <ae/input.h>
#include <ae/random.h>
#include <ae/assets.h>
#include <ae/actions.h>
#include <ae/audio.h>
#include <ae/util.h>
#include <ae/framelimit.h>
#include <config.h>
#include <constants.h>
#include <menu.h>
#include <SDL.h>
#include <string>

_Framework Framework;

// Processes parameters and initializes the game
void _Framework::Init(int ArgumentCount, char **Arguments) {
	FrameLimit = nullptr;
	TimeStep = DEFAULT_TIMESTEP;
	TimeStepAccumulator = 0.0;
	RequestedState = nullptr;
	FrameworkState = INIT;
	State = &PlayState;
	Done = false;

	// Settings
	bool AudioEnabled = true;
	DedicatedState.SetNetworkPort(Config.NetworkPort);

	// Process arguments
	std::string Token;
	int TokensRemaining;
	for(int i = 1; i < ArgumentCount; i++) {
		Token = std::string(Arguments[i]);
		TokensRemaining = ArgumentCount - i - 1;
		if(Token == "-server") {
			State = &DedicatedState;
		}
		else if(Token == "-port" && TokensRemaining > 0) {
			DedicatedState.SetNetworkPort(ToNumber<uint16_t>(Arguments[++i]));
		}
		else if(Token == "-username" && TokensRemaining > 0) {
			Menu.SetUsername(Arguments[++i]);
		}
		else if(Token == "-password" && TokensRemaining > 0) {
			Menu.SetPassword(Arguments[++i]);
		}
		else if(Token == "-connect" && TokensRemaining > 1) {
			PlayState.ConnectNow = true;
			Config.LastHost = Arguments[++i];
			Config.LastPort = Arguments[++i];
		}
		else if(Token == "-hardcore") {
			PlayState.IsHardcore = true;
			DedicatedState.SetHardcore(true);
		}
		else if(Token == "-editor") {
			State = &EditorState;
			if(TokensRemaining && Arguments[i+1][0] != '-')
				EditorState.SetFilePath(Arguments[++i]);
		}
		else if(Token == "-bot" && TokensRemaining > 1) {
			State = &BotState;
			BotState.HostAddress = Arguments[++i];
			BotState.Port = ToNumber<uint16_t>(Arguments[++i]);
		}
		else if(Token == "-timescale" && TokensRemaining > 0) {
			Config.TimeScale = ToNumber<double>(Arguments[++i]);
		}
		else if(Token == "-test") {
			PlayState.IsTesting = true;
		}
		else if(Token == "-noaudio") {
			AudioEnabled = false;
		}
	}

	// Initialize network subsystem
	_Network::InitializeSystem();

	// Set random seed
	RandomGenerator.seed(SDL_GetPerformanceCounter());

	// Create frame limiter
	FrameLimit = new _FrameLimit(Config.MaxFPS, false);

	// Check state
	if(State == &DedicatedState) {
		Assets.Init(true);
	}
	else if(State != &BotState) {

		// Open log
		//PlayState.Log.Open((Config.ConfigPath + "client.log").c_str());

		// Initialize SDL
		if(SDL_Init(SDL_INIT_VIDEO) < 0)
			throw std::runtime_error("Failed to initialize SDL");

		// Initialize audio
		Audio.Init(AudioEnabled && Config.AudioEnabled);
		Audio.SetSoundVolume(Config.SoundVolume);
		Audio.SetMusicVolume(Config.MusicVolume);

		// Get window settings
		_WindowSettings WindowSettings;
		WindowSettings.WindowTitle = "choria";
		WindowSettings.Fullscreen = Config.Fullscreen;
		WindowSettings.Vsync = Config.Vsync;
		WindowSettings.Size = Config.WindowSize;
		WindowSettings.Position = glm::ivec2(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

		// Set up subsystems
		Graphics.Init(WindowSettings);
		Graphics.SetDepthTest(false);
		Graphics.SetDepthMask(false);
		Assets.Init(false);
		Graphics.SetStaticUniforms();
	}

	Timer = SDL_GetPerformanceCounter();
}

// Shuts down the game
void _Framework::Close() {

	// Close the state
	State->Close();

	// Close subsystems
	Audio.Stop();
	Assets.Close();
	Graphics.Close();
	Audio.Close();
	Config.Close();
	delete FrameLimit;

	_Network::CloseSystem();

	if(SDL_WasInit(SDL_INIT_VIDEO))
		SDL_Quit();
}

// Requests a state change
void _Framework::ChangeState(_State *RequestedState) {
	this->RequestedState = RequestedState;
	FrameworkState = CLOSE;
}

// Updates the current state and manages the state stack
void _Framework::Update() {
	double FrameTime = (SDL_GetPerformanceCounter() - Timer) / (double)SDL_GetPerformanceFrequency();
	Timer = SDL_GetPerformanceCounter();

	SDL_PumpEvents();
	Input.Update(FrameTime * Config.TimeScale);

	// Loop through events
	SDL_Event Event;
	while(SDL_PollEvent(&Event)) {
		if(State && FrameworkState == UPDATE) {
			switch(Event.type) {
				case SDL_KEYDOWN:
				case SDL_KEYUP: {
					if(!GlobalKeyHandler(Event)) {

						_KeyEvent KeyEvent("", Event.key.keysym.scancode, Event.type == SDL_KEYDOWN, Event.key.repeat);
						State->HandleKey(KeyEvent);
						if(!Event.key.repeat) {
							Actions.InputEvent(State, _Input::KEYBOARD, Event.key.keysym.scancode, Event.type == SDL_KEYDOWN);
						}
					}
				} break;
				case SDL_TEXTINPUT: {
					_KeyEvent KeyEvent(Event.text.text, 0, 1, 1);
					State->HandleKey(KeyEvent);
				} break;
				case SDL_MOUSEMOTION: {
					State->HandleMouseMove(glm::ivec2(Event.motion.xrel, Event.motion.yrel));
				} break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP: {
					_MouseEvent MouseEvent(glm::ivec2(Event.motion.x, Event.motion.y), Event.button.button, Event.type == SDL_MOUSEBUTTONDOWN);
					State->HandleMouseButton(MouseEvent);
					Actions.InputEvent(State, _Input::MOUSE_BUTTON, Event.button.button, Event.type == SDL_MOUSEBUTTONDOWN);
				} break;
				case SDL_MOUSEWHEEL: {
					State->HandleMouseWheel(Event.wheel.y);
				} break;
				case SDL_WINDOWEVENT:
					if(Event.window.event)
						State->HandleWindow(Event.window.event);
				break;
				case SDL_QUIT:
					State->HandleQuit();
				break;
			}
		}
	}

	switch(FrameworkState) {
		case INIT: {
			if(State) {
				State->Init();
				FrameworkState = UPDATE;
			}
			else
				Done = true;
		} break;
		case UPDATE: {
			TimeStepAccumulator += FrameTime * Config.TimeScale;
			while(TimeStepAccumulator >= TimeStep) {
				State->Update(TimeStep);
				TimeStepAccumulator -= TimeStep;
			}

			State->Render(TimeStepAccumulator / TimeStep * Config.TimeScale);
		} break;
		case CLOSE: {
			if(State)
				State->Close();

			State = RequestedState;
			FrameworkState = INIT;
		} break;
	}

	Audio.Update(FrameTime * Config.TimeScale);
	Graphics.Flip(FrameTime);

	if(FrameLimit)
		FrameLimit->Update();
}

// Handles global hotkeys
int _Framework::GlobalKeyHandler(const SDL_Event &Event) {

	// Handle alt-enter
	if(Event.type == SDL_KEYDOWN) {
		if((Event.key.keysym.mod & KMOD_ALT) && Event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
			if(!Event.key.repeat)
				Menu.SetFullscreen(!Config.Fullscreen);

			return 1;
		}
		else if((Event.key.keysym.mod & KMOD_CTRL) && Event.key.keysym.scancode == SDL_SCANCODE_S) {
			if(!Event.key.repeat) {
				if(Config.SoundVolume > 0.0f)
					Config.SoundVolume = 0.0f;
				else
					Config.SoundVolume = 1.0f;

				Config.Save();
				Audio.SetSoundVolume(Config.SoundVolume);
			}

			return 1;
		}
		else if((Event.key.keysym.mod & KMOD_CTRL) && Event.key.keysym.scancode == SDL_SCANCODE_M) {
			if(!Event.key.repeat) {
				if(Config.MusicVolume > 0.0f)
					Config.MusicVolume = 0.0f;
				else
					Config.MusicVolume = 1.0f;

				Config.Save();
				Audio.SetMusicVolume(Config.MusicVolume);
			}

			return 1;
		}
	}

	return 0;
}
