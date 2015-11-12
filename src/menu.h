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
//#include <save.h>
#include <string>

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

		// Menu states
		enum StateType {
			STATE_NONE,
			STATE_TITLE,
			STATE_SINGLEPLAYER,
			STATE_OPTIONS,
			STATE_INGAME,
		};

		enum OptionsStateType {
			OPTION_NONE,
			OPTION_ACCEPT_INPUT,
		};

		enum SinglePlayerStateType {
			SINGLEPLAYER_NONE,
			SINGLEPLAYER_NEW_PLAYER,
		};

		enum KeyLabelType {
			LABEL_UP,
			LABEL_DOWN,
			LABEL_LEFT,
			LABEL_RIGHT,
			LABEL_USE,
			LABEL_INVENTORY,
			LABEL_FIRE,
			LABEL_AIM,
			LABEL_RELOAD,
			LABEL_SWITCH,
			LABEL_MEDKIT,
			LABEL_COUNT
		};

		enum ColorType {
			COLOR_BLACK,
			COLOR_RED,
			COLOR_GREEN,
			COLOR_BLUE,
			COLOR_COUNT,
		};

		enum SlotType {
			SAVE_COUNT=10,
		};

		_Menu();

		void InitTitle();
		void InitTutorial();
		void InitSinglePlayer();
		void InitOptions();
		void InitInGame();
		void InitPlay();
		void Close();

		void KeyEvent(const _KeyEvent &KeyEvent);
		void TextEvent(const char *Text);
		void MouseEvent(const _MouseEvent &MouseEvent);

		void Update(double FrameTime);
		void Render();

		const StateType &GetState() const { return State; }

	private:

		void InitNewPlayer();
		void LaunchGame();

		void RefreshInputLabels();
		void RefreshSaveSlots();
		void CancelCreate();
		void CreatePlayer();
		void RemapInput(int InputType, int Input);

		// States
		StateType State;

		// UI
		_Image *Background;
		_Element *CurrentLayout;
		_Label *InputLabels[LABEL_COUNT];
		_Button *SaveSlots[SAVE_COUNT];
		_Button *ColorButtons[4];

		// Double click
		_Element *PreviousClick;
		double PreviousClickTimer;

		// Options
		OptionsStateType OptionsState;
		int CurrentAction;

		// Singleplayer
		SinglePlayerStateType SinglePlayerState;
		int SelectedSlot;
		int SelectedColor;
};

extern _Menu Menu;
