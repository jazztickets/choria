/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020  Alan Witkowski
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
#include <ae/type.h>
#include <ae/log.h>
#include <ae/buffer.h>
#include <enums.h>
#include <glm/vec4.hpp>
#include <unordered_map>
#include <memory>
#include <thread>
#include <list>

// Forward Declarations
class _Map;
class _Battle;
class _Stats;
class _Save;
class _Object;
class _Scripting;
class _BaseItem;

namespace ae {
	template<class T> class _Manager;
	class _ServerNetwork;
	class _Buffer;
	struct _NetworkEvent;
	class _Peer;
}

struct _BattleEvent {
	_Object *Object;
	std::string ZoneID;
	float BountyEarned;
	float BountyClaimed;
	int PVP;
	bool Scripted;
};

// Server class
class _Server {

	public:

		_Server(uint16_t NetworkPort);
		~_Server();

		void Update(double FrameTime);
		void StartThread();
		void JoinThread();
		void StopServer(int Seconds=0);

		void SpawnPlayer(_Object *Player, _Map *LoadMap, EventType Event);
		void QueueBattle(_Object *Object, const std::string &ZoneID, bool Scripted, bool PVP, float BountyEarned, float BountyClaimed);
		void StartTeleport(_Object *Object, double Time);
		void SendMessage(ae::_Peer *Peer, const std::string &Message, const std::string &ColorName);
		void BroadcastMessage(ae::_Peer *IgnorePeer, const std::string &Message, const std::string &ColorName);
		void SendHUD(ae::_Peer *Peer);
		void SendPlayerPosition(ae::_Peer *Peer);
		void RunEventScript(const std::string &Script, _Object *Object);
		void SetClock(double Clock);

		// Packet handling
		void HandleLoginInfo(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleCharacterListRequest(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleCharacterPlay(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleCharacterCreate(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleCharacterDelete(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleMoveCommand(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleUseCommand(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleRespawn(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleInventoryMove(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleInventoryTransfer(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleInventoryUse(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleInventorySplit(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleInventoryDelete(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleVendorExchange(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleTraderAccept(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleChatMessage(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleTradeRequest(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleTradeCancel(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleTradeGold(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleTradeAccept(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandlePartyInfo(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleActionUse(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleActionBarChanged(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleBattleFinished(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandlePlayerStatus(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleBlacksmithUpgrade(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleMinigamePay(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleMinigameGetPrize(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleJoin(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleExit(ae::_Buffer &Data, ae::_Peer *Peer);
		void HandleCommand(ae::_Buffer &Data, ae::_Peer *Peer);

		// Parameters
		bool IsTesting;
		bool Hardcore;

		// State
		bool Done;
		bool StartShutdownTimer;
		bool StartDisconnect;
		bool StartShutdown;
		double ShutdownTime;
		uint16_t TimeSteps;
		double Time;
		double SaveTime;
		double BotTime;
		ae::_LogFile Log;

		// Stats
		const _Stats *Stats;
		_Save *Save;

		// Network
		std::unique_ptr<ae::_ServerNetwork> Network;

		// Scripting
		_Scripting *Scripting;

		// Objects
		ae::_Manager<_Object> *ObjectManager;
		ae::_Manager<_Map> *MapManager;
		ae::_Manager<_Battle> *BattleManager;
		std::list<_BattleEvent> BattleEvents;

	private:

		_Object *CreatePlayer(ae::_Peer *Peer);
		_Object *CreateBot();
		bool ValidatePeer(ae::_Peer *Peer);
		bool CheckAccountUse(ae::_Peer *Peer);
		void StartBattle(_BattleEvent &BattleEvent);

		void HandleConnect(ae::_NetworkEvent &Event);
		void HandleDisconnect(ae::_NetworkEvent &Event);
		void HandlePacket(ae::_Buffer &Data, ae::_Peer *Peer);

		void SendItem(ae::_Peer *Peer, const _BaseItem *Item, int Count);
		void SendPlayerInfo(ae::_Peer *Peer);
		void SendCharacterList(ae::_Peer *Peer);
		void SendTradeInformation(_Object *Sender, _Object *Receiver);
		void SendTradePlayerInventory(_Object *Player);

		// Threading
		std::thread *Thread;
		ae::_Buffer PingPacket;
};
