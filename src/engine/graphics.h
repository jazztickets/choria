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
*	but WITHOUT ANY WARRANY; without even the implied warranty of
*	MERCHANTABILIY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************************/
#ifndef GRAPHICS_H
#define GRAPHICS_H

// Libraries
#include <irrlicht.h>

// Namespaces
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// Classes
class GraphicsClass {

	public:

		enum AlignType {
			ALIGN_LEFT,
			ALIGN_CENTER,
			ALIGN_RIGHT,
		};

		enum FontType {
			FONT_7,
			FONT_8,
			FONT_10,
			FONT_14,
			FONT_18,
			FONT_COUNT,
		};
		
		enum ImageType {
			IMAGE_EMPYSLOT,
			IMAGE_SELECTEDSLOT,
			IMAGE_HEALTH,
			IMAGE_HEALTHMAX,
			IMAGE_MANA,
			IMAGE_MANAMAX,
			IMAGE_TIMER,
			IMAGE_TIMERMAX,
			IMAGE_EXPERIENCE,
			IMAGE_EXPERIENCEMAX,
			IMAGE_BLACK,
			IMAGE_GOLD,
			IMAGE_PVP,
			IMAGE_MENULOGO,
			IMAGE_MENUBLANKSLOT,
			IMAGE_MENUSELECTED,
			IMAGE_WORLDBUSY,
			IMAGE_WORLDTRADE,
			IMAGE_WORLDFIGHTING,
			IMAGE_BATTLESLOTLEFT,
			IMAGE_BATTLESLOTRIGHT,
			IMAGE_BATTLECOINS,
			IMAGE_BATTLEEXPERIENCE,
			IMAGE_BATTLECHEST,
			IMAGE_BATTLETARGET,
			IMAGE_INVENTORY,
			IMAGE_VENDOR,
			IMAGE_TRADE,
			IMAGE_TRADER,
			IMAGE_PLUS,
			IMAGE_MINUS,
			IMAGE_COUNT,
		};

		int Init(int Width, int Height, bool FullScreen, E_DRIVER_TYPE DriverType, IEventReceiver *EventReceiver);
		int Close();

		// Rendering
		void Clear();
		void SetClearColor(const SColor &Color) { ClearColor = Color; }
		void BeginFrame();
		void EndFrame();

		// Fonts
		void SetFont(int Type);
		IGUIFont *GetFont(int Type) const { return Fonts[Type]; }

		// Positions
		rect<s32> GetCenteredRect(int PositionX, int PositionY, int Width, int Height);
		rect<s32> GetRect(int PositionX, int PositionY, int Width, int Height);

		// Text
		IGUIStaticText *AddText(const char *Text, int PositionX, int PositionY, AlignType AlignType=ALIGN_LEFT, IGUIElement *Parent=NULL);
		void RenderText(const char *Text, int PositionX, int PositionY, AlignType AlignType=ALIGN_LEFT, const SColor &Color=SColor(255, 255, 255, 255));

		// Images
		void DrawCenteredImage(const ITexture *Texture, int PositionX, int PositionY, const SColor &Color=SColor(255, 255, 255, 255));
		void DrawImage(ImageType Type, int PositionX, int PositionY, const SColor &Color=SColor(255, 255, 255, 255));
		void DrawBar(ImageType Type, int PositionX, int PositionY, float Percent, int Width, int Height);
		void DrawBackground(ImageType Type, int PositionX, int PositionY, int Width, int Height, const SColor &Color=SColor(255, 255, 255, 255));
		ITexture *GetImage(ImageType Type) { return Images[Type]; }

	private:

		// Graphics state
		SColor ClearColor;
		int Width, Height;
		
		// Theme
		IGUISkin *Skin;

		// Fonts
		IGUIFont *Fonts[FONT_COUNT];
		int CurrentFont;

		// Images
		ITexture *Images[IMAGE_COUNT];

};

// Singletons
extern GraphicsClass Graphics;

#endif
