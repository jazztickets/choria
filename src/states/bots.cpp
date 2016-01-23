/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
#include <states/bots.h>
#include <network/peer.h>
#include <network/clientnetwork.h>
#include <objects/object.h>
#include <objects/battle.h>
#include <framework.h>
#include <buffer.h>
#include <bot.h>
#include <stats.h>
#include <iomanip>

_BotsState BotState;

// Command loop
void RunCommandThread() {
	std::cout << "start" << std::endl;

	bool Done = false;
	while(!Done) {
		std::string Input;
		std::getline(std::cin, Input);
		if(Input == "stop" || std::cin.eof() == 1) {
			BotState.Quit();
			Done = true;
		}
		//else if(Input == "d" || Input == "disconnect")
		//	BotState.Network->Disconnect();
		else
			std::cout << "Command not recognized" << std::endl;
	}
}

// Constructor
_BotsState::_BotsState() :
	Done(false),
	Thread(nullptr) {

}

// Init
void _BotsState::Init() {

	try {
		Thread = new std::thread(RunCommandThread);
	}
	catch(std::exception &Error) {
		std::cerr << Error.what() << std::endl;
		Framework.Done = true;
	}
}

// Close
void _BotsState::Close() {
	if(Thread) {
		Thread->join();
		delete Thread;
	}
}

// Exit
void _BotsState::Quit() {
	Done = true;
}

// Update
void _BotsState::Update(double FrameTime) {

	for(auto &Bot : Bots)
		Bot->Update(FrameTime);

	if(Done)
		Framework.Done = true;
}
