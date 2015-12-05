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
#include <states/editor.h>
#include <ui/element.h>
#include <framework.h>
#include <input.h>
#include <graphics.h>
#include <stats.h>
#include <config.h>
#include <constants.h>
#include <camera.h>
#include <assets.h>
#include <program.h>
#include <font.h>
#include <instances/map.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream>
#include <iomanip>

_EditorState EditorState;

// Constructor
_EditorState::_EditorState() :
	Map(nullptr),
	EditorNewMapElement(nullptr) {
}

// Initializes the state
void _EditorState::Init() {
	EditorNewMapElement = Assets.Elements["element_editor_newmap"];
	EditorNewMapElement->SetVisible(true);

	Stats = new _Stats();

	// Create brush
	Brush = new _Tile();
	BrushSize = 0;
	Brush->Texture = nullptr;
	RefreshTexturePalette();
	if(TexturePalette.size() > 0)
		Brush->Texture = TexturePalette[0];

	// Create camera
	Camera = new _Camera(glm::vec3(0, 0, CAMERA_DISTANCE), CAMERA_EDITOR_DIVISOR);
	Camera->CalculateFrustum(Graphics.AspectRatio);

	// Default map
	Map = nullptr;
	if(Path != "") {
		Map = new _Map();
		Map->Load();
	}

	// Set filters
	Filter = 0;
	Filter |= FILTER_TEXTURE;
	Filter |= FILTER_WALL;

	State = STATE_MAIN;
}

// Shuts the state down
void _EditorState::Close() {
	delete Stats;
	delete Camera;
	delete Brush;

	CloseMap();
}

// Key events
void _EditorState::KeyEvent(const _KeyEvent &KeyEvent) {
	bool Handled = Graphics.Element->HandleKeyEvent(KeyEvent);

	if(KeyEvent.Repeat)
		return;

	switch(State) {
		case STATE_MAIN:
			switch(KeyEvent.Scancode) {
				case SDL_SCANCODE_ESCAPE:
					Framework.Done = true;
				break;
				case SDL_SCANCODE_1:
					Filter = 0;
					Filter |= FILTER_TEXTURE;
					Filter |= FILTER_WALL;
				break;
				case SDL_SCANCODE_2:
					Filter = 0;
					Filter |= FILTER_ZONE;
				break;
				case SDL_SCANCODE_3:
					Filter = 0;
					Filter |= FILTER_PVP;
				break;
				case SDL_SCANCODE_4:
					Filter = 0;
					Filter |= FILTER_EVENTTYPE;
					Filter |= FILTER_EVENTDATA;
				break;
				case SDL_SCANCODE_N:
					InitNewMap();
				break;
				case SDL_SCANCODE_W:
					Brush->Wall = !Brush->Wall;
				break;
				case SDL_SCANCODE_P:
					Brush->PVP = !Brush->PVP;
				break;
				case SDL_SCANCODE_S:
					if(Map)
						Map->Save(Map->Path);
				break;
				case SDL_SCANCODE_L:
					InitLoadMap();
				break;
				case SDL_SCANCODE_T:
					InitTexturePalette();
				break;
				case SDL_SCANCODE_B:
					InitBrushOptions();
				break;
				case SDL_SCANCODE_MINUS:
					if(Brush->EventData > 0)
						Brush->EventData--;
				break;
				case SDL_SCANCODE_EQUALS:
					Brush->EventData++;
				break;
				case SDL_SCANCODE_F1:
					BrushSize = 0;
				break;
				case SDL_SCANCODE_F2:
					BrushSize = 1;
				break;
				case SDL_SCANCODE_F3:
					BrushSize = 2;
				break;
				case SDL_SCANCODE_F4:
					BrushSize = 3;
				break;
				default:
				break;
			}
		break;
	/*
		case STATE_NEWMAP:
			switch(TKey) {
				case SDL_SCANCODE_ESCAPE:
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
				case SDL_SCANCODE_ESCAPE:
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
				case SDL_SCANCODE_ESCAPE:
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
				case SDL_SCANCODE_ESCAPE:
					CloseWindow(BRUSHOPTIONS_WINDOW);
					State = STATE_MAIN;
				break;
				default:
					return false;
				break;
			}
		break;
	*/
	}
}

