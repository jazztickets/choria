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
#include <objects/map.h>
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
#include <SDL_keyboard.h>

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
	Graphics.Element->SetVisible(false);
	Graphics.Element->Visible = true;

	ButtonBarElement = Assets.Elements["element_editor_buttonbar"];
	TexturesElement = Assets.Elements["element_editor_textures"];
	NewMapElement = Assets.Elements["element_editor_newmap"];
	ResizeMapElement = Assets.Elements["element_editor_resizemap"];
	SaveMapElement = Assets.Elements["element_editor_savemap"];
	LoadMapElement = Assets.Elements["element_editor_loadmap"];
	NewMapFilenameTextBox = Assets.TextBoxes["textbox_editor_newmap_filename"];
	NewMapWidthTextBox = Assets.TextBoxes["textbox_editor_newmap_width"];
	NewMapHeightTextBox = Assets.TextBoxes["textbox_editor_newmap_height"];
	ResizeMinXTextBox = Assets.TextBoxes["textbox_editor_resizemap_minx"];
	ResizeMinYTextBox = Assets.TextBoxes["textbox_editor_resizemap_miny"];
	ResizeMaxXTextBox = Assets.TextBoxes["textbox_editor_resizemap_maxx"];
	ResizeMaxYTextBox = Assets.TextBoxes["textbox_editor_resizemap_maxy"];
	SaveMapTextBox = Assets.TextBoxes["textbox_editor_savemap"];
	LoadMapTextBox = Assets.TextBoxes["textbox_editor_loadmap"];
	ButtonBarElement->SetVisible(true);
	TexturesElement->SetVisible(false);
	NewMapElement->SetVisible(false);
	ResizeMapElement->SetVisible(false);
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

	// Set filters
	Layer = 0;
	Filter = 0;
	Filter |= FILTER_TEXTURE;
	Filter |= FILTER_WALL;

	// Default map
	Map = nullptr;
	MapID = 0;
	if(FilePath != "") {
		LoadMapTextBox->Text = FilePath;
		LoadMap();
	}
	else
		ToggleNewMap();

	DrawBounds = false;
	CopyStart = glm::ivec2(0, 0);
	CopyEnd = glm::ivec2(0, 0);
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
				else if(ResizeMapElement->Visible) {
					ResizeMap();
				}
				else if(SaveMapElement->Visible) {
					SaveMap();
				}
				else if(LoadMapElement->Visible) {
					LoadMap();
				}
			}
			else if(KeyEvent.Scancode == SDL_SCANCODE_TAB) {
				if(FocusedElement == NewMapFilenameTextBox)
					FocusedElement = NewMapWidthTextBox;
				else if(FocusedElement == NewMapWidthTextBox)
					FocusedElement = NewMapHeightTextBox;
				else if(FocusedElement == NewMapHeightTextBox)
					FocusedElement = NewMapFilenameTextBox;
				else if(FocusedElement == ResizeMinXTextBox)
					FocusedElement = ResizeMinYTextBox;
				else if(FocusedElement == ResizeMinYTextBox)
					FocusedElement = ResizeMaxXTextBox;
				else if(FocusedElement == ResizeMaxXTextBox)
					FocusedElement = ResizeMaxYTextBox;
				else if(FocusedElement == ResizeMaxYTextBox)
					FocusedElement = ResizeMinXTextBox;
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
			case SDL_SCANCODE_E:
				if(Input.ModKeyDown(KMOD_SHIFT)) {
					Brush->Event.Type--;
					if(Brush->Event.Type >= _Map::EVENT_COUNT)
						Brush->Event.Type = _Map::EVENT_COUNT-1;
				}
				else {
					Brush->Event.Type++;
					if(Brush->Event.Type >= _Map::EVENT_COUNT)
						Brush->Event.Type = _Map::EVENT_NONE;
				}
			break;
			case SDL_SCANCODE_W:
				Brush->Wall = !Brush->Wall;
			break;
			case SDL_SCANCODE_P:
				Brush->PVP = !Brush->PVP;
			break;
			case SDL_SCANCODE_N:
				IgnoreFirstChar = true;
				ToggleNewMap();
			break;
			case SDL_SCANCODE_R:
				IgnoreFirstChar = true;
				ToggleResize();
			break;
			case SDL_SCANCODE_S:
				IgnoreFirstChar = true;
				ToggleSaveMap();
			break;
			case SDL_SCANCODE_L:
				IgnoreFirstChar = true;
				ToggleLoadMap();
			break;
			case SDL_SCANCODE_G:
				IgnoreFirstChar = true;
				Go();
			break;
			case SDL_SCANCODE_V:
				Paste();
			break;
			case SDL_SCANCODE_TAB:
				Layer = !Layer;
			break;
			case SDL_SCANCODE_SPACE:
				ToggleTextures();
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
				case SDL_BUTTON_MIDDLE: {
					DrawBounds = true;
					CopyStart = Map->GetValidCoord(WorldCursor);
				} break;
			}
		}
	}
	// Release left mouse button
	else if(MouseEvent.Button == SDL_BUTTON_LEFT) {

		// Button bar
		if(ButtonBarElement->GetClickedElement()) {
			if(ButtonBarElement->GetClickedElement()->Identifier == "button_editor_buttonbar_new")
				ToggleNewMap();
			else if(ButtonBarElement->GetClickedElement()->Identifier == "button_editor_buttonbar_resize")
				ToggleResize();
			else if(ButtonBarElement->GetClickedElement()->Identifier == "button_editor_buttonbar_save")
				ToggleSaveMap();
			else if(ButtonBarElement->GetClickedElement()->Identifier == "button_editor_buttonbar_load")
				ToggleLoadMap();
		}
		// Texture select
		else if(TexturesElement->GetClickedElement()) {
			if(TexturesElement->GetClickedElement() != TexturesElement) {
				_Button *Button = (_Button *)TexturesElement->GetClickedElement();
				Brush->TextureIndex[Layer] = Button->TextureIndex;
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
		// Resize map screen
		else if(ResizeMapElement->GetClickedElement()) {
			if(ResizeMapElement->GetClickedElement()->Identifier == "button_editor_resizemap_resize") {
				ResizeMap();
			}
			else if(ResizeMapElement->GetClickedElement()->Identifier == "button_editor_resizemap_cancel") {
				CloseWindows();
			}
		}
	}
	// Release middle mouse
	else if(MouseEvent.Button == SDL_BUTTON_MIDDLE) {
		DrawBounds = false;
		GetDrawBounds(CopyStart, CopyEnd);
	}
}

// Mouse scroll wheel
void _EditorState::MouseWheelEvent(int Direction) {
	if(Input.ModKeyDown(KMOD_CTRL)) {
		if(Input.ModKeyDown(KMOD_SHIFT))
			Direction *= 10;

		if(Filter & FILTER_ZONE) {
			AdjustValue(Brush->Zone, Direction);
		}
		else if(Filter & FILTER_EVENTDATA) {
			AdjustValue(Brush->Event.Data, Direction);
		}
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
	Camera->Set3DProjection(BlendFactor);

	// Setup the viewing matrix
	Graphics.SetProgram(Assets.Programs["pos"]);
	glUniformMatrix4fv(Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["pos_uv_static"]);
	glUniformMatrix4fv(Assets.Programs["pos_uv_static"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	Graphics.SetProgram(Assets.Programs["text"]);
	glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

	// Render map
	if(Map)
		Map->Render(Camera, Stats, nullptr, BlendFactor, Filter | FILTER_BOUNDARY);

	Graphics.SetColor(COLOR_WHITE);
	Graphics.SetProgram(Assets.Programs["pos"]);
	Graphics.SetVBO(VBO_CIRCLE);
	Graphics.DrawCircle(glm::vec3(WorldCursor, 0.0f), BrushRadius);

	if(DrawBounds) {
		Graphics.SetVBO(VBO_NONE);
		Graphics.SetColor(COLOR_TGRAY);

		glm::ivec2 Start, End;
		GetDrawBounds(Start, End);
		Graphics.DrawRectangle(Start, End, true);
	}

	Graphics.Setup2D();
	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["ortho_pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Graphics.Ortho));
	Graphics.SetProgram(Assets.Programs["text"]);
	glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Graphics.Ortho));

	// Render brush
	RenderBrush();

	// Draw world cursor
	std::stringstream Buffer;
	Buffer << FilePath;
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), glm::vec2(15, 25), COLOR_WHITE);
	Buffer.str("");

	Buffer << (int)WorldCursor.x << ", " << (int)WorldCursor.y;
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), glm::vec2(15, Graphics.ViewportSize.y - 15), COLOR_WHITE);
	Buffer.str("");

	Buffer << Graphics.FramesPerSecond << " FPS";
	Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::vec2(15, 50));
	Buffer.str("");

	// Draw UI
	ButtonBarElement->Render();
	TexturesElement->Render();
	NewMapElement->Render();
	ResizeMapElement->Render();
	SaveMapElement->Render();
	LoadMapElement->Render();

}

