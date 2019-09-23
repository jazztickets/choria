/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2019  Alan Witkowski
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
#include <ae/camera.h>
#include <ae/assets.h>
#include <ae/atlas.h>
#include <ae/program.h>
#include <ae/audio.h>
#include <ae/font.h>
#include <ae/ui.h>
#include <ae/input.h>
#include <ae/console.h>
#include <ae/graphics.h>
#include <ae/framebuffer.h>
#include <objects/object.h>
#include <objects/map.h>
#include <framework.h>
#include <stats.h>
#include <config.h>
#include <actiontype.h>
#include <constants.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream>
#include <iomanip>
#include <SDL_keycode.h>
#include <SDL_mouse.h>

// Constants
const double EDITOR_CLOCK_SPEED = 200.0;

_EditorState EditorState;

// Initializes the state
void _EditorState::Init() {
	ae::Graphics.Element->SetActive(false);
	ae::Graphics.Element->Active = true;

	ae::Audio.StopMusic();

	// Setup UI
	EditorElement = ae::Assets.Elements["element_editor"];
	ButtonBarElement = ae::Assets.Elements["element_editor_buttonbar"];
	ClockElement = ae::Assets.Elements["element_editor_clock"];
	TexturesElement = ae::Assets.Elements["element_editor_textures"];
	EventsElement = ae::Assets.Elements["element_editor_events"];
	EventTypesElement = ae::Assets.Elements["element_editor_eventtypes"];
	EventDataElement = ae::Assets.Elements["textbox_editor_eventdata"];
	NewMapElement = ae::Assets.Elements["element_editor_newmap"];
	ResizeMapElement = ae::Assets.Elements["element_editor_resizemap"];
	SaveMapElement = ae::Assets.Elements["element_editor_savemap"];
	LoadMapElement = ae::Assets.Elements["element_editor_loadmap"];
	NewMapFilenameTextBox = ae::Assets.Elements["textbox_editor_newmap_filename"];
	NewMapWidthTextBox = ae::Assets.Elements["textbox_editor_newmap_width"];
	NewMapHeightTextBox = ae::Assets.Elements["textbox_editor_newmap_height"];
	ResizeMinXTextBox = ae::Assets.Elements["textbox_editor_resizemap_minx"];
	ResizeMinYTextBox = ae::Assets.Elements["textbox_editor_resizemap_miny"];
	ResizeMaxXTextBox = ae::Assets.Elements["textbox_editor_resizemap_maxx"];
	ResizeMaxYTextBox = ae::Assets.Elements["textbox_editor_resizemap_maxy"];
	SaveMapTextBox = ae::Assets.Elements["textbox_editor_savemap"];
	LoadMapTextBox = ae::Assets.Elements["textbox_editor_loadmap"];
	EditorElement->SetActive(true);
	TexturesElement->SetActive(false);
	EventsElement->SetActive(false);
	NewMapElement->SetActive(false);
	ResizeMapElement->SetActive(false);
	SaveMapElement->SetActive(false);
	LoadMapElement->SetActive(false);

	// Load stats database
	Stats = new _Stats();

	// Create brush
	Brush = new _Tile();
	BrushRadius = 0.5f;

	// Create camera
	Camera = new ae::_Camera(glm::vec3(0, 0, CAMERA_DISTANCE), CAMERA_EDITOR_DIVISOR, CAMERA_FOVY, CAMERA_NEAR, CAMERA_FAR);
	Camera->CalculateFrustum(ae::Graphics.AspectRatio);

	// Create framebuffer for lights
	Framebuffer = new ae::_Framebuffer(ae::Graphics.CurrentSize);

	// Set filters
	ShowTransitions = false;
	Filter = 0;
	Filter |= MAP_RENDER_TEXTURE;
	Filter |= MAP_RENDER_WALL;

	// Default map
	Clock = 60.0 * 12.0;
	UseClockAmbientLight = false;
	Map = nullptr;
	CopyBuffer = nullptr;
	if(FilePath != "") {
		LoadMapTextBox->Text = FilePath;
		LoadMap();
	}
	else
		ToggleNewMap();

	Copied = false;
	DrawCopyBounds = false;
	CopyStart = glm::ivec2(0, 0);
	CopyEnd = glm::ivec2(0, 0);
	WorldCursor = glm::vec2(0.0f, 0.0f);
	Mode = EditorModeType::TILE;
	ObjectType = 0;
	ObjectData = 1;
}

