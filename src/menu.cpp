/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
#include <menu.h>
#include <states/play.h>
#include <states/editor.h>
#include <network/clientnetwork.h>
#include <ui/element.h>
#include <ui/label.h>
#include <ui/button.h>
#include <ui/image.h>
#include <ui/style.h>
#include <ui/textbox.h>
#include <objects/object.h>
#include <constants.h>
#include <input.h>
#include <actions.h>
#include <actiontype.h>
#include <graphics.h>
#include <assets.h>
#include <config.h>
#include <hud.h>
#include <framework.h>
#include <buffer.h>
#include <audio.h>
#include <stats.h>
#include <packet.h>
#include <utils.h>
#include <audio.h>
#include <version.h>
#include <sstream>
#include <iomanip>
#include <SDL_keyboard.h>

_Menu Menu;

const std::string InputBoxPrefix = "button_options_input_";
const std::string CharacterButtonPrefix = "button_characters_slot";
const std::string NewCharacterPortraitPrefix = "button_newcharacter_portrait";
const std::string NewCharacterBuildPrefix = "button_newcharacter_build";

// Constructor
_Menu::_Menu() {
	State = STATE_NONE;
	PreSelectedSlot = NOSLOT;
	CurrentLayout = nullptr;
	CharactersState = CHARACTERS_NONE;
	PreviousClickTimer = 0.0;
	CharacterSlots.resize(ACCOUNT_MAX_CHARACTER_SLOTS);
	HardcoreServer = false;

	ResetInGameState();
}

// Change the current layout
void _Menu::ChangeLayout(const std::string &ElementIdentifier) {
	if(CurrentLayout) {
		CurrentLayout->SetVisible(false);

		if(CurrentLayout == Assets.Elements["element_menu_options"])
			Config.Save();
	}

	CurrentLayout = Assets.Elements[ElementIdentifier];
	CurrentLayout->SetVisible(true);
}

// Initialize
void _Menu::InitTitle(bool Disconnect) {
	if(Disconnect)
		PlayState.Network->Disconnect(true);

	std::string BuildNumber = "";
	if(GAME_BUILD)
		BuildNumber = "r" + std::to_string(GAME_BUILD);

	Assets.Labels["label_menu_title_version"]->Text = std::string(GAME_VERSION) + BuildNumber;
	Assets.Labels["label_menu_title_message"]->Text = "";

	ChangeLayout("element_menu_title");

	Audio.PlayMusic(Assets.Music["intro.ogg"]);

	ResetInGameState();
	State = STATE_TITLE;
}

// Init character select screen
void _Menu::InitCharacters() {
	ChangeLayout("element_menu_characters");

	// Set label
	_Label *HardcoreLabel = Assets.Labels["label_menu_characters_hardcore"];
	HardcoreLabel->SetVisible(false);
	if(HardcoreServer)
		HardcoreLabel->SetVisible(true);

	Audio.PlayMusic(Assets.Music["intro.ogg"]);

	ResetInGameState();
	CharactersState = CHARACTERS_NONE;
	State = STATE_CHARACTERS;
}

// Init new player popup
void _Menu::InitNewCharacter() {
	_Button *CreateButton = Assets.Buttons["button_newcharacter_create"];
	_Button *CreateHardcoreButton = Assets.Buttons["button_newcharacter_createhardcore"];
	CreateButton->SetEnabled(false);
	CreateHardcoreButton->SetEnabled(false);

	_TextBox *Name = Assets.TextBoxes["textbox_newcharacter_name"];
	Name->SetText("");

	_Label *Label = Assets.Labels["label_menu_newcharacter_name"];
	Label->Text = "Name";
	Label->Color = COLOR_WHITE;

	LoadPortraitButtons();
	LoadBuildButtons();

	FocusedElement = Name;
	Name->ResetCursor();

	Assets.Elements["element_menu_character_slots"]->SetClickable(false);

	CurrentLayout = Assets.Elements["element_menu_new"];
	CurrentLayout->SetVisible(true);

	CharactersState = CHARACTERS_CREATE;
}

// In-game menu
void _Menu::InitInGame() {
	ChangeLayout("element_menu_ingame");
	if(!ShowRespawn)
		Assets.Buttons["button_ingame_respawn"]->SetVisible(false);

	if(!ShowExitWarning)
		Assets.Labels["label_menu_ingame_exitwarning"]->SetVisible(false);

	PlayState.SendStatus(_Object::STATUS_PAUSE);
	State = STATE_INGAME;
	FromInGame = true;
}

// Return to play
void _Menu::InitPlay() {
	if(CurrentLayout)
		CurrentLayout->SetVisible(false);
	CurrentLayout = nullptr;

	PlayState.SendStatus(_Object::STATUS_NONE);
	State = STATE_NONE;
}

// Open options
void _Menu::InitOptions() {
	ChangeLayout("element_menu_options");

	// Assign values from config
	UpdateOptions();

	State = STATE_OPTIONS;
}

// Show the confirm screen
void _Menu::ConfirmAction() {
	CurrentLayout = Assets.Elements["element_menu_confirm"];
	CurrentLayout->SetVisible(true);
}

