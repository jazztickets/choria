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
#pragma once

// Libraries
#include <enet/enet.h>
#include <string>
#include <list>
#include <queue>
#include <cstdint>

// Forward Declarations
class _Buffer;
class _Peer;

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
	float Time;
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
		uint32_t GetSentSpeed() { return SentSpeed; }
		uint32_t GetReceiveSpeed() { return ReceiveSpeed; }

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
		uint32_t SentSpeed, ReceiveSpeed;
		double SecondTimer;

		// Fake lag
		double FakeLag;
		std::queue<_NetworkEvent> NetworkEvents;
};
