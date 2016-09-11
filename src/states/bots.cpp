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
#include <stats.h>
#include <framework.h>
#include <buffer.h>
#include <bot.h>
#include <constants.h>
#include <utils.h>
#include <stats.h>
#include <iomanip>
#include <sstream>
#include <SDL_timer.h>

_BotsState BotState;

// Command loop
void RunCommandThread() {
	std::cout << "start" << std::endl;

	bool Done = false;
	while(!Done) {
		std::string Input;
		std::getline(std::cin, Input);
		if(std::cin.eof() == 1)
			Done = true;

		// Split by space
		std::vector<std::string> Tokens;
		std::stringstream Buffer(Input);
		std::string Token;
		while(std::getline(Buffer, Token, ' ')) {
			Tokens.push_back(Token);
		}

		// Handle command
		if(Tokens.size()) {
			if(Tokens[0] == "stop")
				Done = true;
			else if(Tokens[0] == "a" || Tokens[0] == "add") {
				int Count = 1;
				if(Tokens.size() > 1) {
					Count = ToNumber<int>(Tokens[1]);
				}

				for(int i = 0; i < Count; i++) {
					BotState.Add();
					SDL_Delay(1);
				}
			}
			else if(Tokens[0] == "d" || Tokens[0] == "disconnect")
				BotState.DisconnectAll();
			else
				std::cout << "Command not recognized" << std::endl;
		}
	}

	BotState.QuitEvent();
}

// Constructor
_BotsState::_BotsState() :
	HostAddress("127.0.0.1"),
	Port(DEFAULT_NETWORKPORT),
	Done(false),
	Thread(nullptr),
	Stats(nullptr) {

}

// Init
void _BotsState::Init() {
	NextBotNumber = 0;

	try {
		Thread = new std::thread(RunCommandThread);
	}
	catch(std::exception &Error) {
		std::cerr << Error.what() << std::endl;
		Framework.Done = true;
		return;
	}

	Stats = new _Stats(true);
}

// Close
void _BotsState::Close() {
	if(Thread) {
		Thread->join();
		delete Thread;
	}

	delete Stats;
}

// Exit
void _BotsState::QuitEvent() {
	Done = true;
	DisconnectAll();
}

// Update
void _BotsState::Update(double FrameTime) {

	for(auto Iterator = Bots.begin(); Iterator != Bots.end(); ) {
		_Bot *Bot = *Iterator;

		// Update
		Bot->Update(FrameTime);

		// Delete
		if(Bot->Network->IsDisconnected()) {

			delete Bot;
			Iterator = Bots.erase(Iterator);
		}
		else
			++Iterator;
	}

	if(Done && Bots.size() == 0)
		Framework.Done = true;
}

// Add a bot and connect
void _BotsState::Add() {

	try {
		std::string Credentials = "bot" + std::to_string(NextBotNumber);
		_Bot *Bot = new _Bot(Stats, Credentials, Credentials, HostAddress, Port);
		Bots.push_back(Bot);
		NextBotNumber++;
	}
	catch(std::exception &Error) {
		std::cout << Error.what() << std::endl;
	}
}

// Disconnect all bots
void _BotsState::DisconnectAll() {

	for(auto &Bot : Bots)
		Bot->Network->Disconnect();

}