// Exit game and return to character select
void _Menu::ExitGame() {

	// Notify server
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_EXIT);
	PlayState.Network->SendPacket(Packet);

	// Shutdown game
	PlayState.StopGame();

	// Show character select
	InitCharacters();
}

// Init connect screen
void _Menu::InitConnect(bool UseConfig, bool ConnectNow) {
	PlayState.Network->Disconnect();

	ChangeLayout("element_menu_connect");

	_TextBox *Host = Assets.TextBoxes["textbox_connect_host"];
	if(UseConfig)
		Host->SetText(Config.LastHost);

	_TextBox *Port = Assets.TextBoxes["textbox_connect_port"];
	if(UseConfig)
		Port->SetText(Config.LastPort);

	_Label *Label = Assets.Labels["label_menu_connect_message"];
	Label->Color = COLOR_WHITE;
	Label->Text = "";

	_Button *Button = Assets.Buttons["button_connect_connect"];
	((_Label *)Button->Children.front())->Text = "Connect";

	// Set focus
	FocusedElement = Host;
	Host->ResetCursor();

	State = STATE_CONNECT;
	if(ConnectNow)
		ConnectToHost();
}

// Init account info screen
void _Menu::InitAccount() {
	ChangeLayout("element_menu_account");

	_TextBox *Username = Assets.TextBoxes["textbox_account_username"];
	Username->SetText(DefaultUsername);

	_TextBox *Password = Assets.TextBoxes["textbox_account_password"];
	Password->SetText(DefaultPassword);
	Password->Password = true;

	_Label *Label = Assets.Labels["label_menu_account_message"];
	Label->Color = COLOR_WHITE;
	Label->Text = "";

	_Button *Button = Assets.Buttons["button_account_login"];
	Button->SetEnabled(true);

	// Set focus
	FocusedElement = Username;
	Username->ResetCursor();

	State = STATE_ACCOUNT;
}

// Get the selected portrait id
uint32_t _Menu::GetSelectedIconID(_Element *ParentElement) {

	// Check for selected portrait
	for(auto &Element : ParentElement->Children) {
		_Button *Button = (_Button *)Element;
		if(Button->Checked)
			return (uint32_t)(intptr_t)Button->UserData;
	}

	return 0;
}

// Get the selected character slot
size_t _Menu::GetSelectedCharacter() {
	size_t Index = 0;

	// Check for selected character
	_Element *CharactersElement = Assets.Elements["element_menu_character_slots"];
	for(auto &Element : CharactersElement->Children) {
		if(Element->Identifier == CharacterButtonPrefix) {
			_Button *Button = (_Button *)Element;
			if(Button->Checked)
				return Index;

			Index++;
		}
	}

	return CharacterSlots.size();
}

// Create character
void _Menu::CreateCharacter(bool Hardcore) {

	// Check length
	_TextBox *Name = Assets.TextBoxes["textbox_newcharacter_name"];
	if(Name->Text.length() == 0)
		return;

	// Get portrait id
	uint32_t PortraitID = GetSelectedIconID(Assets.Elements["element_menu_new_portraits"]);
	if(PortraitID == 0)
		return;

	// Get build id
	uint32_t BuildID = GetSelectedIconID(Assets.Elements["element_menu_new_builds"]);
	if(BuildID == 0)
		return;

	// Get slot
	size_t SelectedSlot = GetSelectedCharacter();
	if(SelectedSlot >= CharacterSlots.size())
		return;

	// Save selection
	PreSelectedSlot = SelectedSlot;

	// Send information
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::CREATECHARACTER_INFO);
	Packet.WriteBit(Hardcore);
	Packet.WriteString(Name->Text.c_str());
	Packet.Write<uint32_t>(PortraitID);
	Packet.Write<uint32_t>(BuildID);
	Packet.Write<uint8_t>((uint8_t)SelectedSlot);
	PlayState.Network->SendPacket(Packet);
}

void _Menu::ConnectToHost() {
	_TextBox *Host = Assets.TextBoxes["textbox_connect_host"];
	_TextBox *Port = Assets.TextBoxes["textbox_connect_port"];
	if(Host->Text.length() == 0) {
		FocusedElement = Host;
		return;
	}

	if(Port->Text.length() == 0) {
		FocusedElement = Port;
		return;
	}

	PlayState.HostAddress = Host->Text;
	PlayState.ConnectPort = ToNumber<uint16_t>(Port->Text);
	PlayState.Connect(false);

	_Label *Label = Assets.Labels["label_menu_connect_message"];
	Label->Color = COLOR_WHITE;
	Label->Text = "Connecting...";

	_Button *Button = Assets.Buttons["button_connect_connect"];
	((_Label *)Button->Children.front())->Text = "Cancel";

	FocusedElement = nullptr;
}

// Send character to play
void _Menu::PlayCharacter(size_t Slot) {
	if(!CharacterSlots[Slot].CanPlay)
		return;

	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::CHARACTERS_PLAY);
	Packet.Write<uint8_t>((uint8_t)Slot);
	PlayState.Network->SendPacket(Packet);

	CharactersState = CHARACTERS_PLAYSENT;

	Audio.StopMusic();
	PlayState.HUD->Reset();
}

