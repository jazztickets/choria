/******************************************************************************
* Copyright (c) 2017 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*******************************************************************************/
#include <ae/network.h>
#include <ae/peer.h>
#include <ae/buffer.h>
#include <enet/enet.h>
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