// Mouse events
void _EditorState::MouseEvent(const _MouseEvent &MouseEvent) {
	FocusedElement = nullptr;
	Graphics.Element->HandleInput(MouseEvent.Pressed);

	if(Map) {
		if(MouseEvent.Pressed) {
			switch(MouseEvent.Button) {
				// Eyedropper tool
				case SDL_BUTTON_LEFT:
					if(Input.ModKeyDown(KMOD_CTRL) && Map->IsValidPosition(WorldCursor))
						*Brush = *Map->GetTile(WorldCursor);
				break;
				// Scroll map
				case SDL_BUTTON_RIGHT:
					Camera->Set2DPosition(WorldCursor);
				break;
			}
		}
	}
}

// Mouse scroll wheel
void _EditorState::MouseWheelEvent(int Direction) {
	if(Input.ModKeyDown(KMOD_CTRL)) {
		Brush->Zone += Direction;
		if(Brush->Zone < 0)
			Brush->Zone = 0;
	}
	else {

		// Zoom
		float Multiplier = 1.0f * Direction;
		if(Input.ModKeyDown(KMOD_SHIFT))
			Multiplier = 10.0f * Direction;

		Camera->UpdateDistance(-Multiplier);
	}
}

// Window events
void _EditorState::WindowEvent(uint8_t Event) {
	if(Camera && Event == SDL_WINDOWEVENT_SIZE_CHANGED)
		Camera->CalculateFrustum(Graphics.AspectRatio);
}

// Updates the current state
void _EditorState::Update(double FrameTime) {
	Graphics.Element->Update(FrameTime, Input.GetMouse());

	if(!Map)
		return;

	// Update camera
	if(Camera) {
		Camera->Update(FrameTime);

		// Get world cursor
		Camera->ConvertScreenToWorld(Input.GetMouse(), WorldCursor);
		WorldCursor = Map->GetValidPosition(WorldCursor);
	}

	switch(State) {
		case STATE_MAIN:
			if(Input.MouseDown(SDL_BUTTON_LEFT) && !(Input.ModKeyDown(KMOD_CTRL))) {
				switch(BrushSize) {
					case 0:
						ApplyBrush(WorldCursor);
						//if(!Map->IsValidPosition(BrushPosition.x, BrushPosition.y))
						//	Map->SetNoZoneTexture(Brush->Texture);
					break;
					case 1:
						ApplyBrushSize(WorldCursor, 3);
					break;
					case 2:
						ApplyBrushSize(WorldCursor, 6);
					break;
					case 3:
						ApplyBrushSize(WorldCursor, 12);
					break;
				}
			}
		break;
	}
}

// Draws the current state
void _EditorState::Render(double BlendFactor) {

	Graphics.Setup3D();

	// Set lights
	glm::vec4 AmbientLight(1.0f, 1.0f, 1.0f, 1.0f);
	Assets.Programs["pos_uv"]->AmbientLight = AmbientLight;
	Assets.Programs["pos_uv_norm"]->AmbientLight = AmbientLight;

	// Setup the viewing matrix
	Graphics.Setup3D();
	Camera->Set3DProjection(BlendFactor);
	Graphics.SetProgram(Assets.Programs["pos"]);
	glUniformMatrix4fv(Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["pos_uv_norm"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv_norm"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["text"]);
	glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

	// Render map
	if(Map)
		Map->Render(Camera, Stats, nullptr, Filter);

	Graphics.Setup2D();
	Graphics.SetProgram(Assets.Programs["text"]);
	glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Graphics.Ortho));

	// Render brush
	RenderBrush();

	// Draw world cursor
	std::stringstream Buffer;
	Buffer << std::fixed << std::setprecision(1) << WorldCursor.x << ", " << WorldCursor.y;
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), glm::vec2(15, Graphics.ViewportSize.y - 15), COLOR_WHITE);

	if(EditorNewMapElement->Visible)
		EditorNewMapElement->Render();

}