// Send login info
void _Menu::SendAccountInfo(bool CreateAccount) {
	_TextBox *Username = Assets.TextBoxes["textbox_account_username"];
	_TextBox *Password = Assets.TextBoxes["textbox_account_password"];
	_Label *Label = Assets.Labels["label_menu_account_message"];

	// Check username
	if(Username->Text.length() == 0) {
		FocusedElement = Username;
		Label->Color = COLOR_RED;
		Label->Text = "Enter a username";

		return;
	}

	// Check password
	if(Password->Text.length() == 0) {
		FocusedElement = Password;
		Label->Color = COLOR_RED;
		Label->Text = "Enter a password";

		return;
	}

	// Update UI
	Label->Color = COLOR_WHITE;
	Label->Text = "Logging in...";

	_Button *Button = Assets.Buttons["button_account_login"];
	Button->SetEnabled(false);

	// Send information
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::ACCOUNT_LOGININFO);
	Packet.WriteBit(CreateAccount);
	Packet.WriteString(Username->Text.c_str());
	Packet.WriteString(Password->Text.c_str());
	Packet.Write<uint64_t>(0);
	PlayState.Network->SendPacket(Packet);

	FocusedElement = nullptr;
}

// Request character list from server
void _Menu::RequestCharacterList() {

	// Request character list
	_Buffer Packet;
	Packet.Write<PacketType>(PacketType::CHARACTERS_REQUEST);
	PlayState.Network->SendPacket(Packet);
}

// Load character slots
void _Menu::LoadCharacterSlots() {
	glm::vec2 Offset(0, 0);
	glm::vec2 Size(128, 128);
	glm::vec2 Padding(42, 82);

	// Clear old children
	ClearCharacterSlots();

	// Iterate over slots
	_Element *CharacterSlotsElement = Assets.Elements["element_menu_character_slots"];
	for(size_t i = 0; i < ACCOUNT_MAX_CHARACTER_SLOTS; i++) {

		// Add button
		_Button *Button = new _Button();
		Button->Identifier = CharacterButtonPrefix;
		Button->Parent = CharacterSlotsElement;
		Button->Offset = Offset;
		Button->Size = Size;
		Button->Style = Assets.Styles["style_menu_button"];
		Button->DisabledStyle = Assets.Styles["style_menu_button_disabled"];
		Button->HoverStyle = Assets.Styles["style_menu_button_hover"];
		Button->UserData = (void *)(intptr_t)i;
		Button->UserCreated = true;
		Button->Checked = i == PreSelectedSlot ? true : false;
		CharacterSlotsElement->Children.push_back(Button);

		// Add image for portrait
		_Image *Image = new _Image();
		Image->Parent = Button;
		Image->Alignment = CENTER_MIDDLE;
		Image->Offset = glm::vec2(0, 0);
		Image->UserCreated = true;
		Button->Children.push_back(Image);

		// Add name label
		_Label *NameLabel = new _Label();
		NameLabel->Parent = Button;
		NameLabel->Offset = glm::vec2(0, 150);
		NameLabel->Alignment = CENTER_BASELINE;
		NameLabel->Font = Assets.Fonts["hud_medium"];
		NameLabel->UserCreated = true;
		Button->Children.push_back(NameLabel);

		// Add level label
		_Label *LevelLabel = new _Label();
		LevelLabel->Parent = Button;
		LevelLabel->Offset = glm::vec2(0, 170);
		LevelLabel->Alignment = CENTER_BASELINE;
		LevelLabel->Font = Assets.Fonts["hud_small"];
		LevelLabel->UserCreated = true;
		Button->Children.push_back(LevelLabel);

		// Add hardcore label
		_Label *HardcoreLabel = new _Label();
		HardcoreLabel->Parent = Button;
		HardcoreLabel->Offset = glm::vec2(0, 187);
		HardcoreLabel->Alignment = CENTER_BASELINE;
		HardcoreLabel->Color = COLOR_RED;
		HardcoreLabel->Font = Assets.Fonts["hud_small"];
		HardcoreLabel->UserCreated = true;
		Button->Children.push_back(HardcoreLabel);

		// Reset state
		CharacterSlots[i].Button = Button;
		CharacterSlots[i].Name = NameLabel;
		CharacterSlots[i].Level = LevelLabel;
		CharacterSlots[i].Hardcore = HardcoreLabel;
		CharacterSlots[i].Image = Image;
		CharacterSlots[i].Name->Text = "Empty Slot";
		CharacterSlots[i].Level->Text = "";
		CharacterSlots[i].Hardcore->Text = "";
		CharacterSlots[i].Image->Texture = nullptr;
		CharacterSlots[i].Image->Clickable = false;
		CharacterSlots[i].Used = false;
		CharacterSlots[i].CanPlay = true;

		// Update position
		Offset.x += Size.x + Padding.x;
		if(Offset.x > CharacterSlotsElement->Size.x - Size.x) {
			Offset.y += Size.y + Padding.y;
			Offset.x = 0;
		}
	}

	CharacterSlotsElement->CalculateBounds();
	CharacterSlotsElement->SetVisible(true);
}

