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
#include <instances/map.h>
#include <ui/element.h>
#include <ui/label.h>
#include <ui/textbox.h>
#include <ui/button.h>
#include <ui/style.h>
#include <framework.h>
#include <input.h>
#include <graphics.h>
#include <stats.h>
#include <config.h>
#include <constants.h>
#include <camera.h>
#include <assets.h>
#include <atlas.h>
#include <program.h>
#include <font.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream>
#include <iomanip>

_EditorState EditorState;

// Constructor
_EditorState::_EditorState() :
	Map(nullptr),
	ButtonBarElement(nullptr),
	TexturesElement(nullptr),
	NewMapElement(nullptr),
	SaveMapElement(nullptr),
	LoadMapElement(nullptr),
	NewMapFilenameTextBox(nullptr),
	NewMapWidthTextBox(nullptr),
	NewMapHeightTextBox(nullptr),
	SaveMapTextBox(nullptr),
	LoadMapTextBox(nullptr) {
}

// Initializes the state
void _EditorState::Init() {
	ButtonBarElement = Assets.Elements["element_editor_buttonbar"];
	TexturesElement = Assets.Elements["element_editor_textures"];
	NewMapElement = Assets.Elements["element_editor_newmap"];
	SaveMapElement = Assets.Elements["element_editor_savemap"];
	LoadMapElement = Assets.Elements["element_editor_loadmap"];
	NewMapFilenameTextBox = Assets.TextBoxes["textbox_editor_newmap_filename"];
	NewMapWidthTextBox = Assets.TextBoxes["textbox_editor_newmap_width"];
	NewMapHeightTextBox = Assets.TextBoxes["textbox_editor_newmap_height"];
	SaveMapTextBox = Assets.TextBoxes["textbox_editor_savemap"];
	LoadMapTextBox = Assets.TextBoxes["textbox_editor_loadmap"];
	ButtonBarElement->SetVisible(true);
	TexturesElement->SetVisible(false);
	NewMapElement->SetVisible(false);
	SaveMapElement->SetVisible(false);
	LoadMapElement->SetVisible(false);
	IgnoreFirstChar = false;

	// Load stats database
	Stats = new _Stats();

	// Create brush
	Brush = new _Tile();
	BrushRadius = 0.5f;

	// Create camera
	Camera = new _Camera(glm::vec3(0, 0, CAMERA_DISTANCE), CAMERA_EDITOR_DIVISOR);
	Camera->CalculateFrustum(Graphics.AspectRatio);

	Brush->TextureIndex[0] = 0;

	// Set filters
	Filter = 0;
	Filter |= FILTER_TEXTURE;
	Filter |= FILTER_WALL;

	// Default map
	Map = nullptr;
	if(FilePath != "") {
		LoadMapTextBox->Text = FilePath;
		LoadMap();
	}
	else
		ToggleNewMap();
}

// Shuts the state down
void _EditorState::Close() {
	delete Stats;
	delete Camera;
	delete Brush;

	ClearTextures();
	CloseMap();
}

