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
			_Button *Button;
			_Label *Name;
			_Label *Level;
		};

		// Menu states
		enum StateType {
			STATE_NONE,
			STATE_TITLE,
			STATE_CHARACTERS,
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

		enum SlotType {
			SAVE_COUNT=6,
		};

		_Menu();

		void InitTitle();
		void InitCharacters();
		void InitNewCharacter();
		void InitOptions();
		void InitInGame();
		void InitPlay();
		void Close();

		void KeyEvent(const _KeyEvent &KeyEvent);
		void TextEvent(const char *Text);
		void MouseEvent(const _MouseEvent &MouseEvent);

		void Update(double FrameTime);
		void Render();

		// Network
		void HandleConnect(ENetEvent *TEvent);
		void HandleDisconnect(ENetEvent *TEvent);
		void HandlePacket(ENetEvent *TEvent);

		const StateType &GetState() const { return State; }

	private:

		void Connect(const std::string &Address, bool Fake=false);

		void LaunchGame();

		int GetSelectedPortrait();
		void LoadPortraitButtons();
		void ValidateCreateCharacter();
		void RefreshInputLabels();
		void CreateCharacter();
		void RequestCharacterList();
		void RemapInput(int InputType, int Input);

		// States
		StateType State;

		// UI
		_Image *Background;
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
		int SelectedSlot;
};

extern _Menu Menu;