// Clear character slots
void _Menu::ClearCharacterSlots() {
	std::list<_Element *> &Children = Assets.Elements["element_menu_character_slots"]->Children;
	for(auto &Child : Children) {
		if(Child->UserCreated) {

			// Delete children
			for(auto &SubChild : Child->Children) {
				if(SubChild->UserCreated)
					delete SubChild;
			}

			delete Child;
		}
	}

	Children.clear();
}

// Load portraits
void _Menu::LoadPortraitButtons() {

	// Clear old children
	_Element *PortraitsElement = Assets.Elements["element_menu_new_portraits"];
	ClearPortraits();

	glm::vec2 Offset(10, 50);
	size_t i = 0;

	// Load portraits
	std::list<_Portrait> Portraits;
	PlayState.Stats->GetPortraits(Portraits);

	// Iterate over portraits
	for(const auto &Portrait : Portraits) {
		if(!Portrait.Texture)
			throw std::runtime_error("Cannot find texture for portrait id " + std::to_string(Portrait.ID));

		// Create style
		_Style *Style = new _Style();
		Style->TextureColor = COLOR_WHITE;
		Style->Program = Assets.Programs["ortho_pos_uv"];
		Style->Texture = Portrait.Texture;
		Style->UserCreated = true;

		// Add button
		_Button *Button = new _Button();
		Button->Identifier = NewCharacterPortraitPrefix;
		Button->Parent = PortraitsElement;
		Button->Offset = Offset;
		Button->Size = Portrait.Texture->Size;
		Button->Alignment = LEFT_TOP;
		Button->Style = Style;
		Button->HoverStyle = Assets.Styles["style_menu_portrait_hover"];
		Button->UserData = (void *)(intptr_t)Portrait.ID;
		Button->UserCreated = true;
		PortraitsElement->Children.push_back(Button);

		// Update position
		Offset.x += Portrait.Texture->Size.x + 10;
		if(Offset.x > PortraitsElement->Size.x - Portrait.Texture->Size.x - 10) {
			Offset.y += Portrait.Texture->Size.y + 10;
			Offset.x = 10;
		}

		i++;
	}

	PortraitsElement->CalculateBounds();
}

// Clear memory used by portraits
void _Menu::ClearPortraits() {

	std::list<_Element *> &Children = Assets.Elements["element_menu_new_portraits"]->Children;
	for(auto &Child : Children) {
		if(Child->UserCreated) {
			delete Child->Style;
			delete Child;
		}
	}

	Children.clear();
}

// Load builds
void _Menu::LoadBuildButtons() {

	// Clear old children
	_Element *BuildsElement = Assets.Elements["element_menu_new_builds"];
	ClearBuilds();

	glm::vec2 Offset(10, 50);
	size_t i = 0;

	// Load builds
	std::list<_Build> Builds;
	PlayState.Stats->GetStartingBuilds(Builds);

	// Iterate over builds
	for(const auto &Build : Builds) {
		if(!Build.Texture)
			throw std::runtime_error("Cannot find texture for build id " + std::to_string(Build.ID));

		// Create style
		_Style *Style = new _Style();
		Style->TextureColor = COLOR_WHITE;
		Style->Program = Assets.Programs["ortho_pos_uv"];
		Style->Texture = Build.Texture;
		Style->UserCreated = true;

		// Add button
		_Button *Button = new _Button();
		Button->Identifier = NewCharacterBuildPrefix;
		Button->Parent = BuildsElement;
		Button->Offset = Offset;
		Button->Size = Build.Texture->Size;
		Button->Alignment = LEFT_TOP;
		Button->Style = Style;
		Button->HoverStyle = Assets.Styles["style_menu_portrait_hover"];
		Button->UserData = (void *)(intptr_t)Build.ID;
		Button->UserCreated = true;
		BuildsElement->Children.push_back(Button);

		// Add label
		_Label *Label = new _Label();
		Label->Font = Assets.Fonts["hud_small"];
		Label->Text = Build.Name;
		Label->Color = COLOR_WHITE;
		Label->Parent = Button;
		Label->Offset = glm::vec2(0, 80);
		Label->Alignment = CENTER_BASELINE;
		Label->UserCreated = true;
		Label->Clickable = false;
		Button->Children.push_back(Label);

		// Update position
		Offset.x += Build.Texture->Size.x + 10;
		if(Offset.x > BuildsElement->Size.x - Build.Texture->Size.x - 10) {
			Offset.y += Build.Texture->Size.y + 10;
			Offset.x = 10;
		}

		i++;
	}

	BuildsElement->CalculateBounds();
}

// Clear memory used by portraits
void _Menu::ClearBuilds() {

	std::list<_Element *> &Children = Assets.Elements["element_menu_new_builds"]->Children;
	for(auto &Child : Children) {
		if(Child->UserCreated) {
			delete Child->Style;

			// Delete labels
			for(auto &LabelChild : Child->Children) {
				if(LabelChild->UserCreated)
					delete LabelChild;
			}

			delete Child;
		}
	}

	Children.clear();
}