// GUI events
/*
void _MapEditorState::HandleGUI(gui::EGUI_EVENT_TYPE EventType, gui::IGUIElement *TElement) {

	switch(EventType) {
		case gui::EGET_ELEMENT_CLOSED:
			State = STATE_MAIN;
		break;
		case gui::EGET_EDITBOX_ENTER: {
			gui::IGUISpinBox *SpinBox = static_cast<gui::IGUISpinBox *>(TElement);
			switch(TElement->getID()) {
				case NEWMAP_FILE:
				case NEWMAP_WIDTH:
				case NEWMAP_HEIGHT:
					CreateMap();
				break;
				case BRUSHOPTIONS_ZONE:
					Brush->Zone = (int)SpinBox->getValue();
				break;
				case BRUSHOPTIONS_EVENTDATA:
					Brush->EventData = (int)SpinBox->getValue();
				default:
				break;
			}
		}
		break;
		case gui::EGET_BUTTON_CLICKED:
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
						Brush->Texture = TexturePalette[TextureIndex];

						CloseWindow(TEXTUREPALETTE_WINDOW);
						State = STATE_MAIN;
					}
				break;
			}
		break;
		case gui::EGET_CHECKBOX_CHANGED: {
			gui::IGUICheckBox *CheckBox = static_cast<gui::IGUICheckBox *>(TElement);

			switch(TElement->getID()) {
				case BRUSHOPTIONS_WALL:
					Brush->Wall = CheckBox->isChecked();
				break;
				case BRUSHOPTIONS_PVP:
					Brush->PVP = CheckBox->isChecked();
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
		case gui::EGET_SPINBOX_CHANGED: {
			gui::IGUISpinBox *SpinBox = static_cast<gui::IGUISpinBox *>(TElement);
			switch(TElement->getID()) {
				case BRUSHOPTIONS_ZONE:
					Brush->Zone = (int)SpinBox->getValue();
				break;
				case BRUSHOPTIONS_EVENTDATA:
					Brush->EventData = (int)SpinBox->getValue();
				break;
			}
		}
		break;
		case gui::EGET_COMBO_BOX_CHANGED: {
			gui::IGUIComboBox *ComboBox = static_cast<gui::IGUIComboBox *>(TElement);
			switch(TElement->getID()) {
				case BRUSHOPTIONS_EVENTTYPE:
					Brush->EventType = (int)ComboBox->getSelected();
				break;
			}
		}
		break;
		case gui::EGET_FILE_SELECTED: {
			gui::IGUIFileOpenDialog *FileOpen = static_cast<gui::IGUIFileOpenDialog *>(TElement);
			//irrFile->changeWorkingDirectoryTo(WorkingDirectory);

			CloseMap();
			//Map = new _Map(FileOpen->getFileName());
			if(!Map->LoadMap()) {
				CloseMap();
			}

			State = STATE_MAIN;
		}
		break;
		case gui::EGET_FILE_CHOOSE_DIALOG_CANCELLED:
			State = STATE_MAIN;
		break;
		default:
		break;
	}
}
*/

// Close a window by element
void _EditorState::CloseWindow(int Element) {
}

// Initializes the new map screen
void _EditorState::InitNewMap() {
/*
	// Main dialog window
	gui::IGUIWindow *Window = irrGUI->addWindow(Graphics.GetCenteredRect(400, 300, 300, 300), false, L"New Map", 0, NEWMAP_WINDOW);
	irrGUI->setFocus(Window);

	// Filename
	Graphics.AddText("File", 80, 54, _Graphics::ALIGN_RIGHT, Window);
	gui::IGUIEditBox *EditName = irrGUI->addEditBox(L"test.map", Graphics.GetRect(90, 50, 150, 25), true, Window, NEWMAP_FILE);
	EditName->setMax(15);

	// Map width
	Graphics.AddText("Width", 80, 84, _Graphics::ALIGN_RIGHT, Window);
	gui::IGUIEditBox *EditWidth = irrGUI->addEditBox(L"100", Graphics.GetRect(90, 80, 100, 25), true, Window, NEWMAP_WIDTH);
	EditWidth->setMax(15);

	// Map height
	Graphics.AddText("Height", 80, 114, _Graphics::ALIGN_RIGHT, Window);
	gui::IGUIEditBox *EditHeight = irrGUI->addEditBox(L"100", Graphics.GetRect(90, 110, 100, 25), true, Window, NEWMAP_HEIGHT);
	EditHeight->setMax(15);

	// Buttons
	irrGUI->addButton(Graphics.GetRect(20, 160, 110, 25), Window, NEWMAP_CREATE, L"Create");
	irrGUI->addButton(Graphics.GetRect(150, 160, 110, 25), Window, NEWMAP_CANCEL, L"Cancel");

	// Error
	irrGUI->addStaticText(L"", Graphics.GetRect(20, 230, 400, 25), false, false, Window, NEWMAP_ERROR);

	irrGUI->setFocus(EditName);
*/
	State = STATE_NEWMAP;
}

