/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2018  Alan Witkowski
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
#include <objects/object.h>
#include <objects/components/character.h>
#include <states/play.h>
#include <states/editor.h>
#include <hud/hud.h>
#include <ae/clientnetwork.h>
#include <ae/ui.h>
#include <ae/input.h>
#include <ae/actions.h>
#include <ae/buffer.h>
#include <ae/audio.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <ae/util.h>
#include <ae/audio.h>
#include <config.h>
#include <actiontype.h>
#include <constants.h>
#include <framework.h>
#include <stats.h>
#include <packet.h>
#include <version.h>
#include <SDL_keycode.h>
#include <sstream>
#include <iomanip>

_Menu Menu;

const std::string CharacterButtonPrefix = "button_characters_slot";
const std::string NewCharacterPortraitPrefix = "button_newcharacter_portrait";
const std::string NewCharacterBuildPrefix = "button_newcharacter_build";

const int KeyBindings[] = {
	Action::GAME_UP,
	Action::GAME_DOWN,
	Action::GAME_LEFT,
	Action::GAME_RIGHT,
	Action::GAME_JOIN,
	Action::GAME_INVENTORY,
	Action::GAME_SKILLS,
	Action::GAME_TRADE,
	Action::GAME_PARTY,
	Action::GAME_CHAT,
	Action::GAME_USE,
	Action::GAME_SKILL1,
	Action::GAME_SKILL2,
	Action::GAME_SKILL3,
	Action::GAME_SKILL4,
	Action::GAME_SKILL5,
	Action::GAME_SKILL6,
	Action::GAME_SKILL7,
	Action::GAME_SKILL8,
	Action::MISC_CONSOLE,
};

const char *KeyBindingNames[] = {
	"Move Up",
	"Move Down",
	"Move Left",
	"Move Right",
	"Join Battle",
	"Inventory",
	"Skills",
	"Trade",
	"Party",
	"Chat",
	"Use",
	"Skill 1",
	"Skill 2",
	"Skill 3",
	"Skill 4",
	"Skill 5",
	"Skill 6",
	"Skill 7",
	"Skill 8",
	"Console",
};

// Constructor
_Menu::_Menu() {
	State = STATE_NONE;
	PreSelectedSlot = NOSLOT;
	CurrentLayout = nullptr;
	CharactersState = CHARACTERS_NONE;
	PreviousClickTimer = 0.0;
	CharacterSlots.resize(ACCOUNT_MAX_CHARACTER_SLOTS);
	HardcoreServer = false;
	RebindType = -1;

	ResetInGameState();
}

// Change the current layout
void _Menu::ChangeLayout(const std::string &ElementName) {
	ae::Assets.Elements["label_menu_title_version"]->SetActive(false);

	if(CurrentLayout) {
		CurrentLayout->SetActive(false);

		if(CurrentLayout == ae::Assets.Elements["element_menu_options"])
			Config.Save();
	}

	CurrentLayout = ae::Assets.Elements[ElementName];
	CurrentLayout->SetActive(true);
}

// Initialize
void _Menu::InitTitle(bool Disconnect) {
	if(Disconnect)
		PlayState.Network->Disconnect(true);

	std::string BuildNumber = "";
	if(BUILD_NUMBER)
		BuildNumber = "r" + std::to_string(BUILD_NUMBER);

	ae::Assets.Elements["label_menu_title_version"]->Text = std::string(GAME_VERSION) + BuildNumber;
	ae::Assets.Elements["label_menu_title_message"]->Text = "";

	ChangeLayout("element_menu_title");
	ae::Assets.Elements["label_menu_title_version"]->SetActive(true);

	ae::Audio.PlayMusic(ae::Assets.Music["intro.ogg"]);

	ResetInGameState();
	State = STATE_TITLE;
}

// Init character select screen
void _Menu::InitCharacters() {
	ChangeLayout("element_menu_characters");
	ae::Assets.Elements["element_menu_characters"]->SetClickable(true);

	// Set label
	ae::_Element *HardcoreLabel = ae::Assets.Elements["label_menu_characters_hardcore"];
	HardcoreLabel->SetActive(false);
	if(HardcoreServer)
		HardcoreLabel->SetActive(true);

	ae::Audio.PlayMusic(ae::Assets.Music["intro.ogg"]);

	ResetInGameState();
	CharactersState = CHARACTERS_NONE;
	State = STATE_CHARACTERS;
}

// Init new player popup
void _Menu::InitNewCharacter() {
	ae::_Element *CreateButton = ae::Assets.Elements["button_newcharacter_create"];
	ae::_Element *CreateHardcoreButton = ae::Assets.Elements["button_newcharacter_createhardcore"];
	CreateButton->SetEnabled(false);
	CreateHardcoreButton->SetEnabled(false);

	ae::_Element *Name = ae::Assets.Elements["textbox_newcharacter_name"];
	Name->SetText("");

	ae::_Element *Label = ae::Assets.Elements["label_menu_newcharacter_name"];
	Label->Text = "Name";
	Label->Color = glm::vec4(1.0f);

	LoadPortraitButtons();
	LoadBuildButtons();

	ae::FocusedElement = Name;
	Name->ResetCursor();

	ae::Assets.Elements["element_menu_character_slots"]->SetClickable(false);

	CurrentLayout = ae::Assets.Elements["element_menu_new"];
	CurrentLayout->SetActive(true);

	CharactersState = CHARACTERS_CREATE;
}

