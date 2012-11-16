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
#ifndef CLIENT_H
#define CLIENT_H

// Libraries
#include "engine/state.h"

// Forward Declarations
class DatabaseClass;
class ObjectManagerClass;
class InstanceClass;
class PacketClass;
class PlayerClass;
class MapClass;
class ClientBattleClass;

// Classes
class ClientStateClass : public StateClass {

	public:
		
		enum StateType {
			STATE_CONNECTING,
			STATE_MAINMENU,
			STATE_WALK,
			STATE_BATTLE,
			STATE_TOWNPORTAL,
			STATE_INVENTORY,
			STATE_VENDOR,
			STATE_TRADER,
			STATE_SKILLS,
			STATE_TRADE,
		};

		ClientStateClass();

		int Init();
		int Close();

		void HandleConnect(ENetEvent *Event);
		void HandleDisconnect(ENetEvent *Event);
		void HandlePacket(ENetEvent *Event);
		bool HandleKeyPress(EKEY_CODE Key);
		bool HandleKeyRelease(EKEY_CODE Key) { return false; }
		void HandleMouseMotion(int MouseX, int MouseY);
		bool HandleMousePress(int Button, int MouseX, int MouseY);
		void HandleMouseRelease(int Button, int MouseX, int MouseY);
		void HandleGUI(EGUI_EVENT_TYPE EventType, IGUIElement *Element);
	
		void Update(u32 FrameTime);
		void Draw();

		void SetCharacterSlot(int Slot) { CharacterSlot = Slot; }

		int *GetState() { return &State; }

	private:

		void HandleYourCharacterInfo(PacketClass *Packet);
		void HandleChangeMaps(PacketClass *Packet);
		void HandleCreateObject(PacketClass *Packet);
		void HandleDeleteObject(PacketClass *Packet);
		void HandleObjectUpdates(PacketClass *Packet);
		void HandleStartBattle(PacketClass *Packet);
		void HandleBattleUpdate(PacketClass *Packet);
		void HandleBattleEnd(PacketClass *Packet);
		void HandleBattleAction(PacketClass *Packet);		
		void HandleHUD(PacketClass *Packet);
		void HandlePlayerPosition(PacketClass *Packet);
		void HandleEventStart(PacketClass *Packet);
		void HandleInventoryUse(PacketClass *Packet);
		void HandleChatMessage(PacketClass *Packet);
		void HandleTradeRequest(PacketClass *Packet);
		void HandleTradeCancel(PacketClass *Packet);
		void HandleTradeItem(PacketClass *Packet);
		void HandleTradeGold(PacketClass *Packet);
		void HandleTradeAccept(PacketClass *Packet);
		void HandleTradeExchange(PacketClass *Packet);

		void SendMoveCommand(int Direction);
		void SendAttackPlayer();
		
		// States
		int State, CharacterSlot;

		// Time
		u32 ClientTime, SentClientTime;

		// Objects
		PlayerClass *Player;
		MapClass *Map;
		ClientBattleClass *Battle;
		ObjectManagerClass *ObjectManager;
		InstanceClass *Instances;

};

extern ClientStateClass ClientState;

#endif
