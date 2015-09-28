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
#include <mapeditor.h>
#include <engine/game.h>
#include <engine/input.h>
#include <engine/graphics.h>
#include <engine/globals.h>
#include <engine/stats.h>
#include <engine/config.h>
#include <mainmenu.h>

_MapEditorState MapEditorState;

// Initializes the state
int _MapEditorState::Init() {

	// Textures
	BrushSize = 0;
	Brush.Texture = NULL;
	RefreshTexturePalette();
	if(TexturePalette.size() > 0)
		Brush.Texture = TexturePalette[0];

	// Default map
	stringc SavePath = Config.GetSaveMapPath("test.map");
	Map = new MapClass(SavePath, 50, 50);

	// Set filters
	ResetFilters();
	Filters[FILTER_TEXTURE] = true;
	Filters[FILTER_WALL] = true;

	State = STATE_MAIN;

	return 1;
}

// Shuts the state down
int _MapEditorState::Close() {

	CloseMap();

	return 1;
}

// Deletes the map
void _MapEditorState::CloseMap() {

	if(Map) {
		delete Map;
		Map = NULL;
	}
}

// Updates the current state
void _MapEditorState::Update(u32 TDeltaTime) {
	if(!Map)
		return;

	switch(State) {
		case STATE_MAIN:
			if(Input.GetMouseState(InputClass::MOUSE_LEFT) && !(Input.GetKeyState(KEY_CONTROL) || Input.GetKeyState(KEY_LCONTROL))) {
				switch(BrushSize) {
					case 0:
						ApplyBrush(BrushPosition.X, BrushPosition.Y);
						if(!Map->IsValidPosition(BrushPosition.X, BrushPosition.Y))
							Map->SetNoZoneTexture(Brush.Texture);
					break;
					case 1:
						ApplyBrushSize(BrushPosition.X, BrushPosition.Y, 3);
					break;
					case 2:
						ApplyBrushSize(BrushPosition.X, BrushPosition.Y, 6);
					break;
					case 3:
						ApplyBrushSize(BrushPosition.X, BrushPosition.Y, 12);
					break;
				}
			}
		break;
	}
}

// Draws the current state
void _MapEditorState::Draw() {

	if(Map)
		Map->RenderForMapEditor(Filters[FILTER_WALL], Filters[FILTER_ZONE], Filters[FILTER_PVP]);
	RenderBrush();

	stringc GridBrushPositionText = stringc(BrushPosition.X) + stringc(" ") + stringc(BrushPosition.Y);

	Graphics.SetFont(GraphicsClass::FONT_8);
	Graphics.RenderText(GridBrushPositionText.c_str(), 10, 10);

	irrGUI->drawAll();
}

// Key presses
bool _MapEditorState::HandleKeyPress(EKEY_CODE TKey) {

	switch(State) {
		case STATE_MAIN:
			switch(TKey) {
				case KEY_ESCAPE:
					//Game.SetDone(true);
					Game.ChangeState(&MainMenuState);
				break;
				case KEY_KEY_N:
					InitNewMap();
				break;
				case KEY_KEY_W:
					Brush.Wall = !Brush.Wall;
				break;
				case KEY_KEY_P:
					Brush.PVP = !Brush.PVP;
				break;
				case KEY_KEY_S:
					if(Map)
						Map->SaveMap();
				break;
				case KEY_KEY_L:
					InitLoadMap();
				break;
				case KEY_KEY_T:
					InitTexturePalette();
				break;
				case KEY_KEY_B:
					InitBrushOptions();
				break;
				case KEY_MINUS:
				case KEY_SUBTRACT:
					if(Brush.EventData > 0)
						Brush.EventData--;
				break;
				case KEY_ADD:
				case KEY_PLUS:
					Brush.EventData++;
				break;
				case KEY_KEY_1:
					ResetFilters();
					Filters[FILTER_TEXTURE] = true;
					Filters[FILTER_WALL] = true;
				break;
				case KEY_KEY_2:
					ResetFilters();
					Filters[FILTER_ZONE] = true;
				break;
				case KEY_KEY_3:
					ResetFilters();
					Filters[FILTER_PVP] = true;
				break;
				case KEY_KEY_4:
					ResetFilters();
					Filters[FILTER_EVENTTYPE] = true;
					Filters[FILTER_EVENTDATA] = true;
				break;
				case KEY_F1:
					BrushSize = 0;
				break;
				case KEY_F2:
					BrushSize = 1;
				break;
				case KEY_F3:
					BrushSize = 2;
				break;
				case KEY_F4:
					BrushSize = 3;
				break;
				default:
					return false;
				break;
			}
		break;
		case STATE_NEWMAP:
			switch(TKey) {
				case KEY_ESCAPE:
					CloseWindow(NEWMAP_WINDOW);
					State = STATE_MAIN;
				break;
				default:
					return false;
				break;
			}
		break;
		case STATE_LOADMAP:
			switch(TKey) {
				case KEY_ESCAPE:
					CloseWindow(NEWMAP_WINDOW);
					State = STATE_MAIN;
				break;
				default:
					return false;
				break;
			}
		break;
		case STATE_TEXTUREPALETTE:
			switch(TKey) {
				case KEY_ESCAPE:
					CloseWindow(TEXTUREPALETTE_WINDOW);
					State = STATE_MAIN;
				break;
				default:
					return false;
				break;
			}
		break;
		case STATE_BRUSHOPTIONS:
			switch(TKey) {
				case KEY_ESCAPE:
					CloseWindow(BRUSHOPTIONS_WINDOW);
					State = STATE_MAIN;
				break;
				default:
					return false;
				break;
			}
		break;
	}

	return true;
}

