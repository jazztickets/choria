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

// Forward Declarations
class _Database;
class _ObjectManager;
class _Instance;
class _Buffer;
class _Player;
class _Map;
class _Camera;
class _ClientBattle;

// Classes
class _ClientState : public _State {

	public:

		enum StateType {
			STATE_CONNECTING,
			STATE_WALK,
			STATE_BATTLE,
			STATE_TELEPORT,
			STATE_INVENTORY,
			STATE_VENDOR,
			STATE_TRADER,
			STATE_SKILLS,
			STATE_TRADE,
		};

		_ClientState();

		void Init() override;
		void Close() override;

		bool HandleAction(int InputType, int Action, int Value) override;
		void KeyEvent(const _KeyEvent &KeyEvent) override;
		void TextEvent(const char *Text) override;
		void MouseEvent(const _MouseEvent &MouseEvent) override;
		void WindowEvent(uint8_t Event) override;

		void HandleConnect(ENetEvent *Event) override;
		void HandleDisconnect(ENetEvent *Event) override;
		void HandlePacket(ENetEvent *Event) override;

		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		void SetCharacterSlot(int Slot) { CharacterSlot = Slot; }
		void SetIsTesting(bool Value) { IsTesting = Value; }

		int *GetState() { return &State; }

		void SendBusy(bool Value);

	private:

		void HandleYourCharacterInfo(_Buffer *Packet);
		void HandleChangeMaps(_Buffer *Packet);
		void HandleCreateObject(_Buffer *Packet);
		void HandleDeleteObject(_Buffer *Packet);
		void HandleObjectUpdates(_Buffer *Packet);
		void HandleStartBattle(_Buffer *Packet);
		void HandleBattleTurnResults(_Buffer *Packet);
		void HandleBattleEnd(_Buffer *Packet);
		void HandleBattleCommand(_Buffer *Packet);
		void HandleHUD(_Buffer *Packet);
		void HandlePlayerPosition(_Buffer *Packet);
		void HandleEventStart(_Buffer *Packet);
		void HandleInventoryUse(_Buffer *Packet);
		void HandleChatMessage(_Buffer *Packet);
		void HandleTradeRequest(_Buffer *Packet);
		void HandleTradeCancel(_Buffer *Packet);
		void HandleTradeItem(_Buffer *Packet);
		void HandleTradeGold(_Buffer *Packet);
		void HandleTradeAccept(_Buffer *Packet);
		void HandleTradeExchange(_Buffer *Packet);

		void SendMoveCommand(int Direction);
		void SendAttackPlayer();
		void SynchronizeTime();

		// States
		int State;
		int CharacterSlot;
		bool IsTesting;

		// Time
		double ClientTime, SentClientTime;

		// Graphics
		_Camera *Camera;

		// Objects
		_Player *Player;
		_Map *Map;
		_ClientBattle *Battle;
		_ObjectManager *ObjectManager;
		_Instance *Instances;

};

extern _ClientState ClientState;
