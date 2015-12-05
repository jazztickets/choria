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
#include <ui/label.h>
#include <ui/textbox.h>
#include <ui/button.h>
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
	EditorNewMapElement(nullptr),
	NewMapWidthTextBox(nullptr),
	NewMapHeightTextBox(nullptr) {
}

// Initializes the state
void _EditorState::Init() {
	EditorNewMapElement = Assets.Elements["element_editor_newmap"];
	NewMapWidthTextBox = Assets.TextBoxes["textbox_editor_newmap_width"];
	NewMapHeightTextBox = Assets.TextBoxes["textbox_editor_newmap_height"];

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

	InitNewMap();
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
	if(Handled)
		return;

	if(KeyEvent.Repeat)
		return;

	if(KeyEvent.Pressed) {
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
				ToggleNewMap();
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
	}
}

// Mouse events
void _EditorState::MouseEvent(const _MouseEvent &MouseEvent) {
	FocusedElement = nullptr;
	Graphics.Element->HandleInput(MouseEvent.Pressed);

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

		// New map screen
		if(EditorNewMapElement->GetClickedElement()) {
			if(EditorNewMapElement->GetClickedElement()->Identifier == "button_editor_newmap_create") {
				CreateMap();
			}
			else if(EditorNewMapElement->GetClickedElement()->Identifier == "button_editor_newmap_cancel") {
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
	Graphics.SetProgram(Assets.Programs["text"]);
	glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Graphics.Ortho));

	// Render brush
	RenderBrush();

	// Draw world cursor
	std::stringstream Buffer;
	Buffer << std::fixed << std::setprecision(1) << WorldCursor.x << ", " << WorldCursor.y;
	Assets.Fonts["hud_small"]->DrawText(Buffer.str().c_str(), glm::vec2(15, Graphics.ViewportSize.y - 15), COLOR_WHITE);

	// Draw UI
	if(EditorNewMapElement->Visible)
		EditorNewMapElement->Render();
}

// Initializes the new map screen
void _EditorState::InitNewMap() {
	EditorNewMapElement->SetVisible(true);

	_TextBox *FilenameTextBox = Assets.TextBoxes["textbox_editor_newmap_filename"];
	FilenameTextBox->Text = "";
	FocusedElement = FilenameTextBox;

	NewMapWidthTextBox->Text = "100";
	NewMapHeightTextBox->Text = "100";
}

// Close all open windows
bool _EditorState::CloseWindows() {
	EditorNewMapElement->SetVisible(false);

	return true;
}

// Creates a map with the given parameters
void _EditorState::CreateMap() {
	CloseMap();

	// Get parameters
	glm::ivec2 Size(0, 0);
	std::stringstream Buffer(NewMapWidthTextBox->Text + " " + NewMapHeightTextBox->Text);
	Buffer >> Size.x >> Size.y;

	// Create map
	Map = new _Map(Size);

	CloseWindows();
}

// Initialize the load map screen
void _EditorState::InitLoadMap() {

	// Main dialog window
	std::string StartPath = std::string(Config.ConfigPath.c_str());
}

// Opens the texture palette dialog
void _EditorState::InitTexturePalette() {
}

// Opens the brush filter dialog
void _EditorState::InitBrushOptions() {
}

// Loads all map textures from a directory
void _EditorState::RefreshTexturePalette() {
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

// Toggle new map screen
void _EditorState::ToggleNewMap() {
	if(!EditorNewMapElement->Visible) {
		CloseWindows();
		InitNewMap();
	}
	else {
		CloseWindows();
	}
}