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
#pragma once

// Libraries
#include <string>
#include <list>
#include <queue>
#include <cstdint>

// Forward Declarations
class _Buffer;
class _Peer;
typedef struct _ENetEvent ENetEvent;
typedef struct _ENetHost ENetHost;

// Network Event
struct _NetworkEvent {

	// Types
	enum EventType {
		CONNECT,
		DISCONNECT,
		PACKET,
	};

	_NetworkEvent() : Data(nullptr), Peer(nullptr) { }

	EventType Type;
	double Time;
	_Buffer *Data;
	_Peer *Peer;
};

class _Network {

	public:

		// Different ways to send data
		enum SendType {
			RELIABLE = 1,
			UNSEQUENCED = 2,
		};

		_Network();
		virtual ~_Network();

		void Update(double FrameTime);

		// Settings
		void SetFakeLag(double Value) { FakeLag = Value; }

		// Updates
		bool GetNetworkEvent(_NetworkEvent &NetworkEvent);
		bool HasConnection() { return Connection != nullptr; }

		// Stats
		double GetSentSpeed() { return SentSpeed; }
		double GetReceiveSpeed() { return ReceiveSpeed; }

		// Internals
		void SetUpdatePeriod(double UpdatePeriod) { this->UpdatePeriod = UpdatePeriod; }
		double GetUpdatePeriod() const { return UpdatePeriod; }
		bool NeedsUpdate() { return UpdateTimer >= UpdatePeriod; }
		void ResetUpdateTimer() { UpdateTimer = 0.0; }

		// Static functions
		static void InitializeSystem();
		static void CloseSystem();

		// Determine if an ack is newer or the same as another
		static bool MoreRecentAck(uint16_t Previous, uint16_t Current, uint16_t Max) {
			return (Current > Previous && Current - Previous <= Max / 2) || (Previous > Current && Previous - Current > Max / 2);
		}

	protected:

		virtual void CreateEvent(_NetworkEvent &Event, double Time, ENetEvent &EEvent) { }
		virtual void HandleEvent(_NetworkEvent &Event, ENetEvent &EEvent) { }

		// State
		ENetHost *Connection;
		double Time;

		// Updates
		double UpdateTimer, UpdatePeriod;

		// Stats
		double SentSpeed;
		double ReceiveSpeed;
		double SecondTimer;

		// Fake lag
		double FakeLag;
		std::queue<_NetworkEvent> NetworkEvents;
};
