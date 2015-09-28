/******************************************************************************
*	choria - https://github.com/jazztickets/choria
*	Copyright (C) 2015  Alan Witkowski
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
*******************************************************************************/
#include <graphics.h>
#include <globals.h>
#include <irrlicht.h>

GraphicsClass Graphics;

using namespace irr;

// Initializes the graphics system
int GraphicsClass::Init(int TWidth, int THeight, bool TFullScreen, video::E_DRIVER_TYPE TDriverType, IEventReceiver *TEventReceiver) {

	Width = TWidth;
	Height = THeight;

	// Create the irrlicht device
	irrDevice = createDevice(TDriverType, core::dimension2du(TWidth, THeight), 32, TFullScreen, true, false, TEventReceiver);
	if(irrDevice == NULL)
		return 0;

	irrDevice->setWindowCaption(L"choria");
	irrDevice->setEventReceiver(TEventReceiver);
	ILogger *Logger = irrDevice->getLogger();
	Logger->setLogLevel(ELL_ERROR);

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
	Skin->setColor(gui::EGDC_BUTTON_TEXT, video::SColor(255, 255, 255, 255));
	Skin->setColor(gui::EGDC_WINDOW, video::SColor(255, 0, 0, 20));
	Skin->setColor(gui::EGDC_WINDOW_SYMBOL, video::SColor(255, 255, 255, 255));
	Skin->setColor(gui::EGDC_3D_FACE, video::SColor(255, 0, 0, 20));
	Skin->setColor(gui::EGDC_3D_SHADOW, video::SColor(255, 0, 0, 20));
	Skin->setColor(gui::EGDC_3D_HIGH_LIGHT, video::SColor(255, 120, 120, 120));
	Skin->setColor(gui::EGDC_3D_DARK_SHADOW, video::SColor(255, 50, 50, 50));
	Skin->setColor(gui::EGDC_GRAY_WINDOW_SYMBOL, video::SColor(255, 128, 128, 128));
	Skin->setColor(gui::EGDC_GRAY_EDITABLE, video::SColor(255, 0, 0, 0));
	Skin->setColor(gui::EGDC_FOCUSED_EDITABLE, video::SColor(255, 0, 0, 0));
	Skin->setColor(gui::EGDC_EDITABLE, video::SColor(255, 0, 0, 0));

	// Load images
	Images[IMAGE_EMPTYSLOT] = irrDriver->getTexture("textures/interface/emptyslot.png");
	Images[IMAGE_SELECTEDSLOT] = irrDriver->getTexture("textures/interface/selectedslot.png");
	Images[IMAGE_HEALTH] = irrDriver->getTexture("textures/interface/health.png");
	Images[IMAGE_HEALTHMAX] = irrDriver->getTexture("textures/interface/healthmax.png");
	Images[IMAGE_MANA] = irrDriver->getTexture("textures/interface/mana.png");
	Images[IMAGE_MANAMAX] = irrDriver->getTexture("textures/interface/manamax.png");
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
void GraphicsClass::DrawCenteredImage(const video::ITexture *TTexture, int TPositionX, int TPositionY, const video::SColor &TColor) {

	if(TTexture)
		irrDriver->draw2DImage(
					TTexture,
					core::position2di(TPositionX - (TTexture->getSize().Width >> 1), TPositionY - (TTexture->getSize().Height >> 1)),
					core::recti(0, 0, TTexture->getSize().Width, TTexture->getSize().Height),
					0,
					TColor,
					true
					);
}

// Sets the current font
void GraphicsClass::SetFont(int TType) {

	CurrentFont = TType;
	Skin->setFont(Fonts[CurrentFont]);
}

// Adds text to the screen
gui::IGUIStaticText *GraphicsClass::AddText(const char *TText, int TPositionX, int TPositionY, AlignType TAlignType, gui::IGUIElement *TParent) {

	// Convert string
	core::stringw Text(TText);

	// Get dimensions
	core::dimension2du TextArea = Fonts[CurrentFont]->getDimension(Text.c_str());

	switch(TAlignType) {
		case ALIGN_LEFT:
		break;
		case ALIGN_CENTER:
			TPositionX -= TextArea.Width >> 1;
		break;
		case ALIGN_RIGHT:
			TPositionX -= TextArea.Width;
		break;
	}

	// Draw text
	return irrGUI->addStaticText(Text.c_str(), core::recti(TPositionX, TPositionY, TPositionX + TextArea.Width, TPositionY + TextArea.Height), 0, false, TParent);
}

// Draws text to the screen
void GraphicsClass::RenderText(const char *TText, int TPositionX, int TPositionY, AlignType TAlignType, const video::SColor &TColor) {

	// Convert string
	core::stringw Text(TText);

	// Get dimensions
	core::dimension2du TextArea = Fonts[CurrentFont]->getDimension(Text.c_str());

	switch(TAlignType) {
		case ALIGN_LEFT:
		break;
		case ALIGN_CENTER:
			TPositionX -= TextArea.Width >> 1;
		break;
		case ALIGN_RIGHT:
			TPositionX -= TextArea.Width;
		break;
	}

	// Draw text
	Fonts[CurrentFont]->draw(Text.c_str(), core::recti(TPositionX, TPositionY, TPositionX + TextArea.Width, TPositionY + TextArea.Height), TColor);
}

// Return a centered rect
irr::core::recti GraphicsClass::GetCenteredRect(int TPositionX, int TPositionY, int TWidth, int THeight) {
	TWidth >>= 1;
	THeight >>= 1;

	return core::recti(TPositionX - TWidth, TPositionY - THeight, TPositionX + TWidth, TPositionY + THeight);
}

// Return a rect
irr::core::recti GraphicsClass::GetRect(int TPositionX, int TPositionY, int TWidth, int THeight) {

	return core::recti(TPositionX, TPositionY, TPositionX + TWidth, TPositionY + THeight);
}

// Clear all the GUI elements
void GraphicsClass::Clear() {

	irrGUI->clear();
}

// Draws an interface image
void GraphicsClass::DrawImage(ImageType TType, int TPositionX, int TPositionY, const video::SColor &TColor) {

	Graphics.DrawCenteredImage(Images[TType], TPositionX, TPositionY, TColor);
}

// Draws a health or mana bar
void GraphicsClass::DrawBar(ImageType TType, int TPositionX, int TPositionY, float TPercent, int TWidth, int THeight) {

	irrDriver->draw2DImage(Images[TType + 1], core::position2di(TPositionX, TPositionY), core::recti(0, 0, TWidth, THeight), 0, video::SColor(255, 255, 255, 255), true);
	irrDriver->draw2DImage(Images[TType], core::position2di(TPositionX, TPositionY), core::recti(0, 0, (int)(TWidth * TPercent), THeight), 0, video::SColor(255, 255, 255, 255), true);
}

// Draws a tiled background
void GraphicsClass::DrawBackground(ImageType TType, int TPositionX, int TPositionY, int TWidth, int THeight, const video::SColor &TColor) {

	irrDriver->draw2DImage(Images[TType], core::position2di(TPositionX, TPositionY), core::recti(0, 0, TWidth, THeight), 0, TColor, true);
}