// In-game menu
void _Menu::InitInGame() {
	ChangeLayout("element_menu_ingame");
	if(!ShowRespawn)
		ae::Assets.Elements["button_ingame_respawn"]->SetActive(false);

	if(!ShowExitWarning)
		ae::Assets.Elements["label_menu_ingame_exitwarning"]->SetActive(false);

	PlayState.SendStatus(_Character::STATUS_MENU);
	State = STATE_INGAME;
	FromInGame = true;
}

// Return to play
void _Menu::InitPlay() {
	if(CurrentLayout)
		CurrentLayout->SetActive(false);
	CurrentLayout = nullptr;

	PlayState.SendStatus(_Character::STATUS_NONE);
	State = STATE_NONE;
}

// Open options
void _Menu::InitOptions() {
	ChangeLayout("element_menu_options");

	// Assign values from config
	UpdateOptions();

	State = STATE_OPTIONS;
}

// Initialize key bindings menu
void _Menu::InitKeybindings() {
	ChangeLayout("element_menu_keybindings");
	LoadKeybindings();
	CurrentLayout->SetActive(true);
	ae::Assets.Elements["element_menu_keybindings_newkey"]->SetActive(false);

	State = STATE_KEYBINDINGS;
}

// Show the confirm screen
void _Menu::ConfirmAction() {
	CurrentLayout = ae::Assets.Elements["element_menu_confirm"];
	ae::Assets.Elements["label_menu_confirm_warning"]->Text = "Are you sure?";

	CurrentLayout->SetActive(true);
}

// Exit game and return to character select
void _Menu::ExitGame() {

	// Notify server
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_EXIT);
	PlayState.Network->SendPacket(Packet);

	// Shutdown game
	PlayState.StopGame();

	// Show character select
	RequestCharacterList();
}

// Init connect screen
void _Menu::InitConnect(bool UseConfig, bool ConnectNow) {
	PlayState.Network->Disconnect();

	ChangeLayout("element_menu_connect");

	ae::_Element *Host = ae::Assets.Elements["textbox_connect_host"];
	if(UseConfig)
		Host->SetText(Config.LastHost);

	ae::_Element *Port = ae::Assets.Elements["textbox_connect_port"];
	if(UseConfig)
		Port->SetText(Config.LastPort);

	ae::_Element *Label = ae::Assets.Elements["label_menu_connect_message"];
	Label->Color = glm::vec4(1.0f);
	Label->Text = "";

	ae::_Element *Button = ae::Assets.Elements["button_connect_connect"];
	Button->Children.front()->Text = "Connect";

	// Set focus
	ae::FocusedElement = Host;
	Host->ResetCursor();

	State = STATE_CONNECT;
	if(ConnectNow)
		ConnectToHost();
}

// Init account info screen
void _Menu::InitAccount() {
	ChangeLayout("element_menu_account");

	ae::_Element *Username = ae::Assets.Elements["textbox_account_username"];
	Username->SetText(DefaultUsername);

	ae::_Element *Password = ae::Assets.Elements["textbox_account_password"];
	Password->SetText(DefaultPassword);
	Password->Password = true;

	ae::_Element *Label = ae::Assets.Elements["label_menu_account_message"];
	Label->Color = glm::vec4(1.0f);
	Label->Text = "";

	ae::_Element *Button = ae::Assets.Elements["button_account_login"];
	Button->SetEnabled(true);

	// Set focus
	ae::FocusedElement = Username;
	Username->ResetCursor();

	State = STATE_ACCOUNT;
}

// Get the selected portrait id
uint32_t _Menu::GetSelectedIconID(ae::_Element *ParentElement) {

	// Check for selected portrait
	for(auto &Element : ParentElement->Children) {
		ae::_Element *Button = Element;
		if(Button->Checked)
			return (uint32_t)Button->Index;
	}

	return 0;
}

