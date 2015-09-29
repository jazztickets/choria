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

// Forward Declarations
class DatabaseClass;
class ObjectManagerClass;
class InstanceClass;
class _Packet;
class PlayerClass;
class _Map;
class _ClientBattle;

// Classes
class _PlayClientState : public _State {

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

		_PlayClientState();

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

		void Update(uint32_t TDeltaTime);
		void Draw();

		void SetCharacterSlot(int TSlot) { CharacterSlot = TSlot; }

		int *GetState() { return &State; }

	private:

		void HandleYourCharacterInfo(_Packet *TPacket);
		void HandleChangeMaps(_Packet *TPacket);
		void HandleCreateObject(_Packet *TPacket);
		void HandleDeleteObject(_Packet *TPacket);
		void HandleObjectUpdates(_Packet *TPacket);
		void HandleStartBattle(_Packet *TPacket);
		void HandleBattleTurnResults(_Packet *TPacket);
		void HandleBattleEnd(_Packet *TPacket);
		void HandleBattleCommand(_Packet *TPacket);
		void HandleHUD(_Packet *TPacket);
		void HandlePlayerPosition(_Packet *TPacket);
		void HandleEventStart(_Packet *TPacket);
		void HandleInventoryUse(_Packet *TPacket);
		void HandleChatMessage(_Packet *TPacket);
		void HandleTradeRequest(_Packet *TPacket);
		void HandleTradeCancel(_Packet *TPacket);
		void HandleTradeItem(_Packet *TPacket);
		void HandleTradeGold(_Packet *TPacket);
		void HandleTradeAccept(_Packet *TPacket);
		void HandleTradeExchange(_Packet *TPacket);

		void SendMoveCommand(int TDirection);
		void SendAttackPlayer();
		void SynchronizeTime();

		// States
		int State, CharacterSlot;

		// Time
		uint32_t ClientTime, SentClientTime;

		// Objects
		PlayerClass *Player;
		_Map *Map;
		_ClientBattle *Battle;
		ObjectManagerClass *ObjectManager;
		InstanceClass *Instances;

};

extern _PlayClientState PlayClientState;