// Shuts the state down
void _EditorState::Close() {
	delete Stats;
	delete Camera;
	delete Brush;
	delete Framebuffer;

	ClearTextures();
	ClearEvents();
	CloseCopy();
	CloseMap();
}

// Handle actions
bool _EditorState::HandleAction(int InputType, size_t Action, int Value) {
	if(Value == 0)
		return true;

	if(Action == Action::MISC_CONSOLE)
		Framework.Console->Toggle();

	return true;
}

// Key events
bool _EditorState::HandleKey(const ae::_KeyEvent &KeyEvent) {
	bool Handled = ae::Graphics.Element->HandleKey(KeyEvent);
	if(Handled)
		return true;

	if(KeyEvent.Repeat)
		return true;

	if(KeyEvent.Pressed) {
		if(ae::FocusedElement) {
			if(KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
				CloseWindows();
			else if(KeyEvent.Scancode == SDL_SCANCODE_RETURN) {
				if(NewMapElement->Active) {
					CreateMap();
				}
				else if(ResizeMapElement->Active) {
					ResizeMap();
				}
				else if(SaveMapElement->Active) {
					SaveMap();
				}
				else if(LoadMapElement->Active) {
					LoadMap();
				}
			}
			else if(KeyEvent.Scancode == SDL_SCANCODE_TAB) {
				if(ae::FocusedElement == NewMapFilenameTextBox)
					ae::FocusedElement = NewMapWidthTextBox;
				else if(ae::FocusedElement == NewMapWidthTextBox)
					ae::FocusedElement = NewMapHeightTextBox;
				else if(ae::FocusedElement == NewMapHeightTextBox)
					ae::FocusedElement = NewMapFilenameTextBox;
				else if(ae::FocusedElement == ResizeMinXTextBox)
					ae::FocusedElement = ResizeMinYTextBox;
				else if(ae::FocusedElement == ResizeMinYTextBox)
					ae::FocusedElement = ResizeMaxXTextBox;
				else if(ae::FocusedElement == ResizeMaxXTextBox)
					ae::FocusedElement = ResizeMaxYTextBox;
				else if(ae::FocusedElement == ResizeMaxYTextBox)
					ae::FocusedElement = ResizeMinXTextBox;

				if(ae::FocusedElement)
					ae::FocusedElement->ResetCursor();
			}

			return true;
		}

		switch(KeyEvent.Scancode) {
			case SDL_SCANCODE_ESCAPE:
				if(!CloseWindows())
					Framework.Done = true;
			break;
			case SDL_SCANCODE_1:
			case SDL_SCANCODE_2:
			case SDL_SCANCODE_3:
			case SDL_SCANCODE_4:
			case SDL_SCANCODE_5:
				SwitchBrushModes((int)(KeyEvent.Scancode - SDL_SCANCODE_1 + 1));
			break;
			case SDL_SCANCODE_T:
				if(ae::Input.ModKeyDown(KMOD_CTRL))
					UseClockAmbientLight = !UseClockAmbientLight;
			break;
			case SDL_SCANCODE_E:
				ToggleEvents();
			break;
			case SDL_SCANCODE_W:
				Brush->Wall = !Brush->Wall;
			break;
			case SDL_SCANCODE_P:
				Brush->PVP = !Brush->PVP;
			break;
			case SDL_SCANCODE_N:
				Framework.IgnoreNextInputEvent = true;
				ToggleNewMap();
			break;
			case SDL_SCANCODE_R:
				Framework.IgnoreNextInputEvent = true;
				ToggleResize();
			break;
			case SDL_SCANCODE_S:
				Framework.IgnoreNextInputEvent = true;
				ToggleSaveMap();
			break;
			case SDL_SCANCODE_L:
				Framework.IgnoreNextInputEvent = true;
				ToggleLoadMap();
			break;
			case SDL_SCANCODE_G:
				Framework.IgnoreNextInputEvent = true;
				Go();
			break;
			case SDL_SCANCODE_V:
				PasteTiles();
			break;
			case SDL_SCANCODE_D:
				Map->DeleteStaticObject(WorldCursor);
			break;
			case SDL_SCANCODE_TAB:
				ShowTransitions = !ShowTransitions;
				Map->BuildLayers(ShowTransitions);
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
			case SDL_SCANCODE_Z:
				if(Map) {
					Map->CurrentZoneColors++;
					if(Map->CurrentZoneColors > Map->MaxZoneColors)
						Map->CurrentZoneColors = 1;
				}
			break;
			default:
			break;
		}
	}

	return true;
}

// Mouse events
void _EditorState::HandleMouseButton(const ae::_MouseEvent &MouseEvent) {
	ae::FocusedElement = nullptr;
	ae::Graphics.Element->HandleMouseButton(MouseEvent.Pressed);

	// Mouse press
	if(MouseEvent.Pressed) {
		if(Map) {
			switch(MouseEvent.Button) {
				// Eyedropper tool
				case SDL_BUTTON_LEFT:
					if(ae::Input.ModKeyDown(KMOD_CTRL) && Map->IsValidPosition(WorldCursor)) {
						*Brush = *Map->GetTile(WorldCursor);
						EventDataElement->Text = Brush->Event.Data;
					}
				break;
				// Scroll map
				case SDL_BUTTON_RIGHT:
					if(!ae::Graphics.Element->HitElement)
						Camera->Set2DPosition(WorldCursor);
				break;
				case SDL_BUTTON_MIDDLE: {
					if(Mode == EditorModeType::TILE) {
						DrawCopyBounds = true;
						CopyStart = Map->GetValidCoord(WorldCursor);
					}
				} break;
			}
		}
	}
	// Release left mouse button
	else if(MouseEvent.Button == SDL_BUTTON_LEFT) {

		// Button bar
		if(ButtonBarElement->GetClickedElement()) {
			if(ButtonBarElement->GetClickedElement()->Name == "button_editor_buttonbar_new")
				ToggleNewMap();
			else if(ButtonBarElement->GetClickedElement()->Name == "button_editor_buttonbar_resize")
				ToggleResize();
			else if(ButtonBarElement->GetClickedElement()->Name == "button_editor_buttonbar_save")
				ToggleSaveMap();
			else if(ButtonBarElement->GetClickedElement()->Name == "button_editor_buttonbar_load")
				ToggleLoadMap();
		}
		// Texture select
		else if(TexturesElement->GetClickedElement()) {
			if(TexturesElement->GetClickedElement() != TexturesElement) {
				ae::_Element *Button = TexturesElement->GetClickedElement();
				Brush->BaseTextureIndex = Button->TextureIndex;
				CloseWindows();
			}
		}
		// Event select
		else if(EventsElement->GetClickedElement()) {
			ae::_Element *ClickedElement = EventsElement->GetClickedElement();

			// Clicked event type button
			if(ClickedElement->Parent && ClickedElement->Parent == EventTypesElement) {
				ae::_Element *Button = ClickedElement;

				// Validate data
				switch((EventType)Button->Index) {
					case EventType::VENDOR:
						if(Stats->Vendors.find(EventDataElement->Text) == Stats->Vendors.end())
							return;
					break;
				}
				Brush->Event.Type = (EventType)Button->Index;
				Brush->Event.Data = EventDataElement->Text;
				SwitchBrushModes(4);
				CloseWindows();
			}
		}
		// New map screen
		else if(NewMapElement->GetClickedElement()) {
			if(NewMapElement->GetClickedElement()->Name == "button_editor_newmap_create") {
				CreateMap();
			}
			else if(NewMapElement->GetClickedElement()->Name == "button_editor_newmap_cancel") {
				CloseWindows();
			}
		}
		// Resize map screen
		else if(ResizeMapElement->GetClickedElement()) {
			if(ResizeMapElement->GetClickedElement()->Name == "button_editor_resizemap_resize") {
				ResizeMap();
			}
			else if(ResizeMapElement->GetClickedElement()->Name == "button_editor_resizemap_cancel") {
				CloseWindows();
			}
		}

		// Place object
		if(Mode == EditorModeType::OBJECT) {
			_Object *Object = new _Object;
			Object->Position = WorldCursor;
			Object->Light = (int)ObjectData;
			Map->StaticObjects.push_back(Object);
		}
	}
	// Release middle mouse
	else if(MouseEvent.Button == SDL_BUTTON_MIDDLE) {
		CopyTiles();
	}
}

// Mouse scroll wheel
void _EditorState::HandleMouseWheel(int Direction) {
	if(ae::Input.ModKeyDown(KMOD_CTRL)) {
		if(ae::Input.ModKeyDown(KMOD_SHIFT))
			Direction *= 10;

		if(Mode == EditorModeType::TILE) {
			if(Filter & MAP_RENDER_ZONE) {
				AdjustValue(Brush->Zone, Direction);
			}
		}
		else if(Mode == EditorModeType::OBJECT) {
			AdjustValue(ObjectData, Direction);
		}
	}
	else {

		// Zoom
		float Multiplier = 1.0f * Direction;
		if(ae::Input.ModKeyDown(KMOD_SHIFT))
			Multiplier = 10.0f * Direction;

		Camera->UpdateDistance(-Multiplier);
	}
}

// Window events
void _EditorState::HandleWindow(uint8_t Event) {
	if(Camera && Event == SDL_WINDOWEVENT_SIZE_CHANGED)
		Camera->CalculateFrustum(ae::Graphics.AspectRatio);

	if(Framebuffer) {
		Framebuffer->Resize(ae::Graphics.CurrentSize);
		ae::Graphics.DirtyState();
	}
}

// Quit
void _EditorState::HandleQuit() {
	Framework.Done = true;
}

// Updates the current state
void _EditorState::Update(double FrameTime) {
	ae::Graphics.Element->Update(FrameTime, ae::Input.GetMouse());
	//if(ae::Graphics.Element->HitElement)
	//	std::cout << ae::Graphics.Element->HitElement->Name << std::endl;

	// Don't update unless there's a map
	if(!Map)
		return;

	// Set clock
	Map->Clock = Clock;

	// Update camera
	if(Camera) {
		Camera->Update(FrameTime);

		// Get world cursor
		Camera->ConvertScreenToWorld(ae::Input.GetMouse(), WorldCursor);
		WorldCursor = Map->GetValidPosition(WorldCursor);
	}

	// Handle mouse input
	if(Mode == EditorModeType::TILE) {
		if(ae::Input.MouseDown(SDL_BUTTON_LEFT) && !(ae::Input.ModKeyDown(KMOD_CTRL)) && ae::Graphics.Element->HitElement == nullptr) {
			ApplyBrush(WorldCursor);
		}
	}

	// Handle key input
	if(ae::Input.KeyDown(SDL_SCANCODE_T) && !ae::Input.ModKeyDown(KMOD_CTRL)) {
		if(ae::Input.ModKeyDown(KMOD_SHIFT)) {
			Clock -= FrameTime * EDITOR_CLOCK_SPEED;
			if(Clock < 0)
				Clock += MAP_DAY_LENGTH;
		}
		else {
			Clock += FrameTime * EDITOR_CLOCK_SPEED;
			if(Clock >= MAP_DAY_LENGTH)
				Clock -= MAP_DAY_LENGTH;
		}
	}
}

// Draws the current state
void _EditorState::Render(double BlendFactor) {

	ae::Graphics.Setup3D();
	Camera->Set3DProjection(BlendFactor);

	// Setup the viewing matrix
	ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["pos_uv_static"]);
	glUniformMatrix4fv(ae::Assets.Programs["pos_uv_static"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["text"]);
	glUniformMatrix4fv(ae::Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));
	ae::Graphics.SetProgram(ae::Assets.Programs["map"]);
	glUniformMatrix4fv(ae::Assets.Programs["map"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Camera->Transform));

	// Render map
	if(Map) {
		int RenderFilter = Filter | MAP_RENDER_BOUNDARY;
		if(!UseClockAmbientLight)
			RenderFilter |= MAP_RENDER_EDITOR_AMBIENT;
		Map->Render(Camera, Framebuffer, nullptr, BlendFactor, RenderFilter);
	}

	// Draw tile brush size
	if(Mode == EditorModeType::TILE) {
		ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
		ae::Graphics.SetColor(glm::vec4(1.0f));
		ae::Graphics.DrawCircle(glm::vec3(WorldCursor, 0.0f), BrushRadius);

		// Draw copy tool boundaries
		if(DrawCopyBounds) {
			ae::Graphics.SetColor(ae::Assets.Colors["editor_select"]);

			glm::ivec2 Start, End;
			GetDrawBounds(Start, End);
			ae::Graphics.DrawRectangle(Start, End + 1, true);
		}
	}

	// Setup UI
	ae::Graphics.Setup2D();
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
	glUniformMatrix4fv(ae::Assets.Programs["ortho_pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(ae::Graphics.Ortho));
	ae::Graphics.SetProgram(ae::Assets.Programs["text"]);
	glUniformMatrix4fv(ae::Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(ae::Graphics.Ortho));

	// Draw brush info
	DrawBrushInfo();

	// Draw map name
	std::stringstream Buffer;
	Buffer << FilePath;
	ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), glm::vec2(20, 36) * ae::_Element::GetUIScale());
	Buffer.str("");

	// Cursor position
	Buffer << (int)WorldCursor.x << ", " << (int)WorldCursor.y;
	ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), glm::vec2(20 * ae::_Element::GetUIScale(), ae::Graphics.ViewportSize.y - 20 * ae::_Element::GetUIScale()));
	Buffer.str("");

	// FPS
	Buffer << ae::Graphics.FramesPerSecond << " FPS";
	ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::vec2(20, 60) * ae::_Element::GetUIScale());
	Buffer.str("");

	// Clock
	if(Map) {
		Map->GetClockAsString(Buffer);
		ClockElement->Text = Buffer.str();
		Buffer.str("");
	}

	// Draw UI
	EditorElement->Render();
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