// Get the selected character slot
size_t _Menu::GetSelectedCharacter() {
	size_t Index = 0;

	// Check for selected character
	ae::_Element *CharactersElement = ae::Assets.Elements["element_menu_character_slots"];
	for(auto &Element : CharactersElement->Children) {
		if(Element->Name == CharacterButtonPrefix) {
			ae::_Element *Button = Element;
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
	ae::_Element *Name = ae::Assets.Elements["textbox_newcharacter_name"];
	if(Name->Text.length() == 0)
		return;

	// Get portrait id
	uint32_t PortraitID = GetSelectedIconID(ae::Assets.Elements["element_menu_new_portraits"]);
	if(PortraitID == 0)
		return;

	// Get build id
	uint32_t BuildID = GetSelectedIconID(ae::Assets.Elements["element_menu_new_builds"]);
	if(BuildID == 0)
		return;

	// Get slot
	size_t SelectedSlot = GetSelectedCharacter();
	if(SelectedSlot >= CharacterSlots.size())
		return;

	// Save selection
	PreSelectedSlot = SelectedSlot;

	// Send information
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::CREATECHARACTER_INFO);
	Packet.WriteBit(Hardcore);
	Packet.WriteString(Name->Text.c_str());
	Packet.Write<uint32_t>(PortraitID);
	Packet.Write<uint32_t>(BuildID);
	Packet.Write<uint8_t>((uint8_t)SelectedSlot);
	PlayState.Network->SendPacket(Packet);
}

void _Menu::ConnectToHost() {
	ae::_Element *Host = ae::Assets.Elements["textbox_connect_host"];
	ae::_Element *Port = ae::Assets.Elements["textbox_connect_port"];
	if(Host->Text.length() == 0) {
		ae::FocusedElement = Host;
		return;
	}

	if(Port->Text.length() == 0) {
		ae::FocusedElement = Port;
		return;
	}

	PlayState.HostAddress = Host->Text;
	PlayState.ConnectPort = ae::ToNumber<uint16_t>(Port->Text);
	PlayState.Connect(false);

	ae::_Element *Label = ae::Assets.Elements["label_menu_connect_message"];
	Label->Color = glm::vec4(1.0f);
	Label->Text = "Connecting...";

	ae::_Element *Button = ae::Assets.Elements["button_connect_connect"];
	Button->Children.front()->Text = "Cancel";

	ae::FocusedElement = nullptr;
}

// Send character to play
void _Menu::PlayCharacter(size_t Slot) {
	if(!CharacterSlots[Slot].CanPlay)
		return;

	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::CHARACTERS_PLAY);
	Packet.Write<uint8_t>((uint8_t)Slot);
	PlayState.Network->SendPacket(Packet);

	CharactersState = CHARACTERS_PLAYSENT;

	ae::Audio.StopMusic();
	PlayState.HUD->Reset();
}

// Send login info
void _Menu::SendAccountInfo(bool CreateAccount) {
	ae::_Element *Username = ae::Assets.Elements["textbox_account_username"];
	ae::_Element *Password = ae::Assets.Elements["textbox_account_password"];
	ae::_Element *Label = ae::Assets.Elements["label_menu_account_message"];

	// Check username
	if(Username->Text.length() == 0) {
		ae::FocusedElement = Username;
		Label->Color = ae::Assets.Colors["red"];
		Label->Text = "Enter a username";

		return;
	}

	// Check password
	if(Password->Text.length() == 0) {
		ae::FocusedElement = Password;
		Label->Color = ae::Assets.Colors["red"];
		Label->Text = "Enter a password";

		return;
	}

	// Update UI
	Label->Color = glm::vec4(1.0f);
	Label->Text = "Logging in...";

	ae::_Element *Button = ae::Assets.Elements["button_account_login"];
	Button->SetEnabled(false);

	// Send information
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::ACCOUNT_LOGININFO);
	Packet.WriteBit(CreateAccount);
	Packet.WriteString(Username->Text.c_str());
	Packet.WriteString(Password->Text.c_str());
	Packet.Write<uint64_t>(0);
	PlayState.Network->SendPacket(Packet);

	ae::FocusedElement = nullptr;
}