// Creates a map with the given parameters
void _EditorState::CreateMap() {
/*
	// Get window
	gui::IGUIWindow *Window = static_cast<gui::IGUIWindow *>(irrGUI->getRootGUIElement()->getElementFromId(NEWMAP_WINDOW));

	// Get buttons
	gui::IGUIEditBox *EditFile = static_cast<gui::IGUIEditBox *>(Window->getElementFromId(NEWMAP_FILE));
	gui::IGUIEditBox *EditWidth = static_cast<gui::IGUIEditBox *>(Window->getElementFromId(NEWMAP_WIDTH));
	gui::IGUIEditBox *EditHeight = static_cast<gui::IGUIEditBox *>(Window->getElementFromId(NEWMAP_HEIGHT));

	gui::IGUIStaticText *TextError = static_cast<gui::IGUIStaticText *>(Window->getElementFromId(NEWMAP_ERROR));

	// Get values
	std::string File(EditFile->getText());
	int Width = atoi(std::string(EditWidth->getText()).c_str());
	int Height = atoi(std::string(EditHeight->getText()).c_str());

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
	//std::string SavePath = Config.SaveMapPath + File;
	std::string SavePath = File;
	Map = new _Map(SavePath.c_str(), Width, Height);

	CloseWindow(NEWMAP_WINDOW);
	State = STATE_MAIN;*/
}

// Initialize the load map screen
void _EditorState::InitLoadMap() {

	// Main dialog window
	std::string StartPath = std::string(Config.ConfigPath.c_str());
	//irrGUI->addFileOpenDialog(L"Load Map", true, 0, -1, true, (std::string::char_type *)StartPath.c_str());

	State = STATE_LOADMAP;
}

// Opens the texture palette dialog
void _EditorState::InitTexturePalette() {
/*
	// Main dialog window
	gui::IGUIWindow *Window = irrGUI->addWindow(Graphics.GetCenteredRect(400, 300, 600, 400), false, L"Texture Palette", 0, TEXTUREPALETTE_WINDOW);
	irrGUI->setFocus(Window);

	// Load texture buttons
	int StartX = 10;
	glm::ivec2 TexturePosition(StartX, 30);
	for(size_t i = 0; i < TexturePalette.size(); i++) {

		gui::IGUIButton *Button = irrGUI->addButton(Graphics.GetRect(TexturePosition.x, TexturePosition.y, TexturePalette[i]->getSize().Width, TexturePalette[i]->getSize().Height), Window, TEXTURES_ID+i);
		Button->setImage(TexturePalette[i]);

		TexturePosition.x += MAP_TILE_WIDTH;
		if(TexturePosition.x > 600 - MAP_TILE_WIDTH) {
			TexturePosition.x = StartX;
			TexturePosition.y += MAP_TILE_HEIGHT;
		}
	}

	State = STATE_TEXTUREPALETTE;
	*/
}

