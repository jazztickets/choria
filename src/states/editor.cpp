/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2021 Alan Witkowski
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

_EditorState EditorState;

// Constants
const char * const MAPS_PATH = "maps/";

// Constructor
_EditorState::_EditorState() :
	Framebuffer(nullptr),
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
	ae::Graphics.Element->SetActive(false);
	ae::Graphics.Element->Active = true;

	ae::Audio.StopMusic();

	// Setup UI
	ButtonBarElement = ae::Assets.Elements["element_editor_buttonbar"];
	TexturesElement = ae::Assets.Elements["element_editor_textures"];
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
	ButtonBarElement->SetActive(true);
	TexturesElement->SetActive(false);
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
	Layer = 0;
	Filter = 0;
	Filter |= MAP_RENDER_TEXTURE;
	Filter |= MAP_RENDER_WALL;

	// Default map
	Clock = 60.0 * 12.0;
	UseClockAmbientLight = false;
	Map = nullptr;
	MapID = 0;
	if(FilePath != "") {
		LoadMapTextBox->Text = FilePath;
		LoadMap();
	}
	else
		ToggleNewMap();

	DrawCopyBounds = false;
	CopyStart = glm::ivec2(0, 0);
	CopyEnd = glm::ivec2(0, 0);
	WorldCursor = glm::vec2(0.0f, 0.0f);
	BrushMode = EDITOR_BRUSH_MODE_TILE;
	ObjectType = 0;
	ObjectData = 1;
	ShowBackgroundMap = true;
	MapView = false;
}

// Shuts the state down
void _EditorState::Close() {
	delete Stats;
	delete Camera;
	delete Brush;
	delete Framebuffer;

	ClearTextures();
	CloseMap();
}

