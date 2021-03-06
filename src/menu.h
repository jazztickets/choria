/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2021 Alan Witkowski
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
#include <packet.h>
#include <string>
#include <vector>

// Forward Declarations
class _Stats;

namespace ae {
	class _Element;
	class _Buffer;
	class _ClientNetwork;
	struct _MouseEvent;
	struct _KeyEvent;
}

struct _ConnectServer {
	std::string IP;
	uint16_t Port;
	double Ping;
	int Players;
	int MaxPlayers;
	bool Hardcore;
};

// Classes
class _Menu {

	public:

		struct _CharacterSlot {
			_CharacterSlot() : Button(nullptr), Name(nullptr), Level(nullptr), Hardcore(nullptr), CanPlay(false), Used(false) {}
			ae::_Element *Button;
			ae::_Element *Name;
			ae::_Element *Level;
			ae::_Element *Hardcore;
			ae::_Element *Image;
			bool CanPlay;
			bool Used;
		};

		// Menu states
		enum StateType {
			STATE_NONE,
			STATE_TITLE,
			STATE_CHARACTERS,
			STATE_BROWSE,
			STATE_ACCOUNT,
			STATE_OPTIONS,
			STATE_KEYBINDINGS,
			STATE_INGAME,
		};

		enum CharactersStateType {
			CHARACTERS_NONE,
			CHARACTERS_CREATE,
			CHARACTERS_PLAYSENT,
			CHARACTERS_DELETE,
		};

		enum KeyLabelType {
			LABEL_COUNT=1,
		};

		_Menu();

		void InitTitle(bool Disconnect=false);
		void InitCharacters();
		void InitNewCharacter();
		void InitBrowseServers(bool UseConfig, bool ConnectNow=false);
		void InitAccount();
		void InitInGame();
		void InitPlay(bool SendStatus=true);
		void InitOptions();
		void InitKeybindings();
		void ConfirmAction();
		void ExitGame();
		void Close();

		bool HandleAction(int InputType, std::size_t Action, int Value);
		bool HandleKey(const ae::_KeyEvent &KeyEvent);
		void HandleMouseButton(const ae::_MouseEvent &MouseEvent);

		void SetFullscreen(bool Fullscreen);
		void Update(double FrameTime);
		void Render();

		// Network
		void HandleConnect();
		void HandleDisconnect(bool WasSinglePlayer);
		void HandlePacket(ae::_Buffer &Buffer, PacketType Type);

		void AddConnectServer(_ConnectServer &ConnectServer);
		void SetUsername(const std::string &Value) { DefaultUsername = Value; }
		void SetPassword(const std::string &Value) { DefaultPassword = Value; }
		void SetTitleMessage(const std::string &Message);

		// State
		StateType State;
		std::size_t PreSelectedSlot;
		bool ShowExitWarning;
		bool ShowRespawn;

	private:

		void PlayClickSound();

		void RenderBrowser();
		void ChangeLayout(const std::string &ElementName);

		uint32_t GetSelectedIconID(ae::_Element *ParentElement);
		std::size_t GetSelectedCharacter();
		void ValidateCreateCharacter();
		void UpdateCharacterButtons();
		void CreateCharacter(bool Hardcore=false);
		void ConnectToHost();
		void PlayCharacter(std::size_t Slot);
		void SendAccountInfo(bool CreateAccount=false);
		void RequestCharacterList();

		void SetAccountMessage(const std::string &Message);
		void FocusNextElement();

		void RefreshServers();
		void ValidateConnect();
		void SplitHost(const std::string &Host, std::string &IP, std::string &Port);
		void LoadCharacterSlots();
		void LoadPortraitButtons();
		void LoadBuildButtons();
		void LoadKeybindings();
		void ClearCharacterSlots();
		void ClearPortraits();
		void ClearBuilds();
		void ClearKeybindingElements();
		void UpdateOptions();
		void UpdateVolume();
		void ResetInGameState();

		void ShowNewKey(ae::_Element *Button, int Type);
		void ClearAction(int Action, int Type);
		void RemapInput(int InputType, int Input);

		// States
		std::string DefaultUsername;
		std::string DefaultPassword;
		double ConnectTimer;
		bool FromInGame;
		bool BadGameVersion;
		int RebindType;
		int RebindAction;

		// UI
		ae::_Element *CurrentLayout;
		std::vector<_CharacterSlot> CharacterSlots;

		// Double click
		ae::_Element *PreviousClick;
		double PreviousClickTimer;

		// Singleplayer
		CharactersStateType CharactersState;
		bool HardcoreServer;

		// Multiplayer
		uint64_t PingTime;
		std::vector<_ConnectServer> ConnectServers;

};

extern _Menu Menu;