// Mouse buttons
bool _MapEditorState::HandleMousePress(int TButton, int TMouseX, int TMouseY) {

	if(Map) {
		switch(TButton) {
			case InputClass::MOUSE_LEFT:
				if(Input.GetKeyState(KEY_CONTROL) || Input.GetKeyState(KEY_LCONTROL)) {
					if(Map->IsValidPosition(BrushPosition.X, BrushPosition.Y))
						Brush = *Map->GetTile(BrushPosition.X, BrushPosition.Y);
				}
			break;
			case InputClass::MOUSE_RIGHT: {
				Map->ScreenToGrid(position2di(TMouseX, TMouseY), BrushPosition);
				Map->SetCameraScroll(BrushPosition);
			} break;
		}
	}

	return false;
}

// Scroll wheel
void _MapEditorState::HandleMouseWheel(float TDirection) {

	Brush.Zone += (int)(TDirection);
	if(Brush.Zone < 0)
		Brush.Zone = 0;
}

// Mouse movement
void _MapEditorState::HandleMouseMotion(int TMouseX, int TMouseY) {

	if(Map) {
		Map->ScreenToGrid(position2di(TMouseX, TMouseY), BrushPosition);
	}
}

// GUI events
void _MapEditorState::HandleGUI(EGUI_EVENT_TYPE TEventType, IGUIElement *TElement) {

	switch(TEventType) {
		case EGET_ELEMENT_CLOSED:
			State = STATE_MAIN;
		break;
		case EGET_EDITBOX_ENTER: {
			IGUISpinBox *SpinBox = static_cast<IGUISpinBox *>(TElement);
			switch(TElement->getID()) {
				case NEWMAP_FILE:
				case NEWMAP_WIDTH:
				case NEWMAP_HEIGHT:
					CreateMap();
				break;
				case BRUSHOPTIONS_ZONE:
					Brush.Zone = (int)SpinBox->getValue();
				break;
				case BRUSHOPTIONS_EVENTDATA:
					Brush.EventData = (int)SpinBox->getValue();
				default:
				break;
			}
		}
		break;
		case EGET_BUTTON_CLICKED:
			switch(TElement->getID()) {
				case NEWMAP_CREATE:
					CreateMap();
				break;
				case NEWMAP_CANCEL:
					CloseWindow(NEWMAP_WINDOW);
					State = STATE_MAIN;
				break;
				case BRUSHOPTIONS_FILTERCLOSE:
					CloseWindow(BRUSHOPTIONS_WINDOW);
					State = STATE_MAIN;
				break;
				default:

					// Texture palette
					if(TElement->getID() >= TEXTURES_ID) {
						int TextureIndex = TElement->getID() - TEXTURES_ID;
						Brush.Texture = TexturePalette[TextureIndex];

						CloseWindow(TEXTUREPALETTE_WINDOW);
						State = STATE_MAIN;
					}
				break;
			}
		break;
		case EGET_CHECKBOX_CHANGED: {
			IGUICheckBox *CheckBox = static_cast<IGUICheckBox *>(TElement);

			switch(TElement->getID()) {
				case BRUSHOPTIONS_WALL:
					Brush.Wall = CheckBox->isChecked();
				break;
				case BRUSHOPTIONS_PVP:
					Brush.PVP = CheckBox->isChecked();
				break;
				case BRUSHOPTIONS_FILTERTEXTURE:
					Filters[FILTER_TEXTURE] = CheckBox->isChecked();
				break;
				case BRUSHOPTIONS_FILTERWALL:
					Filters[FILTER_WALL] = CheckBox->isChecked();
				break;
				case BRUSHOPTIONS_FILTERZONE:
					Filters[FILTER_ZONE] = CheckBox->isChecked();
				break;
				case BRUSHOPTIONS_FILTERPVP:
					Filters[FILTER_PVP] = CheckBox->isChecked();
				break;
				case BRUSHOPTIONS_FILTEREVENTTYPE:
					Filters[FILTER_EVENTTYPE] = CheckBox->isChecked();
				break;
				case BRUSHOPTIONS_FILTEREVENTDATA:
					Filters[FILTER_EVENTDATA] = CheckBox->isChecked();
				break;
			}
		}
		break;
		case EGET_SPINBOX_CHANGED: {
			IGUISpinBox *SpinBox = static_cast<IGUISpinBox *>(TElement);
			switch(TElement->getID()) {
				case BRUSHOPTIONS_ZONE:
					Brush.Zone = (int)SpinBox->getValue();
				break;
				case BRUSHOPTIONS_EVENTDATA:
					Brush.EventData = (int)SpinBox->getValue();
				break;
			}
		}
		break;
		case EGET_COMBO_BOX_CHANGED: {
			IGUIComboBox *ComboBox = static_cast<IGUIComboBox *>(TElement);
			switch(TElement->getID()) {
				case BRUSHOPTIONS_EVENTTYPE:
					Brush.EventType = (int)ComboBox->getSelected();
				break;
			}
		}
		break;
		case EGET_FILE_SELECTED: {
			IGUIFileOpenDialog *FileOpen = static_cast<IGUIFileOpenDialog *>(TElement);

			CloseMap();
			Map = new MapClass(FileOpen->getFileName());
			if(!Map->LoadMap()) {
				CloseMap();
			}

			State = STATE_MAIN;
		}
		break;
		case EGET_FILE_CHOOSE_DIALOG_CANCELLED:
			State = STATE_MAIN;
		break;
		default:
		break;
	}
}