// Update option ui values
void _Menu::UpdateOptions() {
	std::stringstream Buffer;
	Buffer << std::fixed << std::setprecision(2);

	// Set fullscreen
	_Label *FullscreenCheck = Assets.Labels["label_menu_options_fullscreen_check"];
	FullscreenCheck->Text = Config.Fullscreen ? "X" : "";

	// Set sound volume
	_TextBox *SoundVolume = Assets.TextBoxes["textbox_options_soundvolume"];
	Buffer << Config.SoundVolume;
	SoundVolume->Text = Buffer.str();
	Buffer.str("");

	// Set music volume
	_TextBox *MusicVolume = Assets.TextBoxes["textbox_options_musicvolume"];
	Buffer << Config.MusicVolume;
	MusicVolume->Text = Buffer.str();
	Buffer.str("");
}

// Update config and audio volumes from option textboxes
void _Menu::UpdateVolume() {
	_TextBox *SoundVolume = Assets.TextBoxes["textbox_options_soundvolume"];
	_TextBox *MusicVolume = Assets.TextBoxes["textbox_options_musicvolume"];
	Config.SoundVolume = ToNumber<float>(SoundVolume->Text);
	Config.MusicVolume = ToNumber<float>(MusicVolume->Text);
	Audio.SetSoundVolume(Config.SoundVolume);
	Audio.SetMusicVolume(Config.MusicVolume);
}

// Reset variables used for in-game menu
void _Menu::ResetInGameState() {
	FromInGame = false;
	ShowExitWarning = false;
	ShowRespawn = false;
}

// Check new character screen for portrait and name
void _Menu::ValidateCreateCharacter() {
	bool NameValid = false;
	uint32_t PortraitID = GetSelectedIconID(Assets.Elements["element_menu_new_portraits"]);
	uint32_t BuildID = GetSelectedIconID(Assets.Elements["element_menu_new_builds"]);

	// Check name length
	_Button *CreateButton = Assets.Buttons["button_newcharacter_create"];
	_Button *CreateHardcoreButton = Assets.Buttons["button_newcharacter_createhardcore"];
	_TextBox *Name = Assets.TextBoxes["textbox_newcharacter_name"];
	if(Name->Text.length() > 0)
		NameValid = true;
	else
		FocusedElement = Name;

	// Enable button
	if(PortraitID != 0 && BuildID != 0 && NameValid) {
		CreateButton->SetEnabled(true);
		CreateHardcoreButton->SetEnabled(true);
	}
	else {
		CreateButton->SetEnabled(false);
		CreateHardcoreButton->SetEnabled(false);
	}
}

// Update ui button states
void _Menu::UpdateCharacterButtons() {
	_Button *DeleteButton = Assets.Buttons["button_characters_delete"];
	_Button *PlayButton = Assets.Buttons["button_characters_play"];
	DeleteButton->SetEnabled(false);
	PlayButton->SetEnabled(false);

	size_t SelectedSlot = GetSelectedCharacter();
	if(SelectedSlot < CharacterSlots.size() && CharacterSlots[SelectedSlot].Used) {
		DeleteButton->SetEnabled(true);
		if(CharacterSlots[SelectedSlot].CanPlay)
			PlayButton->SetEnabled(true);
	}
}

// Shutdown
void _Menu::Close() {
	ClearCharacterSlots();
	ClearPortraits();
	ClearBuilds();
}

// Handle action, return true to stop handling same input
bool _Menu::HandleAction(int InputType, size_t Action, int Value) {
	if(State == STATE_NONE)
		return true;

	switch(State) {
		case STATE_TITLE: {
			switch(Action) {
				case Action::MENU_GO:
					PlayState.Connect(true);
				break;
				case Action::MENU_BACK:
					Framework.Done = true;
				break;
			}
		} break;
		case STATE_CHARACTERS: {
			if(CharactersState == CHARACTERS_NONE) {
				switch(Action) {
					case Action::MENU_GO: {
						size_t SelectedSlot = GetSelectedCharacter();
						if(SelectedSlot >= CharacterSlots.size())
							SelectedSlot = 0;

						if(CharacterSlots[SelectedSlot].Used) {
							PlayCharacter(SelectedSlot);
						}
					} break;
					case Action::MENU_BACK:
						PlayState.Network->Disconnect();
					break;
				}
			}
			else if(CharactersState == CHARACTERS_CREATE) {
				ValidateCreateCharacter();

				switch(Action) {
					case Action::MENU_GO:
						CreateCharacter();
					break;
					case Action::MENU_BACK:
						RequestCharacterList();
					break;
				}
			}
			else if(CharactersState == CHARACTERS_DELETE) {
				switch(Action) {
					case Action::MENU_BACK:
						RequestCharacterList();
					break;
				}
			}
		} break;
		case STATE_CONNECT: {
			switch(Action) {
				case Action::MENU_GO:
					ConnectToHost();
				break;
				case Action::MENU_BACK:
					InitTitle(true);
				break;
				case Action::MENU_DOWN:
					FocusNextElement();
				break;
			}
		} break;
		case STATE_ACCOUNT: {
			switch(Action) {
				case Action::MENU_GO:
					SendAccountInfo();
				break;
				case Action::MENU_BACK:
					InitConnect(true);
				break;
				case Action::MENU_DOWN:
					FocusNextElement();
				break;
			}
		} break;
		case STATE_OPTIONS: {
			UpdateVolume();

			switch(Action) {
				case Action::MENU_BACK: {
					if(FromInGame)
						InitInGame();
					else
						InitTitle(true);
				} break;
			}
		} break;
		case STATE_INGAME:
			switch(Action) {
				case Action::MENU_BACK:
				case Action::MENU_PAUSE:
					Menu.InitPlay();
				break;
			}
		break;
		default:
		break;
	}

	return false;
}

