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
#include "config.h"
#include <fstream>
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <sys/stat.h>
#endif

using namespace std;

// Constructor
int ConfigClass::Init() {

	#ifdef _WIN32
		SavePath = "save/";
		CreateDirectory(SavePath.c_str(), NULL);
	#else
		SavePath = stringc(getenv("HOME")) + stringc("/.choria/");
		mkdir(SavePath.c_str(), S_IRWXU | S_IXGRP | S_IRGRP | S_IXOTH | S_IROTH);
	#endif

	LastIPAddress = "127.0.0.1";
	LastAccountName = "";

	return 1;
}

// Destructor
void ConfigClass::Close() {

}

// Gets the save path to a file
void ConfigClass::GetSavePath(const stringc &TFile, stringc &TPath) {

	TPath = SavePath + stringc(TFile);
}

// Loads settings
bool ConfigClass::LoadSettings() {

	// Get filename
	stringc SaveFile;
	GetSavePath("settings.cfg", SaveFile);

	// Open file
	ifstream File;
	File.open(SaveFile.c_str(), ios::in);
	if(!File.is_open()) {
		File.clear();

		return SaveSettings();
	}
	File.width(32);

	// Get IP address
	char IPAddress[32];
	File >> IPAddress;

	// Get account name
	char AccountName[32];
	File >> AccountName;

	LastIPAddress = IPAddress;
	LastAccountName = AccountName;

	// Close
	File.clear();
	File.close();

	return true;
}

// Saves settings
bool ConfigClass::SaveSettings() {

	// Get filename
	stringc SaveFile;
	GetSavePath("settings.cfg", SaveFile);

	// Open file
	ofstream File;
	File.open(SaveFile.c_str(), ios::out);
	if(!File.is_open()) {
		File.clear();

		return false;
	}

	// Save settings
	File << LastIPAddress.c_str() << endl;
	if(LastAccountName == "")
		File << endl;
	else
		File << LastAccountName.c_str() << endl;

	// Close
	File.clear();
	File.close();

	return true;
}