// Close a window by element
void _MapEditorState::CloseWindow(int TElement) {

	IGUIWindow *Window = static_cast<IGUIWindow *>(irrGUI->getRootGUIElement()->getElementFromId(TElement));
	if(Window)
		irrGUI->getRootGUIElement()->removeChild(Window);
}

// Initializes the new map screen
void _MapEditorState::InitNewMap() {

	// Main dialog window
	IGUIWindow *Window = irrGUI->addWindow(Graphics.GetCenteredRect(400, 300, 300, 300), false, L"New Map", 0, NEWMAP_WINDOW);
	irrGUI->setFocus(Window);

	// Filename
	Graphics.AddText("File", 80, 54, GraphicsClass::ALIGN_RIGHT, Window);
	IGUIEditBox *EditName = irrGUI->addEditBox(L"test.map", Graphics.GetRect(90, 50, 150, 25), true, Window, NEWMAP_FILE);
	EditName->setMax(15);

	// Map width
	Graphics.AddText("Width", 80, 84, GraphicsClass::ALIGN_RIGHT, Window);
	IGUIEditBox *EditWidth = irrGUI->addEditBox(L"100", Graphics.GetRect(90, 80, 100, 25), true, Window, NEWMAP_WIDTH);
	EditWidth->setMax(15);

	// Map height
	Graphics.AddText("Height", 80, 114, GraphicsClass::ALIGN_RIGHT, Window);
	IGUIEditBox *EditHeight = irrGUI->addEditBox(L"100", Graphics.GetRect(90, 110, 100, 25), true, Window, NEWMAP_HEIGHT);
	EditHeight->setMax(15);

	// Buttons
	irrGUI->addButton(Graphics.GetRect(20, 160, 110, 25), Window, NEWMAP_CREATE, L"Create");
	irrGUI->addButton(Graphics.GetRect(150, 160, 110, 25), Window, NEWMAP_CANCEL, L"Cancel");

	// Error
	irrGUI->addStaticText(L"", Graphics.GetRect(20, 230, 400, 25), false, false, Window, NEWMAP_ERROR);

	irrGUI->setFocus(EditName);

	State = STATE_NEWMAP;
}

