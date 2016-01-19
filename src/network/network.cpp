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
#include <network/network.h>
#include <network/peer.h>
#include <buffer.h>
#include <stdexcept>

// Constructor
_Network::_Network()
:	Connection(nullptr),
	Time(0.0),
	UpdateTimer(0.0),
	UpdatePeriod(1 / 20.0),
	SentSpeed(0),
	ReceiveSpeed(0),
	SecondTimer(0.0),
	FakeLag(0.0) {
}

// Destructor
_Network::~_Network() {

	// Delete events
	while(!NetworkEvents.empty()) {
		delete NetworkEvents.front().Data;
		NetworkEvents.pop();
	}

	// Destroy connection
	if(Connection)
		enet_host_destroy(Connection);
}

// Initializes enet
void _Network::InitializeSystem() {
	if(enet_initialize() != 0)
		throw std::runtime_error("enet_initialize() error");
}

// Closes enet
void _Network::CloseSystem() {
	enet_deinitialize();
}

// Get network event
bool _Network::GetNetworkEvent(_NetworkEvent &NetworkEvent) {

	// Check for new events
	if(!NetworkEvents.empty()) {
		const _NetworkEvent &PeekEvent = NetworkEvents.front();
		if(Time >= PeekEvent.Time) {
			NetworkEvent = NetworkEvents.front();
			NetworkEvents.pop();
			return true;
		}
	}

	return false;
}

// Update
void _Network::Update(double FrameTime) {
	if(!Connection)
		return;

	// Update time
	Time += FrameTime;
	UpdateTimer += FrameTime;
	SecondTimer += FrameTime;

	// Get events from enet
	ENetEvent EEvent;
	while(enet_host_service(Connection, &EEvent, 0) > 0) {

		// Create a _NetworkEvent
		_NetworkEvent Event;
		CreateEvent(Event, Time + FakeLag, EEvent);

		// Handle internally
		HandleEvent(Event, EEvent);

		// Add to queue
		NetworkEvents.push(Event);
	}

	// Update speed variables
	if(SecondTimer >= 1.0) {
		SentSpeed = Connection->totalSentData / SecondTimer;
		ReceiveSpeed = Connection->totalReceivedData / SecondTimer;
		Connection->totalSentData = 0;
		Connection->totalReceivedData = 0;
		SecondTimer -= 1.0;
	}
}