// Allocate buffer for copy and paste
void _EditorState::AllocateCopy() {
	if(!Map)
		return;

	CloseCopy();

	CopyBuffer = new _Tile*[Map->Size.x];
	for(int i = 0; i < Map->Size.x; i++)
		CopyBuffer[i] = new _Tile[Map->Size.y];

	Copied = false;
}

// Free memory used by copy and paste buffer
void _EditorState::CloseCopy() {
	if(CopyBuffer && Map) {
		for(int i = 0; i < Map->Size.x; i++)
			delete[] CopyBuffer[i];
		delete[] CopyBuffer;

		CopyBuffer = nullptr;
	}
}

// Draw information about the brush
void _EditorState::DrawBrushInfo() {
	if(!Map)
		return;

	std::stringstream Buffer;
	glm::vec2 DrawPosition = ae::Graphics.Element->Bounds.End - glm::vec2(84, 210) * ae::_Element::GetUIScale();
	glm::vec4 Color(glm::vec4(1.0f));
	float TextSpacingY = 20 * ae::_Element::GetUIScale();

	// Draw backdrop
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos"]);
	ae::Graphics.SetColor(glm::vec4(0, 0, 0, 0.8f));
	ae::Graphics.DrawRectangle(DrawPosition - glm::vec2(64, 64) * ae::_Element::GetUIScale(), DrawPosition + glm::vec2(64, 194) * ae::_Element::GetUIScale(), true);
	if(Mode == EditorModeType::TILE) {

		// Draw texture
		ae::_Bounds TextureBounds;
		TextureBounds.Start = DrawPosition - glm::vec2(Map->TileAtlas->Size) / 2.0f;
		TextureBounds.End = DrawPosition + glm::vec2(Map->TileAtlas->Size) / 2.0f;
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.SetColor(glm::vec4(1.0f));
		ae::Graphics.DrawAtlas(TextureBounds, Map->TileAtlas->Texture, Map->TileAtlas->GetTextureCoords(Brush->BaseTextureIndex));

		DrawPosition.y += 70 * ae::_Element::GetUIScale();

		// Draw layer
		if(ShowTransitions)
			Buffer << "Transitions";
		else
			Buffer << "Base";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");

		DrawPosition.y += TextSpacingY;

		// Draw wall
		if(Brush->Wall)
			Buffer << "Wall";
		else
			Buffer << "Floor";

		Filter & MAP_RENDER_WALL ? Color.a = 1.0f : Color.a = 0.5f;
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");

		DrawPosition.y += TextSpacingY;

		// Draw zone
		Buffer << "Zone " << Brush->Zone;

		Filter & MAP_RENDER_ZONE ? Color.a = 1.0f : Color.a = 0.5f;
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");

		DrawPosition.y += TextSpacingY;

		// Draw PVP
		if(Brush->PVP)
			Buffer << "PVP";
		else
			Buffer << "Safe";

		Filter & MAP_RENDER_PVP ? Color.a = 1.0f : Color.a = 0.5f;
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");

		DrawPosition.y += TextSpacingY;

		// Draw event type
		Buffer << Stats->EventTypes.at(Brush->Event.Type).second;

		Filter & MAP_RENDER_EVENTTYPE ? Color.a = 1.0f : Color.a = 0.5f;
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");

		DrawPosition.y += TextSpacingY;

		// Draw event data
		Buffer << Brush->Event.Data;

		Filter & MAP_RENDER_EVENTDATA ? Color.a = 1.0f : Color.a = 0.5f;
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");
	}
	else if(Mode == EditorModeType::OBJECT) {

		// Draw object type
		Buffer << "Lights";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");

		DrawPosition.y += TextSpacingY;

		// Draw object data
		Buffer << "Data " << ObjectData;
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");
	}
}