// Handle key event
void _Menu::HandleKey(const _KeyEvent &KeyEvent) {
	if(State == STATE_NONE)
		return;

	switch(State) {
		case STATE_CHARACTERS: {
			if(CharactersState == CHARACTERS_CREATE) {
				ValidateCreateCharacter();
			}
		} break;
		default:
		break;
	}
}

// Handle mouse event
void _Menu::HandleMouseButton(const _MouseEvent &MouseEvent) {
	if(State == STATE_NONE)
		return;

	if(!CurrentLayout)
		return;

	// Get clicked element
	_Element *Clicked = CurrentLayout->GetClickedElement();
	if(Clicked) {
		bool DoubleClick = false;
		if(PreviousClick == Clicked && PreviousClickTimer < MENU_DOUBLECLICK_TIME) {
			PreviousClick = nullptr;
			DoubleClick = true;
		}
		else
			PreviousClick = Clicked;
		PreviousClickTimer = 0.0;

		switch(State) {
			case STATE_TITLE: {
				if(Clicked->Identifier == "button_title_play") {
					PlayState.Connect(true);
					PlayClickSound();
				}
				else if(Clicked->Identifier == "button_title_joinserver") {
					InitConnect(true);
					PlayClickSound();
				}
				else if(Clicked->Identifier == "button_title_options") {
					InitOptions();
					PlayClickSound();
				}
				else if(Clicked->Identifier == "button_title_exit") {
					Framework.Done = true;
				}
			} break;
			case STATE_CHARACTERS: {
				if(CharactersState == CHARACTERS_NONE) {

					if(Clicked->Identifier == "button_characters_delete") {
						CharactersState = CHARACTERS_DELETE;
						Assets.Elements["element_menu_character_slots"]->SetClickable(false);
						ConfirmAction();
						PlayClickSound();
					}
					else if(Clicked->Identifier == "button_characters_play") {
						size_t SelectedSlot = GetSelectedCharacter();
						if(SelectedSlot < CharacterSlots.size() && CharacterSlots[SelectedSlot].Used) {
							PlayCharacter(SelectedSlot);
						}

						PlayClickSound();
					}
					else if(Clicked->Identifier == "button_characters_back") {
						PlayState.Network->Disconnect();
						PlayClickSound();
					}
					else if(Clicked->Identifier == CharacterButtonPrefix) {

						// Deselect slots
						_Element *CharactersElement = Assets.Elements["element_menu_character_slots"];
						for(auto &Element : CharactersElement->Children) {
							if(Element->Identifier == CharacterButtonPrefix) {
								_Button *Button = (_Button *)Element;
								Button->Checked = false;
							}
						}

						// Set selection
						size_t SelectedSlot = (size_t)(intptr_t)Clicked->UserData;
						CharacterSlots[SelectedSlot].Button->Checked = true;

						// Open new character screen
						if(!CharacterSlots[SelectedSlot].Used)
							InitNewCharacter();

						UpdateCharacterButtons();

						if(DoubleClick && SelectedSlot < CharacterSlots.size()) {
							PlayCharacter(SelectedSlot);
						}
					}
				}
				else if(CharactersState == CHARACTERS_CREATE) {
					if(Clicked->Identifier == NewCharacterPortraitPrefix || Clicked->Identifier == NewCharacterBuildPrefix) {
						size_t SelectedID = (size_t)(intptr_t)Clicked->UserData;

						// Unselect all portraits and select the clicked element
						for(auto &Element : Clicked->Parent->Children) {
							_Button *Button = (_Button *)Element;
							Button->Checked = false;
							if((size_t)(intptr_t)Button->UserData == SelectedID) {
								_TextBox *Name = Assets.TextBoxes["textbox_newcharacter_name"];
								FocusedElement = Name;
								Name->ResetCursor();
								Button->Checked = true;
							}
						}

						ValidateCreateCharacter();
					}
					else if(Clicked->Identifier == "button_newcharacter_create") {
						CreateCharacter();
						PlayClickSound();
					}
					else if(Clicked->Identifier == "button_newcharacter_createhardcore") {
						CreateCharacter(true);
						PlayClickSound();
					}
					else if(Clicked->Identifier == "button_newcharacter_cancel") {
						RequestCharacterList();
						PlayClickSound();
					}
				}
				else if(CharactersState == CHARACTERS_DELETE) {
					if(Clicked->Identifier == "button_confirm_ok") {
						PlayClickSound();

						size_t SelectedSlot = GetSelectedCharacter();
						if(SelectedSlot < CharacterSlots.size() && CharacterSlots[SelectedSlot].Used) {
							_Buffer Packet;
							Packet.Write<PacketType>(PacketType::CHARACTERS_DELETE);
							Packet.Write<uint8_t>((uint8_t)SelectedSlot);
							PlayState.Network->SendPacket(Packet);
						}
					}
					else if(Clicked->Identifier == "button_confirm_cancel") {
						RequestCharacterList();
						PlayClickSound();
					}
				}
			} break;
			case STATE_CONNECT: {
				if(Clicked->Identifier == "button_connect_connect") {
					if(!PlayState.Network->IsDisconnected()) {
						PlayState.Network->Disconnect(true);
						InitConnect(false);
					}
					else
						ConnectToHost();

					PlayClickSound();
				}
				else if(Clicked->Identifier == "button_connect_back") {
					InitTitle(true);
					PlayClickSound();
				}
			} break;
			case STATE_ACCOUNT: {
				if(Clicked->Identifier == "button_account_login") {
					SendAccountInfo();
					PlayClickSound();
				}
				else if(Clicked->Identifier == "button_account_create") {
					SendAccountInfo(true);
					PlayClickSound();
				}
				else if(Clicked->Identifier == "button_account_back") {
					InitConnect(true);
					PlayClickSound();
				}
			} break;
			case STATE_OPTIONS: {
				if(Clicked->Identifier == "button_options_fullscreen") {
					SetFullscreen(!Config.Fullscreen);
					UpdateOptions();
				}
				else if(Clicked->Identifier == "button_options_back") {
					if(FromInGame)
						InitInGame();
					else
						InitTitle(true);

					PlayClickSound();
				}
			} break;
			case STATE_INGAME: {
				if(Clicked->Identifier == "button_ingame_respawn") {
					_Buffer Packet;
					Packet.Write<PacketType>(PacketType::WORLD_RESPAWN);
					PlayState.Network->SendPacket(Packet);
					InitPlay();
					PlayClickSound();
				}
				else if(Clicked->Identifier == "button_ingame_resume") {
					InitPlay();
					PlayClickSound();
				}
				else if(Clicked->Identifier == "button_ingame_options") {
					InitOptions();
					PlayClickSound();
				}
				else if(Clicked->Identifier == "button_ingame_exit") {
					ExitGame();
					PlayClickSound();
				}
			} break;
			default:
			break;
		}
	}
}

