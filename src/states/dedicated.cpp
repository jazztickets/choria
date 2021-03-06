/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2021 Alan Witkowski
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
#include <ae/manager.h>
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
#include <constants.h>
#include <enet/enet.h>
#include <iomanip>

_DedicatedState DedicatedState;

// Command loop
void RunCommandThread(_Server *Server) {
	std::cout << "Type help to list commands" << std::endl;

	bool Done = false;
	while(!Done) {
		std::string Input;
		std::getline(std::cin, Input);
		Server->Log << "[SERVER_COMMAND] " << Input << std::endl;
		if(Input.substr(0, 3) == "ban") {
			std::vector<std::string> Parameters;
			Parameters.reserve(4);
			ae::TokenizeString(Input, Parameters);
			if(Parameters.size() != 4)
				std::cout << "Bad parameters" << std::endl;
			else
				Server->Ban(std::stoi(Parameters[1]), Parameters[2] + " " + Parameters[3]);
		}
		else if(Input == "b" || Input == "battles") {
			DedicatedState.ShowBattles();
		}
		else if(Input.substr(0, 3) == "log" && Input.size() > 4) {
			ae::NetworkIDType PlayerID = std::stoi(Input.substr(4, std::string::npos));
			try {
				bool Mode = Server->StartLog(PlayerID);
				std::cout << "Logging has been " << (Mode ? "enabled" : "disabled") << std::endl;
			}
			catch (std::exception &Error) {
				std::cout << Error.what() << std::endl;
			}
		}
		else if(Input == "help") {
			DedicatedState.ShowCommands();
		}
		else if(Input == "p" || Input == "players") {
			DedicatedState.ShowPlayers();
		}
		else if(Input.substr(0, 4) == "mute") {
			std::vector<std::string> Parameters;
			Parameters.reserve(3);
			ae::TokenizeString(Input, Parameters);
			if(Parameters.size() != 3)
				std::cout << "Bad parameters" << std::endl;
			else
				Server->Mute(std::stoi(Parameters[1]), std::stoi(Parameters[2]));
		}
		else if(Input.substr(0, 3) == "say" && Input.size() > 4) {
			Server->BroadcastMessage(nullptr, Input.substr(4, std::string::npos), "purple");
		}
		else if(Input.substr(0, 4) == "slap" && Input.size() > 5) {
			ae::NetworkIDType PlayerID = std::stoi(Input.substr(5, std::string::npos));
			Server->Slap(PlayerID, 25);
		}
		else if(Input.substr(0, 4) == "stop" || std::cin.eof() == 1) {
			int Seconds = 0;
			if(Input.size() > 5)
				Seconds = std::stoi(Input.substr(5, std::string::npos));
			Server->StopServer(Seconds);
			Done = true;
		}
		else {
			std::cout << "Command not recognized" << std::endl;
		}
	}

	Server->StopServer();
}

// Constructor
_DedicatedState::_DedicatedState() :
	Server(nullptr),
	Thread(nullptr),
	NetworkPort(0),
	Hardcore(false),
	NoPVP(false),
	DevMode(false) {

}

// Init
void _DedicatedState::Init() {

	// Setup server
	try {
		Server = new _Server(NetworkPort);
		Server->Hardcore = Hardcore;
		Server->NoPVP = NoPVP;
		Server->IsTesting = DevMode;

		if(Hardcore)
			std::cout << "Hardcore only is on" << std::endl;
		if(NoPVP)
			std::cout << "PVP is disabled" << std::endl;

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
	if(Framework.GetTimeStepAccumulator() > DEBUG_STALL_THRESHOLD)
		Server->Log << "[STALL] TimeStepAccumulator=" << Framework.GetTimeStepAccumulator() << std::endl;

	// Update server
	Server->Update(FrameTime);

	if(Server->Done) {
		Framework.Done = true;
	}
}

// List available commands
void _DedicatedState::ShowCommands() {
	std::cout << std::endl;
	std::cout << "ban      <account_id> <time>    ban player (E.g. ban 1 5 days)" << std::endl;
	std::cout << "battles                         show current battles" << std::endl;
	std::cout << "log      <network_id>           toggle logging player data" << std::endl;
	std::cout << "mute     <account_id> <value>   mute player (E.g. mute 1 1)" << std::endl;
	std::cout << "players                         show players" << std::endl;
	std::cout << "stop     [seconds]              stop server" << std::endl;
	std::cout << "slap     <network_id>           slap player" << std::endl;
	std::cout << "say      <message>              broadcast message" << std::endl;
}

// Show all players
void _DedicatedState::ShowPlayers() {
	auto &Peers = Server->Network->GetPeers();

	std::cout << "peer count=" << Peers.size() << std::endl;
	std::size_t i = 0;
	for(auto &Peer : Peers) {
		char Buffer[16];
		ENetAddress *Address = &Peer->ENetPeer->address;
		enet_address_get_host_ip(Address, Buffer, 16);

		std::cout << std::setw(3) << i << ": ip=" << Buffer << ", account_id=" << Peer->AccountID;
		if(Peer->Object) {
			uint32_t MapID = 0;
			if(Peer->Object->Map)
				MapID = Peer->Object->Map->NetworkID;

			std::cout
				<< ", network_id=" << Peer->Object->NetworkID
				<< ", map_id=" << MapID
				<< ", name=" << Peer->Object->Name
				<< ", muted=" << Peer->Object->Muted
				<< ", hardcore=" << (int)Peer->Object->Character->Hardcore;
		}

		std::cout << std::endl;
		i++;
	}

	std::cout << std::endl;
}

// Show all battles
void _DedicatedState::ShowBattles() {
	auto &Battles = Server->BattleManager->Objects;

	std::cout << "battle count=" << Battles.size() << std::endl;
	std::size_t i = 0;
	for(auto &Battle : Battles) {
		std::cout << i << ": id=" << Battle->NetworkID << std::endl;
		for(auto &Object : Battle->Objects) {
			std::cout << "\tnetwork_id=" << Object->NetworkID << "\thealth=" << ae::Round(Object->Character->GetHealthPercent()) << "\tname=" << Object->Name << std::endl;
		}

		i++;
		std::cout << std::endl;
	}
}
