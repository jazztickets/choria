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
#include <config.h>
#include <files.h>
#include <constants.h>
#include <actions.h>
#include <input.h>
#include <sstream>
#include <fstream>
#include <SDL_filesystem.h>

// Globals
_Config Config;

// Initializes the config system
void _Config::Init(const std::string &ConfigFile) {

	// Create config path
	char *PrefPath = SDL_GetPrefPath("", "choria");
	if(PrefPath) {
		ConfigPath = PrefPath;
		SDL_free(PrefPath);
	}
	else {
		throw std::runtime_error("Cannot create config path!");
	}

	this->ConfigFile = ConfigPath + ConfigFile;

	// Load defaults
	SetDefaults();

	// Load config
	Load();
}

// Closes the config system
void _Config::Close() {
	Save();
}

// Set defaults
void _Config::SetDefaults() {

	// Gets set after SDL
	FullscreenSize = glm::ivec2(0, 0);

	// Set defaults
	WindowSize = DEFAULT_WINDOW_SIZE;
	MSAA = 0;
	Anisotropy = DEFAULT_ANISOTROPY;
	Fullscreen = DEFAULT_FULLSCREEN;
	Vsync = DEFAULT_VSYNC;
	MaxFPS = DEFAULT_MAXFPS;
	AudioEnabled = DEFAULT_AUDIOENABLED;

	SoundVolume = 1.0f;
	MusicVolume = 1.0f;

	FakeLag = 0.0f;
	NetworkRate = DEFAULT_NETWORKRATE;
	NetworkPort = DEFAULT_NETWORKPORT;

	LoadDefaultInputBindings();
}

// Load default key bindings
void _Config::LoadDefaultInputBindings() {

	// Clear mappings
	for(int i = 0; i < _Input::INPUT_COUNT; i++)
		Actions.ClearMappings(i);

/*	Actions.AddInputMap(_Input::KEYBOARD, DEFAULT_KEYUP, _Actions::UP);
	Actions.AddInputMap(_Input::KEYBOARD, DEFAULT_KEYDOWN, _Actions::DOWN);
	Actions.AddInputMap(_Input::KEYBOARD, DEFAULT_KEYLEFT, _Actions::LEFT);
	Actions.AddInputMap(_Input::KEYBOARD, DEFAULT_KEYRIGHT, _Actions::RIGHT);
	*/
}

// Use SDL to determine desktop size
void _Config::SetDefaultFullscreenSize() {
	if(Config.FullscreenSize != glm::ivec2(0))
		return;

	SDL_DisplayMode DisplayMode;
	if(SDL_GetDesktopDisplayMode(0, &DisplayMode) != 0)
		Config.FullscreenSize = DEFAULT_WINDOW_SIZE;
	else
		Config.FullscreenSize = glm::ivec2(DisplayMode.w, DisplayMode.h);
}

// Load the config file
void _Config::Load() {

	// Open file
	std::ifstream File(ConfigFile.c_str());
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

				Map[Field] = Value;
			}
		}
	}
	File.close();

	// Read config
	GetValue("window_width", WindowSize.x);
	GetValue("window_height", WindowSize.y);
	GetValue("fullscreen_width", FullscreenSize.x);
	GetValue("fullscreen_height", FullscreenSize.y);
	GetValue("fullscreen", Fullscreen);
	GetValue("vsync", Vsync);
	GetValue("max_fps", MaxFPS);
	GetValue("anisotropy", Anisotropy);
	GetValue("msaa", MSAA);
	GetValue("audio_enabled", AudioEnabled);
	GetValue("sound_volume", SoundVolume);
	GetValue("music_volume", MusicVolume);
	GetValue("fake_lag", FakeLag);
	GetValue("network_rate", NetworkRate);
	GetValue("network_port", NetworkPort);

	if(NetworkRate < 0.01)
		NetworkRate = 0.01;

	// Load bindings
	for(int i = 0; i < _Actions::COUNT; i++) {
		std::ostringstream Buffer;
		Buffer << "action_" << i;

		// Get input key/button
		std::string InputString;
		GetValue(Buffer.str(), InputString);

		// Clear out current map
		Actions.ClearAllMappingsForAction(i);

		// Skip empty
		if(!InputString.size())
			continue;

		// Parse input bind
		int InputType, Input;
		char Dummy;
		std::stringstream Stream(InputString);
		Stream >> InputType >> Dummy >> Input;
		Actions.AddInputMap(InputType, Input, i);
	}
}

// Save variables to the config file
void _Config::Save() {

	std::ofstream File(ConfigFile.c_str());
	if(!File.is_open()) {
		return;
	}

	// Write variables
	File << "window_width=" << WindowSize.x << std::endl;
	File << "window_height=" << WindowSize.y << std::endl;
	File << "fullscreen_width=" << FullscreenSize.x << std::endl;
	File << "fullscreen_height=" << FullscreenSize.y << std::endl;
	File << "fullscreen=" << Fullscreen << std::endl;
	File << "vsync=" << Vsync << std::endl;
	File << "max_fps=" << MaxFPS << std::endl;
	File << "msaa=" << MSAA << std::endl;
	File << "anisotropy=" << Anisotropy << std::endl;
	File << "audio_enabled=" << AudioEnabled << std::endl;
	File << "sound_volume=" << SoundVolume << std::endl;
	File << "music_volume=" << MusicVolume << std::endl;
	File << "fake_lag=" << FakeLag << std::endl;
	File << "network_rate=" << NetworkRate << std::endl;
	File << "network_port=" << NetworkPort << std::endl;

	// Write out input map
	for(int i = 0; i < _Actions::COUNT; i++) {
		File << "action_" << i << "=";
		for(int j = 0; j < _Input::INPUT_COUNT; j++) {
			int Input = Actions.GetInputForAction(j, i);
			if(Input != -1) {
				File << j << "_" << Input;
				break;
			}
		}
		File << std::endl;
	}

	File.close();
}