// Creates a map with the given parameters
void _MapEditorState::CreateMap() {

	// Get window
	IGUIWindow *Window = static_cast<IGUIWindow *>(irrGUI->getRootGUIElement()->getElementFromId(NEWMAP_WINDOW));

	// Get buttons
	IGUIEditBox *EditFile = static_cast<IGUIEditBox *>(Window->getElementFromId(NEWMAP_FILE));
	IGUIEditBox *EditWidth = static_cast<IGUIEditBox *>(Window->getElementFromId(NEWMAP_WIDTH));
	IGUIEditBox *EditHeight = static_cast<IGUIEditBox *>(Window->getElementFromId(NEWMAP_HEIGHT));

	IGUIStaticText *TextError = static_cast<IGUIStaticText *>(Window->getElementFromId(NEWMAP_ERROR));

	// Get values
	stringc File(EditFile->getText());
	int Width = atoi(stringc(EditWidth->getText()).c_str());
	int Height = atoi(stringc(EditHeight->getText()).c_str());

	// Check filename
	File.make_lower();
	File.trim();
	if(File == "" || File.size() < 5 || File.find(".map") == -1) {
		TextError->setText(L"Invalid file name");
		irrGUI->setFocus(EditFile);
		return;
	}

	// Check width
	if(Width < 5 || Width > 255) {
		TextError->setText(L"Width must be between 5-255");
		irrGUI->setFocus(EditWidth);
		return;
	}

	// Check height
	if(Height < 5 || Height > 255) {
		TextError->setText(L"Height must be between 5-255");
		irrGUI->setFocus(EditHeight);
		return;
	}

	// Delete old map
	CloseMap();

	// Create map
	stringc SavePath = Config.GetSaveMapPath(File);
	Map = new MapClass(SavePath.c_str(), Width, Height);

	CloseWindow(NEWMAP_WINDOW);
	State = STATE_MAIN;
}

// Initialize the load map screen
void _MapEditorState::InitLoadMap() {

	// Main dialog window
	stringc StartPath = Config.GetSaveMapPath("");
	irrGUI->addFileOpenDialog(L"Load Map", true, 0, -1, true, (stringc::char_type *)StartPath.c_str());

	State = STATE_LOADMAP;
}

// Opens the texture palette dialog
void _MapEditorState::InitTexturePalette() {

	// Main dialog window
	IGUIWindow *Window = irrGUI->addWindow(Graphics.GetCenteredRect(400, 300, 600, 400), false, L"Texture Palette", 0, TEXTUREPALETTE_WINDOW);
	irrGUI->setFocus(Window);

	// Load texture buttons
	int StartX = 10;
	position2di TexturePosition(StartX, 30);
	for(u32 i = 0; i < TexturePalette.size(); i++) {

		IGUIButton *Button = irrGUI->addButton(Graphics.GetRect(TexturePosition.X, TexturePosition.Y, TexturePalette[i]->getSize().Width, TexturePalette[i]->getSize().Height), Window, TEXTURES_ID+i);
		Button->setImage(TexturePalette[i]);

		TexturePosition.X += MAP_TILE_WIDTH;
		if(TexturePosition.X > 600 - MAP_TILE_WIDTH) {
			TexturePosition.X = StartX;
			TexturePosition.Y += MAP_TILE_HEIGHT;
		}
	}

	State = STATE_TEXTUREPALETTE;
}