// Set fullscreen of game
void _Menu::SetFullscreen(bool Fullscreen) {
	if(Graphics.SetFullscreen(Fullscreen)) {
		Config.Fullscreen = Fullscreen;
		Config.Save();
	}
}

// Update phase
void _Menu::Update(double FrameTime) {
	if(State == STATE_NONE)
		return;

	PreviousClickTimer += FrameTime;
}

// Draw phase
void _Menu::Render() {
	if(State == STATE_NONE)
		return;

	Graphics.Setup2D();

	switch(State) {
		case STATE_TITLE: {
			if(CurrentLayout)
				CurrentLayout->Render();
			Assets.Labels["label_menu_title_version"]->Render(true);
			Assets.Images["image_title_logo"]->Render(true);
		} break;
		case STATE_CHARACTERS: {
			Assets.Elements["element_menu_characters"]->Render();

			if(CharactersState == CHARACTERS_CREATE) {
				Graphics.FadeScreen(MENU_ACCEPTINPUT_FADE);
				if(CurrentLayout)
					CurrentLayout->Render();
			}
			else if(CharactersState == CHARACTERS_DELETE) {
				Graphics.FadeScreen(MENU_ACCEPTINPUT_FADE);
				if(CurrentLayout)
					CurrentLayout->Render();
			}
		} break;
		case STATE_CONNECT: {
			Assets.Elements["element_menu_connect"]->Render();
		} break;
		case STATE_ACCOUNT: {
			Assets.Elements["element_menu_account"]->Render();
		} break;
		case STATE_OPTIONS: {
			if(FromInGame)
			   Graphics.FadeScreen(MENU_PAUSE_FADE);

			Assets.Elements["element_menu_options"]->Render();
		} break;
		case STATE_INGAME: {
			Graphics.FadeScreen(MENU_PAUSE_FADE);

			if(CurrentLayout)
				CurrentLayout->Render();
		} break;
		default:
		break;
	}
}

// Connect
void _Menu::HandleConnect() {
	switch(State) {
		case STATE_CONNECT: {
			_TextBox *Host = Assets.TextBoxes["textbox_connect_host"];
			_TextBox *Port = Assets.TextBoxes["textbox_connect_port"];

			// Save connection information
			Config.LastHost = Host->Text;
			Config.LastPort = Port->Text;
			Config.Save();

			InitAccount();
		} break;
		default:
		break;
	}
}

