/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef SERVER_H
#define SERVER_H

// Libraries
#include "engine/state.h"

// Forward Declarations
class DatabaseClass;
class ObjectManagerClass;
class InstanceClass;
class PacketClass;
class ObjectClass;
class PlayerClass;
class ServerBattleClass;

// Classes
class ServerStateClass : public StateClass {

	public:

		int Init();
		int Close();

		void HandleConnect(ENetEvent *Event);
		void HandleDisconnect(ENetEvent *Event);
		void HandlePacket(ENetEvent *Event);

		void Update(u32 FrameTime);
		void DeleteObject(ObjectClass *Object);

		void PlayerTownPortal(PlayerClass *Player);
		u32 GetServerTime() const { return ServerTime; }

	private:

		void CreateDefaultDatabase();

		void HandleLoginInfo(PacketClass *Packet, ENetPeer *Peer);
		void HandleCharacterListRequest(PacketClass *Packet, ENetPeer *Peer);
		void HandleCharacterSelect(PacketClass *Packet, ENetPeer *Peer);
		void HandleCharacterDelete(PacketClass *Packet, ENetPeer *Peer);
		void HandleCharacterCreate(PacketClass *Packet, ENetPeer *Peer);
		void HandleMoveCommand(PacketClass *Packet, ENetPeer *Peer);
		void HandleBattleAction(PacketClass *Packet, ENetPeer *Peer);
		void HandleBattleFinished(PacketClass *Packet, ENetPeer *Peer);
		void HandleInventoryMove(PacketClass *Packet, ENetPeer *Peer);
		void HandleInventoryUse(PacketClass *Packet, ENetPeer *Peer);
		void HandleInventorySplit(PacketClass *Packet, ENetPeer *Peer);
		void HandleEventEnd(PacketClass *Packet, ENetPeer *Peer);
		void HandleVendorExchange(PacketClass *Packet, ENetPeer *Peer);
		void HandleActionBar(PacketClass *Packet, ENetPeer *Peer);
		void HandleSkillAdjust(PacketClass *Packet, ENetPeer *Peer);
		void HandlePlayerBusy(PacketClass *Packet, ENetPeer *Peer);
		void HandleAttackPlayer(PacketClass *Packet, ENetPeer *Peer);
		void HandleChatMessage(PacketClass *Packet, ENetPeer *Peer);
		void HandleTradeRequest(PacketClass *Packet, ENetPeer *Peer);
		void HandleTradeCancel(PacketClass *Packet, ENetPeer *Peer);
		void HandleTradeGold(PacketClass *Packet, ENetPeer *Peer);
		void HandleTradeAccept(PacketClass *Packet, ENetPeer *Peer);
		void HandleTownPortal(PacketClass *Packet, ENetPeer *Peer);
		void HandleTraderAccept(PacketClass *Packet, ENetPeer *Peer);

		void SpawnPlayer(PlayerClass *Player, int NewMapID, int EventType, int EventData);
		void SendHUD(PlayerClass *Player);
		void SendPlayerPosition(PlayerClass *Player);
		void SendCharacterList(PlayerClass *Player);
		void SendEvent(PlayerClass *Player, int Type, int Data);
		void SendTradeInformation(PlayerClass *Sender, PlayerClass *Receiver);

		void BuildTradeItemsPacket(PlayerClass *Player, PacketClass *Packet, int Gold);

		void RemovePlayerFromBattle(PlayerClass *Player);

		DatabaseClass *Database;
		ObjectManagerClass *ObjectManager;
		InstanceClass *Instances;

		u32 ServerTime;
};

extern ServerStateClass ServerState;

#endif