// Update generic value
void _EditorState::AdjustValue(uint32_t &Value, int Direction) {
	int IntVal = (int)Value;
	IntVal += Direction;
	if(IntVal < 0)
		IntVal = 0;

	Value = (uint32_t)IntVal;
}

// Copy tiles
void _EditorState::CopyTiles() {
	if(Mode != EditorModeType::TILE)
		return;

	// Set state
	DrawCopyBounds = false;
	Copied = true;

	// Get copy bounds
	GetDrawBounds(CopyStart, CopyEnd);

	// Copy tiles
	for(int j = 0; j < CopyEnd.y - CopyStart.y + 1; j++) {
		for(int i = 0; i < CopyEnd.x - CopyStart.x + 1; i++) {
			glm::ivec2 CopyCoord = glm::ivec2(i, j) + CopyStart;
			if(Map->IsValidPosition(CopyCoord)) {
				CopyBuffer[CopyCoord.x][CopyCoord.y] = Map->Tiles[CopyCoord.x][CopyCoord.y];
			}
		}
	}
}

// Paste tiles
void _EditorState::PasteTiles() {
	if(Mode != EditorModeType::TILE || !Copied)
		return;

	// Get offsets
	glm::ivec2 CopyPosition = CopyStart;
	glm::ivec2 PastePosition = Map->GetValidCoord(WorldCursor);

	// Copy tiles
	for(int j = 0; j < CopyEnd.y - CopyStart.y + 1; j++) {
		for(int i = 0; i < CopyEnd.x - CopyStart.x + 1; i++) {
			glm::ivec2 CopyCoord = glm::ivec2(i, j) + CopyPosition;
			glm::ivec2 PasteCoord = glm::ivec2(i, j) + PastePosition;
			if(Map->IsValidPosition(CopyCoord) && Map->IsValidPosition(PasteCoord)) {
				Map->Tiles[PasteCoord.x][PasteCoord.y] = CopyBuffer[CopyCoord.x][CopyCoord.y];
			}
		}
	}

	// Rebuild map tiles
	Map->BuildLayers(ShowTransitions);
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
	if(!NewMapElement->Active) {
		CloseWindows();
		InitNewMap();
	}
	else {
		CloseWindows();
	}
}

