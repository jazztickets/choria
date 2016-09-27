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
#pragma once

// Libraries
#include <log.h>
#include <memory>
#include <thread>
#include <list>
#include <manager.h>
#include <unordered_map>
#include <glm/vec4.hpp>

// Forward Declarations
class _ServerNetwork;
class _Buffer;
class _Peer;
class _Map;
class _Battle;
class _Stats;
class _Save;
class _Object;
class _Scripting;
struct _NetworkEvent;

struct _BattleEvent {
	_Object *Object;
	uint32_t Zone;
	bool Scripted;
};

// Server class
class _Server {

	public:

		_Server(_Stats *Stats, uint16_t NetworkPort);
		~_Server();

		void Update(double FrameTime);
		void StartThread();
		void JoinThread();
		void StopServer();

		void SpawnPlayer(_Object *Player, NetworkIDType MapID, uint32_t EventType);
		void QueueBattle(_Object *Object, uint32_t Zone, bool Scripted);
		void StartTeleport(_Object *Object, double Time);
		void SendMessage(_Peer *Peer, const std::string &Message, const glm::vec4 &Color);
		void SendHUD(_Peer *Peer);
		void SendPlayerPosition(_Peer *Peer);
		void RunEventScript(uint32_t ScriptID, _Object *Object);

		// Parameters
		bool IsTesting;

		// State
		bool Done;
		bool StartDisconnect;
		bool StartShutdown;
		uint16_t TimeSteps;
		double Time;
		double SaveTime;
		double Clock;
		_LogFile Log;

		// Stats
		_Stats *Stats;
		_Save *Save;

		// Network
		std::unique_ptr<_ServerNetwork> Network;

		// Scripting
		_Scripting *Scripting;

		// Objects
		_Manager<_Object> *ObjectManager;
		_Manager<_Map> *MapManager;
		_Manager<_Battle> *BattleManager;
		std::list<_BattleEvent> BattleEvents;

	private:

		_Object *CreatePlayer(_Peer *Peer);
		bool ValidatePeer(_Peer *Peer);
		void StartBattle(_BattleEvent &BattleEvent);

		void HandleConnect(_NetworkEvent &Event);
		void HandleDisconnect(_NetworkEvent &Event);
		void HandlePacket(_Buffer &Data, _Peer *Peer);

		void HandleLoginInfo(_Buffer &Data, _Peer *Peer);
		void HandleCharacterListRequest(_Buffer &Data, _Peer *Peer);
		void HandleCharacterPlay(_Buffer &Data, _Peer *Peer);
		void HandleCharacterCreate(_Buffer &Data, _Peer *Peer);
		void HandleCharacterDelete(_Buffer &Data, _Peer *Peer);
		void HandleMoveCommand(_Buffer &Data, _Peer *Peer);
		void HandleRespawn(_Buffer &Data, _Peer *Peer);
		void HandleInventoryMove(_Buffer &Data, _Peer *Peer);
		void HandleInventoryUse(_Buffer &Data, _Peer *Peer);
		void HandleInventorySplit(_Buffer &Data, _Peer *Peer);
		void HandleVendorExchange(_Buffer &Data, _Peer *Peer);
		void HandleTraderAccept(_Buffer &Data, _Peer *Peer);
		void HandleSkillAdjust(_Buffer &Data, _Peer *Peer);
		void HandleChatMessage(_Buffer &Data, _Peer *Peer);
		void HandleTradeRequest(_Buffer &Data, _Peer *Peer);
		void HandleTradeCancel(_Buffer &Data, _Peer *Peer);
		void HandleTradeGold(_Buffer &Data, _Peer *Peer);
		void HandleTradeAccept(_Buffer &Data, _Peer *Peer);
		void HandleActionUse(_Buffer &Data, _Peer *Peer);
		void HandleActionBarChanged(_Buffer &Data, _Peer *Peer);
		void HandleBattleFinished(_Buffer &Data, _Peer *Peer);
		void HandlePlayerStatus(_Buffer &Data, _Peer *Peer);
		void HandleBlacksmithUpgrade(_Buffer &Data, _Peer *Peer);
		void HandleHelp(_Buffer &Data, _Peer *Peer);

		void SendPlayerInfo(_Peer *Peer);
		void SendCharacterList(_Peer *Peer);
		void SendTradeInformation(_Object *Sender, _Object *Receiver);

		// Threading
		std::thread *Thread;
};
