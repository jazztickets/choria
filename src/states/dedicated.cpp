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
#include <network/servernetwork.h>
#include <network/peer.h>
#include <objects/object.h>
#include <objects/battle.h>
#include <framework.h>
#include <server.h>
#include <stats.h>
#include <iomanip>

_DedicatedState DedicatedState;

// Command loop
void RunCommandThread(_Server *Server) {
	Server->Log << "Type stop to stop the server" << std::endl;

	bool Done = false;
	while(!Done) {
		std::string Input;
		std::getline(std::cin, Input);
		if(Input == "stop" || std::cin.eof() == 1)
			Done = true;
		else if(Input == "p" || Input == "players")
			DedicatedState.ShowPlayers();
		else if(Input == "b" || Input == "battles")
			DedicatedState.ShowBattles();
		else
			Server->Log << "Command not recognized" << std::endl;
	}

	Server->Log << "Stopping..." << std::endl;

	Server->StopServer();
}

// Constructor
_DedicatedState::_DedicatedState() :
	Server(nullptr),
	Stats(nullptr),
	Thread(nullptr) {

}

// Init
void _DedicatedState::Init() {

	// Setup server
	try {
		Stats = new _Stats();
		Server = new _Server(Stats, NetworkPort);

		Server->Log << "Listening on port " << NetworkPort << std::endl;

		Thread = new std::thread(RunCommandThread, Server);
	}
	catch(std::exception &Error) {
		std::cerr << Error.what() << std::endl;
		delete Stats;
		Framework.Done = true;
	}
}

// Close
void _DedicatedState::Close() {
	if(Thread) {
		Thread->join();
		delete Thread;
	}

	delete Stats;
	delete Server;
}

// Update
void _DedicatedState::Update(double FrameTime) {
	Server->Update(FrameTime);

	if(Server->Done) {
		Framework.Done = true;
	}
}

// Show all players
void _DedicatedState::ShowPlayers() {
	auto &Peers = Server->Network->GetPeers();

	Server->Log << "peer count=" << Peers.size() << std::endl;
	size_t i = 0;
	for(auto &Peer : Peers) {
		Server->Log << std::setw(3) << i << ": account_id=" << Peer->AccountID;
		if(Peer->Object) {
			Server->Log << ", player_name=" << Peer->Object->Name << ", network_id=" << Peer->Object->NetworkID;
		}

		Server->Log << std::endl;
		i++;
	}

	Server->Log << std::endl;
}

// Show all battles
void _DedicatedState::ShowBattles() {
	auto &Battles = Server->BattleManager->Objects;

	Server->Log << "battle count=" << Battles.size() << std::endl;
	size_t i = 0;
	for(auto &Battle : Battles) {
		Server->Log << i << ": id=" << Battle->NetworkID << std::endl;
		for(auto &Object : Battle->Fighters) {
			if(Object->Peer) {
				Server->Log << "\tplayer_name=" << Object->Name << ", network_id=" << Object->NetworkID;
			}
		}

		i++;
	}

	Server->Log << std::endl;
}