// Get clean map name
std::string _EditorState::GetCleanMapName(const std::string &Path) {
	std::string CleanName = Path;

	// Remove preceding path
	std::string Prefix = "maps/";
	size_t PrefixStart = CleanName.find(Prefix);
	if(PrefixStart != std::string::npos)
		CleanName = CleanName.substr(Prefix.length());

	// Remove extension
	std::string Suffix = ".map.gz";
	size_t SuffixStart = CleanName.find(Suffix);
	if(SuffixStart != std::string::npos)
		CleanName = CleanName.substr(0, SuffixStart);

	return CleanName;
}

// Draw information about the brush
void _EditorState::RenderBrush() {
	if(!Map)
		return;

	glm::vec2 DrawPosition = Graphics.Element->Bounds.End - glm::vec2(60, 150);

	Graphics.SetProgram(Assets.Programs["ortho_pos"]);
	Graphics.SetVBO(VBO_NONE);
	Graphics.SetColor(glm::vec4(0, 0, 0, 0.8f));
	Graphics.DrawRectangle(DrawPosition - glm::vec2(45, 45), DrawPosition + glm::vec2(45, 138), true);

	// Draw texture
	_Bounds TextureBounds;
	TextureBounds.Start = DrawPosition - glm::vec2(Map->TileAtlas->Size) / 2.0f;
	TextureBounds.End = DrawPosition + glm::vec2(Map->TileAtlas->Size) / 2.0f;
	Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
	Graphics.SetColor(COLOR_WHITE);
	Graphics.DrawAtlas(TextureBounds, Map->TileAtlas->Texture, Map->TileAtlas->GetTextureCoords(Brush->TextureIndex[Layer]));

	std::stringstream Buffer;
	glm::vec4 Color(COLOR_WHITE);

	DrawPosition.y += 52;

	// Draw layer
	if(Layer)
		Buffer << "Fore";
	else
		Buffer << "Back";
	Assets.Fonts["hud_tiny"]->DrawText(Buffer.str().c_str(), DrawPosition, Color, CENTER_BASELINE);
	Buffer.str("");

	DrawPosition.y += 15;

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
	Buffer << Stats->EventNames[Brush->Event.Type].Name;

	Filter & FILTER_EVENTTYPE ? Color.a = 1.0f : Color.a = 0.5f;
	Assets.Fonts["hud_tiny"]->DrawText(Buffer.str().c_str(), DrawPosition, Color, CENTER_BASELINE);
	Buffer.str("");

	DrawPosition.y += 15;

	// Draw event data
	Buffer << "Data " << Brush->Event.Data;

	Filter & FILTER_EVENTDATA ? Color.a = 1.0f : Color.a = 0.5f;
	Assets.Fonts["hud_tiny"]->DrawText(Buffer.str().c_str(), DrawPosition, Color, CENTER_BASELINE);
	Buffer.str("");
}

