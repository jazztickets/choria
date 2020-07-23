/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020 Alan Witkowski
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
#include <config.h>
#include <ae/files.h>
#include <ae/actions.h>
#include <ae/input.h>
#include <ae/util.h>
#include <constants.h>
#include <actiontype.h>
#include <sstream>
#include <fstream>
#include <SDL_filesystem.h>
#include <SDL_video.h>

// Globals
_Config Config;

// Initializes the config system
void _Config::Init(const std::string &ConfigFile) {

	// Set names for actions
	ae::Actions.State.resize(Action::COUNT);
	ae::Actions.ResetState();
	ae::Actions.State[Action::GAME_LEFT].Name = "game_left";
	ae::Actions.State[Action::GAME_RIGHT].Name = "game_right";
	ae::Actions.State[Action::GAME_UP].Name = "game_up";
	ae::Actions.State[Action::GAME_DOWN].Name = "game_down";
	ae::Actions.State[Action::GAME_JOIN].Name = "game_join";
	ae::Actions.State[Action::GAME_INVENTORY].Name = "game_inventory";
	ae::Actions.State[Action::GAME_SKILLS].Name = "game_skills";
	ae::Actions.State[Action::GAME_TRADE].Name = "game_trade";
	ae::Actions.State[Action::GAME_PARTY].Name = "game_party";
	ae::Actions.State[Action::GAME_CHAT].Name = "game_chat";
	ae::Actions.State[Action::GAME_USE].Name = "game_use";
	ae::Actions.State[Action::GAME_SKILL1].Name = "game_skill1";
	ae::Actions.State[Action::GAME_SKILL2].Name = "game_skill2";
	ae::Actions.State[Action::GAME_SKILL3].Name = "game_skill3";
	ae::Actions.State[Action::GAME_SKILL4].Name = "game_skill4";
	ae::Actions.State[Action::GAME_SKILL5].Name = "game_skill5";
	ae::Actions.State[Action::GAME_SKILL6].Name = "game_skill6";
	ae::Actions.State[Action::GAME_SKILL7].Name = "game_skill7";
	ae::Actions.State[Action::GAME_SKILL8].Name = "game_skill8";
	ae::Actions.State[Action::MENU_LEFT].Name = "menu_left";
	ae::Actions.State[Action::MENU_RIGHT].Name = "menu_right";
	ae::Actions.State[Action::MENU_UP].Name = "menu_up";
	ae::Actions.State[Action::MENU_DOWN].Name = "menu_down";
	ae::Actions.State[Action::MENU_GO].Name = "menu_go";
	ae::Actions.State[Action::MENU_BACK].Name = "menu_back";
	ae::Actions.State[Action::MENU_PAUSE].Name = "menu_pause";
	ae::Actions.State[Action::MISC_CONSOLE].Name = "misc_console";
	ae::Actions.State[Action::MISC_DEBUG].Name = "misc_debug";

	// Create config path
	char *PrefPath = SDL_GetPrefPath("", "choria");
	if(PrefPath) {
		ConfigPath = PrefPath;
		SDL_free(PrefPath);
	}
	else {
		throw std::runtime_error("Cannot create config path!");
	}

	ConfigFilePath = ConfigPath + ConfigFile;
	LogPath = ConfigPath + "log/";
	ae::MakeDirectory(LogPath);

	// Load defaults
	SetDefaults();

	// Load config
	Load();
}

// Closes the config system
void _Config::Close() {
}

// Set defaults
void _Config::SetDefaults() {

	// Set defaults
	Version = DEFAULT_CONFIG_VERSION;
	TimeScale = 1.0;
	AutoSavePeriod = DEFAULT_AUTOSAVE_PERIOD;
	WindowSize = DEFAULT_WINDOW_SIZE;
	Fullscreen = DEFAULT_FULLSCREEN;
	Vsync = DEFAULT_VSYNC;
	MaxFPS = DEFAULT_MAXFPS;
	AudioEnabled = DEFAULT_AUDIOENABLED;
	SoundVolume = 1.0f;
	MusicVolume = 1.0f;
	MaxClients = DEFAULT_MAXCLIENTS;
	FakeLag = 0.0;
	NetworkRate = DEFAULT_NETWORKRATE;
	NetworkPort = DEFAULT_NETWORKPORT;
	ShowTutorial = 1;
	LastHost = "127.0.0.1";
	LastPort = std::to_string(DEFAULT_NETWORKPORT);

	LoadDefaultInputBindings(false);
}

