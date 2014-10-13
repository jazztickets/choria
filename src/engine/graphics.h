/*************************************************************************************
*	Choria - http://choria.googlecode.com/
*	Copyright (C) 2010  Alan Witkowski
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
#ifndef GRAPHICS_H
#define GRAPHICS_H

// Libraries
#include <irrlicht.h>
#include "singleton.h"

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
			IMAGE_EMPTYSLOT,
			IMAGE_SELECTEDSLOT,
			IMAGE_HEALTH,
			IMAGE_HEALTHMAX,
			IMAGE_MANA,
			IMAGE_MANAMAX,
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

		int Init(int TWidth, int THeight, bool TFullScreen, E_DRIVER_TYPE TDriverType, IEventReceiver *TEventReceiver);
		int Close();

		// Rendering
		void Clear();
		void SetClearColor(const SColor &TColor) { ClearColor = TColor; }
		void BeginFrame();
		void EndFrame();

		// Fonts
		void SetFont(int TType);
		IGUIFont *GetFont(int TType) const { return Fonts[TType]; }

		// Positions
		rect<s32> GetCenteredRect(int TPositionX, int TPositionY, int TWidth, int THeight);
		rect<s32> GetRect(int TPositionX, int TPositionY, int TWidth, int THeight);

		// Text
		IGUIStaticText *AddText(const char *TText, int TPositionX, int TPositionY, AlignType TAlignType=ALIGN_LEFT, IGUIElement *TParent=NULL);
		void RenderText(const char *TText, int TPositionX, int TPositionY, AlignType TAlignType=ALIGN_LEFT, const SColor &TColor=SColor(255, 255, 255, 255));

		// Images
		void DrawCenteredImage(const ITexture *TTexture, int TPositionX, int TPositionY, const SColor &TColor=SColor(255, 255, 255, 255));
		void DrawImage(ImageType TType, int TPositionX, int TPositionY, const SColor &TColor=SColor(255, 255, 255, 255));
		void DrawBar(ImageType TType, int TPositionX, int TPositionY, float TPercent, int TWidth, int THeight);
		void DrawBackground(ImageType TType, int TPositionX, int TPositionY, int TWidth, int THeight, const SColor &TColor=SColor(255, 255, 255, 255));
		ITexture *GetImage(ImageType TType) { return Images[TType]; }

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
typedef SingletonClass<GraphicsClass> Graphics;

#endif