// Opens the brush filter dialog
void _MapEditorState::InitBrushOptions() {
	int StartX, StartY, OffsetY;

	// Main dialog window
	IGUIWindow *Window = irrGUI->addWindow(Graphics.GetCenteredRect(400, 300, 200, 350), false, L"Brush Options", 0, BRUSHOPTIONS_WINDOW);
	irrGUI->setFocus(Window);

	// Wall
	StartX = 75, StartY = 40;
	Graphics.AddText("Wall", StartX - 5, StartY + 3, GraphicsClass::ALIGN_RIGHT, Window);
	irrGUI->addCheckBox(Brush.Wall, Graphics.GetRect(StartX, StartY, 100, 20), Window, BRUSHOPTIONS_WALL);

	Graphics.AddText("PVP", StartX + 50, StartY + 3, GraphicsClass::ALIGN_RIGHT, Window);
	irrGUI->addCheckBox(Brush.PVP, Graphics.GetRect(StartX + 55, StartY, 100, 20), Window, BRUSHOPTIONS_PVP);

	// Zone
	StartY += 30;
	Graphics.AddText("Zone", StartX - 5, StartY + 3, GraphicsClass::ALIGN_RIGHT, Window);
	IGUISpinBox *BrushZone = irrGUI->addSpinBox(L"0", Graphics.GetRect(StartX, StartY, 100, 20), true, Window, BRUSHOPTIONS_ZONE);
	BrushZone->setDecimalPlaces(0);
	BrushZone->setRange(0.0f, 10000.0f);

	// Event Type
	StartY += 30;
	Graphics.AddText("Event Type", StartX - 5, StartY + 3, GraphicsClass::ALIGN_RIGHT, Window);
	IGUIComboBox *BrushEventType = irrGUI->addComboBox(Graphics.GetRect(StartX, StartY, 100, 20), Window, BRUSHOPTIONS_EVENTTYPE);
	for(int i = 0; i < Stats.GetEventCount(); i++)
		BrushEventType->addItem(stringw(Stats.GetEvent(i)->Name.c_str()).c_str());

	BrushEventType->setSelected(Brush.EventType);

	// Event Data
	StartY += 30;
	Graphics.AddText("Event Data", StartX - 5, StartY + 3, GraphicsClass::ALIGN_RIGHT, Window);
	IGUISpinBox *BrushEventData = irrGUI->addSpinBox(L"0", Graphics.GetRect(StartX, StartY, 100, 20), true, Window, BRUSHOPTIONS_EVENTDATA);
	BrushEventData->setDecimalPlaces(0);
	BrushEventData->setRange(0.0f, 10000.0f);

	// Filters
	StartY += 40, OffsetY = 20;
	Graphics.AddText("Filters", 100, StartY, GraphicsClass::ALIGN_CENTER, Window);
	Graphics.AddText("Texture", StartX - 5, StartY + 3 + OffsetY * 1, GraphicsClass::ALIGN_RIGHT, Window);
	Graphics.AddText("Wall", StartX - 5, StartY + 3 + OffsetY * 2, GraphicsClass::ALIGN_RIGHT, Window);
	Graphics.AddText("Zone", StartX - 5, StartY + 3 + OffsetY * 3, GraphicsClass::ALIGN_RIGHT, Window);
	Graphics.AddText("Event Type", StartX - 5, StartY + 3 + OffsetY * 4, GraphicsClass::ALIGN_RIGHT, Window);
	Graphics.AddText("Event Data", StartX - 5, StartY + 3 + OffsetY * 5, GraphicsClass::ALIGN_RIGHT, Window);
	irrGUI->addCheckBox(Filters[FILTER_TEXTURE], Graphics.GetRect(StartX, StartY + OffsetY * 1, 100, 20), Window, BRUSHOPTIONS_FILTERTEXTURE);
	irrGUI->addCheckBox(Filters[FILTER_WALL], Graphics.GetRect(StartX, StartY + OffsetY * 2, 100, 20), Window, BRUSHOPTIONS_FILTERWALL);
	irrGUI->addCheckBox(Filters[FILTER_ZONE], Graphics.GetRect(StartX, StartY + OffsetY * 3, 100, 20), Window, BRUSHOPTIONS_FILTERZONE);
	irrGUI->addCheckBox(Filters[FILTER_EVENTTYPE], Graphics.GetRect(StartX, StartY + OffsetY * 4, 100, 20), Window, BRUSHOPTIONS_FILTEREVENTTYPE);
	irrGUI->addCheckBox(Filters[FILTER_EVENTDATA], Graphics.GetRect(StartX, StartY + OffsetY * 5, 100, 20), Window, BRUSHOPTIONS_FILTEREVENTDATA);

	// Buttons
	irrGUI->addButton(Graphics.GetCenteredRect(100, 320, 75, 25), Window, BRUSHOPTIONS_FILTERCLOSE, L"Close");

	State = STATE_BRUSHOPTIONS;
}

// Loads all map textures from a directory
void _MapEditorState::RefreshTexturePalette() {

	TexturePalette.clear();

	// Load all textures in the directory
	stringc OldWorkingDirectory = irrFile->getWorkingDirectory();
	irrFile->changeWorkingDirectoryTo(WorkingDirectory + "textures/map");
	IFileList *FileList = irrFile->createFileList();
	irrFile->changeWorkingDirectoryTo(OldWorkingDirectory.c_str());

	int FileCount = FileList->getFileCount();
	for(int i = 0; i < FileCount; i++) {
		if(!FileList->isDirectory(i)) {

			// Load texture
			ITexture *Texture = irrDriver->getTexture(WorkingDirectory + "textures/map/" + FileList->getFileName(i));

			// Check size
			if(Texture->getSize() != dimension2du(32, 32)) {
				stringc TextureName = Texture->getName();
				printf("Texture size is not 32x32 for file=%s\n", TextureName.c_str());
				irrDriver->removeTexture(Texture);
			}
			else {

				// Save textures off
				TexturePalette.push_back(Texture);
			}
		}
	}
}