// Request character list from server
void _Menu::RequestCharacterList() {

	// Request character list
	ae::_Buffer Packet;
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
	ae::_Element *CharacterSlotsElement = ae::Assets.Elements["element_menu_character_slots"];
	for(size_t i = 0; i < ACCOUNT_MAX_CHARACTER_SLOTS; i++) {

		// Add button
		ae::_Element *Button = new ae::_Element();
		Button->Name = CharacterButtonPrefix;
		Button->Parent = CharacterSlotsElement;
		Button->Offset = Offset;
		Button->Size = Size;
		Button->Style = ae::Assets.Styles["style_menu_button"];
		Button->DisabledStyle = ae::Assets.Styles["style_menu_button_disabled"];
		Button->HoverStyle = ae::Assets.Styles["style_menu_button_hover"];
		Button->Alignment = ae::LEFT_TOP;
		Button->Index = (int)i;
		Button->Checked = i == PreSelectedSlot ? true : false;
		CharacterSlotsElement->Children.push_back(Button);

		// Add image for portrait
		ae::_Element *Image = new ae::_Element();
		Image->Parent = Button;
		Image->Alignment = ae::CENTER_MIDDLE;
		Image->Offset = glm::vec2(0, 0);
		Button->Children.push_back(Image);

		// Add name label
		ae::_Element *NameLabel = new ae::_Element();
		NameLabel->Parent = Button;
		NameLabel->Offset = glm::vec2(0, 150);
		NameLabel->Alignment = ae::CENTER_BASELINE;
		NameLabel->Font = ae::Assets.Fonts["hud_medium"];
		Button->Children.push_back(NameLabel);

		// Add level label
		ae::_Element *LevelLabel = new ae::_Element();
		LevelLabel->Parent = Button;
		LevelLabel->Offset = glm::vec2(0, 170);
		LevelLabel->Alignment = ae::CENTER_BASELINE;
		LevelLabel->Font = ae::Assets.Fonts["hud_small"];
		Button->Children.push_back(LevelLabel);

		// Add hardcore label
		ae::_Element *HardcoreLabel = new ae::_Element();
		HardcoreLabel->Parent = Button;
		HardcoreLabel->Offset = glm::vec2(0, 187);
		HardcoreLabel->Alignment = ae::CENTER_BASELINE;
		HardcoreLabel->Color = ae::Assets.Colors["red"];
		HardcoreLabel->Font = ae::Assets.Fonts["hud_small"];
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
	CharacterSlotsElement->SetActive(true);
}

// Clear character slots
void _Menu::ClearCharacterSlots() {
	std::list<ae::_Element *> &Children = ae::Assets.Elements["element_menu_character_slots"]->Children;
	for(auto &Child : Children)
		delete Child;

	Children.clear();
}

// Load portraits
void _Menu::LoadPortraitButtons() {

	// Clear old children
	ae::_Element *PortraitsElement = ae::Assets.Elements["element_menu_new_portraits"];
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

		// Add button
		ae::_Element *Button = new ae::_Element();
		Button->Name = NewCharacterPortraitPrefix;
		Button->Parent = PortraitsElement;
		Button->Offset = Offset;
		Button->Size = Portrait.Texture->Size;
		Button->Alignment = ae::LEFT_TOP;
		Button->Texture = Portrait.Texture;
		Button->HoverStyle = ae::Assets.Styles["style_menu_portrait_hover"];
		Button->Index = (int)Portrait.ID;
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

	std::list<ae::_Element *> &Children = ae::Assets.Elements["element_menu_new_portraits"]->Children;
	for(auto &Child : Children)
		delete Child;

	Children.clear();
}

// Load builds
void _Menu::LoadBuildButtons() {

	// Clear old children
	ae::_Element *BuildsElement = ae::Assets.Elements["element_menu_new_builds"];
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

		// Add button
		ae::_Element *Button = new ae::_Element();
		Button->Name = NewCharacterBuildPrefix;
		Button->Parent = BuildsElement;
		Button->Offset = Offset;
		Button->Size = Build.Texture->Size;
		Button->Alignment = ae::LEFT_TOP;
		Button->Texture = Build.Texture;
		Button->HoverStyle = ae::Assets.Styles["style_menu_portrait_hover"];
		Button->Index = (int)Build.ID;
		BuildsElement->Children.push_back(Button);

		// Add label
		ae::_Element *Label = new ae::_Element();
		Label->Font = ae::Assets.Fonts["hud_small"];
		Label->Text = Build.Name;
		Label->Color = glm::vec4(1.0f);
		Label->Parent = Button;
		Label->Offset = glm::vec2(0, 80);
		Label->Alignment = ae::CENTER_BASELINE;
		Label->Clickable = false;
		Button->Children.push_back(Label);

		// Update position
		Offset.x += Build.Texture->Size.x + 10;
		if(Offset.x > BuildsElement->Size.x - Build.Texture->Size.x - 10) {
			Offset.y += Build.Texture->Size.y + 22;
			Offset.x = 10;
		}

		i++;
	}

	BuildsElement->CalculateBounds();
}

// Create ui elements for key bindings
void _Menu::LoadKeybindings() {

	// Clear old children
	ae::_Element *KeyBindingsElement = ae::Assets.Elements["element_menu_keybindings_keys"];
	ClearKeybindings();

	glm::vec2 StartingPosition(185, 50);
	glm::vec2 Spacing(400, 50);
	glm::vec2 Size(100, 35);

	// Iterate over keys
	size_t i = 0;
	glm::vec2 Offset(StartingPosition);
	for(const auto &Action : KeyBindings) {

		// Add primary key button
		ae::_Element *PrimaryButton = new ae::_Element();
		PrimaryButton->Name = "primary";
		PrimaryButton->Parent = KeyBindingsElement;
		PrimaryButton->Offset = Offset;
		PrimaryButton->Size = Size;
		PrimaryButton->Alignment = ae::LEFT_TOP;
		PrimaryButton->Style = ae::Assets.Styles["style_menu_button"];
		PrimaryButton->HoverStyle = ae::Assets.Styles["style_menu_button_hover"];
		PrimaryButton->Index = i;
		KeyBindingsElement->Children.push_back(PrimaryButton);

		// Add primary key
		ae::_Element *PrimaryKey = new ae::_Element();
		PrimaryKey->Font = ae::Assets.Fonts["hud_small"];
		PrimaryKey->Text = ae::Actions.GetInputNameForAction(Action);
		PrimaryKey->Parent = PrimaryButton;
		PrimaryKey->Offset = glm::vec2(0, 23);
		PrimaryKey->Alignment = ae::CENTER_BASELINE;
		PrimaryKey->Clickable = false;
		PrimaryButton->Children.push_back(PrimaryKey);

		// Add secondary key button
		ae::_Element *SecondaryButton = new ae::_Element();
		SecondaryButton->Name = "secondary";
		SecondaryButton->Parent = KeyBindingsElement;
		SecondaryButton->Offset = Offset + glm::vec2(Size.x + 10, 0);
		SecondaryButton->Size = Size;
		SecondaryButton->Alignment = ae::LEFT_TOP;
		SecondaryButton->Style = ae::Assets.Styles["style_menu_button"];
		SecondaryButton->HoverStyle = ae::Assets.Styles["style_menu_button_hover"];
		SecondaryButton->Index = i;
		KeyBindingsElement->Children.push_back(SecondaryButton);

		// Add secondary key
		ae::_Element *SecondaryKey = new ae::_Element();
		SecondaryKey->Font = ae::Assets.Fonts["hud_small"];
		SecondaryKey->Text = ae::Actions.GetInputNameForAction(Action, 1);
		SecondaryKey->Parent = SecondaryButton;
		SecondaryKey->Offset = glm::vec2(0, 23);
		SecondaryKey->Alignment = ae::CENTER_BASELINE;
		SecondaryKey->Clickable = false;
		SecondaryButton->Children.push_back(SecondaryKey);

		// Add label
		ae::_Element *Label = new ae::_Element();
		Label->Font = ae::Assets.Fonts["hud_small"];
		Label->Text = KeyBindingNames[i];
		Label->Parent = PrimaryButton;
		Label->Offset = glm::vec2(-120, 23);
		Label->Alignment = ae::CENTER_BASELINE;
		Label->Clickable = false;
		PrimaryButton->Children.push_back(Label);

		// Add headers
		if(Offset.y == StartingPosition.y) {

			// Add primary
			ae::_Element *PrimaryLabel = new ae::_Element();
			PrimaryLabel->Font = ae::Assets.Fonts["hud_small"];
			PrimaryLabel->Text = "Primary";
			PrimaryLabel->Parent = PrimaryButton;
			PrimaryLabel->Offset = glm::vec2(0, -10);
			PrimaryLabel->Alignment = ae::CENTER_BASELINE;
			PrimaryLabel->Clickable = false;
			PrimaryButton->Children.push_back(PrimaryLabel);

			// Add secondary
			ae::_Element *SecondaryLabel = new ae::_Element();
			SecondaryLabel->Font = ae::Assets.Fonts["hud_small"];
			SecondaryLabel->Text = "Secondary";
			SecondaryLabel->Parent = SecondaryButton;
			SecondaryLabel->Offset = glm::vec2(0, -10);
			SecondaryLabel->Alignment = ae::CENTER_BASELINE;
			SecondaryLabel->Clickable = false;
			SecondaryButton->Children.push_back(SecondaryLabel);
		}

		// Update position
		Offset.y += Spacing.y;
		if(Offset.y > KeyBindingsElement->Size.y - Size.y) {
			Offset.x += Spacing.x;
			Offset.y = StartingPosition.y;
		}
		i++;
	}

	KeyBindingsElement->CalculateBounds();
}