// Disconnect
void _Menu::HandleDisconnect(bool WasSinglePlayer) {
	if(CurrentLayout != Assets.Elements["element_menu_characters"])
		Audio.StopSounds();

	if(WasSinglePlayer) {
		InitTitle();
	}
	else {
		InitConnect(true);

		_Label *Label = Assets.Labels["label_menu_connect_message"];
		Label->Color = COLOR_RED;
		Label->Text = "Disconnected from server";
	}
}

// Handle packet
void _Menu::HandlePacket(_Buffer &Buffer, PacketType Type) {
	switch(Type) {
		case PacketType::VERSION: {
			std::string Version(Buffer.ReadString());
			if(Version != GAME_VERSION) {
				throw std::runtime_error("Wrong game version");
			}
		} break;
		case PacketType::ACCOUNT_SUCCESS: {
			RequestCharacterList();
		} break;
		case PacketType::CHARACTERS_LIST: {

			// Get header
			HardcoreServer = Buffer.Read<uint8_t>();
			uint8_t CharacterCount = Buffer.Read<uint8_t>();

			// Reset ui state
			InitCharacters();

			// Load UI elements
			LoadCharacterSlots();

			// Get characters
			for(size_t i = 0; i < CharacterCount; i++) {
				size_t Slot = Buffer.Read<uint8_t>();
				bool Hardcore = Buffer.Read<uint8_t>();
				CharacterSlots[Slot].Name->Text = Buffer.ReadString();
				uint32_t PortraitID = Buffer.Read<uint32_t>();
				int Health = Buffer.Read<int>();
				int Experience = Buffer.Read<int>();

				// Set level
				std::stringstream Buffer;
				Buffer << "Level " << PlayState.Stats->FindLevel(Experience)->Level;
				CharacterSlots[Slot].Level->Text = Buffer.str();
				CharacterSlots[Slot].Used = true;

				// Set hardcore status
				if(Hardcore) {
					if(Health <= 0) {
						CharacterSlots[Slot].Hardcore->Text = "Dead";
						CharacterSlots[Slot].CanPlay = false;
					}
					else
						CharacterSlots[Slot].Hardcore->Text = "Hardcore";

				}

				// Check server settings
				if(HardcoreServer && !Hardcore)
				   CharacterSlots[Slot].CanPlay = false;

				// Set portrait
				CharacterSlots[Slot].Image->Texture = PlayState.Stats->GetPortraitImage(PortraitID);
				if(CharacterSlots[Slot].Image->Texture)
					CharacterSlots[Slot].Image->Size = CharacterSlots[Slot].Image->Texture->Size;

				CharacterSlots[Slot].Image->CalculateBounds();
			}

			PreSelectedSlot = NOSLOT;

			// Disable ui buttons
			UpdateCharacterButtons();

		} break;
		case PacketType::CREATECHARACTER_SUCCESS: {

			// Close new character screen
			RequestCharacterList();
		} break;
		case PacketType::CREATECHARACTER_INUSE: {
			_Label *Label = Assets.Labels["label_menu_newcharacter_name"];
			Label->Text = "Name in use";
			Label->Color = COLOR_RED;
		} break;
		case PacketType::ACCOUNT_EXISTS: {
			SetAccountMessage("Account already exists");
		} break;
		case PacketType::ACCOUNT_NOTFOUND: {
			SetAccountMessage("Username/password wrong");
		} break;
		case PacketType::ACCOUNT_INUSE: {
			SetAccountMessage("Account in use");
		} break;
		default:
		break;
	}
}

// Set message for account screen
void _Menu::SetAccountMessage(const std::string &Message) {
	_Label *Label = Assets.Labels["label_menu_account_message"];
	Label->Text = Message;
	Label->Color = COLOR_RED;

	_Button *Button = Assets.Buttons["button_account_login"];
	Button->SetEnabled(true);
}

// Set message for title screen
void _Menu::SetTitleMessage(const std::string &Message) {
	_Label *Label = Assets.Labels["label_menu_title_message"];
	Label->Text = Message;
	Label->Color = COLOR_RED;
}

// Play menu click sound
void _Menu::PlayClickSound() {
	Audio.PlaySound(Assets.Sounds["click0.ogg"]);
}

// Cycle focused elements
void _Menu::FocusNextElement() {
	switch(State) {
		case STATE_CONNECT: {
			_TextBox *Host = Assets.TextBoxes["textbox_connect_host"];
			_TextBox *Port = Assets.TextBoxes["textbox_connect_port"];

			if(FocusedElement == Host)
				FocusedElement = Port;
			else if(FocusedElement == Port || FocusedElement == nullptr)
				FocusedElement = Host;

			((_TextBox *)FocusedElement)->ResetCursor();
		} break;
		case STATE_ACCOUNT: {
			_TextBox *Username = Assets.TextBoxes["textbox_account_username"];
			_TextBox *Password = Assets.TextBoxes["textbox_account_password"];

			if(FocusedElement == Username)
				FocusedElement = Password;
			else if(FocusedElement == Password || FocusedElement == nullptr)
				FocusedElement = Username;

			((_TextBox *)FocusedElement)->ResetCursor();
		} break;
		default:
		break;
	}
}