// Applys a brush of varying size
void _MapEditorState::ApplyBrushSize(int TX, int TY, int TSize) {

	for(int i = 0; i < TSize; i++) {
		for(int j = 0; j < TSize; j++) {
			int PositionX = j - TSize / 2;
			int PositionY = i - TSize / 2;

			if(PositionX * PositionX + PositionY * PositionY >= TSize - 1)
				continue;

			ApplyBrush(TX + PositionX, TY + PositionY);
		}
	}
}

// Draws a texture on the map with the current brush
void _MapEditorState::ApplyBrush(int TX, int TY) {

	if(Map) {
		if(!Map->IsValidPosition(TX, TY))
			return;

		// Get existing tile
		TileStruct Tile;
		Map->GetTile(TX, TY, Tile);

		// Apply filters
		if(Filters[FILTER_TEXTURE])
			Tile.Texture = Brush.Texture;
		if(Filters[FILTER_WALL])
			Tile.Wall = Brush.Wall;
		if(Filters[FILTER_ZONE])
			Tile.Zone = Brush.Zone;
		if(Filters[FILTER_PVP])
			Tile.PVP = Brush.PVP;
		if(Filters[FILTER_EVENTTYPE])
			Tile.EventType = Brush.EventType;
		if(Filters[FILTER_EVENTDATA])
			Tile.EventData = Brush.EventData;

		// Set new tile
		Map->SetTile(TX, TY, &Tile);
	}
}

// Draw information about the brush
void _MapEditorState::RenderBrush() {

	SColor Color(255, 255, 255, 255);
	int StartX = 750, StartY = 480;
	Graphics.DrawBackground(GraphicsClass::IMAGE_BLACK, 705, StartY - 10, 90, 125);

	// Draw texture
	StartY += 15;
	if(Brush.Texture != NULL) {
		Filters[FILTER_TEXTURE] ? Color.setAlpha(255) : Color.setAlpha(80);
		Graphics.DrawCenteredImage(Brush.Texture, StartX, StartY, Color);
	}

	// Get wall text
	const char *WallText = "Floor";
	if(Brush.Wall)
		WallText = "Wall";

	// Draw wall info
	StartY += 20;
	Filters[FILTER_WALL] ? Color.setAlpha(255) : Color.setAlpha(128);
	Graphics.SetFont(GraphicsClass::FONT_8);
	Graphics.RenderText(WallText, StartX, StartY, GraphicsClass::ALIGN_CENTER, Color);

	// Draw zone info
	StartY += 15;
	Filters[FILTER_ZONE] ? Color.setAlpha(255) : Color.setAlpha(128);
	stringc ZoneText = stringc("Zone ") + stringc(Brush.Zone);
	Graphics.RenderText(ZoneText.c_str(), StartX, StartY, GraphicsClass::ALIGN_CENTER, Color);

	// Get PVP text
	const char *PVPText = "Safe";
	if(Brush.PVP)
		PVPText = "PVP";

	// Draw pvp info
	StartY += 15;
	Filters[FILTER_PVP] ? Color.setAlpha(255) : Color.setAlpha(128);
	Graphics.RenderText(PVPText, StartX, StartY, GraphicsClass::ALIGN_CENTER, Color);

	// Draw event info
	StartY += 15;
	Filters[FILTER_EVENTTYPE] ? Color.setAlpha(255) : Color.setAlpha(128);
	stringc EventTypeText = stringc("Event: ") + Stats.GetEvent(Brush.EventType)->ShortName;
	Graphics.RenderText(EventTypeText.c_str(), StartX, StartY, GraphicsClass::ALIGN_CENTER, Color);

	// Draw event info
	StartY += 15;
	Filters[FILTER_EVENTDATA] ? Color.setAlpha(255) : Color.setAlpha(128);
	stringc EventDataText = stringc("Event Data: ") + stringc(Brush.EventData);
	Graphics.RenderText(EventDataText.c_str(), StartX, StartY, GraphicsClass::ALIGN_CENTER, Color);
}

// Resets the filters to false
void _MapEditorState::ResetFilters() {
	for(int i = 0; i < FILTER_COUNT; i++) {
		Filters[i] = false;
	}
}
