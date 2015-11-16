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

		void Init() override;
		void Close() override;

		bool HandleAction(int InputType, int Action, int Value) override;
		void KeyEvent(const _KeyEvent &KeyEvent) override;
		void TextEvent(const char *Text) override;
		void MouseEvent(const _MouseEvent &MouseEvent) override;

		void HandleConnect(ENetEvent *TEvent) override;
		void HandleDisconnect(ENetEvent *TEvent) override;
		void HandlePacket(ENetEvent *TEvent) override;

		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		void SetCharacterSlot(int TSlot) { CharacterSlot = TSlot; }

		int *GetState() { return &State; }

	private:

		void HandleYourCharacterInfo(_Buffer *TPacket);
		void HandleChangeMaps(_Buffer *TPacket);
		void HandleCreateObject(_Buffer *TPacket);
		void HandleDeleteObject(_Buffer *TPacket);
		void HandleObjectUpdates(_Buffer *TPacket);
		void HandleStartBattle(_Buffer *TPacket);
		void HandleBattleTurnResults(_Buffer *TPacket);
		void HandleBattleEnd(_Buffer *TPacket);
		void HandleBattleCommand(_Buffer *TPacket);
		void HandleHUD(_Buffer *TPacket);
		void HandlePlayerPosition(_Buffer *TPacket);
		void HandleEventStart(_Buffer *TPacket);
		void HandleInventoryUse(_Buffer *TPacket);
		void HandleChatMessage(_Buffer *TPacket);
		void HandleTradeRequest(_Buffer *TPacket);
		void HandleTradeCancel(_Buffer *TPacket);
		void HandleTradeItem(_Buffer *TPacket);
		void HandleTradeGold(_Buffer *TPacket);
		void HandleTradeAccept(_Buffer *TPacket);
		void HandleTradeExchange(_Buffer *TPacket);

		void SendMoveCommand(int TDirection);
		void SendAttackPlayer();
		void SynchronizeTime();

		// States
		int State, CharacterSlot;

		// Time
		double ClientTime, SentClientTime;

		// Objects
		_Player *Player;
		_Map *Map;
		_ClientBattle *Battle;
		_ObjectManager *ObjectManager;
		_Instance *Instances;

};

extern _PlayClientState PlayClientState;
