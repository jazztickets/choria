/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020  Alan Witkowski
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

#include <glm/vec3.hpp>
#include <ae/type.h>
#include <ae/state.h>
#include <ae/buffer.h>
#include <ae/log.h>
#include <unordered_map>

// Forward Declarations
class _HUD;
class _Map;
class _Battle;
class _Object;
class _BaseItem;
class _Server;
class _Stats;
class _StatChange;
class _Scripting;

namespace ae {
	template<class T> class _Manager;
	class _Font;
	class _Camera;
	class _ClientNetwork;
	class _Buffer;
	class _Console;
	class _Framebuffer;
}

// Play state
class _PlayState : public ae::_State {

	public:

		// Setup
		_PlayState();
		void Init() override;
		void Close() override;
		void StopGame();

		// Network
		void Connect(bool IsLocal);
		void StopLocalServer();
		void SendStatus(uint8_t Status);
		void SendActionUse(uint8_t Slot);
		void SendJoinRequest();
		void SendUseCommand();

		// Input
		bool HandleAction(int InputType, size_t Action, int Value) override;
		bool HandleKey(const ae::_KeyEvent &KeyEvent) override;
		void HandleMouseButton(const ae::_MouseEvent &MouseEvent) override;
		void HandleMouseMove(const glm::ivec2 &Position) override;
		bool HandleCommand(ae::_Console *Console) override;
		void HandleWindow(uint8_t Event) override;
		void HandleQuit() override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		// Sound
		void PlayCoinSound();
		void PlayDeathSound();

		// Menu map
		glm::vec3 GetRandomMapPosition();

		// Parameters
		bool DevMode;
		bool IsHardcore;
		bool FromEditor;
		bool ConnectNow;

		// Game
		const _Stats *Stats;
		ae::_LogFile Log;
		double Time;

		// Graphics
		ae::_Framebuffer *Framebuffer;

		// Scripting
		_Scripting *Scripting;

		// Objects
		ae::_Manager<_Object> *ObjectManager;
		_Object *Player;
		_Map *Map;
		_Battle *Battle;

		// HUD
		_HUD *HUD;

		// Camera
		ae::_Camera *Camera;
		ae::_Camera *MenuCamera;

		// Audio
		bool CoinSoundPlayed;

		// Network
		ae::_ClientNetwork *Network;
		_Server *Server;
		std::string HostAddress;
		uint16_t ConnectPort;

		// Menu
		_Map *MenuMap;
		glm::vec3 MenuCameraTargetPosition;

	protected:

		void StartLocalServer();

		void HandleConnect();
		void HandleDisconnect();
		void HandlePacket(ae::_Buffer &Data);

		void HandleObjectStats(ae::_Buffer &Data);
		void HandleClock(ae::_Buffer &Data);
		void HandleChangeMaps(ae::_Buffer &Data);
		void HandleObjectList(ae::_Buffer &Data);
		void HandleObjectCreate(ae::_Buffer &Data);
		void HandleObjectDelete(ae::_Buffer &Data);
		void HandleObjectUpdates(ae::_Buffer &Data);
		void HandlePlayerPosition(ae::_Buffer &Data);
		void HandleTeleportStart(ae::_Buffer &Data);
		void HandleEventStart(ae::_Buffer &Data);
		void HandleInventory(ae::_Buffer &Data);
		void HandleInventoryAdd(ae::_Buffer &Data);
		void HandleInventorySwap(ae::_Buffer &Data);
		void HandleInventoryUpdate(ae::_Buffer &Data);
		void HandleInventoryGold(ae::_Buffer &Data);
		void HandlePartyInfo(ae::_Buffer &Data);
		void HandleChatMessage(ae::_Buffer &Data);
		void HandleTradeRequest(ae::_Buffer &Data);
		void HandleTradeCancel(ae::_Buffer &Data);
		void HandleTradeInventory(ae::_Buffer &Data);
		void HandleTradeGold(ae::_Buffer &Data);
		void HandleTradeAccept(ae::_Buffer &Data);
		void HandleTradeExchange(ae::_Buffer &Data);
		void HandleBattleStart(ae::_Buffer &Data);
		void HandleBattleAction(ae::_Buffer &Data);
		void HandleBattleJoin(ae::_Buffer &Data);
		void HandleBattleLeave(ae::_Buffer &Data);
		void HandleBattleEnd(ae::_Buffer &Data);
		void HandleActionClear(ae::_Buffer &Data);
		void HandleActionStart(ae::_Buffer &Data);
		void HandleActionApply(ae::_Buffer &Data);
		void HandleStatChange(ae::_Buffer &Data, _StatChange &StatChange);
		void HandleHUD(ae::_Buffer &Data);
		void HandleMinigameSeed(ae::_Buffer &Data);

		_Object *CreateObject(ae::_Buffer &Data, ae::NetworkIDType NetworkID);

		void AssignPlayer(_Object *Object);
		void DeleteBattle();
		void DeleteMap();

		void SetViewProjection(ae::_Camera *CameraUsed);

		// Network
		ae::_Buffer PongPacket;
};

extern _PlayState PlayState;
