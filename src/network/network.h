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
#ifndef NETWORK_H
#define NETWORK_H

// Libraries
#include <enet/enet.h>

// Constants
const int NETWORKING_PORT = 60006;
const int NETWORKING_MESSAGESIZE = 100;

// Forward Declarations
class PacketClass;

class NetworkClass {

	public:

		enum PacketType {
			GAME_VERSION,
			CHAT_MESSAGE,
			ACCOUNT_LOGININFO,
			ACCOUNT_EXISTS,
			ACCOUNT_NOTFOUND,
			ACCOUNT_ALREADYLOGGEDIN,
			ACCOUNT_SUCCESS,
			CHARACTERS_REQUEST,
			CHARACTERS_LIST,
			CHARACTERS_PLAY,
			CHARACTERS_DELETE,
			CREATECHARACTER_INFO,
			CREATECHARACTER_SUCCESS,
			CREATECHARACTER_INUSE,
			WORLD_MOVECOMMAND,
			WORLD_YOURCHARACTERINFO,
			WORLD_CHANGEMAPS,
			WORLD_CREATEOBJECT,
			WORLD_DELETEOBJECT,
			WORLD_OBJECTUPDATES,
			WORLD_HUD,
			WORLD_STARTBATTLE,
			WORLD_BUSY,
			WORLD_ATTACKPLAYER,
			WORLD_TOWNPORTAL,
			WORLD_POSITION,
			BATTLE_ACTION,
			BATTLE_UPDATE,
			BATTLE_CLIENTDONE,
			BATTLE_END,
			EVENT_START,
			EVENT_END,
			INVENTORY_MOVE,
			INVENTORY_USE,
			INVENTORY_SPLIT,
			VENDOR_EXCHANGE,
			TRADER_ACCEPT,
			ACTIONS_ACTIONBAR,
			TRADE_REQUEST,
			TRADE_ITEM,
			TRADE_GOLD,
			TRADE_ACCEPT,
			TRADE_CANCEL,
			TRADE_EXCHANGE,
		};

		NetworkClass() { }
		virtual ~NetworkClass() { }

		virtual int Init(bool Server) { return 0; }
		virtual int Close() { return 0; }

		// Updates
		virtual void Update() { }

		// Connections
		virtual int Connect(const char *IPAddress) { return 0; }
		virtual void Disconnect() { }

		// Packets
		virtual void SendPacketToHost(PacketClass *Packet) { }
		virtual void SendPacketToPeer(PacketClass *Packet, ENetPeer *Peer) { }

	protected:

};

#endif
