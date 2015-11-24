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
#include <string>
#include <constants.h>
#include <enet/enet.h>

// Forward Declarations
class _Element;
class _Button;
class _Label;
class _Image;
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
			STATE_OPTIONS,
			STATE_INGAME,
		};

		enum OptionsStateType {
			OPTION_NONE,
			OPTION_ACCEPT_INPUT,
		};

		enum CharactersStateType {
			CHARACTERS_NONE,
			CHARACTERS_CREATE,
		};

		enum KeyLabelType {
			LABEL_COUNT=1,
		};

		_Menu();

		void InitTitle();
		void InitCharacters();
		void InitNewCharacter();
		void InitConnect(bool ConnectNow=false);
		void InitAccount();
		void InitOptions();
		void InitInGame();
		void InitPlay();
		void Close();

		void HandleAction(int InputType, int Action, int Value);
		void KeyEvent(const _KeyEvent &KeyEvent);
		void MouseEvent(const _MouseEvent &MouseEvent);

		void Update(double FrameTime);
		void Render();

		// Network
		void HandleConnect(ENetEvent *Event);
		void HandleDisconnect(ENetEvent *Event);
		void HandlePacket(ENetEvent *Event);

		const StateType &GetState() const { return State; }

		void SetUsername(const std::string &Value) { DefaultUsername = Value; }
		void SetPassword(const std::string &Value) { DefaultPassword = Value; }

	private:

		void ChangeLayout(const std::string &ElementIdentifier);
		void Connect(const std::string &Address, uint16_t Port, bool Fake);

		int GetSelectedPortraitID();
		int GetSelectedCharacter();
		void LoadPortraitButtons();
		void ValidateCreateCharacter();
		void UpdateCharacterButtons();
		void RefreshInputLabels();
		void CreateCharacter();
		void ConnectToHost();
		void SendAccountInfo(bool CreateAccount=false);
		void RequestCharacterList();
		void RemapInput(int InputType, int Input);

		void SetAccountMessage(const std::string &Message);
		void FocusNextElement(bool ShiftDown=false);

		void ClearPortraits();

		// States
		StateType State;
		std::string DefaultUsername;
		std::string DefaultPassword;

		// UI
		_Element *CurrentLayout;
		_Label *InputLabels[LABEL_COUNT];
		_CharacterSlot CharacterSlots[SAVE_COUNT];

		// Double click
		_Element *PreviousClick;
		double PreviousClickTimer;

		// Options
		OptionsStateType OptionsState;
		int CurrentAction;

		// Singleplayer
		CharactersStateType CharactersState;
};

extern _Menu Menu;
