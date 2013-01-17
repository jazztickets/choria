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
#include "graphics.h"
#include "globals.h"

GraphicsClass Graphics;

// Initializes the graphics system
int GraphicsClass::Init(int Width, int Height, bool FullScreen, E_DRIVER_TYPE DriverType, IEventReceiver *EventReceiver) {

	Width = Width;
	Height = Height;

	// Create the irrlicht device
	irrDevice = createDevice(DriverType, dimension2du(Width, Height), 32, FullScreen, true, false, EventReceiver);
	if(irrDevice == NULL)
		return 0;

	irrDevice->setWindowCaption(L"Choria");
	irrDevice->setEventReceiver(EventReceiver);

	// Save off global pointers
	irrDriver = irrDevice->getVideoDriver();
	irrScene = irrDevice->getSceneManager();
	irrGUI = irrDevice->getGUIEnvironment();
	irrFile = irrDevice->getFileSystem();
	irrTimer = irrDevice->getTimer();

	// Load skin data
	Skin = irrGUI->getSkin();

	// Load fonts
	Fonts[FONT_7] = irrGUI->getFont("fonts/font_7.xml");
	Fonts[FONT_8] = irrGUI->getFont("fonts/font_8.xml");
	Fonts[FONT_10] = irrGUI->getFont("fonts/font_10.xml");
	Fonts[FONT_14] = irrGUI->getFont("fonts/font_14.xml");
	Fonts[FONT_18] = irrGUI->getFont("fonts/font_18.xml");

	// Set font
	SetFont(FONT_10);

	// Set colors
	Skin->setColor(EGDC_BUTTON_TEXT, SColor(255, 255, 255, 255));
	Skin->setColor(EGDC_WINDOW, SColor(255, 0, 0, 20));	
	Skin->setColor(EGDC_WINDOW_SYMBOL, SColor(255, 255, 255, 255));
	Skin->setColor(EGDC_3D_FACE, SColor(255, 0, 0, 20));
	Skin->setColor(EGDC_3D_SHADOW, SColor(255, 0, 0, 20));
	Skin->setColor(EGDC_3D_HIGH_LIGHT, SColor(255, 120, 120, 120));
	Skin->setColor(EGDC_3D_DARK_SHADOW, SColor(255, 50, 50, 50));
	Skin->setColor(EGDC_EDITABLE, SColor(255, 0, 0, 0));
	Skin->setColor(EGDC_FOCUSED_EDITABLE, SColor(255, 0, 0, 0));
	Skin->setColor(EGDC_GRAY_EDITABLE, SColor(255, 0, 0, 0));

	// Load images
	Images[IMAGE_EMPYSLOT] = irrDriver->getTexture("textures/interface/emptyslot.png");
	Images[IMAGE_SELECTEDSLOT] = irrDriver->getTexture("textures/interface/selectedslot.png");
	Images[IMAGE_HEALTH] = irrDriver->getTexture("textures/interface/health.png");
	Images[IMAGE_HEALTHMAX] = irrDriver->getTexture("textures/interface/healthmax.png");
	Images[IMAGE_MANA] = irrDriver->getTexture("textures/interface/mana.png");
	Images[IMAGE_MANAMAX] = irrDriver->getTexture("textures/interface/manamax.png");
	Images[IMAGE_TIMER] = irrDriver->getTexture("textures/interface/timer.png");
	Images[IMAGE_TIMERMAX] = irrDriver->getTexture("textures/interface/timermax.png");
	Images[IMAGE_EXPERIENCE] = irrDriver->getTexture("textures/interface/experience.png");
	Images[IMAGE_EXPERIENCEMAX] = irrDriver->getTexture("textures/interface/experiencemax.png");
	Images[IMAGE_BLACK] = irrDriver->getTexture("textures/interface/black.png");
	Images[IMAGE_GOLD] = irrDriver->getTexture("textures/interface/gold.png");
	Images[IMAGE_PVP] = irrDriver->getTexture("textures/interface/pvp.png");
	Images[IMAGE_MENULOGO] = irrDriver->getTexture("textures/menu/logo.png");
	Images[IMAGE_MENUBLANKSLOT] = irrDriver->getTexture("textures/menu/blankslot.png");
	Images[IMAGE_MENUSELECTED] = irrDriver->getTexture("textures/menu/selected.png");
	Images[IMAGE_WORLDBUSY] = irrDriver->getTexture("textures/world/busy.png");
	Images[IMAGE_WORLDTRADE] = irrDriver->getTexture("textures/world/trade.png");
	Images[IMAGE_BATTLESLOTLEFT] = irrDriver->getTexture("textures/battle/slot_left.png");
	Images[IMAGE_BATTLESLOTRIGHT] = irrDriver->getTexture("textures/battle/slot_right.png");
	Images[IMAGE_BATTLEEXPERIENCE] = irrDriver->getTexture("textures/battle/experience.png");
	Images[IMAGE_BATTLECOINS] = irrDriver->getTexture("textures/battle/coins.png");
	Images[IMAGE_BATTLECHEST] = irrDriver->getTexture("textures/battle/chest.png");
	Images[IMAGE_BATTLETARGET] = irrDriver->getTexture("textures/battle/target.png");
	Images[IMAGE_INVENTORY] = irrDriver->getTexture("textures/interface/inventory.png");
	Images[IMAGE_VENDOR] = irrDriver->getTexture("textures/interface/vendor.png");
	Images[IMAGE_TRADE] = irrDriver->getTexture("textures/interface/trade.png");
	Images[IMAGE_TRADER] = irrDriver->getTexture("textures/interface/trader.png");
	Images[IMAGE_PLUS] = irrDriver->getTexture("textures/interface/plus.png");
	Images[IMAGE_MINUS] = irrDriver->getTexture("textures/interface/minus.png");

	irrDriver->getTexture("textures/interface/hud_spawn.png");
	irrDriver->getTexture("textures/interface/hud_inventory.png");
	irrDriver->getTexture("textures/interface/hud_trade.png");
	irrDriver->getTexture("textures/interface/hud_character.png");
	irrDriver->getTexture("textures/interface/hud_skills.png");
	irrDriver->getTexture("textures/interface/hud_menu.png");

	return 1;
}

