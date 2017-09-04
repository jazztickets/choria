/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#include <ae/servernetwork.h>
#include <ae/peer.h>
#include <ae/util.h>
#include <objects/object.h>
#include <objects/battle.h>
#include <objects/map.h>
#include <objects/components/character.h>
#include <framework.h>
#include <server.h>
#include <stats.h>
#include <iomanip>

_DedicatedState DedicatedState;

// Command loop
void RunCommandThread(_Server *Server) {
	Server->Log << "Type help to list commands" << std::endl;

	bool Done = false;
	while(!Done) {
		std::string Input;
		std::getline(std::cin, Input);
		if(Input == "help")
			DedicatedState.ShowCommands();
		else if(Input == "stop" || std::cin.eof() == 1)
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
	Thread(nullptr),
	NetworkPort(0),
	Hardcore(false) {

}

// Init
void _DedicatedState::Init() {

	// Setup server
	try {
		Server = new _Server(NetworkPort);
		Server->Hardcore = Hardcore;

		if(Hardcore)
			Server->Log << "Hardcore only is on" << std::endl;

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

// List available commands
void _DedicatedState::ShowCommands() {

	Server->Log << std::endl;
	Server->Log << "stop" << std::endl;
	Server->Log << "players" << std::endl;
	Server->Log << "battles" << std::endl;
}

// Show all players
void _DedicatedState::ShowPlayers() {
	auto &Peers = Server->Network->GetPeers();

	Server->Log << "peer count=" << Peers.size() << std::endl;
	size_t i = 0;
	for(auto &Peer : Peers) {
		Server->Log << std::setw(3) << i << ": account_id=" << Peer->AccountID;
		if(Peer->Object) {
			uint32_t MapID = 0;
			if(Peer->Object->Map)
				MapID = Peer->Object->Map->NetworkID;

			Server->Log << ", network_id=" << Peer->Object->NetworkID << ", map_id=" << MapID << ", name=" << Peer->Object->Name;
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
		for(auto &Object : Battle->Objects) {
			Server->Log << "\tnetwork_id=" << Object->NetworkID << "\thealth=" << Round(Object->Character->GetHealthPercent()) << "\tname=" << Object->Name << std::endl;
		}

		i++;
		Server->Log << std::endl;
	}

}
