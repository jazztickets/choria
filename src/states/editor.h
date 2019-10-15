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
#include <ae/bounds.h>
#include <ae/state.h>
#include <vector>
#include <unordered_map>
#include <glm/vec2.hpp>

struct _Tile;
class _Object;
class _Map;
class _Stats;
class _Light;
class _Prop;
class _Scripting;

namespace ae {
	class _Camera;
	class _Texture;
	class _Element;
	class _Framebuffer;
}

enum class EditorModeType : int {
	TILES,
	LIGHTS,
	PROPS,
};

// Classes
class _EditorState : public ae::_State {

	public:

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

		void AllocateCopy();
		void CloseCopy();
		void CloseMap();
		void CreateMap();
		void ResizeMap();
		void SaveMap();
		void LoadMap();
		void Go();

		void ToggleTextures();
		void ToggleZones();
		void ToggleLights();
		void ToggleProps();
		void ToggleEvents();
		void ToggleNewMap();
		void ToggleResize();
		void ToggleSaveMap();
		void ToggleLoadMap(const std::string &TempPath = "");

		void InitTextures();
		void InitZones();
		void InitLights();
		void InitProps();
		void InitEvents();
		void InitNewMap();
		void InitResize();
		void InitSaveMap();
		void InitLoadMap(const std::string &TempPath = "");
		void ClearTextures();
		void ClearZones();
		void ClearLights();
		void ClearProps();
		void ClearEvents();
		bool CloseWindows();

		// UI
		void SetLightUI(const glm::vec4 &Color, const std::string &Script);
		void UpdateSliders();
		void GetDrawBounds(ae::_Bounds &Bounds, bool Round);
		void GetTileDrawBounds(glm::ivec2 &Start, glm::ivec2 &End);

		// Modes
		void SwitchMode(EditorModeType Value);

		// Objects
		void StartDrawingObject();
		bool TouchingSelectedObjects(const glm::vec2 &Position);
		void GetCursorOffset(glm::vec2 &Offset, const glm::vec2 &Start);
		void DeleteSelectedObjects();
		void SetObjectSize(_Object *Object, bool AsRectangle);
		void CopyObjects();
		void PasteObjects();
		_Object *GetSingleSelectedObject();

		// Brushes
		void ApplyBrush(const glm::vec2 &Position);
		void DrawBrushInfo();
		void SwitchBrushModes(int Key);
		float GetObjectRadius();

		// Tiles
		void CopyTiles();
		void PasteTiles();

		// General
		std::vector<std::pair<std::string, float *> > SliderData;
		const _Stats *Stats;
		_Scripting *Scripting;

		// Graphics
		ae::_Camera *Camera;
		ae::_Framebuffer *Framebuffer;
		glm::vec2 WorldCursor;

		// Map
		_Map *Map;
		_Tile **CopyBuffer;
		std::string FilePath;
		double Clock;
		bool UseClockAmbientLight;

		// Copy paste
		bool CopiedTiles;
		bool DrawCopyBounds;
		glm::ivec2 CopyStart;
		glm::ivec2 CopyEnd;

		// Brush
		EditorModeType Mode;
		bool ShowTransitions;
		float BrushRadius;
		_Tile *TileBrush;

		// Objects
		std::unordered_map <_Object *, int> SelectedObjects;
		bool DrawingObject;
		bool DrawingSelect;
		bool MovingObjects;
		bool ResizingObject;
		bool CopiedObjects;
		glm::vec2 CopyPosition;
		glm::vec2 ObjectStart;
		_Light *LightBrush;
		_Prop *PropBrush;

		// Filter
		int Filter;

		// UI
		ae::_Element *EditorElement;
		ae::_Element *ButtonBarElement;
		ae::_Element *ClockElement;
		ae::_Element *TexturesElement;
		ae::_Element *ZonesElement;
		ae::_Element *LightsElement;
		ae::_Element *LightTypesElement;
		ae::_Element *LightDataElement;
		ae::_Element *PropsElement;
		ae::_Element *PropTypesElement;
		ae::_Element *EventsElement;
		ae::_Element *EventTypesElement;
		ae::_Element *EventDataElement;
		ae::_Element *EventMessageElement;
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
