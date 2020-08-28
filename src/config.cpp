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
#include <config.h>
#include <fstream>

_Config Config;

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <sys/stat.h>
#endif

using namespace irr;

// Constructor
int _Config::Init() {

	#ifdef _WIN32
		SavePath = core::stringc(getenv("APPDATA")) + core::stringc("/choria/");
		SaveMapPath = core::stringc(getenv("APPDATA")) + core::stringc("/choria/maps/");
		CreateDirectory(SavePath.c_str(), nullptr);
		CreateDirectory(SaveMapPath.c_str(), nullptr);
	#else
		SavePath = core::stringc(getenv("HOME")) + core::stringc("/.local/");
		mkdir(SavePath.c_str(), 0755);
		SavePath += "share/";
		mkdir(SavePath.c_str(), 0755);
		SavePath += "choria_classic/";
		mkdir(SavePath.c_str(), 0755);

		SaveMapPath = SavePath + core::stringc("maps/");
		mkdir(SavePath.c_str(), 0755);
	#endif

	LastIPAddress = "127.0.0.1";
	LastAccountName = "";

	return 1;
}

// Destructor
void _Config::Close() {

}

// Loads settings
bool _Config::LoadSettings() {

	// Get filename
	core::stringc SaveFile = SavePath + "settings.cfg";

	// Open file
	std::ifstream File;
	File.open(SaveFile.c_str(), std::ios::in);
	if(!File)
		return SaveSettings();

	File.width(32);

	// Get IP address
	char IPAddress[32];
	File.getline(IPAddress, 31);
	LastIPAddress = IPAddress;

	// Get account name
	char AccountName[32];
	if(!File.eof()) {
		File.getline(AccountName, 31);
		LastAccountName = AccountName;
	}

	// Close
	File.close();

	return true;
}

// Saves settings
bool _Config::SaveSettings() {

	// Get filename
	core::stringc SaveFile = SavePath + "settings.cfg";

	// Open file
	std::ofstream File;
	File.open(SaveFile.c_str(), std::ios::out);
	if(!File)
		return false;

	// Save settings
	File << LastIPAddress.c_str() << std::endl;
	if(LastAccountName == "")
		File << std::endl;
	else
		File << LastAccountName.c_str() << std::endl;

	// Close
	File.close();

	return true;
}