// Closes the graphics system
int GraphicsClass::Close() {

	// Close irrlicht
	irrDevice->drop();

	return 1;
}

// Erases the buffer and sets irrlicht up for the next frame
void GraphicsClass::BeginFrame() {
	irrDriver->beginScene(true, true, ClearColor);
}

// Draws the buffer to the screen
void GraphicsClass::EndFrame() {
	irrDriver->endScene();
}

// Draws an 2d image centered about a point
void GraphicsClass::DrawCenteredImage(const ITexture *Texture, int PositionX, int PositionY, const SColor &Color) {

	irrDriver->draw2DImage(Texture, position2di(PositionX - (Texture->getSize().Width >> 1), PositionY - (Texture->getSize().Height >> 1)), rect<s32>(0, 0, Texture->getSize().Width, Texture->getSize().Height), 0, Color, true);
}

// Sets the current font
void GraphicsClass::SetFont(int Type) {

	CurrentFont = Type;
	Skin->setFont(Fonts[CurrentFont]);
}

// Adds text to the screen
IGUIStaticText *GraphicsClass::AddText(const char *Text, int PositionX, int PositionY, AlignType AlignType, IGUIElement *Parent) {

	// Convert string
	stringw TextW(Text);

	// Get dimensions
	dimension2du TextArea = Fonts[CurrentFont]->getDimension(TextW.c_str());

	switch(AlignType) {
		case ALIGN_LEFT:
		break;
		case ALIGN_CENTER:
			PositionX -= TextArea.Width >> 1;
		break;
		case ALIGN_RIGHT:
			PositionX -= TextArea.Width;
		break;
	}

	// Draw text
	return irrGUI->addStaticText(TextW.c_str(), rect<s32>(PositionX, PositionY, PositionX + TextArea.Width, PositionY + TextArea.Height), 0, false, Parent);
}

// Draws text to the screen
void GraphicsClass::RenderText(const char *Text, int PositionX, int PositionY, AlignType AlignType, const SColor &Color) {

	// Convert string
	stringw TextW(Text);

	// Get dimensions
	dimension2du TextArea = Fonts[CurrentFont]->getDimension(TextW.c_str());

	switch(AlignType) {
		case ALIGN_LEFT:
		break;
		case ALIGN_CENTER:
			PositionX -= TextArea.Width >> 1;
		break;
		case ALIGN_RIGHT:
			PositionX -= TextArea.Width;
		break;
	}

	// Draw text
	Fonts[CurrentFont]->draw(TextW.c_str(), rect<s32>(PositionX, PositionY, PositionX + TextArea.Width, PositionY + TextArea.Height), Color);
}

// Return a centered rect
rect<s32> GraphicsClass::GetCenteredRect(int PositionX, int PositionY, int Width, int Height) {
	Width >>= 1;
	Height >>= 1;

	return rect<s32>(PositionX - Width, PositionY - Height, PositionX + Width, PositionY + Height);
}

// Return a rect
rect<s32> GraphicsClass::GetRect(int PositionX, int PositionY, int Width, int Height) {
	
	return rect<s32>(PositionX, PositionY, PositionX + Width, PositionY + Height);
}

// Clear all the GUI elements
void GraphicsClass::Clear() {

	irrGUI->clear();
}

// Draws an interface image
void GraphicsClass::DrawImage(ImageType Type, int PositionX, int PositionY, const SColor &Color) {

	Graphics.DrawCenteredImage(Images[Type], PositionX, PositionY, Color);
}

// Draws a health or mana bar
void GraphicsClass::DrawBar(ImageType Type, int PositionX, int PositionY, float Percent, int Width, int Height) {

	irrDriver->draw2DImage(Images[Type + 1], position2di(PositionX, PositionY), rect<s32>(0, 0, Width, Height), 0, SColor(255, 255, 255, 255), true);
	irrDriver->draw2DImage(Images[Type], position2di(PositionX, PositionY), rect<s32>(0, 0, (int)(Width * Percent), Height), 0, SColor(255, 255, 255, 255), true);
}

// Draws a tiled background
void GraphicsClass::DrawBackground(ImageType Type, int PositionX, int PositionY, int Width, int Height, const SColor &Color) {

	irrDriver->draw2DImage(Images[Type], position2di(PositionX, PositionY), rect<s32>(0, 0, Width, Height), 0, Color, true);
}
