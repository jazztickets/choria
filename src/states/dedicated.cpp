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
#include <states/dedicated.h>
#include <framework.h>
#include <server.h>

_DedicatedState DedicatedState;

// Command loop
void RunCommandThread(_Server *Server) {
	std::cout << "Type stop to stop the server" << std::endl;

	bool Done = false;
	while(!Done) {
		std::string Input;
		std::getline(std::cin, Input);
		if(Input == "stop")
			Done = true;
		else
			std::cout << "Command not recognized" << std::endl;
	}

	Server->StopServer();
}

// Constructor
_DedicatedState::_DedicatedState() :
	Server(nullptr),
	Thread(nullptr),
	Stats(nullptr) {

}

// Init
void _DedicatedState::Init() {
	if(!Stats)
		throw std::runtime_error(std::string(__FUNCTION__) + " - Stats is null!");

	// Setup server
	try {
		Server = new _Server(NetworkPort);
		Server->Stats = Stats;

		std::cout << "Listening on port " << NetworkPort << std::endl;

		Thread = new std::thread(RunCommandThread, Server);
	}
	catch(std::exception &Error) {
		std::cerr << Error.what() << std::endl;
		Framework.Done = true;
	}
}

// Close
void _DedicatedState::Close() {
	if(Thread) {
		Thread->join();
		delete Thread;
	}

	delete Server;
}

// Update
void _DedicatedState::Update(double FrameTime) {
	Server->Update(FrameTime);

	if(Server->Done) {
		Framework.Done = true;
	}
}
