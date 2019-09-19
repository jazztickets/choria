/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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
#include <ae/state.h>
#include <vector>
#include <glm/vec2.hpp>

struct _Tile;
class _Map;
class _Stats;

namespace ae {
	class _Camera;
	class _Texture;
	class _Element;
	class _Framebuffer;
}

enum class EditorModeType : int {
	TILE,
	OBJECT,
};

// Classes
class _EditorState : public ae::_State {

	public:

		_EditorState();

		void Init() override;
		void Close() override;

		// Events
		bool HandleAction(int InputType, size_t Action, int Value) override;
		bool HandleKey(const ae::_KeyEvent &KeyEvent) override;
		void HandleMouseButton(const ae::_MouseEvent &MouseEvent) override;
		void HandleMouseWheel(int Direction) override;
		void HandleWindow(uint8_t Event) override;
		void HandleQuit() override;

		// Update
		void Update(double FrameTime) override;
		void Render(double BlendFactor) override;

		void SetFilePath(const std::string &FilePath) { this->FilePath = FilePath; }
		std::string GetCleanMapName(const std::string &Path);

	private:

		void CloseMap();
		void CreateMap();
		void ResizeMap();
		void SaveMap();
		void LoadMap();
		void SwitchBrushModes(int BrushMode);
		void Go();

		void ToggleTextures();
		void ToggleEvents();
		void ToggleNewMap();
		void ToggleResize();
		void ToggleSaveMap();
		void ToggleLoadMap(const std::string &TempPath = "");

		void InitTextures();
		void InitEvents();
		void InitNewMap();
		void InitResize();
		void InitSaveMap();
		void InitLoadMap(const std::string &TempPath = "");
		void ClearTextures();
		void ClearEvents();
		bool CloseWindows();

		// Brushes
		void ApplyBrush(const glm::vec2 &Position);
		void DrawBrushInfo();
		void AdjustValue(uint32_t &Value, int Direction);

		// Copy/Paste
		void Paste();
		void GetDrawBounds(glm::ivec2 &Start, glm::ivec2 &End);

		// General
		const _Stats *Stats;

		// Graphics
		ae::_Camera *Camera;
		ae::_Framebuffer *Framebuffer;
		glm::vec2 WorldCursor;

		// Map
		_Map *Map;
		std::string FilePath;
		double Clock;
		bool UseClockAmbientLight;

		// Copy paste
		bool DrawCopyBounds;
		glm::ivec2 CopyStart;
		glm::ivec2 CopyEnd;

		// Brush
		EditorModeType Mode;
		bool ShowTransitions;
		float BrushRadius;
		_Tile *Brush;

		// Objects
		int ObjectType;
		uint32_t ObjectData;

		// Filter
		int Filter;

		// UI
		ae::_Element *EditorElement;
		ae::_Element *ButtonBarElement;
		ae::_Element *ClockElement;
		ae::_Element *TexturesElement;
		ae::_Element *EventsElement;
		ae::_Element *EventTypesElement;
		ae::_Element *EventDataElement;
		ae::_Element *NewMapElement;
		ae::_Element *ResizeMapElement;
		ae::_Element *SaveMapElement;
		ae::_Element *LoadMapElement;
		ae::_Element *NewMapFilenameTextBox;
		ae::_Element *NewMapWidthTextBox;
		ae::_Element *NewMapHeightTextBox;
		ae::_Element *ResizeMinXTextBox;
		ae::_Element *ResizeMinYTextBox;
		ae::_Element *ResizeMaxXTextBox;
		ae::_Element *ResizeMaxYTextBox;
		ae::_Element *SaveMapTextBox;
		ae::_Element *LoadMapTextBox;
};

extern _EditorState EditorState;
