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
#pragma once

// Libraries
#include <glm/vec2.hpp>
#include <unordered_map>
#include <sstream>
#include <string>
#include <list>

// Load/save config file
class _Config {

	public:

		void Init(const std::string &ConfigFile);
		void Close();

		void Load();
		void Save();
		void SetDefaults();
		void LoadDefaultInputBindings(bool IfNone);

		// State
		std::string ConfigPath;
		std::string LogPath;
		std::string LogDataPath;
		int Version;

		// Gameplay
		double TimeScale;
		double AutoSavePeriod;
		bool RightClickSell;
		bool HighlightTarget;

		// Graphics
		glm::ivec2 WindowSize;
		double MaxFPS;
		bool Vsync;
		bool Fullscreen;

		// Audio
		bool AudioEnabled;
		float SoundVolume;
		float MusicVolume;

		// Networking
		size_t MaxClients;
		double FakeLag;
		double NetworkRate;
		bool Offline;
		uint16_t NetworkPort;

		// Editor
		std::string BrowserCommand;
		std::string DesignToolURL;

		// Misc
		bool ShowTutorial;
		std::string LastUsername;
		std::string LastHost;
		std::string LastPort;

	private:

		template <typename Type>
		void GetValue(const std::string &Field, Type &Value) {
			const auto &MapIterator = Map.find(Field);
			if(MapIterator != Map.end()) {
				std::stringstream Stream(MapIterator->second.front());
				Stream >> Value;
			}
		}

		// State
		std::string ConfigFilePath;
		std::unordered_map<std::string, std::list<std::string>> Map;
};

extern _Config Config;