// Toggle resize map screen
void _EditorState::ToggleResize() {
	if(!ResizeMapElement->Active) {
		CloseWindows();
		InitResize();
	}
	else {
		CloseWindows();
	}
}

// Show the texture select screen
void _EditorState::ToggleTextures() {
	if(!TexturesElement->Active) {
		CloseWindows();
		InitTextures();
	}
	else {
		CloseWindows();
	}
}

// Show the event select screen
void _EditorState::ToggleEvents() {
	if(!EventsElement->Active) {
		CloseWindows();
		InitEvents();
	}
	else {
		CloseWindows();
	}
}

// Show save map screen
void _EditorState::ToggleSaveMap() {
	if(!Map)
		return;

	if(!SaveMapElement->Active) {
		CloseWindows();
		InitSaveMap();
	}
	else {
		CloseWindows();
	}
}

// Show load map screen
void _EditorState::ToggleLoadMap(const std::string &TempPath) {
	if(!LoadMapElement->Active) {
		CloseWindows();
		InitLoadMap(TempPath);
	}
	else {
		CloseWindows();
	}
}

// Delete memory used by textures screen
void _EditorState::ClearTextures() {
	for(auto &Child : TexturesElement->Children)
		delete Child;

	TexturesElement->Children.clear();
}

// Delete memory used by events screen
void _EditorState::ClearEvents() {
	for(auto &Child : EventTypesElement->Children)
		delete Child;

	EventTypesElement->Children.clear();
}