// Load default key bindings
void _Config::LoadDefaultInputBindings(bool IfNone) {

	// Clear mappings
	if(!IfNone) {
		for(int i = 0; i < ae::_Input::INPUT_COUNT; i++)
			ae::Actions.ClearMappings(i);
	}

	// Movement
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_E, Action::GAME_UP, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(1, ae::_Input::KEYBOARD, SDL_SCANCODE_UP, Action::GAME_UP, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_D, Action::GAME_DOWN, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(1, ae::_Input::KEYBOARD, SDL_SCANCODE_DOWN, Action::GAME_DOWN, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_S, Action::GAME_LEFT, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(1, ae::_Input::KEYBOARD, SDL_SCANCODE_LEFT, Action::GAME_LEFT, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_F, Action::GAME_RIGHT, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(1, ae::_Input::KEYBOARD, SDL_SCANCODE_RIGHT, Action::GAME_RIGHT, 1.0f, -1.0f, IfNone);

	// Game
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_G, Action::GAME_JOIN, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_C, Action::GAME_INVENTORY, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_R, Action::GAME_SKILLS, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_T, Action::GAME_TRADE, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_P, Action::GAME_PARTY, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_RETURN, Action::GAME_CHAT, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(1, ae::_Input::KEYBOARD, SDL_SCANCODE_KP_ENTER, Action::GAME_CHAT, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_SPACE, Action::GAME_USE, 1.0f, -1.0f, IfNone);

	// Skills
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_1, Action::GAME_SKILL1, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_2, Action::GAME_SKILL2, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_3, Action::GAME_SKILL3, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_4, Action::GAME_SKILL4, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_5, Action::GAME_SKILL5, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_6, Action::GAME_SKILL6, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_7, Action::GAME_SKILL7, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_8, Action::GAME_SKILL8, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(1, ae::_Input::KEYBOARD, SDL_SCANCODE_KP_1, Action::GAME_SKILL1, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(1, ae::_Input::KEYBOARD, SDL_SCANCODE_KP_2, Action::GAME_SKILL2, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(1, ae::_Input::KEYBOARD, SDL_SCANCODE_KP_3, Action::GAME_SKILL3, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(1, ae::_Input::KEYBOARD, SDL_SCANCODE_KP_4, Action::GAME_SKILL4, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(1, ae::_Input::KEYBOARD, SDL_SCANCODE_KP_5, Action::GAME_SKILL5, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(1, ae::_Input::KEYBOARD, SDL_SCANCODE_KP_6, Action::GAME_SKILL6, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(1, ae::_Input::KEYBOARD, SDL_SCANCODE_KP_7, Action::GAME_SKILL7, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(1, ae::_Input::KEYBOARD, SDL_SCANCODE_KP_8, Action::GAME_SKILL8, 1.0f, -1.0f, IfNone);

	// Menu
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_UP, Action::MENU_UP, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_DOWN, Action::MENU_DOWN, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(1, ae::_Input::KEYBOARD, SDL_SCANCODE_TAB, Action::MENU_DOWN, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_LEFT, Action::MENU_LEFT, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_RIGHT, Action::MENU_RIGHT, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_RETURN, Action::MENU_GO, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_ESCAPE, Action::MENU_BACK, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_F1, Action::MENU_PAUSE, 1.0f, -1.0f, IfNone);

	// Misc
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_GRAVE, Action::MISC_CONSOLE, 1.0f, -1.0f, IfNone);
	ae::Actions.AddInputMap(0, ae::_Input::KEYBOARD, SDL_SCANCODE_F2, Action::MISC_DEBUG, 1.0f, -1.0f, IfNone);
}

// Load the config file
void _Config::Load() {

	// Open file
	std::ifstream File(ConfigFilePath.c_str());
	if(!File) {
		Save();
		return;
	}

	// Read data into map
	Map.clear();
	char Buffer[256];
	while(File) {
		File.getline(Buffer, 256);
		if(File.good()) {
			std::string Line(Buffer);
			std::size_t Pos = Line.find_first_of('=');
			if(Pos != std::string::npos) {
				std::string Field = Line.substr(0, Pos);
				std::string Value = Line.substr(Pos+1, Line.size());

				Map[Field].push_back(Value);
			}
		}
	}

	// Close
	File.close();

	// Read version
	int ReadVersion = 0;
	GetValue("version", ReadVersion);
	if(ReadVersion != Version) {
		std::rename(ConfigFilePath.c_str(), (ConfigFilePath + "." + std::to_string(ReadVersion)).c_str());
		Save();
		return;
	}

	// Read config
	GetValue("window_width", WindowSize.x);
	GetValue("window_height", WindowSize.y);
	GetValue("fullscreen", Fullscreen);
	GetValue("vsync", Vsync);
	GetValue("max_fps", MaxFPS);
	GetValue("audio_enabled", AudioEnabled);
	GetValue("sound_volume", SoundVolume);
	GetValue("music_volume", MusicVolume);
	GetValue("fake_lag", FakeLag);
	GetValue("max_clients", MaxClients);
	GetValue("network_rate", NetworkRate);
	GetValue("network_port", NetworkPort);
	GetValue("showtutorial", ShowTutorial);
	GetValue("last_host", LastHost);
	GetValue("last_port", LastPort);
	GetValue("autosave_period", AutoSavePeriod);

	if(NetworkRate < 0.01)
		NetworkRate = 0.01;

	// Clear bindings
	for(int i = 0; i < ae::_Input::INPUT_COUNT; i++)
		ae::Actions.ClearMappings(i);

	// Load bindings
	for(size_t i = 0; i < ae::Actions.State.size(); i++) {
		std::ostringstream Buffer;
		Buffer << "action_" << ae::Actions.State[i].Name;

		// Get list of inputs for each action
		const auto &Values = Map[Buffer.str()];
		for(auto &Iterator : Values) {

			// Parse input bind
			int Rank;
			int InputType;
			int Input;
			char Dummy;
			std::stringstream Stream(Iterator);
			Stream >> Rank >> Dummy >> InputType >> Dummy >> Input;
			ae::Actions.AddInputMap(Rank, InputType, Input, i, 1.0f, -1.0f, false);
		}
	}

	// Add missing bindings
	LoadDefaultInputBindings(true);
}

// Save variables to the config file
void _Config::Save() {

	// Open file
	std::ofstream File(ConfigFilePath.c_str());
	if(!File.is_open())
		return;

	// Write variables
	File << "version=" << Version << std::endl;
	File << "window_width=" << WindowSize.x << std::endl;
	File << "window_height=" << WindowSize.y << std::endl;
	File << "fullscreen=" << Fullscreen << std::endl;
	File << "vsync=" << Vsync << std::endl;
	File << "max_fps=" << MaxFPS << std::endl;
	File << "audio_enabled=" << AudioEnabled << std::endl;
	File << "sound_volume=" << SoundVolume << std::endl;
	File << "music_volume=" << MusicVolume << std::endl;
	File << "fake_lag=" << FakeLag << std::endl;
	File << "max_clients=" << MaxClients << std::endl;
	File << "network_rate=" << NetworkRate << std::endl;
	File << "network_port=" << NetworkPort << std::endl;
	File << "showtutorial=" << ShowTutorial << std::endl;
	File << "last_host=" << LastHost << std::endl;
	File << "last_port=" << LastPort << std::endl;
	File << "autosave_period=" << AutoSavePeriod << std::endl;

	// Write out input map
	ae::Actions.Serialize(File, ae::_Input::KEYBOARD);
	ae::Actions.Serialize(File, ae::_Input::MOUSE_AXIS);
	ae::Actions.Serialize(File, ae::_Input::MOUSE_BUTTON);

	File.close();
}
