/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2010  Alan Witkowski
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
**************************************************************************************/
#ifndef PLAYCLIENT_H
#define PLAYCLIENT_H

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
class PlayClientState : public StateClass {

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

		PlayClientState();

		int Init();
		int Close();

		void HandleConnect(ENetEvent *TEvent);
		void HandleDisconnect(ENetEvent *TEvent);
		void HandlePacket(ENetEvent *TEvent);
		bool HandleKeyPress(EKEY_CODE TKey);
		bool HandleKeyRelease(EKEY_CODE TKey) { return false; }
		void HandleMouseMotion(int TMouseX, int TMouseY);
		bool HandleMousePress(int TButton, int TMouseX, int TMouseY);
		void HandleMouseRelease(int TButton, int TMouseX, int TMouseY);
		void HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement);
	
		void Update(u32 TDeltaTime);
		void Draw();

		void SetCharacterSlot(int TSlot) { CharacterSlot = TSlot; }

		int *GetState() { return &State; }

		static PlayClientState *Instance() {
			static PlayClientState ClassInstance;
			return &ClassInstance;
		}

	private:

		void HandleYourCharacterInfo(PacketClass *TPacket);
		void HandleChangeMaps(PacketClass *TPacket);
		void HandleCreateObject(PacketClass *TPacket);
		void HandleDeleteObject(PacketClass *TPacket);
		void HandleObjectUpdates(PacketClass *TPacket);
		void HandleStartBattle(PacketClass *TPacket);
		void HandleBattleTurnResults(PacketClass *TPacket);
		void HandleBattleEnd(PacketClass *TPacket);
		void HandleBattleCommand(PacketClass *TPacket);		
		void HandleHUD(PacketClass *TPacket);
		void HandlePlayerPosition(PacketClass *TPacket);
		void HandleEventStart(PacketClass *TPacket);
		void HandleInventoryUse(PacketClass *TPacket);
		void HandleChatMessage(PacketClass *TPacket);
		void HandleTradeRequest(PacketClass *TPacket);
		void HandleTradeCancel(PacketClass *TPacket);
		void HandleTradeItem(PacketClass *TPacket);
		void HandleTradeGold(PacketClass *TPacket);
		void HandleTradeAccept(PacketClass *TPacket);
		void HandleTradeExchange(PacketClass *TPacket);

		void SendMoveCommand(int TDirection);
		void SendAttackPlayer();
		void SynchronizeTime();

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

#endif