// Handle actions
bool _EditorState::HandleAction(int InputType, std::size_t Action, int Value) {
	if(Value == 0)
		return false;

	if(Action == Action::MISC_CONSOLE)
		Framework.Console->Toggle();

	return false;
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
				BrushMode = EDITOR_BRUSH_MODE_TILE;
				Filter = 0;
				Filter |= MAP_RENDER_TEXTURE;
				Filter |= MAP_RENDER_WALL;
			break;
			case SDL_SCANCODE_2:
				BrushMode = EDITOR_BRUSH_MODE_TILE;
				Filter = 0;
				Filter |= MAP_RENDER_ZONE;
			break;
			case SDL_SCANCODE_3:
				BrushMode = EDITOR_BRUSH_MODE_TILE;
				Filter = 0;
				Filter |= MAP_RENDER_PVP;
			break;
			case SDL_SCANCODE_4:
				BrushMode = EDITOR_BRUSH_MODE_TILE;
				Filter = 0;
				Filter |= MAP_RENDER_EVENTTYPE;
				Filter |= MAP_RENDER_EVENTDATA;
			break;
			case SDL_SCANCODE_5:
				BrushMode = EDITOR_BRUSH_MODE_OBJECT;
				Filter = 0;
			break;
			case SDL_SCANCODE_T:
				if(ae::Input.ModKeyDown(KMOD_CTRL))
					UseClockAmbientLight = !UseClockAmbientLight;
			break;
			case SDL_SCANCODE_E:
				if(ae::Input.ModKeyDown(KMOD_SHIFT)) {
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
				Framework.IgnoreNextInputEvent = true;
				ToggleNewMap();
			break;
			case SDL_SCANCODE_M:
				ShowBackgroundMap = !ShowBackgroundMap;
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
				Paste();
			break;
			case SDL_SCANCODE_D:
				if(ae::Input.ModKeyDown(KMOD_CTRL) && ae::Input.ModKeyDown(KMOD_ALT)) {
					for(auto &Object : Map->StaticObjects)
						delete Object;

					Map->StaticObjects.clear();
				}
				else {
					glm::vec2 MousePosition;
					Camera->ConvertScreenToWorld(ae::Input.GetMouse(), MousePosition);
					Map->DeleteStaticObject(MousePosition);
				}
			break;
			case SDL_SCANCODE_H:
				MapView = !MapView;
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
					if(ae::Input.ModKeyDown(KMOD_CTRL) && Map->IsValidPosition(WorldCursor))
						*Brush = *Map->GetTile(WorldCursor);
				break;
				// Scroll map
				case SDL_BUTTON_RIGHT:
					Camera->Set2DPosition(WorldCursor);
				break;
				case SDL_BUTTON_MIDDLE: {
					if(BrushMode == EDITOR_BRUSH_MODE_TILE) {
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
				Brush->TextureIndex[Layer] = Button->TextureIndex;
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
		else if(BrushMode == EDITOR_BRUSH_MODE_OBJECT) {

			// Place light
			_Object *Object = new _Object;
			Object->Position = WorldCursor;
			Object->Light = ObjectData;
			Map->StaticObjects.push_back(Object);
		}
	}
	// Release middle mouse
	else if(MouseEvent.Button == SDL_BUTTON_MIDDLE) {
		DrawCopyBounds = false;
		GetDrawBounds(CopyStart, CopyEnd);
	}
}

// Mouse scroll wheel
void _EditorState::HandleMouseWheel(int Direction) {
	if(ae::Input.ModKeyDown(KMOD_CTRL)) {
		if(ae::Input.ModKeyDown(KMOD_SHIFT))
			Direction *= 10;

		if(BrushMode == EDITOR_BRUSH_MODE_TILE) {
			if(Filter & MAP_RENDER_ZONE) {
				AdjustValue(Brush->Zone, Direction);
			}
			else if(Filter & MAP_RENDER_EVENTDATA) {
				AdjustValue(Brush->Event.Data, Direction);
			}
		}
		else if(BrushMode == EDITOR_BRUSH_MODE_OBJECT) {
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
		ae::Graphics.ResetState();
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
	//	std::cout << ae::Graphics.Element->HitElement << std::endl;

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
	if(BrushMode == EDITOR_BRUSH_MODE_TILE) {
		if(ae::Input.MouseDown(SDL_BUTTON_LEFT) && !(ae::Input.ModKeyDown(KMOD_CTRL)) && ae::Graphics.Element->HitElement == nullptr) {
			ApplyBrush(WorldCursor);
		}
	}

	// Handle key input
	if(ae::Input.KeyDown(SDL_SCANCODE_T) && !ae::Input.ModKeyDown(KMOD_CTRL)) {
		if(ae::Input.ModKeyDown(KMOD_SHIFT)) {
			Clock -= FrameTime * MAP_EDITOR_CLOCK_SPEED;
			if(Clock < 0)
				Clock += MAP_DAY_LENGTH;
		}
		else {
			Clock += FrameTime * MAP_EDITOR_CLOCK_SPEED;
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
		int RenderFilter = Filter | MAP_RENDER_BOUNDARY | MAP_RENDER_EDGE_BOUNDARY;

		if(MapView)
			RenderFilter = MAP_RENDER_MAP_VIEW;

		if(!ShowBackgroundMap)
			RenderFilter |= MAP_RENDER_NOBACKGROUND;

		if(!UseClockAmbientLight)
			RenderFilter |= MAP_RENDER_EDITOR_AMBIENT;

		Map->Render(Camera, Framebuffer, nullptr, BlendFactor, RenderFilter);
	}

	// Draw tile brush size
	if(!MapView) {
		ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
		ae::Graphics.SetColor(glm::vec4(1.0f));
		if(BrushMode == EDITOR_BRUSH_MODE_TILE) {
			ae::Graphics.DrawCircle(glm::vec3(WorldCursor, 0.0f), BrushRadius);

			// Draw copy tool boundaries
			if(DrawCopyBounds) {
				ae::Graphics.SetColor(ae::Assets.Colors["editor_select"]);

				glm::ivec2 Start, End;
				GetDrawBounds(Start, End);
				ae::Graphics.DrawRectangle(Start, End + 1, true);
			}
		}
		else if(BrushMode == EDITOR_BRUSH_MODE_OBJECT) {
			for(const auto &Object : Map->StaticObjects) {
				if(Object->Light)
					ae::Graphics.DrawCircle(glm::vec3(Object->Position, 0) + glm::vec3(0.5f, 0.5f, 0), Stats->Lights.at(Object->Light).Radius);
			}
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
		Map->GetClockAsString(Buffer, Config.Clock24Hour);
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), glm::vec2(ae::Graphics.ViewportSize.x - 84 * ae::_Element::GetUIScale(), 50 * ae::_Element::GetUIScale()));
		Buffer.str("");
	}

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
	std::size_t PrefixStart = CleanName.find(Prefix);
	if(PrefixStart != std::string::npos)
		CleanName = CleanName.substr(Prefix.length());

	// Remove extension
	std::string Suffix = ".map.gz";
	std::size_t SuffixStart = CleanName.find(Suffix);
	if(SuffixStart != std::string::npos)
		CleanName = CleanName.substr(0, SuffixStart);

	return CleanName;
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
	if(BrushMode == EDITOR_BRUSH_MODE_TILE) {

		// Draw texture
		ae::_Bounds TextureBounds;
		TextureBounds.Start = DrawPosition - UI_TILE_SIZE / 2.0f;
		TextureBounds.End = DrawPosition + UI_TILE_SIZE / 2.0f;
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.SetColor(glm::vec4(1.0f));
		ae::Graphics.DrawAtlasTexture(TextureBounds, Map->TileAtlas->Texture, Map->TileAtlas->GetTextureCoords(Brush->TextureIndex[Layer]));

		DrawPosition.y += 70 * ae::_Element::GetUIScale();

		// Draw layer
		if(Layer)
			Buffer << "Fore";
		else
			Buffer << "Back";
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
		Buffer << Stats->EventNames[Brush->Event.Type].Name;

		Filter & MAP_RENDER_EVENTTYPE ? Color.a = 1.0f : Color.a = 0.5f;
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");

		DrawPosition.y += TextSpacingY;

		// Draw event data
		Buffer << "Data " << Brush->Event.Data;

		Filter & MAP_RENDER_EVENTDATA ? Color.a = 1.0f : Color.a = 0.5f;
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");
	}
	else if(BrushMode == EDITOR_BRUSH_MODE_OBJECT) {

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

// Paste tiles
void _EditorState::Paste() {
	if(BrushMode != EDITOR_BRUSH_MODE_TILE)
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

// Init texture select
void _EditorState::InitTextures() {

	// Clear old children
	ClearTextures();

	glm::vec2 Start = glm::vec2(20, 20);
	glm::vec2 Spacing = glm::vec2(20, 20);
	glm::vec2 Offset(Start);

	uint32_t TextureCount = (uint32_t)(Map->TileAtlas->Texture->Size.x * Map->TileAtlas->Texture->Size.y / (Map->TileAtlas->Size.x * Map->TileAtlas->Size.y));

	// Iterate over textures
	for(uint32_t i = 0; i < TextureCount; i++) {

		// Add button
		ae::_Element *Button = new ae::_Element();
		Button->Name = "button_skills_skill";
		Button->Parent = TexturesElement;
		Button->BaseOffset = Offset;
		Button->BaseSize = UI_TILE_SIZE;
		Button->Alignment = ae::LEFT_TOP;
		Button->Atlas = Map->TileAtlas;
		Button->TextureIndex = i;
		Button->Clickable = true;
		TexturesElement->Children.push_back(Button);

		// Update position
		Offset.x += UI_TILE_SIZE.x + Spacing.x;
		if(Offset.x > TexturesElement->BaseSize.x - UI_TILE_SIZE.x) {
			Offset.y += UI_TILE_SIZE.y + Spacing.y;
			Offset.x = Start.x;
		}
	}

	TexturesElement->CalculateBounds();
	TexturesElement->SetActive(true);
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
	bool WasOpen = TexturesElement->Active | NewMapElement->Active | ResizeMapElement->Active | SaveMapElement->Active | LoadMapElement->Active;

	TexturesElement->SetActive(false);
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
	Map->Size = Size;
	Map->InitAtlas(MAP_DEFAULT_TILESET);
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
	uint32_t OldMapID = MapID;
	try {
		MapID = Stats->GetMapIDByPath(Path);
		NewMap->Load(&Stats->Maps.at(MapID));
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
		glm::ivec2 Position(Map->Size.x/2, Map->Size.y/2);
		if(!Map->FindEvent(_Event(_Map::EVENT_SPAWN, 0), Position)) {
			if(!Map->FindEvent(_Event(_Map::EVENT_MAPCHANGE, OldMapID), Position))
				Map->FindEvent(_Event(_Map::EVENT_MAPENTRANCE, OldMapID), Position);
		}
		Camera->ForcePosition(glm::vec3(Position, CAMERA_DISTANCE));

		// Save map id
		MapID = Stats->GetMapIDByPath(Path);
		UseClockAmbientLight = false;
		if(Map->OutsideFlag)
			UseClockAmbientLight = true;
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
				ToggleLoadMap(GetCleanMapName(Stats->Maps.at(Tile->Event.Data).File));
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
			case _Map::EVENT_SCRIPT: {
				std::stringstream Buffer;
				Buffer << Config.BrowserCommand << " \"" << Config.DesignToolURL << "/?table=script\"";
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
			if(Filter & MAP_RENDER_TEXTURE)
				Tile.TextureIndex[Layer] = Brush->TextureIndex[Layer];
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
}

// Deletes the map
void _EditorState::CloseMap() {
	delete Map;
	Map = nullptr;
}