// Opens the brush filter dialog
void _EditorState::InitBrushOptions() {
	/*
	int StartX, StartY, OffsetY;

	// Main dialog window
	gui::IGUIWindow *Window = irrGUI->addWindow(Graphics.GetCenteredRect(400, 300, 200, 350), false, L"Brush Options", 0, BRUSHOPTIONS_WINDOW);
	irrGUI->setFocus(Window);

	// Wall
	StartX = 75, StartY = 40;
	Graphics.AddText("Wall", StartX - 5, StartY + 3, _Graphics::ALIGN_RIGHT, Window);
	irrGUI->addCheckBox(Brush->Wall, Graphics.GetRect(StartX, StartY, 100, 20), Window, BRUSHOPTIONS_WALL);

	Graphics.AddText("PVP", StartX + 50, StartY + 3, _Graphics::ALIGN_RIGHT, Window);
	irrGUI->addCheckBox(Brush->PVP, Graphics.GetRect(StartX + 55, StartY, 100, 20), Window, BRUSHOPTIONS_PVP);

	// Zone
	StartY += 30;
	Graphics.AddText("Zone", StartX - 5, StartY + 3, _Graphics::ALIGN_RIGHT, Window);
	gui::IGUISpinBox *BrushZone = irrGUI->addSpinBox(L"0", Graphics.GetRect(StartX, StartY, 100, 20), true, Window, BRUSHOPTIONS_ZONE);
	BrushZone->setDecimalPlaces(0);
	BrushZone->setRange(0.0f, 10000.0f);

	// Event Type
	StartY += 30;
	Graphics.AddText("Event Type", StartX - 5, StartY + 3, _Graphics::ALIGN_RIGHT, Window);
	gui::IGUIComboBox *BrushEventType = irrGUI->addComboBox(Graphics.GetRect(StartX, StartY, 100, 20), Window, BRUSHOPTIONS_EVENTTYPE);
	for(int i = 0; i < Stats.GetEventCount(); i++)
		BrushEventType->addItem(core::stringw(Stats.GetEvent(i)->Name.c_str()).c_str());

	BrushEventType->setSelected(Brush->EventType);

	// Event Data
	StartY += 30;
	Graphics.AddText("Event Data", StartX - 5, StartY + 3, _Graphics::ALIGN_RIGHT, Window);
	gui::IGUISpinBox *BrushEventData = irrGUI->addSpinBox(L"0", Graphics.GetRect(StartX, StartY, 100, 20), true, Window, BRUSHOPTIONS_EVENTDATA);
	BrushEventData->setDecimalPlaces(0);
	BrushEventData->setRange(0.0f, 10000.0f);

	// Filters
	StartY += 40, OffsetY = 20;
	Graphics.AddText("Filters", 100, StartY, _Graphics::ALIGN_CENTER, Window);
	Graphics.AddText("Texture", StartX - 5, StartY + 3 + OffsetY * 1, _Graphics::ALIGN_RIGHT, Window);
	Graphics.AddText("Wall", StartX - 5, StartY + 3 + OffsetY * 2, _Graphics::ALIGN_RIGHT, Window);
	Graphics.AddText("Zone", StartX - 5, StartY + 3 + OffsetY * 3, _Graphics::ALIGN_RIGHT, Window);
	Graphics.AddText("Event Type", StartX - 5, StartY + 3 + OffsetY * 4, _Graphics::ALIGN_RIGHT, Window);
	Graphics.AddText("Event Data", StartX - 5, StartY + 3 + OffsetY * 5, _Graphics::ALIGN_RIGHT, Window);
	irrGUI->addCheckBox(Filters[FILTER_TEXTURE], Graphics.GetRect(StartX, StartY + OffsetY * 1, 100, 20), Window, BRUSHOPTIONS_FILTERTEXTURE);
	irrGUI->addCheckBox(Filters[FILTER_WALL], Graphics.GetRect(StartX, StartY + OffsetY * 2, 100, 20), Window, BRUSHOPTIONS_FILTERWALL);
	irrGUI->addCheckBox(Filters[FILTER_ZONE], Graphics.GetRect(StartX, StartY + OffsetY * 3, 100, 20), Window, BRUSHOPTIONS_FILTERZONE);
	irrGUI->addCheckBox(Filters[FILTER_EVENTTYPE], Graphics.GetRect(StartX, StartY + OffsetY * 4, 100, 20), Window, BRUSHOPTIONS_FILTEREVENTTYPE);
	irrGUI->addCheckBox(Filters[FILTER_EVENTDATA], Graphics.GetRect(StartX, StartY + OffsetY * 5, 100, 20), Window, BRUSHOPTIONS_FILTEREVENTDATA);

	// Buttons
	irrGUI->addButton(Graphics.GetCenteredRect(100, 320, 75, 25), Window, BRUSHOPTIONS_FILTERCLOSE, L"Close");
*/
	State = STATE_BRUSHOPTIONS;
}

// Loads all map textures from a directory
void _EditorState::RefreshTexturePalette() {
	/*
	TexturePalette.clear();

	// Load all textures in the directory
	WorkingDirectory = irrFile->getWorkingDirectory();
	irrFile->changeWorkingDirectoryTo("textures/map");
	io::IFileList *FileList = irrFile->createFileList();
	irrFile->changeWorkingDirectoryTo(WorkingDirectory.c_str());

	int FileCount = FileList->getFileCount();
	for(int i = 0; i < FileCount; i++) {
		if(!FileList->isDirectory(i)) {

			// Load texture
			_Texture *Texture = irrDriver->getTexture(std::string("textures/map/") + FileList->getFileName(i));

			// Check size
			if(Texture->getSize() != core::dimension2du(32, 32)) {
				std::string TextureName = Texture->getName();
				printf("Texture size is not 32x32 for file=%s\n", TextureName.c_str());
				irrDriver->removeTexture(Texture);
			}
			else {

				// Save textures off
				TexturePalette.push_back(Texture);
			}
		}
	}*/
}