// Clear memory used by portraits
void _Menu::ClearBuilds() {
	std::list<ae::_Element *> &Children = ae::Assets.Elements["element_menu_new_builds"]->Children;
	for(auto &Child : Children)
		delete Child;

	Children.clear();
}

// Clear memory used by key bindings
void _Menu::ClearKeybindings() {
	std::list<ae::_Element *> &Children = ae::Assets.Elements["element_menu_keybindings_keys"]->Children;
	for(auto &Child : Children)
		delete Child;

	Children.clear();
}

// Update option ui values
void _Menu::UpdateOptions() {
	std::stringstream Buffer;
	Buffer << std::fixed << std::setprecision(2);

	// Set fullscreen
	ae::Assets.Elements["label_menu_options_fullscreen_check"]->Text = Config.Fullscreen ? "X" : "";

	// Set sound volume
	Buffer << Config.SoundVolume;
	ae::Assets.Elements["label_options_soundvolume"]->Text = Buffer.str();
	Buffer.str("");

	// Set music volume
	Buffer << Config.MusicVolume;
	ae::Assets.Elements["label_options_musicvolume"]->Text = Buffer.str();
	Buffer.str("");

	ae::Assets.Elements["button_options_soundvolume"]->SetOffsetPercent(glm::vec2(Config.SoundVolume, 0));
	ae::Assets.Elements["button_options_musicvolume"]->SetOffsetPercent(glm::vec2(Config.MusicVolume, 0));
}

// Update config and audio volumes from options
void _Menu::UpdateVolume() {
	ae::_Element *SoundSlider = ae::Assets.Elements["element_options_soundvolume"];
	ae::_Element *MusicSlider = ae::Assets.Elements["element_options_musicvolume"];
	ae::_Element *SoundVolume = ae::Assets.Elements["label_options_soundvolume"];
	ae::_Element *MusicVolume = ae::Assets.Elements["label_options_musicvolume"];
	ae::_Element *SoundButton = ae::Assets.Elements["button_options_soundvolume"];
	ae::_Element *MusicButton = ae::Assets.Elements["button_options_musicvolume"];

	// Handle clicking inside slider elements
	if(!SoundButton->PressedElement && SoundSlider->PressedElement) {
		SoundButton->PressedOffset = SoundButton->Size / 2.0f;
		SoundButton->PressedElement = SoundButton;
	}
	if(!MusicButton->PressedElement && MusicSlider->PressedElement) {
		MusicButton->PressedOffset = MusicButton->Size / 2.0f;
		MusicButton->PressedElement = MusicButton;
	}

	// Update volume
	if(SoundButton->PressedElement || MusicButton->PressedElement) {

		// Convert slider percent to number
		std::stringstream Buffer;
		Buffer << std::fixed << std::setprecision(2) << SoundButton->GetOffsetPercent().x;
		SoundVolume->Text = Buffer.str();
		Buffer.str("");
		Buffer << std::fixed << std::setprecision(2) << MusicButton->GetOffsetPercent().x;
		MusicVolume->Text = Buffer.str();
		Buffer.str("");

		// Set volumes
		Config.SoundVolume = ae::ToNumber<float>(SoundVolume->Text);
		Config.MusicVolume = ae::ToNumber<float>(MusicVolume->Text);
		ae::Audio.SetSoundVolume(Config.SoundVolume);
		ae::Audio.SetMusicVolume(Config.MusicVolume);
	}
}

