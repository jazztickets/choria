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
#include <state.h>
#include <thread>

// Forward Declarations
class _Database;
class _ObjectManager;
class _Instance;
class _Buffer;
class _Object;
class _Player;
class _ServerBattle;

// Classes
class _ServerState : public _State {

	public:

		void Init();
		void Close();

		void HandleConnect(ENetEvent *Event);
		void HandleDisconnect(ENetEvent *Event);
		void HandlePacket(ENetEvent *Event);

		void Update(double FrameTime);
		void DeleteObject(_Object *Object);

		void PlayerTeleport(_Player *Player);
		double GetServerTime() const { return ServerTime; }

		void StartCommandThread();
		void StopServer() { StopRequested = true; }

	private:

		void CreateDefaultDatabase();

		void HandleLoginInfo(_Buffer *Packet, ENetPeer *Peer);
		void HandleCharacterListRequest(_Buffer *Packet, ENetPeer *Peer);
		void HandleCharacterSelect(_Buffer *Packet, ENetPeer *Peer);
		void HandleCharacterDelete(_Buffer *Packet, ENetPeer *Peer);
		void HandleCharacterCreate(_Buffer *Packet, ENetPeer *Peer);
		void HandleMoveCommand(_Buffer *Packet, ENetPeer *Peer);
		void HandleBattleCommand(_Buffer *Packet, ENetPeer *Peer);
		void HandleBattleFinished(_Buffer *Packet, ENetPeer *Peer);
		void HandleInventoryMove(_Buffer *Packet, ENetPeer *Peer);
		void HandleInventoryUse(_Buffer *Packet, ENetPeer *Peer);
		void HandleInventorySplit(_Buffer *Packet, ENetPeer *Peer);
		void HandleEventEnd(_Buffer *Packet, ENetPeer *Peer);
		void HandleVendorExchange(_Buffer *Packet, ENetPeer *Peer);
		void HandleSkillBar(_Buffer *Packet, ENetPeer *Peer);
		void HandleSkillAdjust(_Buffer *Packet, ENetPeer *Peer);
		void HandlePlayerBusy(_Buffer *Packet, ENetPeer *Peer);
		void HandleAttackPlayer(_Buffer *Packet, ENetPeer *Peer);
		void HandleChatMessage(_Buffer *Packet, ENetPeer *Peer);
		void HandleTradeRequest(_Buffer *Packet, ENetPeer *Peer);
		void HandleTradeCancel(_Buffer *Packet, ENetPeer *Peer);
		void HandleTradeGold(_Buffer *Packet, ENetPeer *Peer);
		void HandleTradeAccept(_Buffer *Packet, ENetPeer *Peer);
		void HandleTeleport(_Buffer *Packet, ENetPeer *Peer);
		void HandleTraderAccept(_Buffer *Packet, ENetPeer *Peer);

		void SendPlayerPosition(_Player *Player);
		void SpawnPlayer(_Player *Player, int NewMapID, int EventType, int EventData);
		void SendHUD(_Player *Player);
		void SendCharacterList(_Player *Player);
		void SendEvent(_Player *Player, int Type, int Data);
		void SendTradeInformation(_Player *Sender, _Player *Receiver);

		void BuildTradeItemsPacket(_Player *Player, _Buffer *Packet, int Gold);

		void RemovePlayerFromBattle(_Player *Player);

		_Database *Database;
		_ObjectManager *ObjectManager;
		_Instance *Instances;

		bool StopRequested;

		std::thread *CommandThread;

		double ServerTime;
};

extern _ServerState ServerState;
