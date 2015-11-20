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

		void HandleConnect(ENetEvent *TEvent);
		void HandleDisconnect(ENetEvent *TEvent);
		void HandlePacket(ENetEvent *TEvent);

		void Update(double FrameTime);
		void DeleteObject(_Object *TObject);

		void PlayerTeleport(_Player *TPlayer);
		double GetServerTime() const { return ServerTime; }

		void StartCommandThread();
		void StopServer() { StopRequested = true; }

	private:

		void CreateDefaultDatabase();

		void HandleLoginInfo(_Buffer *Packet, ENetPeer *TPeer);
		void HandleCharacterListRequest(_Buffer *Packet, ENetPeer *TPeer);
		void HandleCharacterSelect(_Buffer *Packet, ENetPeer *TPeer);
		void HandleCharacterDelete(_Buffer *Packet, ENetPeer *TPeer);
		void HandleCharacterCreate(_Buffer *Packet, ENetPeer *TPeer);
		void HandleMoveCommand(_Buffer *Packet, ENetPeer *TPeer);
		void HandleBattleCommand(_Buffer *Packet, ENetPeer *TPeer);
		void HandleBattleFinished(_Buffer *Packet, ENetPeer *TPeer);
		void HandleInventoryMove(_Buffer *Packet, ENetPeer *TPeer);
		void HandleInventoryUse(_Buffer *Packet, ENetPeer *TPeer);
		void HandleInventorySplit(_Buffer *Packet, ENetPeer *TPeer);
		void HandleEventEnd(_Buffer *Packet, ENetPeer *TPeer);
		void HandleVendorExchange(_Buffer *Packet, ENetPeer *TPeer);
		void HandleSkillBar(_Buffer *Packet, ENetPeer *TPeer);
		void HandleSkillAdjust(_Buffer *Packet, ENetPeer *TPeer);
		void HandlePlayerBusy(_Buffer *Packet, ENetPeer *TPeer);
		void HandleAttackPlayer(_Buffer *Packet, ENetPeer *TPeer);
		void HandleChatMessage(_Buffer *Packet, ENetPeer *TPeer);
		void HandleTradeRequest(_Buffer *Packet, ENetPeer *TPeer);
		void HandleTradeCancel(_Buffer *Packet, ENetPeer *TPeer);
		void HandleTradeGold(_Buffer *Packet, ENetPeer *TPeer);
		void HandleTradeAccept(_Buffer *Packet, ENetPeer *TPeer);
		void HandleTeleport(_Buffer *Packet, ENetPeer *TPeer);
		void HandleTraderAccept(_Buffer *Packet, ENetPeer *TPeer);

		void SendPlayerPosition(_Player *TPlayer);
		void SpawnPlayer(_Player *TPlayer, int TNewMapID, int TEventType, int TEventData);
		void SendHUD(_Player *TPlayer);
		void SendCharacterList(_Player *TPlayer);
		void SendEvent(_Player *TPlayer, int TType, int TData);
		void SendTradeInformation(_Player *TSender, _Player *TReceiver);

		void BuildTradeItemsPacket(_Player *TPlayer, _Buffer *Packet, int TGold);

		void RemovePlayerFromBattle(_Player *TPlayer);

		_Database *Database;
		_ObjectManager *ObjectManager;
		_Instance *Instances;

		bool StopRequested;

		std::thread *CommandThread;

		double ServerTime;
};

extern _ServerState ServerState;