// Key events
void _EditorState::KeyEvent(const _KeyEvent &KeyEvent) {
	if(IgnoreFirstChar) {
		IgnoreFirstChar = false;
		return;
	}

	bool Handled = Graphics.Element->HandleKeyEvent(KeyEvent);
	if(Handled)
		return;

	if(KeyEvent.Repeat)
		return;

	if(KeyEvent.Pressed) {
		if(FocusedElement) {
			if(KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
				CloseWindows();
			else if(KeyEvent.Scancode == SDL_SCANCODE_RETURN) {
				if(NewMapElement->Visible) {
					CreateMap();
				}
				else if(SaveMapElement->Visible) {
					SaveMap();
				}
				else if(LoadMapElement->Visible) {
					LoadMap();
				}
			}

			return;
		}

		switch(KeyEvent.Scancode) {
			case SDL_SCANCODE_ESCAPE:
				if(!CloseWindows())
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
				IgnoreFirstChar = true;
				ToggleNewMap();
			break;
			case SDL_SCANCODE_W:
				Brush->Wall = !Brush->Wall;
			break;
			case SDL_SCANCODE_P:
				Brush->PVP = !Brush->PVP;
			break;
			case SDL_SCANCODE_S:
				IgnoreFirstChar = true;
				ToggleSaveMap();
			break;
			case SDL_SCANCODE_L:
				IgnoreFirstChar = true;
				ToggleLoadMap();
			break;
			case SDL_SCANCODE_SPACE:
				ToggleTextures();
			break;
			case SDL_SCANCODE_MINUS:
				if(Brush->EventData > 0)
					Brush->EventData--;
			break;
			case SDL_SCANCODE_EQUALS:
				Brush->EventData++;
			break;
			case SDL_SCANCODE_F1:
				BrushRadius = 0.5f;
			break;
			case SDL_SCANCODE_F2:
				BrushRadius = 1.5f;
			break;
			case SDL_SCANCODE_F3:
				BrushRadius = 2.5f;
			break;
			case SDL_SCANCODE_F4:
				BrushRadius = 5.0f;
			break;
			case SDL_SCANCODE_F5:
				BrushRadius = 10.0f;
			break;
			default:
			break;
		}
	}
}

// Mouse events
void _EditorState::MouseEvent(const _MouseEvent &MouseEvent) {
	FocusedElement = nullptr;
	Graphics.Element->HandleInput(MouseEvent.Pressed);

	// Mouse press
	if(MouseEvent.Pressed) {
		if(Map) {
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
	// Release left mouse button
	else if(MouseEvent.Button == SDL_BUTTON_LEFT) {

		// Button bar
		if(ButtonBarElement->GetClickedElement()) {
			if(ButtonBarElement->GetClickedElement()->Identifier == "button_editor_buttonbar_new")
				ToggleNewMap();
			else if(ButtonBarElement->GetClickedElement()->Identifier == "button_editor_buttonbar_save")
				ToggleSaveMap();
			else if(ButtonBarElement->GetClickedElement()->Identifier == "button_editor_buttonbar_load")
				ToggleLoadMap();
		}
		// Texture select
		else if(TexturesElement->GetClickedElement()) {
			if(TexturesElement->GetClickedElement() != TexturesElement) {
				_Button *Button = (_Button *)TexturesElement->GetClickedElement();
				Brush->TextureIndex[0] = Button->TextureIndex;
				CloseWindows();
			}
		}
		// New map screen
		else if(NewMapElement->GetClickedElement()) {
			if(NewMapElement->GetClickedElement()->Identifier == "button_editor_newmap_create") {
				CreateMap();
			}
			else if(NewMapElement->GetClickedElement()->Identifier == "button_editor_newmap_cancel") {
				CloseWindows();
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

	if(Input.MouseDown(SDL_BUTTON_LEFT) && !(Input.ModKeyDown(KMOD_CTRL)) && Graphics.Element->HitElement == Graphics.Element) {
		ApplyBrush(WorldCursor);
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
		Map->Render(Camera, Stats, nullptr, Filter | FILTER_BOUNDARY);

	Graphics.Setup2D();
	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["ortho_pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Graphics.Ortho));
	Graphics.SetProgram(Assets.Programs["text"]);
	glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Graphics.Ortho));

	// Render brush
	RenderBrush();

	// Draw world cursor
	std::stringstream Buffer;
	Buffer << std::fixed << std::setprecision(1) << WorldCursor.x << ", " << WorldCursor.y;
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), glm::vec2(15, Graphics.ViewportSize.y - 15), COLOR_WHITE);

	// Draw UI
	ButtonBarElement->Render();
	TexturesElement->Render();
	SaveMapElement->Render();
	LoadMapElement->Render();
	NewMapElement->Render();

}

// Draw information about the brush
void _EditorState::RenderBrush() {
	glm::ivec2 DrawPosition = Graphics.Element->Bounds.End - glm::ivec2(50, 125);

	Graphics.SetProgram(Assets.Programs["ortho_pos"]);
	Graphics.SetVBO(VBO_NONE);
	Graphics.SetColor(glm::vec4(0, 0, 0, 0.8f));
	Graphics.DrawRectangle(DrawPosition - glm::ivec2(32, 32), DrawPosition + glm::ivec2(32, 110), true);

	// Draw texture
	_Bounds TextureBounds;
	TextureBounds.Start = DrawPosition - glm::ivec2(Map->TileAtlas->Size)/2;
	TextureBounds.End = DrawPosition + glm::ivec2(Map->TileAtlas->Size)/2;
	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	Graphics.SetColor(COLOR_WHITE);
	Graphics.DrawAtlas(TextureBounds, Map->TileAtlas->Texture, Map->TileAtlas->GetTextureCoords(Brush->TextureIndex[0]));

	std::stringstream Buffer;
	glm::vec4 Color(COLOR_WHITE);

	DrawPosition.y += 32 + 8;

	// Draw wall
	if(Brush->Wall)
		Buffer << "Wall";
	else
		Buffer << "Floor";

	Filter & FILTER_WALL ? Color.a = 1.0f : Color.a = 0.5f;
	Assets.Fonts["hud_tiny"]->DrawText(Buffer.str().c_str(), DrawPosition, Color, CENTER_BASELINE);
	Buffer.str("");

	DrawPosition.y += 15;

	// Draw zone
	Buffer << "Zone " << Brush->Zone;

	Filter & FILTER_ZONE ? Color.a = 1.0f : Color.a = 0.5f;
	Assets.Fonts["hud_tiny"]->DrawText(Buffer.str().c_str(), DrawPosition, Color, CENTER_BASELINE);
	Buffer.str("");

	DrawPosition.y += 15;

	// Draw PVP
	if(Brush->PVP)
		Buffer << "PVP";
	else
		Buffer << "Safe";

	Filter & FILTER_PVP ? Color.a = 1.0f : Color.a = 0.5f;
	Assets.Fonts["hud_tiny"]->DrawText(Buffer.str().c_str(), DrawPosition, Color, CENTER_BASELINE);
	Buffer.str("");

	DrawPosition.y += 15;

	// Draw event type
	Buffer << "Event " << Brush->EventType;

	Filter & FILTER_EVENTTYPE ? Color.a = 1.0f : Color.a = 0.5f;
	Assets.Fonts["hud_tiny"]->DrawText(Buffer.str().c_str(), DrawPosition, Color, CENTER_BASELINE);
	Buffer.str("");

	DrawPosition.y += 15;

	// Draw event data
	Buffer << "Data " << Brush->EventData;

	Filter & FILTER_EVENTDATA ? Color.a = 1.0f : Color.a = 0.5f;
	Assets.Fonts["hud_tiny"]->DrawText(Buffer.str().c_str(), DrawPosition, Color, CENTER_BASELINE);
	Buffer.str("");
}

// Toggle new map screen
void _EditorState::ToggleNewMap() {
	if(!NewMapElement->Visible) {
		CloseWindows();
		InitNewMap();
	}
	else {
		CloseWindows();
	}
}

// Show the texture select screen
void _EditorState::ToggleTextures() {
	if(!TexturesElement->Visible) {
		CloseWindows();
		InitTextures();
	}
	else {
		CloseWindows();
	}
}

// Show save map screen
void _EditorState::ToggleSaveMap() {
	if(!SaveMapElement->Visible) {
		CloseWindows();
		InitSaveMap();
	}
	else {
		CloseWindows();
	}
}

// Show load map screen
void _EditorState::ToggleLoadMap() {
	if(!LoadMapElement->Visible) {
		CloseWindows();
		InitLoadMap();
	}
	else {
		CloseWindows();
	}
}

// Delete memory used by textures screen
void _EditorState::ClearTextures() {
	std::vector<_Element *> &Children = TexturesElement->Children;
	for(size_t i = 0; i < Children.size(); i++) {
		if(Children[i]->Style && Children[i]->Style->UserCreated)
			delete Children[i]->Style;

		if(Children[i]->UserCreated)
			delete Children[i];
	}
	Children.clear();
}

// Init texture select
void _EditorState::InitTextures() {

	// Clear old children
	ClearTextures();

	glm::ivec2 Start(10, 25);
	glm::ivec2 Offset(Start);
	glm::ivec2 Spacing(10, 10);

	int TextureCount = Map->TileAtlas->Texture->Size.x * Map->TileAtlas->Texture->Size.y / (Map->TileAtlas->Size.x * Map->TileAtlas->Size.y);

	// Iterate over textures
	for(int i = 0; i < TextureCount; i++) {

		// Create style
		_Style *Style = new _Style();
		Style->TextureColor = COLOR_WHITE;
		Style->Atlas = Map->TileAtlas;
		Style->Program = Assets.Programs["ortho_pos_uv"];
		Style->UserCreated = true;

		// Add button
		_Button *Button = new _Button();
		Button->Identifier = "button_skills_skill";
		Button->Parent = TexturesElement;
		Button->Offset = Offset;
		Button->Size = Map->TileAtlas->Size;
		Button->Alignment = LEFT_TOP;
		Button->Style = Style;
		Button->UserCreated = true;
		Button->TextureIndex = i;
		TexturesElement->Children.push_back(Button);

		// Update position
		Offset.x += Map->TileAtlas->Size.x + Spacing.x;
		if(Offset.x > TexturesElement->Size.x - Map->TileAtlas->Size.x) {
			Offset.y += Map->TileAtlas->Size.y + Spacing.y;
			Offset.x = Start.x;
		}
	}

	TexturesElement->CalculateBounds();
	TexturesElement->SetVisible(true);
}

// Init new map
void _EditorState::InitNewMap() {
	NewMapElement->SetVisible(true);

	_TextBox *FilenameTextBox = Assets.TextBoxes["textbox_editor_newmap_filename"];
	FilenameTextBox->Text = "";
	FocusedElement = FilenameTextBox;

	NewMapWidthTextBox->Text = "100";
	NewMapHeightTextBox->Text = "100";
}

// Init save map
void _EditorState::InitSaveMap() {
	SaveMapElement->SetVisible(true);
	FocusedElement = SaveMapTextBox;

	SaveMapTextBox->Text = FilePath;
}

// Init load map
void _EditorState::InitLoadMap() {
	LoadMapElement->SetVisible(true);
	FocusedElement = LoadMapTextBox;

	LoadMapTextBox->Text = FilePath;
}

// Close all open windows
bool _EditorState::CloseWindows() {
	bool WasOpen = TexturesElement->Visible | NewMapElement->Visible | SaveMapElement->Visible | LoadMapElement->Visible;

	TexturesElement->SetVisible(false);
	NewMapElement->SetVisible(false);
	SaveMapElement->SetVisible(false);
	LoadMapElement->SetVisible(false);
	FocusedElement = nullptr;

	return WasOpen;
}

// Creates a map with the given parameters
void _EditorState::CreateMap() {
	CloseMap();

	// Get parameters
	glm::ivec2 Size(0, 0);
	std::stringstream Buffer(NewMapWidthTextBox->Text + " " + NewMapHeightTextBox->Text);
	Buffer >> Size.x >> Size.y;

	// Create map
	Map = new _Map();
	Map->Size = Size;
	Map->InitAtlas(TEXTURES_MAP + MAP_DEFAULT_TILESET);
	Map->AllocateMap();
	FilePath = NewMapFilenameTextBox->Text;

	CloseWindows();
}

// Save the map
void _EditorState::SaveMap() {

	// Get textbox value
	std::string Path = SaveMapTextBox->Text;
	if(Path == "")
		return;

	// Check for path prefix
	if(Path.find(ASSETS_MAPS_PATH, 0) == std::string::npos)
		Path = ASSETS_MAPS_PATH + Path;

	// Check for extension
	if(Path.find(".map.gz", 0) == std::string::npos)
		Path = Path + ".map.gz";

	// Save map
	Map->Save(Path);
	FilePath = SaveMapTextBox->Text;

	// Close
	CloseWindows();
}

// Load the map
void _EditorState::LoadMap() {

	// Get textbox value
	std::string Path = LoadMapTextBox->Text;
	if(Path == "")
		return;

	// Check for path prefix
	if(Path.find(ASSETS_MAPS_PATH, 0) == std::string::npos)
		Path = ASSETS_MAPS_PATH + Path;

	// Check for extension
	if(Path.find(".map.gz", 0) == std::string::npos)
		Path = Path + ".map.gz";

	// Attempt to load map
	_Map *NewMap = new _Map();
	try {
		NewMap->Load(Path);
	}
	catch(std::exception &Error) {
		delete NewMap;
		NewMap = nullptr;
	}

	// Set new map
	if(NewMap) {
		CloseMap();
		Map = NewMap;
		FilePath = LoadMapTextBox->Text;
	}

	CloseWindows();
}

// Apply brush to map
void _EditorState::ApplyBrush(const glm::vec2 &Position) {

	for(int j = 0; j < BrushRadius * 2; j++) {
		for(int i = 0; i < BrushRadius * 2; i++) {

			// Get offset from center
			glm::ivec2 Offset(i - (int)BrushRadius, j - (int)BrushRadius);
			if(Offset.x * Offset.x + Offset.y * Offset.y > BrushRadius * BrushRadius)
				continue;

			// Get valid position of tile
			glm::ivec2 TilePosition = Map->GetValidCoord(WorldCursor + glm::vec2(Offset));

			// Get existing tile
			_Tile Tile;
			Map->GetTile(TilePosition, Tile);

			// Apply filters
			if(Filter & FILTER_TEXTURE)
				Tile.TextureIndex[0] = Brush->TextureIndex[0];
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
			Map->SetTile(TilePosition, &Tile);
		}
	}
}

// Deletes the map
void _EditorState::CloseMap() {
	delete Map;
	Map = nullptr;
}
