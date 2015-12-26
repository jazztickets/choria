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
#include <vector>

struct _Tile;
class _Camera;
class _Map;
class _Texture;
class _Stats;
class _Element;
class _TextBox;

enum FilterType {
	FILTER_BOUNDARY  = (1 << 1),
	FILTER_TEXTURE   = (1 << 2),
	FILTER_WALL      = (1 << 3),
	FILTER_PVP       = (1 << 4),
	FILTER_ZONE      = (1 << 5),
	FILTER_EVENTTYPE = (1 << 6),
	FILTER_EVENTDATA = (1 << 7),
};

// Classes
class _EditorState : public _State {

	public:

		_EditorState();

		void Init() override;
		void Close() override;

		// Events
		void KeyEvent(const _KeyEvent &KeyEvent) override;
		void MouseEvent(const _MouseEvent &MouseEvent) override;
		void MouseWheelEvent(int Direction) override;
		void WindowEvent(uint8_t Event) override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		void SetFilePath(const std::string &FilePath) { this->FilePath = FilePath; }

	private:

		void ClearTextures();
		void CloseMap();
		void CreateMap();
		void ResizeMap();
		void SaveMap();
		void LoadMap();

		void ToggleTextures();
		void ToggleNewMap();
		void ToggleResize();
		void ToggleSaveMap();
		void ToggleLoadMap();

		void InitTextures();
		void InitNewMap();
		void InitResize();
		void InitSaveMap();
		void InitLoadMap();
		bool CloseWindows();

		// Brushes
		void ApplyBrush(const glm::vec2 &Position);
		void RenderBrush();

		// General
		_Stats *Stats;

		// Graphics
		_Camera *Camera;
		glm::vec2 WorldCursor;

		// Map
		_Map *Map;
		std::string FilePath;

		// Brush
		int Layer;
		float BrushRadius;
		_Tile *Brush;

		// Filter
		int Filter;

		// UI
		bool IgnoreFirstChar;
		_Element *ButtonBarElement;
		_Element *TexturesElement;
		_Element *NewMapElement;
		_Element *ResizeMapElement;
		_Element *SaveMapElement;
		_Element *LoadMapElement;
		_TextBox *NewMapFilenameTextBox;
		_TextBox *NewMapWidthTextBox;
		_TextBox *NewMapHeightTextBox;
		_TextBox *ResizeMinXTextBox;
		_TextBox *ResizeMinYTextBox;
		_TextBox *ResizeMaxXTextBox;
		_TextBox *ResizeMaxYTextBox;
		_TextBox *SaveMapTextBox;
		_TextBox *LoadMapTextBox;
};

extern _EditorState EditorState;