// Reset variables used for in-game menu
void _Menu::ResetInGameState() {
	FromInGame = false;
	ShowExitWarning = false;
	ShowRespawn = false;
}

// Show keybinding dialog
void _Menu::ShowNewKey(ae::_Element *Button, int Type) {
	ae::_Element *NewKeyElement = ae::Assets.Elements["element_menu_keybindings_newkey"];
	ae::_Element *NewKeyActionElement = ae::Assets.Elements["label_menu_keybindings_newkey_action"];
	NewKeyElement->SetActive(true);

	// Set action text
	NewKeyActionElement->Text = KeyBindingNames[Button->Index];
	RebindType = Type;
}

// Remap a key/button
void _Menu::RemapInput(int InputType, int Input) {
	ae::Assets.Elements["element_menu_keybindings_newkey"]->SetActive(false);
	if(InputType == ae::_Input::KEYBOARD && Input == SDL_SCANCODE_ESCAPE)
		return;

	// Remove duplicate keys/buttons
	for(int i = 0; i < Action::COUNT; i++) {
		if(ae::Actions.GetInputForAction(InputType, i) == Input) {
			//ae::Actions.ClearMappingsForAction(InputType, i);
		}
	}

	// Clear out existing action
	//ae::Actions.ClearMappingsForAction(ae::_Input::KEYBOARD, CurrentAction);
	//ae::Actions.ClearMappingsForAction(ae::_Input::MOUSE_BUTTON, CurrentAction);

	// Add new binding
	//ae::Actions.AddInputMap(InputType, Input, CurrentAction, false);

	// Update menu labels
	//RefreshInputLabels();
}


