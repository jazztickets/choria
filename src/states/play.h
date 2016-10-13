/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
#include <manager.h>
#include <unordered_map>

// Forward Declarations
class _Font;
class _HUD;
class _Map;
class _Battle;
class _Object;
class _Item;
class _Camera;
class _ClientNetwork;
class _Server;
class _Buffer;
class _Stats;
class _StatChange;
class _Scripting;

// Play state
class _PlayState : public _State {

	public:

		// Setup
		_PlayState();
		void Init() override;
		void Close() override;

		// Network
		void Connect(bool IsLocal);
		void StopLocalServer();
		void SendStatus(uint8_t Status);
		void SendActionUse(uint8_t Slot);
		void SendHelpRequest();

		// Input
		bool HandleAction(int InputType, int Action, int Value) override;
		void KeyEvent(const _KeyEvent &KeyEvent) override;
		void MouseMotionEvent(const glm::ivec2 &Position) override;
		void MouseEvent(const _MouseEvent &MouseEvent) override;
		void WindowEvent(uint8_t Event) override;
		void QuitEvent() override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		// Sound
		void PlayCoinSound();
		void PlayDeathSound();

		// Parameters
		bool IsTesting;
		bool IsHardcore;
		bool FromEditor;
		bool ConnectNow;

		// Game
		_Stats *Stats;
		_LogFile Log;
		double Time;

		// Scripting
		_Scripting *Scripting;

		// Objects
		_Manager<_Object> *ObjectManager;
		_Object *Player;
		_Map *Map;
		_Battle *Battle;

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

		void HandleObjectStats(_Buffer &Data);
		void HandleChangeMaps(_Buffer &Data);
		void HandleObjectList(_Buffer &Data);
		void HandleObjectCreate(_Buffer &Data);
		void HandleObjectDelete(_Buffer &Data);
		void HandleObjectUpdates(_Buffer &Data);
		void HandlePlayerPosition(_Buffer &Data);
		void HandleTeleportStart(_Buffer &Data);
		void HandleEventStart(_Buffer &Data);
		void HandleInventory(_Buffer &Data);
		void HandleInventoryUse(_Buffer &Data);
		void HandleInventorySwap(_Buffer &Data);
		void HandleInventoryUpdate(_Buffer &Data);
		void HandleInventoryGold(_Buffer &Data);
		void HandleChatMessage(_Buffer &Data);
		void HandleTradeRequest(_Buffer &Data);
		void HandleTradeCancel(_Buffer &Data);
		void HandleTradeItem(_Buffer &Data);
		void HandleTradeGold(_Buffer &Data);
		void HandleTradeAccept(_Buffer &Data);
		void HandleTradeExchange(_Buffer &Data);
		void HandleBattleStart(_Buffer &Data);
		void HandleBattleAction(_Buffer &Data);
		void HandleBattleJoin(_Buffer &Data);
		void HandleBattleLeave(_Buffer &Data);
		void HandleBattleEnd(_Buffer &Data);
		void HandleActionClear(_Buffer &Data);
		void HandleActionResults(_Buffer &Data);
		void HandleStatChange(_Buffer &Data, _StatChange &StatChange);
		void HandleHUD(_Buffer &Data);

		_Object *CreateObject(_Buffer &Data, NetworkIDType NetworkID);

		void AssignPlayer(_Object *Object);
		void DeleteBattle();
		void DeleteMap();
};

extern _PlayState PlayState;
