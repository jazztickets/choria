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

// Libraries
#include <packet.h>
#include <string>
#include <vector>

// Forward Declarations
class _Element;
class _Button;
class _Label;
class _Image;
class _Buffer;
class _ClientNetwork;
class _Stats;
struct _MouseEvent;
struct _KeyEvent;

// Classes
class _Menu {

	public:

		struct _CharacterSlot {
			_CharacterSlot() : Button(nullptr), Name(nullptr), Level(nullptr), Used(false) {}
			_Button *Button;
			_Label *Name;
			_Label *Level;
			_Image *Image;
			bool Used;
		};

		// Menu states
		enum StateType {
			STATE_NONE,
			STATE_TITLE,
			STATE_CHARACTERS,
			STATE_CONNECT,
			STATE_ACCOUNT,
			STATE_INGAME,
		};

		enum CharactersStateType {
			CHARACTERS_NONE,
			CHARACTERS_CREATE,
			CHARACTERS_PLAYSENT,
		};

		enum KeyLabelType {
			LABEL_COUNT=1,
		};

		_Menu();

		void InitTitle(bool Disconnect=false);
		void InitCharacters();
		void InitNewCharacter();
		void InitConnect(bool UseConfig, bool ConnectNow=false);
		void InitAccount();
		void InitOptions();
		void InitInGame();
		void InitPlay();
		void InitEditor();
		void Close();

		void HandleAction(int InputType, int Action, int Value);
		void KeyEvent(const _KeyEvent &KeyEvent);
		void MouseEvent(const _MouseEvent &MouseEvent);

		void Update(double FrameTime);
		void Render();

		// Network
		void HandleConnect();
		void HandleDisconnect(bool WasSinglePlayer);
		void HandlePacket(_Buffer &Buffer, PacketType Type);

		void SetUsername(const std::string &Value) { DefaultUsername = Value; }
		void SetPassword(const std::string &Value) { DefaultPassword = Value; }
		void SetTitleMessage(const std::string &Message);

		StateType State;
		size_t PreSelectedSlot;

	private:

		void ChangeLayout(const std::string &ElementIdentifier);

		uint32_t GetSelectedIconID(_Element *ParentElement);
		size_t GetSelectedCharacter();
		void LoadPortraitButtons();
		void LoadBuildButtons();
		void ValidateCreateCharacter();
		void UpdateCharacterButtons();
		void CreateCharacter();
		void ConnectToHost();
		void PlayCharacter(size_t Slot);
		void SendAccountInfo(bool CreateAccount=false);
		void RequestCharacterList();

		void SetAccountMessage(const std::string &Message);
		void FocusNextElement();

		void ClearPortraits();
		void ClearBuilds();

		// States
		std::string DefaultUsername;
		std::string DefaultPassword;

		// UI
		_Element *CurrentLayout;
		std::vector<_CharacterSlot> CharacterSlots;

		// Double click
		_Element *PreviousClick;
		double PreviousClickTimer;

		// Singleplayer
		CharactersStateType CharactersState;
};

extern _Menu Menu;
