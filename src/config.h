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
#include <glm/vec2.hpp>
#include <string>
#include <sstream>
#include <unordered_map>

// Load/save config file
class _Config {

	public:

		void Init(const std::string &ConfigFile);
		void Close();

		void Load();
		void Save();
		void SetDefaults();
		void LoadDefaultInputBindings();
		void SetDefaultFullscreenSize();

		// State
		std::string ConfigPath;

		// Graphics
		glm::ivec2 WindowSize;
		glm::ivec2 FullscreenSize;
		int MaxFPS;
		int Vsync;
		int MSAA;
		int Anisotropy;
		int Fullscreen;

		// Audio
		int AudioEnabled;
		float SoundVolume;
		float MusicVolume;

		// Networking
		double FakeLag;
		double NetworkRate;
		uint16_t NetworkPort;

		std::string LastHost;
		std::string LastPort;

	private:

		template <typename Type>
		void GetValue(const std::string &Field, Type &Value) {
			auto MapIterator = Map.find(Field);
			if(MapIterator != Map.end()) {
				std::stringstream Stream(MapIterator->second);
				Stream >> Value;
			}
		}

		// State
		std::string ConfigFile;
		std::unordered_map<std::string, std::string> Map;
};

extern _Config Config;
