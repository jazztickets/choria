/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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
#include <states/test.h>
#include <states/benchmark.h>
#include <ae/network.h>
#include <ae/graphics.h>
#include <ae/input.h>
#include <ae/random.h>
#include <ae/assets.h>
#include <ae/texture_array.h>
#include <ae/tilemap.h>
#include <ae/actions.h>
#include <ae/audio.h>
#include <ae/util.h>
#include <ae/ui.h>
#include <ae/console.h>
#include <ae/framelimit.h>
#include <objects/map.h>
#include <config.h>
#include <constants.h>
#include <menu.h>
#include <SDL.h>
#include <string>
#include <algorithm>

_Framework Framework;

// Processes parameters and initializes the game
void _Framework::Init(int ArgumentCount, char **Arguments) {
	Console = nullptr;
	FrameLimit = nullptr;
	TimeStep = DEFAULT_TIMESTEP;
	TimeStepAccumulator = 0.0;
	RequestedState = nullptr;
	FrameworkState = INIT;
	State = &PlayState;
	Done = false;
	IgnoreNextInputEvent = false;

	// Settings
	bool LoadClientAssets = true;
	bool AudioEnabled = true;
	DedicatedState.SetNetworkPort(Config.NetworkPort);

	// Process arguments
	std::string Token;
	int TokensRemaining;
	for(int i = 1; i < ArgumentCount; i++) {
		Token = std::string(Arguments[i]);
		TokensRemaining = ArgumentCount - i - 1;
		if(Token == "-server") {
			if(!Config.MaxFPS)
				Config.MaxFPS = DEFAULT_MAXFPS;
			State = &DedicatedState;
			LoadClientAssets = false;
		}
		else if(Token == "-port" && TokensRemaining > 0) {
			DedicatedState.SetNetworkPort(ae::ToNumber<uint16_t>(Arguments[++i]));
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
			BotState.Port = ae::ToNumber<uint16_t>(Arguments[++i]);
			LoadClientAssets = false;
		}
		else if(Token == "-timescale" && TokensRemaining > 0) {
			Config.TimeScale = ae::ToNumber<double>(Arguments[++i]);
		}
		else if(Token == "-dev") {
			PlayState.DevMode = true;
		}
		else if(Token == "-test") {
			State = &TestState;
		}
		else if(Token == "-benchmark") {
			State = &BenchmarkState;
			Config.Vsync = false;
			Config.MaxFPS = 0;
		}
		else if(Token == "-noaudio") {
			AudioEnabled = false;
		}
	}

	// Initialize network subsystem
	ae::_Network::InitializeSystem();

	// Set random seed
	ae::RandomGenerator.seed(SDL_GetPerformanceCounter());

	// Create frame limiter
	FrameLimit = new ae::_FrameLimit(Config.MaxFPS, false);

	// Load client assets
	if(LoadClientAssets) {

		// Open log
		//PlayState.Log.Open((Config.ConfigPath + "client.log").c_str());

		// Initialize SDL
		if(SDL_Init(SDL_INIT_VIDEO) < 0)
			throw std::runtime_error("Failed to initialize SDL");

		// Initialize audio
		ae::Audio.Init(AudioEnabled && Config.AudioEnabled);
		ae::Audio.SetSoundVolume(Config.SoundVolume);
		ae::Audio.SetMusicVolume(Config.MusicVolume);

		// Get window settings
		ae::_WindowSettings WindowSettings;
		WindowSettings.WindowTitle = "choria";
		WindowSettings.Fullscreen = Config.Fullscreen;
		WindowSettings.Vsync = Config.Vsync;
		WindowSettings.Size = Config.WindowSize;
		WindowSettings.Position = glm::ivec2(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

		// Set up subsystems
		ae::Graphics.Init(WindowSettings);
		ae::Graphics.SetDepthTest(false);
		ae::Graphics.SetDepthMask(false);
		LoadAssets();
		ae::Graphics.SetStaticUniforms();

		// Setup console
		Console = new ae::_Console(ae::Assets.Programs["ortho_pos"], ae::Assets.Fonts["console"]);
		Console->CommandList.push_back("maxfps");
		Console->CommandList.push_back("quit");
		Console->CommandList.push_back("volume");
		Console->CommandList.push_back("vsync");

		// Add dev mode commands
		if(PlayState.DevMode) {
			Console->CommandList.push_back("battle");
			Console->CommandList.push_back("bounty");
			Console->CommandList.push_back("clock");
			Console->CommandList.push_back("event");
			Console->CommandList.push_back("experience");
			Console->CommandList.push_back("give");
			Console->CommandList.push_back("gold");
			Console->CommandList.push_back("map");
			Console->CommandList.push_back("move");
		}

		// Sort commands
		std::sort(Console->CommandList.begin(), Console->CommandList.end());
	}

	Timer = SDL_GetPerformanceCounter();
}

// Shuts down the game
void _Framework::Close() {

	// Close the state
	State->Close();

	// Close subsystems
	ae::Audio.Stop();
	ae::Assets.Close();
	ae::Graphics.Close();
	ae::Audio.Close();
	Config.Close();
	delete Console;
	delete FrameLimit;

	ae::_Network::CloseSystem();

	if(SDL_WasInit(SDL_INIT_VIDEO))
		SDL_Quit();
}

// Requests a state change
void _Framework::ChangeState(ae::_State *RequestedState) {
	this->RequestedState = RequestedState;
	FrameworkState = CLOSE;
}

// Updates the current state
void _Framework::Update() {

	// Get frame time
	double FrameTime = (SDL_GetPerformanceCounter() - Timer) / (double)SDL_GetPerformanceFrequency();
	Timer = SDL_GetPerformanceCounter();

	// Get events from SDL
	SDL_PumpEvents();
	ae::Input.Update(FrameTime * Config.TimeScale);

	// Loop through events
	SDL_Event Event;
	while(SDL_PollEvent(&Event)) {
		if(State && FrameworkState == UPDATE) {
			switch(Event.type) {
				case SDL_KEYDOWN:
				case SDL_KEYUP: {
					if(!GlobalKeyHandler(Event)) {
						ae::_KeyEvent KeyEvent("", Event.key.keysym.scancode, Event.type == SDL_KEYDOWN, Event.key.repeat);

						// Handle console input
						bool SendAction = true;
						if(Console->IsOpen())
							ae::Graphics.Element->HandleKey(KeyEvent);
						else
							SendAction = State->HandleKey(KeyEvent);

						// Pass keys to action handler
						if(!Event.key.repeat && SendAction)
							ae::Actions.InputEvent(State, ae::_Input::KEYBOARD, Event.key.keysym.scancode, Event.type == SDL_KEYDOWN);
					}
				} break;
				case SDL_TEXTINPUT: {
					if(!IgnoreNextInputEvent) {

						ae::_KeyEvent KeyEvent(Event.text.text, 0, 1, 1);
						if(Console->IsOpen())
							ae::Graphics.Element->HandleKey(KeyEvent);
						else
							State->HandleKey(KeyEvent);
					}

					IgnoreNextInputEvent = false;
				} break;
				case SDL_MOUSEMOTION: {
					if(!Console->IsOpen())
						State->HandleMouseMove(glm::ivec2(Event.motion.xrel, Event.motion.yrel));
				} break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP: {
					if(!Console->IsOpen()) {
						ae::_MouseEvent MouseEvent(glm::ivec2(Event.motion.x, Event.motion.y), Event.button.button, Event.type == SDL_MOUSEBUTTONDOWN);
						State->HandleMouseButton(MouseEvent);
						ae::Actions.InputEvent(State, ae::_Input::MOUSE_BUTTON, Event.button.button, Event.type == SDL_MOUSEBUTTONDOWN);
					}
				} break;
				case SDL_MOUSEWHEEL: {
					if(!Console->IsOpen())
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
				if(Console) {
					Console->Update(TimeStep);
					if(!Console->Command.empty()) {
						bool Handled = State->HandleCommand(Console);
						if(!Handled)
							HandleCommand(Console);
						Console->Command = "";
					}
				}
				TimeStepAccumulator -= TimeStep;
			}

			double BlendFactor = TimeStepAccumulator / TimeStep * Config.TimeScale;
			State->Render(BlendFactor);
			if(Console)
				Console->Render(BlendFactor);
		} break;
		case CLOSE: {
			if(State)
				State->Close();

			State = RequestedState;
			FrameworkState = INIT;
		} break;
	}

	ae::Audio.Update(FrameTime * Config.TimeScale);
	ae::Graphics.Flip(FrameTime);

	if(FrameLimit)
		FrameLimit->Update();
}

// Handles global hotkeys
int _Framework::GlobalKeyHandler(const SDL_Event &Event) {

	if(Event.type == SDL_KEYDOWN) {

		// Handle alt-enter
		if((Event.key.keysym.mod & KMOD_ALT) && (Event.key.keysym.scancode == SDL_SCANCODE_RETURN || Event.key.keysym.scancode == SDL_SCANCODE_KP_ENTER)) {
			if(!Event.key.repeat) {
				Menu.SetFullscreen(!Config.Fullscreen);
				if(Console)
					Console->UpdateSize();
			}

			return 1;
		}
		// Ctrl-s
		else if((Event.key.keysym.mod & KMOD_CTRL) && Event.key.keysym.scancode == SDL_SCANCODE_S) {
			if(!Event.key.repeat) {
				if(Config.SoundVolume > 0.0f)
					Config.SoundVolume = 0.0f;
				else
					Config.SoundVolume = 1.0f;

				Config.Save();
				ae::Audio.SetSoundVolume(Config.SoundVolume);
			}

			return 1;
		}
		// Ctrl-m
		else if((Event.key.keysym.mod & KMOD_CTRL) && Event.key.keysym.scancode == SDL_SCANCODE_M) {
			if(!Event.key.repeat) {
				if(Config.MusicVolume > 0.0f)
					Config.MusicVolume = 0.0f;
				else
					Config.MusicVolume = 1.0f;

				Config.Save();
				ae::Audio.SetMusicVolume(Config.MusicVolume);
			}

			return 1;
		}
	}

	return 0;
}

// Handle generic console command
void _Framework::HandleCommand(ae::_Console *Console) {

	// Get parameters
	std::vector<std::string> Parameters;
	ae::TokenizeString(Console->Parameters, Parameters);

	if(Console->Command == "maxfps") {
		if(Parameters.size() == 1) {
			Config.MaxFPS = ae::ToNumber<int>(Parameters[0]);
			Framework.FrameLimit->FrameRate = Config.MaxFPS;
			Config.Save();
		}
		else {
			Console->AddMessage("maxfps = " + std::to_string(Config.MaxFPS));
			Console->AddMessage("usage: maxfps [value]");
		}
	}
	else if(Console->Command == "volume") {
		if(Console->Parameters.size() == 1) {
			Config.SoundVolume = Config.MusicVolume = glm::clamp(ae::ToNumber<float>(Console->Parameters), 0.0f, 1.0f);
			ae::Audio.SetSoundVolume(Config.SoundVolume);
			ae::Audio.SetMusicVolume(Config.MusicVolume);
			Config.Save();
		}
		else
			Console->AddMessage("usage: volume [value]");
	}
	else if(Console->Command == "vsync") {
		if(Console->Parameters.size() == 1) {
			Config.Vsync = ae::ToNumber<bool>(Console->Parameters);
			ae::Graphics.SetVsync(Config.Vsync);
			Config.Save();
		}
		else {
			Console->AddMessage("vsync = " + std::to_string(ae::Graphics.GetVsync()));
			Console->AddMessage("usage: vsync [value]");
		}
	}
	else {
		Console->AddMessage("Command \"" + Console->Command + "\" not found");
	}
}

// Load assets
void _Framework::LoadAssets() {

	// Load textures
	ae::Assets.LoadTextureDirectory("textures/battle/");
	ae::Assets.LoadTextureDirectory("textures/buffs/");
	ae::Assets.LoadTextureDirectory("textures/builds/");
	ae::Assets.LoadTextureDirectory("textures/editor/");
	ae::Assets.LoadTextureDirectory("textures/hud/");
	ae::Assets.LoadTextureDirectory("textures/hud_repeat/", false, true);
	ae::Assets.LoadTextureDirectory("textures/interface/");
	ae::Assets.LoadTextureDirectory("textures/items/");
	ae::Assets.LoadTextureDirectory("textures/lights/");
	ae::Assets.LoadTextureDirectory("textures/map/");
	ae::Assets.LoadTextureDirectory("textures/menu/");
	ae::Assets.LoadTextureDirectory("textures/minigames/");
	ae::Assets.LoadTextureDirectory("textures/monsters/");
	ae::Assets.LoadTextureDirectory("textures/portraits/");
	ae::Assets.LoadTextureDirectory("textures/models/");
	ae::Assets.LoadTextureDirectory("textures/skills/");
	ae::Assets.LoadTextureDirectory("textures/status/");

	// Load texture array
	ae::Assets.TextureArrays["default"] = new ae::_TextureArray(glm::ivec2(MAP_TILE_WIDTH, MAP_TILE_HEIGHT), 100);
	ae::Assets.TextureArrays["default"]->AddTexture("textures/tiles/default/none.png");
	ae::Assets.TextureArrays["default"]->AddTexture("textures/tiles/default/grass0.png");
	ae::Assets.TextureArrays["default"]->AddTexture("textures/tiles/default/sand0.png");
	ae::Assets.TextureArrays["default"]->AddTexture("textures/tiles/default/water0.png");
	ae::Assets.TextureArrays["trans"] = new ae::_TextureArray(glm::ivec2(MAP_TILE_WIDTH, MAP_TILE_HEIGHT), 32);
	for(int i = 0; i < 32; i++) {
		std::string Filename = i < 10 ? "0" + std::to_string(i) : std::to_string(i);
		ae::Assets.TextureArrays["trans"]->AddTexture("textures/tiles/trans/" + Filename + ".png");
	}

	// Load tables
	ae::Assets.LoadPrograms("shaders/programs.tsv");
	ae::Assets.LoadColors("ui/colors.tsv");
	ae::Assets.LoadStyles("ui/styles.tsv");
	ae::Assets.LoadSounds("sounds/");
	ae::Assets.LoadMusic("music/");

	// Load tile maps
	ae::Assets.TileMaps["default"] = new ae::_TileMap("data/tiles.tsv");

	// Load font names only
	ae::Assets.LoadFonts("ui/fonts.tsv", false);

	// Load UI
	ae::Assets.LoadUI("ui/ui.xml");
	//ae::Assets.SaveUI("ui/ui_new.xml");

	// Load font files
	ae::Assets.LoadFonts("ui/fonts.tsv");
}
