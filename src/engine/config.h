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
#ifndef CONFIG_H
#define CONFIG_H

// Libraries
#include <irrlicht.h>
#include <engine/singleton.h>

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Classes
class ConfigClass {

	public:

		int Init();
		void Close();

		stringc GetSavePath(const stringc &TFile);
		stringc GetSaveMapPath(const stringc &TFile);

		bool LoadSettings();
		bool SaveSettings();

		void SetLastIPAddress(const stringc &TValue) { LastIPAddress = TValue; }
		void SetLastAccountName(const stringc &TValue) { LastAccountName = TValue; }
		const stringc &GetLastIPAddress() const { return LastIPAddress; }
		const stringc &GetLastAccountName() const { return LastAccountName; }

	private:

		// Paths
		stringc SavePath, SaveMapPath;

		// Config
		stringc LastIPAddress, LastAccountName;

};

// Singletons
typedef SingletonClass<ConfigClass> Config;

#endif