// Update generic value
void _EditorState::AdjustValue(uint32_t &Value, int Direction) {
	int IntVal = (int)Value;
	IntVal += Direction;
	if(IntVal < 0)
		IntVal = 0;

	Value = (uint32_t)IntVal;
}

// Paste tiles
void _EditorState::Paste() {

	// Get offsets
	glm::ivec2 CopyPosition = CopyStart;
	glm::ivec2 PastePosition = Map->GetValidCoord(WorldCursor);

	// Copy tiles
	for(int j = 0; j < CopyEnd.y - CopyStart.y + 1; j++) {
		for(int i = 0; i < CopyEnd.x - CopyStart.x + 1; i++) {
			glm::ivec2 CopyCoord = glm::ivec2(i, j) + CopyPosition;
			glm::ivec2 PasteCoord = glm::ivec2(i, j) + PastePosition;
			if(Map->IsValidPosition(CopyCoord) && Map->IsValidPosition(PasteCoord)) {
				Map->Tiles[PasteCoord.x][PasteCoord.y] = Map->Tiles[CopyCoord.x][CopyCoord.y];
			}
		}
	}
}

// Get tile range from anchor point to world cursor
void _EditorState::GetDrawBounds(glm::ivec2 &Start, glm::ivec2 &End) {
	Start = CopyStart;
	End = Map->GetValidCoord(WorldCursor);

	if(End.x < Start.x)
		std::swap(Start.x, End.x);
	if(End.y < Start.y)
		std::swap(Start.y, End.y);
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

// Toggle resize map screen
void _EditorState::ToggleResize() {
	if(!ResizeMapElement->Visible) {
		CloseWindows();
		InitResize();
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
	if(!Map)
		return;

	if(!SaveMapElement->Visible) {
		CloseWindows();
		InitSaveMap();
	}
	else {
		CloseWindows();
	}
}

// Show load map screen
void _EditorState::ToggleLoadMap(const std::string &TempPath) {
	if(!LoadMapElement->Visible) {
		CloseWindows();
		InitLoadMap(TempPath);
	}
	else {
		CloseWindows();
	}
}

// Delete memory used by textures screen
void _EditorState::ClearTextures() {
	for(auto &Child : TexturesElement->Children) {
		if(Child->Style && Child->Style->UserCreated)
			delete Child->Style;

		if(Child->UserCreated)
			delete Child;
	}

	TexturesElement->Children.clear();
}

// Init texture select
void _EditorState::InitTextures() {

	// Clear old children
	ClearTextures();

	glm::vec2 Start(10, 25);
	glm::vec2 Offset(Start);
	glm::vec2 Spacing(10, 10);

	uint32_t TextureCount = (uint32_t)(Map->TileAtlas->Texture->Size.x * Map->TileAtlas->Texture->Size.y / (Map->TileAtlas->Size.x * Map->TileAtlas->Size.y));

	// Iterate over textures
	for(uint32_t i = 0; i < TextureCount; i++) {

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
	FilenameTextBox->SetText("");
	FocusedElement = FilenameTextBox;

	NewMapWidthTextBox->SetText("100");
	NewMapHeightTextBox->SetText("100");
}

// Init resize map screen
void _EditorState::InitResize() {
	if(!Map)
		return;

	ResizeMapElement->SetVisible(true);

	ResizeMinXTextBox->SetText("0");
	ResizeMinYTextBox->SetText("0");
	ResizeMaxXTextBox->SetText(std::to_string(Map->Size.x));
	ResizeMaxYTextBox->SetText(std::to_string(Map->Size.y));
}

// Init save map
void _EditorState::InitSaveMap() {
	if(!Map)
		return;

	SaveMapElement->SetVisible(true);
	FocusedElement = SaveMapTextBox;

	SaveMapTextBox->SetText(FilePath);
}

// Init load map
void _EditorState::InitLoadMap(const std::string &TempPath) {
	LoadMapElement->SetVisible(true);
	FocusedElement = LoadMapTextBox;

	if(TempPath != "")
		LoadMapTextBox->SetText(TempPath);
	else
		LoadMapTextBox->SetText(FilePath);
}

// Close all open windows
bool _EditorState::CloseWindows() {
	bool WasOpen = TexturesElement->Visible | NewMapElement->Visible | ResizeMapElement->Visible | SaveMapElement->Visible | LoadMapElement->Visible;

	TexturesElement->SetVisible(false);
	NewMapElement->SetVisible(false);
	ResizeMapElement->SetVisible(false);
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

	// Clamp size
	Size = glm::clamp(Size, 2, 255);

	// Create map
	Map = new _Map();
	Map->Size = Size;
	Map->InitAtlas(TEXTURES_MAP + MAP_DEFAULT_TILESET);
	Map->AllocateMap();
	FilePath = NewMapFilenameTextBox->Text;

	CloseWindows();
}

// Resize the map
void _EditorState::ResizeMap() {

	// Get min parameters
	glm::ivec2 Min(0, 0);
	{
		std::stringstream Buffer(ResizeMinXTextBox->Text + " " + ResizeMinYTextBox->Text);
		Buffer >> Min.x >> Min.y;
	}

	// Get max parameters
	glm::ivec2 Max(0, 0);
	{
		std::stringstream Buffer(ResizeMaxXTextBox->Text + " " + ResizeMaxYTextBox->Text);
		Buffer >> Max.x >> Max.y;
	}

	// Validate new size
	glm::ivec2 NewSize = glm::clamp(Max - Min, 2, 255);

	// Resize map
	Map->ResizeMap(Min, NewSize);

	// Close
	CloseWindows();
}

// Save the map
void _EditorState::SaveMap() {
	if(!Map)
		return;

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

		// Set map
		Map = NewMap;
		FilePath = LoadMapTextBox->Text;

		// Set camera position
		glm::ivec2 Position(0, 0);
		if(!Map->FindEvent(_Event(_Map::EVENT_SPAWN, 0), Position)) {
			Map->FindEvent(_Event(_Map::EVENT_MAPCHANGE, MapID), Position);
		}
		Camera->ForcePosition(glm::vec3(Position, CAMERA_DISTANCE));

		// Save map id
		MapID = Stats->GetMapIDByPath(Path);
	}

	CloseWindows();
}

// Open browser or load map under cursor
void _EditorState::Go() {

	// Event inspector
	if(Map->IsValidPosition(WorldCursor)) {
		const _Tile *Tile = Map->GetTile(WorldCursor);
		switch(Tile->Event.Type) {
			case _Map::EVENT_MAPENTRANCE:
			case _Map::EVENT_MAPCHANGE: {
				ToggleLoadMap(GetCleanMapName(Stats->Maps[Tile->Event.Data].File));
			} break;
			case _Map::EVENT_VENDOR: {
				std::stringstream Buffer;
				Buffer << Config.BrowserCommand << " \"" << Config.DesignToolURL << "/?table=vendoritem&vendor_id=" << Tile->Event.Data << "\"";
				system(Buffer.str().c_str());
			} break;
			case _Map::EVENT_TRADER: {
				std::stringstream Buffer;
				Buffer << Config.BrowserCommand << " \"" << Config.DesignToolURL << "/?table=traderitem&trader_id=" << Tile->Event.Data << "\"";
				system(Buffer.str().c_str());
			} break;
		}
	}
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
			glm::ivec2 TilePosition = Map->GetValidCoord(Position + glm::vec2(Offset));

			// Get existing tile
			_Tile Tile;
			Map->GetTile(TilePosition, Tile);

			// Apply filters
			if(Filter & FILTER_TEXTURE)
				Tile.TextureIndex[Layer] = Brush->TextureIndex[Layer];
			if(Filter & FILTER_WALL)
				Tile.Wall = Brush->Wall;
			if(Filter & FILTER_ZONE)
				Tile.Zone = Brush->Zone;
			if(Filter & FILTER_PVP)
				Tile.PVP = Brush->PVP;
			if(Filter & FILTER_EVENTTYPE)
				Tile.Event.Type = Brush->Event.Type;
			if(Filter & FILTER_EVENTDATA)
				Tile.Event.Data = Brush->Event.Data;

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