// Check new character screen for portrait and name
void _Menu::ValidateCreateCharacter() {
	bool NameValid = false;
	uint32_t PortraitID = GetSelectedIconID(ae::Assets.Elements["element_menu_new_portraits"]);
	uint32_t BuildID = GetSelectedIconID(ae::Assets.Elements["element_menu_new_builds"]);

	// Check name length
	ae::_Element *CreateButton = ae::Assets.Elements["button_newcharacter_create"];
	ae::_Element *CreateHardcoreButton = ae::Assets.Elements["button_newcharacter_createhardcore"];
	ae::_Element *Name = ae::Assets.Elements["textbox_newcharacter_name"];
	if(Name->Text.length() > 0)
		NameValid = true;
	else
		ae::FocusedElement = Name;

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
	ae::_Element *DeleteButton = ae::Assets.Elements["button_characters_delete"];
	ae::_Element *PlayButton = ae::Assets.Elements["button_characters_play"];
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
	ClearKeybindings();
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
			switch(Action) {
				case Action::MENU_BACK: {
					if(FromInGame)
						InitInGame();
					else
						InitTitle(true);
				} break;
			}
		} break;
		case STATE_KEYBINDINGS: {
			switch(Action) {
				case Action::MENU_BACK: {
					ae::_Element *NewKeyElement = ae::Assets.Elements["element_menu_keybindings_newkey"];
					if(!NewKeyElement->Active)
						InitOptions();
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
bool _Menu::HandleKey(const ae::_KeyEvent &KeyEvent) {
	if(State == STATE_NONE)
		return true;

	switch(State) {
		case STATE_CHARACTERS: {
			if(CharactersState == CHARACTERS_CREATE) {
				ValidateCreateCharacter();
			}
		} break;
		case STATE_KEYBINDINGS: {

			// Check for new key binding
			ae::_Element *NewKeyElement = ae::Assets.Elements["element_menu_keybindings_newkey"];
			if(NewKeyElement->Active && KeyEvent.Pressed) {
				RemapInput(ae::_Input::KEYBOARD, KeyEvent.Scancode);
				return false;
			}
		} break;
		default:
		break;
	}

	return true;
}

// Handle mouse event
void _Menu::HandleMouseButton(const ae::_MouseEvent &MouseEvent) {
	if(State == STATE_NONE)
		return;

	if(!CurrentLayout)
		return;


	// Check for new button binding
	switch(State) {
		case STATE_KEYBINDINGS: {
			ae::_Element *NewKeyElement = ae::Assets.Elements["element_menu_keybindings_newkey"];
			if(NewKeyElement->Active) {
				if(MouseEvent.Pressed)
					RemapInput(ae::_Input::MOUSE_BUTTON, MouseEvent.Button);

				return;
			}
		} break;
		default:
		break;
	}

	// Get clicked element
	ae::_Element *Clicked = CurrentLayout->GetClickedElement();
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
				if(Clicked->Name == "button_title_play") {
					PlayState.Connect(true);
					PlayClickSound();
				}
				else if(Clicked->Name == "button_title_joinserver") {
					InitConnect(true);
					PlayClickSound();
				}
				else if(Clicked->Name == "button_title_options") {
					InitOptions();
					PlayClickSound();
				}
				else if(Clicked->Name == "button_title_exit") {
					Framework.Done = true;
				}
			} break;
			case STATE_CHARACTERS: {
				if(CharactersState == CHARACTERS_NONE) {

					if(Clicked->Name == "button_characters_delete") {
						CharactersState = CHARACTERS_DELETE;
						ae::Assets.Elements["element_menu_characters"]->SetClickable(false);
						ConfirmAction();
						PlayClickSound();
					}
					else if(Clicked->Name == "button_characters_play") {
						size_t SelectedSlot = GetSelectedCharacter();
						if(SelectedSlot < CharacterSlots.size() && CharacterSlots[SelectedSlot].Used) {
							PlayCharacter(SelectedSlot);
						}

						PlayClickSound();
					}
					else if(Clicked->Name == "button_characters_back") {
						PlayState.Network->Disconnect();
						PlayClickSound();
					}
					else if(Clicked->Name == CharacterButtonPrefix) {

						// Deselect slots
						ae::_Element *CharactersElement = ae::Assets.Elements["element_menu_character_slots"];
						for(auto &Element : CharactersElement->Children) {
							if(Element->Name == CharacterButtonPrefix) {
								ae::_Element *Button = Element;
								Button->Checked = false;
							}
						}

						// Set selection
						size_t SelectedSlot = (size_t)Clicked->Index;
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
					if(Clicked->Name == NewCharacterPortraitPrefix || Clicked->Name == NewCharacterBuildPrefix) {
						size_t SelectedID = (size_t)Clicked->Index;

						// Unselect all portraits and select the clicked element
						for(auto &Element : Clicked->Parent->Children) {
							ae::_Element *Button = Element;
							Button->Checked = false;
							if((size_t)Button->Index == SelectedID) {
								ae::_Element *Name = ae::Assets.Elements["textbox_newcharacter_name"];
								ae::FocusedElement = Name;
								Name->ResetCursor();
								Button->Checked = true;
							}
						}

						ValidateCreateCharacter();
					}
					else if(Clicked->Name == "button_newcharacter_create") {
						CreateCharacter();
						PlayClickSound();
					}
					else if(Clicked->Name == "button_newcharacter_createhardcore") {
						CreateCharacter(true);
						PlayClickSound();
					}
					else if(Clicked->Name == "button_newcharacter_cancel") {
						RequestCharacterList();
						PlayClickSound();
					}
				}
				else if(CharactersState == CHARACTERS_DELETE) {
					if(Clicked->Name == "button_confirm_ok") {
						PlayClickSound();

						size_t SelectedSlot = GetSelectedCharacter();
						if(SelectedSlot < CharacterSlots.size() && CharacterSlots[SelectedSlot].Used) {
							ae::_Buffer Packet;
							Packet.Write<PacketType>(PacketType::CHARACTERS_DELETE);
							Packet.Write<uint8_t>((uint8_t)SelectedSlot);
							PlayState.Network->SendPacket(Packet);
						}
					}
					else if(Clicked->Name == "button_confirm_cancel") {
						RequestCharacterList();
						PlayClickSound();
					}
				}
			} break;
			case STATE_CONNECT: {
				if(Clicked->Name == "button_connect_connect") {
					if(!PlayState.Network->IsDisconnected()) {
						PlayState.Network->Disconnect(true);
						InitConnect(false);
					}
					else
						ConnectToHost();

					PlayClickSound();
				}
				else if(Clicked->Name == "button_connect_back") {
					InitTitle(true);
					PlayClickSound();
				}
			} break;
			case STATE_ACCOUNT: {
				if(Clicked->Name == "button_account_login") {
					SendAccountInfo();
					PlayClickSound();
				}
				else if(Clicked->Name == "button_account_create") {
					SendAccountInfo(true);
					PlayClickSound();
				}
				else if(Clicked->Name == "button_account_back") {
					InitConnect(true);
					PlayClickSound();
				}
			} break;
			case STATE_OPTIONS: {
				if(Clicked->Name == "button_options_fullscreen") {
					SetFullscreen(!Config.Fullscreen);
					UpdateOptions();
				}
				else if(Clicked->Name == "button_options_keybindings") {
					InitKeybindings();

					PlayClickSound();
				}
				else if(Clicked->Name == "button_options_back") {
					if(FromInGame)
						InitInGame();
					else
						InitTitle(true);

					PlayClickSound();
				}
			} break;
			case STATE_KEYBINDINGS: {
				if(Clicked->Name == "button_menu_keybindings_back") {
					InitOptions();

					PlayClickSound();
				}
				else if(Clicked->Name == "primary") {
					ShowNewKey(Clicked, 0);
				}
				else if(Clicked->Name == "secondary") {
					ShowNewKey(Clicked, 1);
				}
			} break;
			case STATE_INGAME: {
				if(Clicked->Name == "button_ingame_respawn") {
					ae::_Buffer Packet;
					Packet.Write<PacketType>(PacketType::WORLD_RESPAWN);
					PlayState.Network->SendPacket(Packet);
					InitPlay();
					PlayClickSound();
				}
				else if(Clicked->Name == "button_ingame_resume") {
					InitPlay();
					PlayClickSound();
				}
				else if(Clicked->Name == "button_ingame_options") {
					InitOptions();
					PlayClickSound();
				}
				else if(Clicked->Name == "button_ingame_exit") {
					ExitGame();
					PlayClickSound();
				}
			} break;
			default:
			break;
		}
	}
}

// Set fullscreen state of game
void _Menu::SetFullscreen(bool Fullscreen) {
	if(ae::Graphics.SetFullscreen(Fullscreen)) {
		Config.Fullscreen = Fullscreen;
		Config.Save();
	}
}

// Update
void _Menu::Update(double FrameTime) {
	if(State == STATE_NONE)
		return;

	UpdateVolume();

	PreviousClickTimer += FrameTime;
}

// Render
void _Menu::Render() {
	if(State == STATE_NONE)
		return;

	ae::Graphics.Setup2D();
	ae::Graphics.SetStaticUniforms();

	switch(State) {
		case STATE_TITLE: {
			if(CurrentLayout)
				CurrentLayout->Render();
			ae::Assets.Elements["label_menu_title_version"]->Render();
		} break;
		case STATE_CHARACTERS: {
			ae::Assets.Elements["element_menu_characters"]->Render();

			if(CharactersState == CHARACTERS_CREATE) {
				ae::Graphics.FadeScreen(MENU_ACCEPTINPUT_FADE);
				if(CurrentLayout)
					CurrentLayout->Render();
			}
			else if(CharactersState == CHARACTERS_DELETE) {
				ae::Graphics.FadeScreen(MENU_ACCEPTINPUT_FADE);
				if(CurrentLayout)
					CurrentLayout->Render();
			}
		} break;
		case STATE_CONNECT: {
			ae::Assets.Elements["element_menu_connect"]->Render();
		} break;
		case STATE_ACCOUNT: {
			ae::Assets.Elements["element_menu_account"]->Render();
		} break;
		case STATE_OPTIONS: {
			if(FromInGame)
			   ae::Graphics.FadeScreen(MENU_PAUSE_FADE);

			ae::Assets.Elements["element_menu_options"]->Render();
		} break;
		case STATE_KEYBINDINGS: {
			if(FromInGame)
			   ae::Graphics.FadeScreen(MENU_PAUSE_FADE);

			CurrentLayout->Render();
			if(ae::Assets.Elements["element_menu_keybindings_newkey"]->Active) {
				ae::Graphics.FadeScreen(MENU_ACCEPTINPUT_FADE);
				ae::Assets.Elements["element_menu_keybindings_newkey"]->Render();
			}
		} break;
		case STATE_INGAME: {
			ae::Graphics.FadeScreen(MENU_PAUSE_FADE);

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
			ae::_Element *Host = ae::Assets.Elements["textbox_connect_host"];
			ae::_Element *Port = ae::Assets.Elements["textbox_connect_port"];

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
	if(CurrentLayout != ae::Assets.Elements["element_menu_characters"])
		ae::Audio.StopSounds();

	if(WasSinglePlayer) {
		InitTitle();
	}
	else {
		InitConnect(true);

		ae::_Element *Label = ae::Assets.Elements["label_menu_connect_message"];
		Label->Color = ae::Assets.Colors["red"];
		Label->Text = "Disconnected from server";
	}
}

// Handle packet
void _Menu::HandlePacket(ae::_Buffer &Buffer, PacketType Type) {
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
			ae::_Element *Label = ae::Assets.Elements["label_menu_newcharacter_name"];
			Label->Text = "Name in use";
			Label->Color = ae::Assets.Colors["red"];
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
	ae::_Element *Label = ae::Assets.Elements["label_menu_account_message"];
	Label->Text = Message;
	Label->Color = ae::Assets.Colors["red"];

	ae::_Element *Button = ae::Assets.Elements["button_account_login"];
	Button->SetEnabled(true);
}

// Set message for title screen
void _Menu::SetTitleMessage(const std::string &Message) {
	ae::_Element *Label = ae::Assets.Elements["label_menu_title_message"];
	Label->Text = Message;
	Label->Color = ae::Assets.Colors["red"];
}

// Play menu click sound
void _Menu::PlayClickSound() {
	ae::Audio.PlaySound(ae::Assets.Sounds["click0.ogg"]);
}

// Cycle focused elements
void _Menu::FocusNextElement() {
	switch(State) {
		case STATE_CONNECT: {
			ae::_Element *Host = ae::Assets.Elements["textbox_connect_host"];
			ae::_Element *Port = ae::Assets.Elements["textbox_connect_port"];

			if(ae::FocusedElement == Host)
				ae::FocusedElement = Port;
			else if(ae::FocusedElement == Port || ae::FocusedElement == nullptr)
				ae::FocusedElement = Host;

			ae::FocusedElement->ResetCursor();
		} break;
		case STATE_ACCOUNT: {
			ae::_Element *Username = ae::Assets.Elements["textbox_account_username"];
			ae::_Element *Password = ae::Assets.Elements["textbox_account_password"];

			if(ae::FocusedElement == Username)
				ae::FocusedElement = Password;
			else if(ae::FocusedElement == Password || ae::FocusedElement == nullptr)
				ae::FocusedElement = Username;

			ae::FocusedElement->ResetCursor();
		} break;
		default:
		break;
	}
}