// Applys a brush of varying size
void _EditorState::ApplyBrushSize(const glm::vec2 &Position, int BrushSize) {

	for(int i = 0; i < BrushSize; i++) {
		for(int j = 0; j < BrushSize; j++) {
			glm::vec2 Offset = glm::vec2(j, i) - glm::vec2(BrushSize/2);

			if(Offset.x * Offset.x + Offset.y * Offset.y >= BrushSize - 1)
				continue;

			ApplyBrush(Position + Offset);
		}
	}
}

// Draws a texture on the map with the current brush
void _EditorState::ApplyBrush(const glm::vec2 &Position) {

	if(Map) {
		if(!Map->IsValidPosition(Position))
			return;

		// Get existing tile
		_Tile Tile;
		Map->GetTile(Position, Tile);

		// Apply filters
		if(Filter & FILTER_TEXTURE)
			Tile.Texture = Brush->Texture;
		if(Filter & FILTER_WALL)
			Tile.Wall = Brush->Wall;
		if(Filter & FILTER_ZONE)
			Tile.Zone = Brush->Zone;
		if(Filter & FILTER_PVP)
			Tile.PVP = Brush->PVP;
		if(Filter & FILTER_EVENTTYPE)
			Tile.EventType = Brush->EventType;
		if(Filter & FILTER_EVENTDATA)
			Tile.EventData = Brush->EventData;

		// Set new tile
		Map->SetTile(Position, &Tile);
	}
}

// Draw information about the brush
void _EditorState::RenderBrush() {

	/*
	video::SColor Color(255, 255, 255, 255);
	int StartX = 750, StartY = 480;
	Graphics.DrawBackground(_Graphics::IMAGE_BLACK, 705, StartY - 10, 90, 125);

	// Draw texture
	StartY += 15;
	if(Brush->Texture != nullptr) {
		Filters[FILTER_TEXTURE] ? Color.setAlpha(255) : Color.setAlpha(80);
		Graphics.DrawCenteredImage(Brush->Texture, StartX, StartY, Color);
	}

	// Get wall text
	const char *WallText = "Floor";
	if(Brush->Wall)
		WallText = "Wall";

	// Draw wall info
	StartY += 20;
	Filters[FILTER_WALL] ? Color.setAlpha(255) : Color.setAlpha(128);
	//Graphics.SetFont(_Graphics::FONT_8);
	//Graphics.RenderText(WallText, StartX, StartY, _Graphics::ALIGN_CENTER, Color);

	// Draw zone info
	StartY += 15;
	Filters[FILTER_ZONE] ? Color.setAlpha(255) : Color.setAlpha(128);
	std::string ZoneText = std::string("Zone ") + std::to_string(Brush->Zone);
	//Graphics.RenderText(ZoneText.c_str(), StartX, StartY, _Graphics::ALIGN_CENTER, Color);

	// Get PVP text
	const char *PVPText = "Safe";
	if(Brush->PVP)
		PVPText = "PVP";

	// Draw pvp info
	StartY += 15;
	Filters[FILTER_PVP] ? Color.setAlpha(255) : Color.setAlpha(128);
	//Graphics.RenderText(PVPText, StartX, StartY, _Graphics::ALIGN_CENTER, Color);

	// Draw event info
	StartY += 15;
	Filters[FILTER_EVENTTYPE] ? Color.setAlpha(255) : Color.setAlpha(128);
	std::string EventTypeText = std::string("Event: ") + Stats.GetEvent(Brush->EventType)->ShortName;
	//Graphics.RenderText(EventTypeText.c_str(), StartX, StartY, _Graphics::ALIGN_CENTER, Color);

	// Draw event info
	StartY += 15;
	Filters[FILTER_EVENTDATA] ? Color.setAlpha(255) : Color.setAlpha(128);
	std::string EventDataText = std::string("Event Data: ") + std::to_string(Brush->EventData);
	//Graphics.RenderText(EventDataText.c_str(), StartX, StartY, _Graphics::ALIGN_CENTER, Color);
	*/
}

// Deletes the map
void _EditorState::CloseMap() {
	delete Map;
	Map = nullptr;
}