// Init texture select
void _EditorState::InitTextures() {
	ClearTextures();

	glm::vec2 Start = glm::vec2(20, 20);
	glm::vec2 Spacing = glm::vec2(20, 20);
	glm::vec2 Offset(Start);

	uint32_t TextureCount = (uint32_t)(Map->TileAtlas->Texture->Size.x * Map->TileAtlas->Texture->Size.y / (Map->TileAtlas->Size.x * Map->TileAtlas->Size.y));
	for(uint32_t i = 0; i < TextureCount; i++) {

		// Add button
		ae::_Element *Button = new ae::_Element();
		Button->Parent = TexturesElement;
		Button->BaseOffset = Offset;
		Button->BaseSize = Map->TileAtlas->Size;
		Button->Alignment = ae::LEFT_TOP;
		Button->Atlas = Map->TileAtlas;
		Button->TextureIndex = i;
		Button->Clickable = true;
		TexturesElement->Children.push_back(Button);

		// Update position
		Offset.x += Map->TileAtlas->Size.x + Spacing.x;
		if(Offset.x > TexturesElement->BaseSize.x - Map->TileAtlas->Size.x) {
			Offset.y += Map->TileAtlas->Size.y + Spacing.y;
			Offset.x = Start.x;
		}
	}

	TexturesElement->CalculateBounds();
	TexturesElement->SetActive(true);
}

