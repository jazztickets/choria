/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#pragma once

// Libraries
#include <state.h>
#include <thread>

// Forward Declarations
class DatabaseClass;
class ObjectManagerClass;
class InstanceClass;
class PacketClass;
class ObjectClass;
class PlayerClass;
class ServerBattleClass;

// Classes
class _PlayServerState : public _State {

	public:

		int Init();
		int Close();

		void HandleConnect(ENetEvent *TEvent);
		void HandleDisconnect(ENetEvent *TEvent);
		void HandlePacket(ENetEvent *TEvent);

		void Update(u32 TDeltaTime);
		void DeleteObject(ObjectClass *TObject);

		void PlayerTownPortal(PlayerClass *TPlayer);
		u32 GetServerTime() const { return ServerTime; }

		void StartCommandThread();
		void StopServer() { StopRequested = true; }

	private:

		void CreateDefaultDatabase();

		void HandleLoginInfo(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleCharacterListRequest(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleCharacterSelect(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleCharacterDelete(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleCharacterCreate(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleMoveCommand(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleBattleCommand(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleBattleFinished(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleInventoryMove(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleInventoryUse(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleInventorySplit(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleEventEnd(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleVendorExchange(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleSkillBar(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleSkillAdjust(PacketClass *TPacket, ENetPeer *TPeer);
		void HandlePlayerBusy(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleAttackPlayer(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleChatMessage(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleTradeRequest(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleTradeCancel(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleTradeGold(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleTradeAccept(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleTownPortal(PacketClass *TPacket, ENetPeer *TPeer);
		void HandleTraderAccept(PacketClass *TPacket, ENetPeer *TPeer);

		void SendPlayerPosition(PlayerClass *TPlayer);
		void SpawnPlayer(PlayerClass *TPlayer, int TNewMapID, int TEventType, int TEventData);
		void SendHUD(PlayerClass *TPlayer);
		void SendCharacterList(PlayerClass *TPlayer);
		void SendEvent(PlayerClass *TPlayer, int TType, int TData);
		void SendTradeInformation(PlayerClass *TSender, PlayerClass *TReceiver);

		void BuildTradeItemsPacket(PlayerClass *TPlayer, PacketClass *TPacket, int TGold);

		void RemovePlayerFromBattle(PlayerClass *TPlayer);

		DatabaseClass *Database;
		ObjectManagerClass *ObjectManager;
		InstanceClass *Instances;

		bool StopRequested;

		std::thread *CommandThread;

		u32 ServerTime;
};

extern _PlayServerState PlayServerState;
