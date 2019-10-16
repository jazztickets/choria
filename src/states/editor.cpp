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
#include <ae/texture_array.h>
#include <ae/tilemap.h>
#include <ae/program.h>
#include <ae/audio.h>
#include <ae/font.h>
#include <ae/ui.h>
#include <ae/input.h>
#include <ae/console.h>
#include <ae/graphics.h>
#include <ae/files.h>
#include <ae/framebuffer.h>
#include <ae/util.h>
#include <objects/components/light.h>
#include <objects/components/prop.h>
#include <objects/object.h>
#include <objects/map.h>
#include <framework.h>
#include <stats.h>
#include <scripting.h>
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
	InfoElement = ae::Assets.Elements["element_editor_info"];
	InfoMusicElement = ae::Assets.Elements["textbox_editor_info_music"];
	ZonesElement = ae::Assets.Elements["element_editor_zones"];
	LightsElement = ae::Assets.Elements["element_editor_lights"];
	LightTypesElement = ae::Assets.Elements["element_editor_light_types"];
	LightScriptElement = ae::Assets.Elements["textbox_editor_light_script"];
	PropsElement = ae::Assets.Elements["element_editor_props"];
	PropTypesElement = ae::Assets.Elements["element_editor_prop_types"];
	EventsElement = ae::Assets.Elements["element_editor_events"];
	EventTypesElement = ae::Assets.Elements["element_editor_event_types"];
	EventDataElement = ae::Assets.Elements["textbox_editor_event_data"];
	EventMessageElement = ae::Assets.Elements["label_editor_event_message"];
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
	InfoElement->SetActive(false);
	TexturesElement->SetActive(false);
	ZonesElement->SetActive(false);
	LightsElement->SetActive(false);
	PropsElement->SetActive(false);
	EventsElement->SetActive(false);
	NewMapElement->SetActive(false);
	ResizeMapElement->SetActive(false);
	SaveMapElement->SetActive(false);
	LoadMapElement->SetActive(false);

	// Load stats database
	Stats = new _Stats();
	Scripting = new _Scripting();
	Scripting->Setup(Stats, GAME_SCRIPTS);

	// Create brush
	TileBrush = new _Tile();
	LightBrush = new _Light();
	LightBrush->Color = glm::vec4(1.0f);
	PropBrush = new _Prop();
	BrushRadius = 0.5f;

	// Create camera
	Camera = new ae::_Camera(glm::vec3(0, 0, CAMERA_DISTANCE), CAMERA_EDITOR_DIVISOR, CAMERA_FOVY, CAMERA_NEAR, CAMERA_FAR);
	Camera->CalculateFrustum(ae::Graphics.AspectRatio);

	// Create framebuffer for lights
	Framebuffer = new ae::_Framebuffer(ae::Graphics.CurrentSize);

	// Set filters
	ShowTransitions = true;
	Filter = 0;
	Filter |= MAP_RENDER_TEXTURE;
	Filter |= MAP_RENDER_WALL;

	// Default map
	Clock = 60.0 * 12.0;
	UseClockAmbientLight = true;
	Map = nullptr;
	CopyBuffer = nullptr;
	if(FilePath != "") {
		LoadMapTextBox->Text = FilePath;
		LoadMap();
	}
	else
		ToggleNewMap();

	CopiedTiles = false;
	DrawCopyBounds = false;
	DrawingObject = false;
	DrawingSelect = false;
	MovingObjects = false;
	ResizingObject = false;
	CopiedObjects = false;
	CopyPosition = glm::vec2(0.0f, 0.0f);
	ObjectStart = glm::vec2(0.0f, 0.0f);
	CopyStart = glm::ivec2(0, 0);
	CopyEnd = glm::ivec2(0, 0);
	WorldCursor = glm::vec2(0.0f, 0.0f);
	Mode = EditorModeType::TILES;

	// Set up slider data
	LightSliderData = {
		{ "light_r", &LightBrush->Color.r },
		{ "light_g", &LightBrush->Color.g },
		{ "light_b", &LightBrush->Color.b },
		{ "light_a", &LightBrush->Color.a },
	};

	SetLightUI(glm::vec4(1.0f), "");
}