// Init event select
void _EditorState::InitEvents() {
	ClearEvents();

	glm::vec2 Start = glm::vec2(20, 20);
	glm::vec2 Spacing = glm::vec2(20, 20);
	glm::vec2 Size = glm::vec2(160, 64);
	glm::vec2 Offset(Start);

	size_t Count = Stats->EventTypes.size();
	EventsElement->BaseSize.x = Start.x + (Spacing.x + Size.x) * 4;
	EventsElement->BaseSize.y = Start.y + (Spacing.y + Size.y) * (Count / 4) + 80;
	EventTypesElement->BaseSize = EventsElement->BaseSize;
	for(size_t i = 0; i < Count; i++) {

		// Add button
		ae::_Element *Button = new ae::_Element();
		Button->Parent = EventTypesElement;
		Button->BaseOffset = Offset;
		Button->BaseSize = Size;
		Button->Alignment = ae::LEFT_TOP;
		Button->Clickable = true;
		Button->Index = (int)i;
		Button->Style = ae::Assets.Styles["style_menu_button"];
		Button->HoverStyle = ae::Assets.Styles["style_menu_button_hover"];
		EventTypesElement->Children.push_back(Button);

		ae::_Element *Label = new ae::_Element();
		Label->Parent = Button;
		Label->BaseOffset = glm::vec2(0, 38);
		Label->Alignment = ae::CENTER_BASELINE;
		Label->Text = Stats->EventTypes.at((EventType)i).second;
		Label->Font = ae::Assets.Fonts["hud_small"];
		Button->Children.push_back(Label);

		// Update position
		Offset.x += Size.x + Spacing.x;
		if(Offset.x > EventsElement->BaseSize.x - Size.x) {
			Offset.y += Size.y + Spacing.y;
			Offset.x = Start.x;
		}
	}

	EventsElement->CalculateBounds();
	EventsElement->SetActive(true);
}

// Init new map
void _EditorState::InitNewMap() {
	NewMapElement->SetActive(true);

	ae::_Element *FilenameTextBox = ae::Assets.Elements["textbox_editor_newmap_filename"];
	FilenameTextBox->SetText("");
	ae::FocusedElement = FilenameTextBox;

	NewMapWidthTextBox->SetText("100");
	NewMapHeightTextBox->SetText("100");
}

// Init resize map screen
void _EditorState::InitResize() {
	if(!Map)
		return;

	ResizeMapElement->SetActive(true);

	ResizeMinXTextBox->SetText("0");
	ResizeMinYTextBox->SetText("0");
	ResizeMaxXTextBox->SetText(std::to_string(Map->Size.x));
	ResizeMaxYTextBox->SetText(std::to_string(Map->Size.y));
}

// Init save map
void _EditorState::InitSaveMap() {
	if(!Map)
		return;

	SaveMapElement->SetActive(true);
	ae::FocusedElement = SaveMapTextBox;

	SaveMapTextBox->SetText(FilePath);
}

// Init load map
void _EditorState::InitLoadMap(const std::string &TempPath) {
	LoadMapElement->SetActive(true);
	ae::FocusedElement = LoadMapTextBox;

	if(TempPath != "")
		LoadMapTextBox->SetText(TempPath);
	else
		LoadMapTextBox->SetText(FilePath);
}

