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

#include <state.h>
#include <log.h>
#include <packet.h>

// Forward Declarations
class _Font;
class _HUD;
class _Map;
class _Object;
class _Item;
class _Camera;
class _ClientNetwork;
class _Server;
class _Buffer;
class _Stats;

// Play state
class _ClientState : public _State {

	public:

		// Setup
		_ClientState();
		void Init() override;
		void Close() override;

		// Network
		void Connect(bool IsLocal);
		void StopLocalServer();
		void SendStatus(int Status);

		// Input
		bool HandleAction(int InputType, int Action, int Value) override;
		void KeyEvent(const _KeyEvent &KeyEvent) override;
		void MouseEvent(const _MouseEvent &MouseEvent) override;
		void WindowEvent(uint8_t Event) override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		// Parameters
		bool IsTesting;
		bool FromEditor;
		bool ConnectNow;

		// Game
		_Stats *Stats;
		_LogFile Log;
		double Time;

		// Objects
		_Object *Player;
		_Map *Map;

		// HUD
		_HUD *HUD;

		// Camera
		_Camera *Camera;

		// Network
		_ClientNetwork *Network;
		_Server *Server;
		std::string HostAddress;
		uint16_t ConnectPort;

	protected:

		void StartLocalServer();

		void HandleConnect();
		void HandleDisconnect();
		void HandlePacket(_Buffer &Data);

		void HandleYourCharacterInfo(_Buffer &Data);
		void HandleChangeMaps(_Buffer &Data);
		void HandleObjectList(_Buffer &Data);
		void HandleCreateObject(_Buffer &Data);
		void HandleDeleteObject(_Buffer &Data);
		void HandleObjectUpdates(_Buffer &Data);
		void HandlePlayerPosition(_Buffer &Data);
		void HandleEventStart(_Buffer &Data);
		void HandleInventoryUse(_Buffer &Data);
		void HandleChatMessage(_Buffer &Data);
		void HandleTradeRequest(_Buffer &Data);
		void HandleTradeCancel(_Buffer &Data);
		void HandleTradeItem(_Buffer &Data);
		void HandleTradeGold(_Buffer &Data);
		void HandleTradeAccept(_Buffer &Data);
		void HandleTradeExchange(_Buffer &Data);
		void HandleBattleStart(_Buffer &Data);
		void HandleBattleTurnResults(_Buffer &Data);
		void HandleBattleEnd(_Buffer &Data);
		void HandleBattleCommand(_Buffer &Data);
		void HandleHUD(_Buffer &Data);

		_Object *CreateObject(_Buffer &Data, NetworkIDType NetworkID);

		//void SendAttackPlayer();
};

extern _ClientState ClientState;
