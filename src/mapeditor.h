/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2012  Alan Witkowski
*
*	This program is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef MAPEDITOR_H
#define MAPEDITOR_H

// Libraries
#include "engine/state.h"
#include "instances/map.h"

// Classes
class MapEditorStateClass : public StateClass {

	public:

		enum ElementType {
			NEWMAP_WINDOW,
			NEWMAP_FILE,
			NEWMAP_WIDTH,
			NEWMAP_HEIGHT,
			NEWMAP_CREATE,
			NEWMAP_CANCEL,
			NEWMAP_ERROR,
			BRUSHOPTIONS_WINDOW,
			BRUSHOPTIONS_WALL,
			BRUSHOPTIONS_PVP,
			BRUSHOPTIONS_ZONE,
			BRUSHOPTIONS_EVENTTYPE,
			BRUSHOPTIONS_EVENTDATA,
			BRUSHOPTIONS_FILTERTEXTURE,
			BRUSHOPTIONS_FILTERWALL,
			BRUSHOPTIONS_FILTERZONE,
			BRUSHOPTIONS_FILTERPVP,
			BRUSHOPTIONS_FILTEREVENTTYPE,
			BRUSHOPTIONS_FILTEREVENTDATA,
			BRUSHOPTIONS_FILTERCLOSE,
			LOADMAP_WINDOW,
			TEXTUREPALETTE_WINDOW,
			TEXTURES_ID=100,
		};

		enum StateType {
			STATE_MAIN,
			STATE_NEWMAP,
			STATE_LOADMAP,
			STATE_TEXTUREPALETTE,
			STATE_BRUSHOPTIONS,
		};

		enum FilterType {
			FILTER_TEXTURE,
			FILTER_WALL,
			FILTER_PVP,
			FILTER_ZONE,
			FILTER_EVENTTYPE,
			FILTER_EVENTDATA,
			FILTER_COUNT
		};

		int Init();
		int Close();

		bool HandleKeyPress(EKEY_CODE Key);
		bool HandleKeyRelease(EKEY_CODE Key) { return false; }
		bool HandleMousePress(int Button, int MouseX, int MouseY);
		void HandleMouseRelease(int Button, int MouseX, int MouseY) { }
		void HandleMouseWheel(float Direction);
		void HandleMouseMotion(int MouseX, int MouseY);
		void HandleGUI(EGUI_EVENT_TYPE EventType, IGUIElement *Element);

		void Update(u32 FrameTime);
		void Draw();

	private:

		void CloseWindow(int Element);
		void CloseMap();

		// New
		void InitNewMap();
		void CreateMap();

		// Loading
		void InitLoadMap();

		// Texture palette
		void InitTexturePalette();
		void RefreshTexturePalette();
		
		// Brushes
		void InitBrushOptions();
		void ApplyBrushSize(int X, int Y, int Size);
		void ApplyBrush(int X, int Y);
		void RenderBrush();
		void ResetFilters();

		// General
		int State;

		// Map
		MapClass *Map;

		// Textures
		array<ITexture *> TexturePalette;

		// Brush
		int BrushSize;
		TileStruct Brush;
		position2di BrushPosition;

		// Filters
		bool Filters[FILTER_COUNT];
};

extern MapEditorStateClass MapEditorState;

#endif