// Close all open windows
bool _EditorState::CloseWindows() {
	bool WasOpen = TexturesElement->Active | EventsElement->Active | NewMapElement->Active | ResizeMapElement->Active | SaveMapElement->Active | LoadMapElement->Active;

	TexturesElement->SetActive(false);
	EventsElement->SetActive(false);
	NewMapElement->SetActive(false);
	ResizeMapElement->SetActive(false);
	SaveMapElement->SetActive(false);
	LoadMapElement->SetActive(false);
	ae::FocusedElement = nullptr;

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
	Map->Stats = Stats;
	Map->UseAtlas = true;
	Map->Size = Size;
	Map->InitAtlas("textures/map/default.png");
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
	CloseCopy();
	Map->ResizeMap(Min, NewSize);
	AllocateCopy();

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
	if(Path.find(MAPS_PATH, 0) == std::string::npos)
		Path = MAPS_PATH + Path;

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
	if(Path.find(MAPS_PATH, 0) == std::string::npos)
		Path = MAPS_PATH + Path;

	// Check for extension
	if(Path.find(".map.gz", 0) == std::string::npos)
		Path = Path + ".map.gz";

	// Attempt to load map
	_Map *NewMap = new _Map();
	NewMap->Stats = Stats;
	NewMap->UseAtlas = true;
	try {
		NewMap->Load(Path);
	}
	catch(std::exception &Error) {
		std::cout << Error.what() << std::endl;
		delete NewMap;
		NewMap = nullptr;
	}

	// Set new map
	if(NewMap) {
		CloseMap();

		// Set map
		Map = NewMap;
		FilePath = LoadMapTextBox->Text;

		// Allocate copy tiles
		AllocateCopy();

		// Set camera position
		glm::ivec2 Position(Map->Size.x/2, Map->Size.y/2);
		if(!Map->FindEvent(_Event(EventType::SPAWN, ""), Position)) {
			//TODO fix
			//if(!Map->FindEvent(_Event(EventType::MAPCHANGE, OldMapID), Position))
			//	Map->FindEvent(_Event(EventType::MAPENTRANCE, OldMapID), Position);
		}
		Camera->ForcePosition(glm::vec3(Position, CAMERA_DISTANCE));

		// Save map id
		UseClockAmbientLight = false;
		if(Map->IsOutside)
			UseClockAmbientLight = true;
	}

	CloseWindows();
}

// Set brush mode
void _EditorState::SwitchBrushModes(int Key) {
	switch(Key) {
		case 1:
			Mode = EditorModeType::TILE;
			Filter = 0;
			Filter |= MAP_RENDER_TEXTURE;
			Filter |= MAP_RENDER_WALL;
		break;
		case 2:
			Mode = EditorModeType::TILE;
			Filter = 0;
			Filter |= MAP_RENDER_ZONE;
		break;
		case 3:
			Mode = EditorModeType::TILE;
			Filter = 0;
			Filter |= MAP_RENDER_PVP;
		break;
		case 4:
			Mode = EditorModeType::TILE;
			Filter = 0;
			Filter |= MAP_RENDER_EVENTTYPE;
			Filter |= MAP_RENDER_EVENTDATA;
		break;
		case 5:
			Mode = EditorModeType::OBJECT;
			Filter = 0;
		break;
	}
}

// Open browser or load map under cursor
void _EditorState::Go() {

	// Event inspector
	if(Map->IsValidPosition(WorldCursor)) {
		const _Tile *Tile = Map->GetTile(WorldCursor);
		switch(Tile->Event.Type) {
			case EventType::MAPENTRANCE:
			case EventType::MAPCHANGE: {
				//TODO fix
				//ToggleLoadMap(GetCleanMapName(Stats->OldMaps.at(Tile->Event.OldData).File));
			} break;
			default:
			break;
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
			if(Filter & MAP_RENDER_TEXTURE)
				Tile.BaseTextureIndex = Brush->BaseTextureIndex;
			if(Filter & MAP_RENDER_WALL)
				Tile.Wall = Brush->Wall;
			if(Filter & MAP_RENDER_ZONE)
				Tile.Zone = Brush->Zone;
			if(Filter & MAP_RENDER_PVP)
				Tile.PVP = Brush->PVP;
			if(Filter & MAP_RENDER_EVENTTYPE)
				Tile.Event.Type = Brush->Event.Type;
			if(Filter & MAP_RENDER_EVENTDATA)
				Tile.Event.Data = Brush->Event.Data;

			// Set new tile
			Map->SetTile(TilePosition, &Tile);
		}
	}

	Map->BuildLayers(ShowTransitions);
}

// Deletes the map
void _EditorState::CloseMap() {
	delete Map;
	Map = nullptr;
}