// Shuts the state down
void _EditorState::Close() {
	delete Scripting;
	delete Stats;
	delete Camera;
	delete TileBrush;
	delete PropBrush;
	delete LightBrush;
	delete Framebuffer;

	ClearTextures();
	ClearLights();
	ClearProps();
	ClearEvents();
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
				else if(LightsElement->Active) {
					if(LightScriptElement == ae::FocusedElement) {
						for(auto &Iterator : SelectedObjects) {
							Iterator.first->Light->Script = ae::TrimString(LightScriptElement->Text);
						}
						ae::FocusedElement = nullptr;
					}
				}
				else if(Map && InfoElement->Active) {
					if(InfoMusicElement == ae::FocusedElement) {
						Map->Music = ae::TrimString(InfoMusicElement->Text);
						ae::FocusedElement = nullptr;
					}
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
				SwitchBrushModes((int)(KeyEvent.Scancode - SDL_SCANCODE_1 + 1));
			break;
			case SDL_SCANCODE_T:
				if(ae::Input.ModKeyDown(KMOD_CTRL))
					UseClockAmbientLight = !UseClockAmbientLight;
			break;
			case SDL_SCANCODE_E:
				SwitchBrushModes(4);
				ToggleEvents();
			break;
			case SDL_SCANCODE_W:
				TileBrush->Wall = !TileBrush->Wall;
			break;
			case SDL_SCANCODE_A:
				ToggleInfo();
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
			case SDL_SCANCODE_C:
				CopyObjects();
			break;
			case SDL_SCANCODE_V:
				if(Mode == EditorModeType::TILES)
					PasteTiles();
				else
					PasteObjects();
			break;
			case SDL_SCANCODE_D:
				DeleteSelectedObjects();
			break;
			case SDL_SCANCODE_TAB:
				ShowTransitions = !ShowTransitions;
				Map->BuildLayers(glm::ivec4(0, 0, Map->Size.x, Map->Size.y), ShowTransitions);
			break;
			case SDL_SCANCODE_SPACE:
				switch(Mode) {
					case EditorModeType::TILES:
						if(Filter & MAP_RENDER_TEXTURE)
							ToggleTextures();
						else if(Filter & MAP_RENDER_ZONE)
							ToggleZones();
						else if(Filter & MAP_RENDER_PVP)
							TileBrush->PVP = !TileBrush->PVP;
						else if(Filter & MAP_RENDER_EVENT_TYPE)
							ToggleEvents();
					break;
					case EditorModeType::LIGHTS:
						ToggleLights();
					break;
					case EditorModeType::PROPS:
						ToggleProps();
					break;
				}
			break;
			case SDL_SCANCODE_F1:
				SwitchMode(EditorModeType::TILES);
			break;
			case SDL_SCANCODE_F2:
				SwitchMode(EditorModeType::LIGHTS);
			break;
			case SDL_SCANCODE_F3:
				SwitchMode(EditorModeType::PROPS);
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
		if(!Map)
			return;

		// Ignore UI elements
		if(ae::Graphics.Element->HitElement)
			return;

		switch(MouseEvent.Button) {
			case SDL_BUTTON_LEFT:
				switch(Mode) {
					// Eyedropper tool
					case EditorModeType::TILES:
						if(ae::Input.ModKeyDown(KMOD_CTRL) && Map->IsValidPosition(WorldCursor)) {
							*TileBrush = *Map->GetTile(WorldCursor);
							EventDataElement->Text = TileBrush->Event.Data;
						}
					break;
					// Start drawing light
					case EditorModeType::LIGHTS:
						if(!LightBrush->Texture)
							break;

						StartDrawingObject();
					break;
					// Start drawing props
					case EditorModeType::PROPS:
						if(!PropBrush->Texture)
							break;

						StartDrawingObject();
					break;
				}
			break;
			// Scroll map
			case SDL_BUTTON_RIGHT:
				Camera->Set2DPosition(WorldCursor);
			break;
			case SDL_BUTTON_MIDDLE: {
				switch(Mode) {
					// Copy tiles
					case EditorModeType::TILES:
						DrawCopyBounds = true;
						CopyStart = Map->GetValidCoord(WorldCursor);
					break;
					// Select or move objects
					case EditorModeType::LIGHTS:
					case EditorModeType::PROPS:
						if(TouchingSelectedObjects(WorldCursor)) {
							if(ae::Input.ModKeyDown(KMOD_SHIFT) && SelectedObjects.size() == 1)
								ResizingObject = true;
							else
								MovingObjects = true;
						}
						else
							DrawingSelect = true;

						if(ResizingObject) {
							_Object *SelectedObject = GetSingleSelectedObject();
							if(SelectedObject->Shape.IsAABB())
								ObjectStart = SelectedObject->Position - SelectedObject->Shape.HalfSize;
							else
								ObjectStart = SelectedObject->Position;
						}
						else
							ObjectStart = WorldCursor;
					break;
				}
			} break;
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
		else if(InfoElement->GetClickedElement()) {
			if(InfoElement->GetClickedElement()->Name == "button_editor_info_outside") {
				ae::_Element *Check = ae::Assets.Elements["label_editor_info_outside_check"];
				Check->Text = Check->Text.empty() ? "X" : "";

				if(Map) {
					Map->IsOutside = !Check->Text.empty();
					UpdateSliders(MapInfoData, true);
				}
			}
		}
		// Texture select
		else if(TexturesElement->GetClickedElement()) {
			if(TexturesElement->GetClickedElement() != TexturesElement) {
				ae::_Element *Button = TexturesElement->GetClickedElement();
				TileBrush->BaseTextureIndex = Button->TextureIndex;
				CloseWindows();
			}
		}
		// Zone select
		else if(ZonesElement->GetClickedElement()) {
			if(ZonesElement->GetClickedElement() != ZonesElement) {
				ae::_Element *Button = ZonesElement->GetClickedElement();
				TileBrush->ZoneID = Button->Children.front()->Text;
				CloseWindows();
			}
		}
		// Light select
		else if(LightsElement->GetClickedElement()) {
			ae::_Element *ClickedElement = LightsElement->GetClickedElement();
			if(ClickedElement->Parent && ClickedElement->Parent == LightTypesElement) {
				LightBrush->Texture = ClickedElement->Texture;
			}
		}
		// Props select
		else if(PropsElement->GetClickedElement()) {
			ae::_Element *ClickedElement = PropsElement->GetClickedElement();
			if(ClickedElement->Parent && ClickedElement->Parent == PropTypesElement) {
				PropBrush->Texture = ClickedElement->Texture;
				CloseWindows();
			}
		}
		// Event select
		else if(EventsElement->GetClickedElement()) {
			ae::_Element *ClickedElement = EventsElement->GetClickedElement();

			// Clicked event type button
			if(ClickedElement->Parent && ClickedElement->Parent == EventTypesElement) {

				// Validate data
				switch((EventType)ClickedElement->Index) {
					case EventType::VENDOR:
						if(Stats->Vendors.find(EventDataElement->Text) == Stats->Vendors.end()) {
							EventMessageElement->Text = "Vendor \"" + EventDataElement->Text + "\" not found!";
							return;
						}
					break;
					case EventType::TRADER:
						if(Stats->Traders.find(EventDataElement->Text) == Stats->Traders.end()) {
							EventMessageElement->Text = "Trader \"" + EventDataElement->Text + "\" not found!";
							return;
						}
					break;
					case EventType::BLACKSMITH:
						if(Stats->Blacksmiths.find(EventDataElement->Text) == Stats->Blacksmiths.end()) {
							EventMessageElement->Text = "Blacksmith \"" + EventDataElement->Text + "\" not found!";
							return;
						}
					break;
					case EventType::MINIGAME:
						if(Stats->Minigames.find(EventDataElement->Text) == Stats->Minigames.end()) {
							EventMessageElement->Text = "Minigame \"" + EventDataElement->Text + "\" not found!";
							return;
						}
					break;
				}

				// Set brush data
				TileBrush->Event.Type = (EventType)ClickedElement->Index;
				TileBrush->Event.Data = ae::TrimString(EventDataElement->Text);
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
		else {

			// Place object
			switch(Mode) {
				case EditorModeType::LIGHTS: {
					if(!DrawingObject)
						break;

					if(!LightBrush->Texture)
						break;

					_Object *Object = new _Object();
					Object->Scripting = Scripting;
					Object->Light->Texture = LightBrush->Texture;
					Object->Light->Color = LightBrush->Color;
					Object->Light->Script = LightScriptElement->Text;
					SetObjectSize(Object, ae::Input.ModKeyDown(KMOD_SHIFT));
					Map->StaticObjects.push_back(Object);
				} break;
				case EditorModeType::PROPS: {
					if(!DrawingObject)
						break;

					if(!PropBrush->Texture)
						break;

					_Object *Object = new _Object();
					Object->Scripting = Scripting;
					Object->Prop = new _Prop(Object);
					Object->Prop->Texture = PropBrush->Texture;
					Object->Prop->Color = PropBrush->Color;
					SetObjectSize(Object, ae::Input.ModKeyDown(KMOD_SHIFT));
					Map->StaticObjects.push_back(Object);
				} break;
			}
		}

		DrawingObject = false;
	}
	// Release middle mouse
	else if(MouseEvent.Button == SDL_BUTTON_MIDDLE) {
		if(Mode == EditorModeType::TILES)
			CopyTiles();

		// Select objects
		if(DrawingSelect) {
			DrawingSelect = false;

			// Append to selection
			bool ToggleMode = ae::Input.ModKeyDown(KMOD_SHIFT);
			if(!ToggleMode) {
				SelectedObjects.clear();
				CopiedObjects = false;
			}

			// Get bounds of drawn box
			ae::_Bounds Bounds;
			GetDrawBounds(Bounds, false);
			for(const auto &Object : Map->StaticObjects) {
				if(Object->CheckAABB(Bounds)) {

					// Add or remove from selection
					const auto &Iterator = SelectedObjects.find(Object);
					if(ToggleMode && Iterator != SelectedObjects.end())
						SelectedObjects.erase(Iterator);
					else
						SelectedObjects[Object] = 1;
				}
			}

			// Eyedropper tool for light
			if(ae::Input.ModKeyDown(KMOD_CTRL)) {
				_Object *Object = GetSingleSelectedObject();
				if(Object) {
					LightBrush->Color = Object->Light->Color;
					LightBrush->Texture = Object->Light->Texture;
					SetLightUI(LightBrush->Color, Object->Light->Script);
				}
			}
		}

		// Translate objects
		if(MovingObjects) {
			MovingObjects = false;

			// Update object positions
			glm::vec2 Offset;
			GetCursorOffset(Offset, ObjectStart);
			for(const auto &Iterator : SelectedObjects)
				Iterator.first->Position += Offset;
		}

		// Resizing an object
		if(ResizingObject) {
			ResizingObject = false;
			_Object *Object = GetSingleSelectedObject();
			if(Object)
				SetObjectSize(Object, Object->Shape.IsAABB());
		}
	}
}

// Mouse scroll wheel
void _EditorState::HandleMouseWheel(int Direction) {
	if(ae::Input.ModKeyDown(KMOD_CTRL)) {
		if(ae::Input.ModKeyDown(KMOD_SHIFT))
			Direction *= 10;

		switch(Mode) {
			case EditorModeType::TILES:
				BrushRadius += Direction;
				if(BrushRadius < 0.5f)
					BrushRadius = 0.5f;
				if(BrushRadius > 128.5f)
					BrushRadius = 128.5f;
			break;
			case EditorModeType::LIGHTS:
			break;
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
	Map->Update(FrameTime);
	Map->Clock = Clock;

	// Update camera
	if(Camera) {
		Camera->Update(FrameTime);

		// Get world cursor
		Camera->ConvertScreenToWorld(ae::Input.GetMouse(), WorldCursor);
		WorldCursor = Map->GetValidPosition(WorldCursor);
	}

	// Handle mouse input
	if(ae::Graphics.Element->HitElement == nullptr) {
		switch(Mode) {
			case EditorModeType::TILES:
				if(ae::Input.MouseDown(SDL_BUTTON_LEFT) && !ae::Input.ModKeyDown(KMOD_CTRL)) {
					ApplyBrush(WorldCursor);
				}
			break;
			case EditorModeType::LIGHTS:
			break;
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

	// Update UI
	if(LightsElement->Active)
		UpdateSliders(LightSliderData);
	if(Map && InfoElement->Active)
		UpdateSliders(MapInfoData);
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
		if(!UseClockAmbientLight)
			RenderFilter |= MAP_RENDER_EDITOR_AMBIENT;

		Map->Render(Camera, Framebuffer, nullptr, BlendFactor, RenderFilter);

		// Handle moving objects
		glm::vec2 Offset(0.0f, 0.0f);
		if(MovingObjects)
			GetCursorOffset(Offset, ObjectStart);

		// Render selected objects
		ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
		ae::Graphics.SetColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.5f));
		for(const auto &Iterator : SelectedObjects) {
			const _Object *Object = Iterator.first;
			glm::vec2 Position = Offset + Object->Position;
			if(Object->Shape.IsAABB())
				ae::Graphics.DrawRectangle3D(Position - Object->Shape.HalfSize, Position + Object->Shape.HalfSize, false);
			else
				ae::Graphics.DrawCircle(glm::vec3(Position, 0), Object->Shape.HalfSize.x);
		}
	}

	glm::vec2 SelectionSize(0, 0);
	bool DrawSelectionSize = false;
	switch(Mode) {
		case EditorModeType::TILES:

			if(DrawCopyBounds) {
				ae::Graphics.SetColor(ae::Assets.Colors["editor_select"]);

				// Draw copy tool boundaries
				glm::ivec2 Start, End;
				GetTileDrawBounds(Start, End);
				ae::Graphics.DrawRectangle(Start, End + 1, true);

				// Get size
				SelectionSize = End - Start + 1;
				DrawSelectionSize = true;
			}
			else {

				// Draw tile brush size
				ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
				ae::Graphics.SetColor(glm::vec4(1.0f));
				ae::Graphics.DrawCircle(glm::vec3(WorldCursor, 0.0f), BrushRadius);
			}
		break;
		case EditorModeType::LIGHTS:
		case EditorModeType::PROPS:

			// Draw potential light
			if(DrawingObject || ResizingObject) {
				ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);

				bool AsRectangle = false;
				if(ae::Input.ModKeyDown(KMOD_SHIFT))
					AsRectangle = !AsRectangle;

				if(ResizingObject) {
					_Object *Object = GetSingleSelectedObject();
					if(Object)
						AsRectangle = Object->Shape.IsAABB() ? true : false;
					ae::Graphics.SetColor(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
				}
				else
					ae::Graphics.SetColor(glm::vec4(1.0f));

				if(AsRectangle) {
					ae::_Bounds Bounds;
					GetDrawBounds(Bounds, true);
					SelectionSize = Bounds.End - Bounds.Start;
					ae::Graphics.DrawRectangle3D(Bounds.Start, Bounds.End, false);
				}
				else {
					float Radius = GetObjectRadius();
					ae::Graphics.DrawCircle(glm::vec3(ObjectStart, 0.0f), Radius);

					SelectionSize = glm::ivec2(Radius * 2);
				}

				DrawSelectionSize = true;
			}
		break;
	}

	// Draw select box
	if(DrawingSelect) {
		ae::Graphics.SetProgram(ae::Assets.Programs["pos"]);
		ae::Graphics.SetColor(glm::vec4(1.0f));
		ae::Graphics.DrawRectangle3D(ObjectStart, WorldCursor, false);
	}

	// Setup UI
	ae::Graphics.Setup2D();
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
	glUniformMatrix4fv(ae::Assets.Programs["ortho_pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(ae::Graphics.Ortho));
	ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv_array"]);
	glUniformMatrix4fv(ae::Assets.Programs["ortho_pos_uv_array"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(ae::Graphics.Ortho));
	ae::Graphics.SetProgram(ae::Assets.Programs["text"]);
	glUniformMatrix4fv(ae::Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(ae::Graphics.Ortho));

	// Draw brush info
	DrawBrushInfo();

	// Draw size of selection
	std::stringstream Buffer;
	if(DrawSelectionSize) {
		Buffer << SelectionSize.x << "x" << SelectionSize.y;
		ae::Assets.Fonts["hud_small"]->DrawText(Buffer.str(), glm::vec2(ae::Input.GetMouse()) + glm::vec2(25, 16) * ae::_Element::GetUIScale());
		Buffer.str("");
	}

	// Draw map name
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

	CopyBuffer = new _Tile*[Map->Size.x];
	for(int i = 0; i < Map->Size.x; i++)
		CopyBuffer[i] = new _Tile[Map->Size.y];

	CopiedTiles = false;
}

// Free memory used by copy and paste buffer
void _EditorState::CloseCopy() {
	if(!CopyBuffer || !Map)
		return;

	for(int i = 0; i < Map->Size.x; i++)
		delete[] CopyBuffer[i];
	delete[] CopyBuffer;

	CopyBuffer = nullptr;
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
	if(Mode == EditorModeType::TILES) {

		// Draw texture
		ae::_Bounds TextureBounds;
		TextureBounds.Start = DrawPosition - glm::vec2(64) / 2.0f;
		TextureBounds.End = DrawPosition + glm::vec2(64) / 2.0f;
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv_array"]);
		ae::Graphics.SetColor(glm::vec4(1.0f));
		ae::Graphics.DrawTextureArray(TextureBounds, ae::Assets.TextureArrays["default"], TileBrush->BaseTextureIndex);

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
		if(TileBrush->Wall)
			Buffer << "Wall";
		else
			Buffer << "Walkable";

		Filter & MAP_RENDER_WALL ? Color.a = 1.0f : Color.a = 0.5f;
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");

		DrawPosition.y += TextSpacingY;

		// Draw zone
		if(TileBrush->ZoneID.length())
			Buffer << TileBrush->ZoneID;
		else
			Buffer << "No zone";

		Filter & MAP_RENDER_ZONE ? Color.a = 1.0f : Color.a = 0.5f;
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");

		DrawPosition.y += TextSpacingY;

		// Draw PVP
		if(TileBrush->PVP)
			Buffer << "PVP";
		else
			Buffer << "Safe";

		Filter & MAP_RENDER_PVP ? Color.a = 1.0f : Color.a = 0.5f;
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");

		DrawPosition.y += TextSpacingY;

		// Draw event type
		Buffer << Stats->EventTypes.at(TileBrush->Event.Type).second;

		Filter & MAP_RENDER_EVENT_TYPE ? Color.a = 1.0f : Color.a = 0.5f;
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");

		DrawPosition.y += TextSpacingY;

		// Draw event data
		Buffer << TileBrush->Event.Data;

		Filter & MAP_RENDER_EVENT_DATA ? Color.a = 1.0f : Color.a = 0.5f;
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");
	}
	else if(Mode == EditorModeType::LIGHTS) {

		// Draw texture
		ae::_Bounds TextureBounds;
		TextureBounds.Start = DrawPosition - glm::vec2(64) / 2.0f;
		TextureBounds.End = DrawPosition + glm::vec2(64) / 2.0f;
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.SetColor(LightBrush->Color);
		if(LightBrush->Texture)
			ae::Graphics.DrawImage(TextureBounds, LightBrush->Texture);

		DrawPosition.y += 70 * ae::_Element::GetUIScale();

		// Draw number of visible lights
		if(Map) {
			Buffer << Map->LightCount << " visible";
			ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
			Buffer.str("");
			DrawPosition.y += TextSpacingY;
		}

		// Draw selected count
		Buffer << SelectedObjects.size() << " selected";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");
		DrawPosition.y += TextSpacingY;
	}
	else if(Mode == EditorModeType::PROPS) {

		// Draw texture
		ae::_Bounds TextureBounds;
		TextureBounds.Start = DrawPosition - glm::vec2(64) / 2.0f;
		TextureBounds.End = DrawPosition + glm::vec2(64) / 2.0f;
		ae::Graphics.SetProgram(ae::Assets.Programs["ortho_pos_uv"]);
		ae::Graphics.SetColor(PropBrush->Color);
		if(PropBrush->Texture)
			ae::Graphics.DrawImage(TextureBounds, PropBrush->Texture);

		DrawPosition.y += 70 * ae::_Element::GetUIScale();

		// Draw number of visible objects
		if(Map) {
			Buffer << Map->PropCount << " visible";
			ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
			Buffer.str("");
			DrawPosition.y += TextSpacingY;
		}

		// Draw selected count
		Buffer << SelectedObjects.size() << " selected";
		ae::Assets.Fonts["hud_tiny"]->DrawText(Buffer.str(), DrawPosition, ae::CENTER_BASELINE, Color);
		Buffer.str("");
		DrawPosition.y += TextSpacingY;
	}
}

// Copy tiles
void _EditorState::CopyTiles() {
	if(!Map || Mode != EditorModeType::TILES)
		return;

	// Set state
	DrawCopyBounds = false;
	CopiedTiles = true;

	// Get copy bounds
	GetTileDrawBounds(CopyStart, CopyEnd);

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
	if(Mode != EditorModeType::TILES || !CopiedTiles)
		return;

	// Get offsets
	glm::ivec2 CopyPosition = CopyStart;
	glm::ivec2 PastePosition = Map->GetValidCoord(WorldCursor);

	// Copy tiles
	int CopyWidth = CopyEnd.x - CopyStart.x + 1;
	int CopyHeight = CopyEnd.y - CopyStart.y + 1;
	for(int j = 0; j < CopyHeight; j++) {
		for(int i = 0; i < CopyWidth; i++) {
			glm::ivec2 CopyCoord = glm::ivec2(i, j) + CopyPosition;
			glm::ivec2 PasteCoord = glm::ivec2(i, j) + PastePosition;
			if(Map->IsValidPosition(CopyCoord) && Map->IsValidPosition(PasteCoord)) {
				Map->Tiles[PasteCoord.x][PasteCoord.y] = CopyBuffer[CopyCoord.x][CopyCoord.y];
			}
		}
	}

	// Rebuild map tiles
	Map->BuildLayers(glm::ivec4(PastePosition - glm::ivec2(1), PastePosition + glm::ivec2(CopyWidth, CopyHeight)), ShowTransitions);
}

// Get bounds of drawn rectangle
void _EditorState::GetDrawBounds(ae::_Bounds &Bounds, bool Round) {
	Bounds.Start.x = std::min(ObjectStart.x, WorldCursor.x);
	Bounds.Start.y = std::min(ObjectStart.y, WorldCursor.y);
	Bounds.End.x = std::max(ObjectStart.x, WorldCursor.x);
	Bounds.End.y = std::max(ObjectStart.y, WorldCursor.y);

	if(Round) {
		Bounds.Start = glm::ivec2(Bounds.Start * 2.0f);
		Bounds.Start *= 0.5f;
		Bounds.End = glm::ivec2(Bounds.End * 2.0f + 1.0f);
		Bounds.End *= 0.5f;

		if(Bounds.Start.x == Bounds.End.x)
			Bounds.End.x += 0.5f;
		if(Bounds.Start.y == Bounds.End.y)
			Bounds.End.y += 0.5f;
	}
}

// Get tile range from anchor point to world cursor
void _EditorState::GetTileDrawBounds(glm::ivec2 &Start, glm::ivec2 &End) {
	if(!Map)
		return;

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

// Show the map info screen
void _EditorState::ToggleInfo() {
	if(!InfoElement->Active) {
		CloseWindows();
		InitInfo();
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

// Show the zone select screen
void _EditorState::ToggleZones() {
	if(!ZonesElement->Active) {
		CloseWindows();
		InitZones();
	}
	else {
		CloseWindows();
	}
}

// Show the light select screen
void _EditorState::ToggleLights() {
	if(!LightsElement->Active) {
		CloseWindows();
		InitLights();
	}
	else {
		CloseWindows();
	}
}

// Show the props screen
void _EditorState::ToggleProps() {
	if(!PropsElement->Active) {
		CloseWindows();
		InitProps();
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

// Delete memory used by info screen
void _EditorState::ClearInfo() {
}

// Delete memory used by textures screen
void _EditorState::ClearTextures() {
	for(auto &Child : TexturesElement->Children)
		delete Child;

	TexturesElement->Children.clear();
}

// Delete memory used by zones screen
void _EditorState::ClearZones() {
	for(auto &Child : ZonesElement->Children)
		delete Child;

	ZonesElement->Children.clear();
}

// Delete memory used by lights screen
void _EditorState::ClearLights() {
	for(auto &Child : LightTypesElement->Children)
		delete Child;

	LightTypesElement->Children.clear();
}

// Delete memory used by props screen
void _EditorState::ClearProps() {
	for(auto &Child : PropTypesElement->Children)
		delete Child;

	PropTypesElement->Children.clear();
}

// Delete memory used by events screen
void _EditorState::ClearEvents() {
	for(auto &Child : EventTypesElement->Children)
		delete Child;

	EventTypesElement->Children.clear();
}

// Show map info screen
void _EditorState::InitInfo() {
	ClearInfo();
	InfoElement->SetActive(true);
}

// Init texture select
void _EditorState::InitTextures() {
	ClearTextures();

	glm::vec2 Start = glm::vec2(20, 20);
	glm::vec2 Spacing = glm::vec2(20, 20);
	glm::vec2 Offset(Start);

	const ae::_TextureArray *TextureArray = ae::Assets.TextureArrays["default"];
	uint32_t TextureCount = (uint32_t)TextureArray->Count;
	for(uint32_t i = 0; i < TextureCount; i++) {

		// Add button
		ae::_Element *Button = new ae::_Element();
		Button->Parent = TexturesElement;
		Button->BaseOffset = Offset;
		Button->BaseSize = TextureArray->Size;
		Button->Alignment = ae::LEFT_TOP;
		Button->TextureArray = TextureArray;
		Button->TextureIndex = i;
		Button->Clickable = true;
		TexturesElement->Children.push_back(Button);

		// Update position
		Offset.x += TextureArray->Size.x + Spacing.x;
		if(Offset.x > TexturesElement->BaseSize.x - TextureArray->Size.x) {
			Offset.y += TextureArray->Size.y + Spacing.y;
			Offset.x = Start.x;
		}
	}

	TexturesElement->CalculateBounds();
	TexturesElement->SetActive(true);
}

// Init zones screen
void _EditorState::InitZones() {
	ClearZones();

	glm::vec2 Start = glm::vec2(20, 20);
	glm::vec2 Spacing = glm::vec2(20, 20);
	glm::vec2 Offset(Start);
	uint16_t ZoneIndex = 1;
	for(const auto &Zone : Stats->Zones) {

		// Add button
		ae::_Element *Button = new ae::_Element();
		Button->Parent = ZonesElement;
		Button->BaseOffset = Offset;
		Button->BaseSize = glm::vec2(200, 32);
		Button->Style = ae::Assets.Styles["style_editor_zone" + std::to_string(ZoneIndex % MAP_ZONE_COLORS)];
		Button->HoverStyle = ae::Assets.Styles["style_menu_hover"];
		Button->Alignment = ae::LEFT_TOP;
		Button->Clickable = true;

		ae::_Element *Label = new ae::_Element();
		Label->Parent = Button;
		Label->BaseOffset = glm::vec2(0, 22);
		Label->Alignment = ae::CENTER_BASELINE;
		Label->Text = Zone.second.ID;
		Label->Font = ae::Assets.Fonts["hud_tiny"];
		Button->Children.push_back(Label);

		ZonesElement->Children.push_back(Button);

		// Update position
		Offset.x += Button->BaseSize.x + Spacing.x;
		if(Offset.x > ZonesElement->BaseSize.x - Button->BaseSize.x) {
			Offset.y += Button->BaseSize.x + Spacing.y;
			Offset.x = Start.x;
		}

		ZoneIndex++;
	}

	ZonesElement->CalculateBounds();
	ZonesElement->SetActive(true);
}

// Init lights screen
void _EditorState::InitLights() {
	ClearLights();

	glm::vec2 Start = glm::vec2(20, 20);
	glm::vec2 Spacing = glm::vec2(20, 20);
	glm::vec2 Offset(Start);

	std::string LightsPath = "textures/lights/";
	ae::_Files Files(LightsPath);
	LightTypesElement->BaseSize = LightsElement->BaseSize;
	for(const auto &File : Files.Nodes) {
		const ae::_Texture *Texture = ae::Assets.Textures[LightsPath + File];

		// Add button
		ae::_Element *Button = new ae::_Element();
		Button->Parent = LightTypesElement;
		Button->BaseOffset = Offset;
		Button->BaseSize = glm::vec2(64, 64);
		Button->Alignment = ae::LEFT_TOP;
		Button->Texture = Texture;
		Button->Clickable = true;
		LightTypesElement->Children.push_back(Button);

		// Update position
		Offset.x += Button->BaseSize.x + Spacing.x;
		if(Offset.x > LightsElement->BaseSize.x - Button->BaseSize.x) {
			Offset.y += Button->BaseSize.y + Spacing.y;
			Offset.x = Start.x;
		}
	}

	LightsElement->CalculateBounds();
	LightsElement->SetActive(true);
}

// Init props screen
void _EditorState::InitProps() {
	ClearProps();

	glm::vec2 Start = glm::vec2(20, 20);
	glm::vec2 Spacing = glm::vec2(20, 20);
	glm::vec2 Offset(Start);

	std::string PropsPath = "textures/props/";
	ae::_Files Files(PropsPath);
	PropTypesElement->BaseSize = PropsElement->BaseSize;
	for(const auto &File : Files.Nodes) {
		const ae::_Texture *Texture = ae::Assets.Textures[PropsPath + File];

		// Add button
		ae::_Element *Button = new ae::_Element();
		Button->Parent = PropTypesElement;
		Button->BaseOffset = Offset;
		Button->BaseSize = glm::vec2(64, 64);
		Button->Alignment = ae::LEFT_TOP;
		Button->Texture = Texture;
		Button->Clickable = true;
		PropTypesElement->Children.push_back(Button);

		// Update position
		Offset.x += Button->BaseSize.x + Spacing.x;
		if(Offset.x > PropsElement->BaseSize.x - Button->BaseSize.x) {
			Offset.y += Button->BaseSize.y + Spacing.y;
			Offset.x = Start.x;
		}
	}

	PropsElement->CalculateBounds();
	PropsElement->SetActive(true);
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
	EventsElement->BaseSize.y = Start.y + (Spacing.y + Size.y) * (Count / 4) + 130;
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

	EventMessageElement->Text = "";
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
	bool WasOpen =
		InfoElement->Active |
		TexturesElement->Active |
		ZonesElement->Active |
		EventsElement->Active |
		NewMapElement->Active |
		ResizeMapElement->Active |
		SaveMapElement->Active |
		LoadMapElement->Active |
		LightsElement->Active |
		PropsElement->Active;

	InfoElement->SetActive(false);
	TexturesElement->SetActive(false);
	ZonesElement->SetActive(false);
	LightsElement->SetActive(false);
	PropsElement->SetActive(false);
	EventsElement->SetActive(false);
	NewMapElement->SetActive(false);
	ResizeMapElement->SetActive(false);
	SaveMapElement->SetActive(false);
	LoadMapElement->SetActive(false);
	ae::FocusedElement = nullptr;

	return WasOpen;
}

// Switch editor modes
void _EditorState::SwitchMode(EditorModeType Value) {
	CloseWindows();

	Mode = Value;
	switch(Mode) {
		case EditorModeType::TILES:
			Filter = 0;
			Filter |= MAP_RENDER_TEXTURE;
			Filter |= MAP_RENDER_WALL;
		break;
		case EditorModeType::LIGHTS:
		case EditorModeType::PROPS:
			Filter = 0;
		break;
	}

	DrawCopyBounds = false;
	DrawingObject = false;
	DrawingSelect = false;
	MovingObjects = false;
	ResizingObject = false;
	CopiedObjects = false;
	SelectedObjects.clear();
}

// Start object drawing
void _EditorState::StartDrawingObject() {
	DrawingObject = true;
	ObjectStart = glm::ivec2(WorldCursor * 2.0f);
	ObjectStart *= 0.5f;
}

// Return true if the position is within one of the selected objects
bool _EditorState::TouchingSelectedObjects(const glm::vec2 &Position) {

	// Test objects
	for(auto &Iterator : SelectedObjects) {
		const _Object *Object = Iterator.first;
		if(Object->CheckAABB(glm::vec4(Position, Position)))
			return true;
	}

	return false;
}

// Get offset while moving objects
void _EditorState::GetCursorOffset(glm::vec2 &Offset, const glm::vec2 &Start) {
	Offset = glm::ivec2((WorldCursor - Start) * 2.0f);
	Offset *= 0.5f;
}

// Delete all the selected objects
void _EditorState::DeleteSelectedObjects() {
	if(!Map)
		return;

	// Flag objects
	for(auto &Iterator : SelectedObjects)
		Iterator.first->Deleted = true;

	// Delete from map
	for(auto Iterator = Map->StaticObjects.begin(); Iterator != Map->StaticObjects.end(); ) {
		if((*Iterator)->Deleted) {
			delete *Iterator;
			Iterator = Map->StaticObjects.erase(Iterator);
		}
		else
			++Iterator;
	}

	SelectedObjects.clear();
	CopiedObjects = false;
}

// Set object size while drawing or resizing
void _EditorState::SetObjectSize(_Object *Object, bool AsRectangle) {
	if(AsRectangle) {
		ae::_Bounds Bounds;
		GetDrawBounds(Bounds, true);
		Object->Shape.HalfSize = (Bounds.End - Bounds.Start) * 0.5f;
		Object->Position = Bounds.Start + Object->Shape.HalfSize;
	}
	else {
		Object->Shape.HalfSize.x = GetObjectRadius();
		Object->Position = ObjectStart;
	}
}

// Copy objects
void _EditorState::CopyObjects() {
	if(!SelectedObjects.size())
		return;

	CopyPosition = WorldCursor;
	CopiedObjects = true;
}

// Paste selected objects
void _EditorState::PasteObjects() {
	if(!CopiedObjects)
		return;

	glm::vec2 Offset;
	GetCursorOffset(Offset, CopyPosition);
	for(const auto &Iterator : SelectedObjects) {
		_Object *SourceObject = Iterator.first;

		// Create new object
		_Object *Object = new _Object();
		Object->Scripting = Scripting;
		if(SourceObject->Prop) {
			Object->Prop = new _Prop(Object);
			Object->Prop->Texture = SourceObject->Prop->Texture;
			Object->Prop->Color = SourceObject->Prop->Color;
		}
		if(SourceObject->Light->Texture) {
			Object->Light->Texture = SourceObject->Light->Texture;
			Object->Light->Color = SourceObject->Light->Color;
			Object->Light->Script = SourceObject->Light->Script;
		}
		Object->Shape = SourceObject->Shape;
		Object->Position = SourceObject->Position + Offset;
		Map->StaticObjects.push_back(Object);
	}
}

// Return the only selected object, null otherwise
_Object *_EditorState::GetSingleSelectedObject() {
	if(SelectedObjects.size() != 1)
		return nullptr;

	return SelectedObjects.begin()->first;
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
	Map->Scripting = Scripting;
	Map->Size = Size;
	Map->InitVertices();
	Map->AllocateMap();
	Map->BuildLayers(glm::ivec4(0, 0, Map->Size.x, Map->Size.y), ShowTransitions);
	AllocateCopy();
	FilePath = NewMapFilenameTextBox->Text;
	SetInfoUI();

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
	Map->BuildLayers(glm::ivec4(0, 0, Map->Size.x, Map->Size.y), ShowTransitions);
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
	NewMap->Scripting = Scripting;
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
		glm::vec2 Position(Map->Size.x/2, Map->Size.y/2);
		if(!Map->FindEvent(_Event(EventType::SPAWN, ""), Position)) {
			//TODO fix
			//if(!Map->FindEvent(_Event(EventType::MAPCHANGE, OldMapID), Position))
			//	Map->FindEvent(_Event(EventType::MAPENTRANCE, OldMapID), Position);
		}
		Camera->ForcePosition(glm::vec3(Position, CAMERA_DISTANCE));
		SetInfoUI();
	}

	CloseWindows();
}

// Set brush mode
void _EditorState::SwitchBrushModes(int Key) {
	switch(Key) {
		case 1:
			SwitchMode(EditorModeType::TILES);
			Filter = 0;
			Filter |= MAP_RENDER_TEXTURE;
			Filter |= MAP_RENDER_WALL;
		break;
		case 2:
			SwitchMode(EditorModeType::TILES);
			Filter = 0;
			Filter |= MAP_RENDER_ZONE;
		break;
		case 3:
			SwitchMode(EditorModeType::TILES);
			Filter = 0;
			Filter |= MAP_RENDER_PVP;
		break;
		case 4:
			SwitchMode(EditorModeType::TILES);
			Filter = 0;
			Filter |= MAP_RENDER_EVENT_TYPE;
			Filter |= MAP_RENDER_EVENT_DATA;
		break;
	}
}

// Get radius of light while drawing
float _EditorState::GetObjectRadius() {

	return std::max(0.5f, 0.5f * int(2.0f * glm::length(ObjectStart - WorldCursor)));
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

// Set light ui state
void _EditorState::SetLightUI(const glm::vec4 &Color, const std::string &Script) {
	for(size_t i = 0; i < LightSliderData.size(); i++) {
		ae::Assets.Elements["button_editor_" + LightSliderData[i].first]->SetOffsetPercent(glm::vec2(Color[(int)i], 0));
		ae::_Element *Value = ae::Assets.Elements["label_editor_" + LightSliderData[i].first + "_value"];
		std::stringstream Buffer;
		Buffer << std::fixed << std::setprecision(2) << Color[(int)i];
		Value->Text = Buffer.str();
	}

	LightScriptElement->Text = Script;
}

// Set UI state of info screen from map
void _EditorState::SetInfoUI() {
	if(!Map)
		return;

	MapInfoData.clear();
	MapInfoData = {
		{ "info_ambient_r", &Map->AmbientLight.r },
		{ "info_ambient_g", &Map->AmbientLight.g },
		{ "info_ambient_b", &Map->AmbientLight.b },
	};

	// Update slider values
	for(size_t i = 0; i < MapInfoData.size(); i++) {
		ae::Assets.Elements["button_editor_" + MapInfoData[i].first]->SetOffsetPercent(glm::vec2(*(MapInfoData[i].second), 0));
		ae::_Element *Value = ae::Assets.Elements["label_editor_" + MapInfoData[i].first + "_value"];
		std::stringstream Buffer;
		Buffer << std::fixed << std::setprecision(2) << *MapInfoData[i].second;
		Value->Text = Buffer.str();
	}

	// Set attributes
	ae::_Element *Check = ae::Assets.Elements["label_editor_info_outside_check"];
	Check->Text = Map->IsOutside ? "X" : "";
	InfoMusicElement->Text = Map->Music;
}

// Update slider boxes
void _EditorState::UpdateSliders(std::vector<std::pair<std::string, float *> > &Data, bool Force) {

	// Loop through sliders
	bool Changed = false;
	for(size_t i = 0; i < Data.size(); i++) {
		ae::_Element *Slider = ae::Assets.Elements["element_editor_" + Data[i].first];
		ae::_Element *Button = ae::Assets.Elements["button_editor_" + Data[i].first];

		// Handle clicking inside slider elements
		if(!Button->PressedElement && Slider->PressedElement) {
			Button->PressedOffset = Button->Size / 2.0f;
			Button->PressedElement = Button;
		}

		// Update value
		if(Button->PressedElement || Force) {
			ae::_Element *Value = ae::Assets.Elements["label_editor_" + Data[i].first + "_value"];

			// Convert slider percent to number
			std::stringstream Buffer;
			Buffer << std::fixed << std::setprecision(2) << Button->GetOffsetPercent().x;
			Value->Text = Buffer.str();
			Buffer.str("");

			// Update data source
			*Data[i].second = std::stof(Value->Text);
			Changed = true;
		}
	}

	// Update selected objects
	if(Changed) {
		if(LightsElement->Active) {
			for(const auto &Iterator : SelectedObjects) {
				Iterator.first->Light->Color = LightBrush->Color;
			}
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
			if(Filter & MAP_RENDER_TEXTURE) {
				Tile.BaseTextureIndex = TileBrush->BaseTextureIndex;
				Tile.Hierarchy = ae::Assets.TileMaps["default"]->Index.at(Tile.BaseTextureIndex)->Hierarchy;
			}
			if(Filter & MAP_RENDER_WALL)
				Tile.Wall = TileBrush->Wall;
			if(Filter & MAP_RENDER_ZONE)
				Tile.ZoneID = TileBrush->ZoneID;
			if(Filter & MAP_RENDER_PVP)
				Tile.PVP = TileBrush->PVP;
			if(Filter & MAP_RENDER_EVENT_TYPE)
				Tile.Event.Type = TileBrush->Event.Type;
			if(Filter & MAP_RENDER_EVENT_DATA)
				Tile.Event.Data = TileBrush->Event.Data;

			// Set new tile
			Map->SetTile(TilePosition, &Tile);
		}
	}

	// Rebuild affected layers
	Map->BuildLayers(glm::ivec4(glm::ivec2(Position) - glm::ivec2(BrushRadius + 1), glm::ivec2(Position) + glm::ivec2(BrushRadius + 1)), ShowTransitions);
}

// Deletes the map
void _EditorState::CloseMap() {
	CloseCopy();

	delete Map;
	Map = nullptr;
}
