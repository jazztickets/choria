/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2020 Alan Witkowski
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
#include <ae/font.h>
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
#include <SDL_timer.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_clipboard.h>
#include <SDL_stdinc.h>
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
	Action::GAME_ITEM1,
	Action::GAME_ITEM2,
	Action::GAME_ITEM3,
	Action::GAME_ITEM4,
	Action::MENU_PAUSE,
	Action::MISC_DEBUG,
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
	"Item 1",
	"Item 2",
	"Item 3",
	"Item 4",
	"Menu",
	"Stats",
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
	RebindAction = -1;
	PingTime = 0;

	ResetInGameState();
}

// Change the current layout
void _Menu::ChangeLayout(const std::string &ElementName) {
	ae::Assets.Elements["label_menu_title_version"]->SetActive(false);

	if(CurrentLayout) {
		CurrentLayout->SetActive(false);

		if(CurrentLayout == ae::Assets.Elements["element_menu_options"] || CurrentLayout == ae::Assets.Elements["element_menu_keybindings"]) {
			Config.Save();
			PlayState.HUD->UpdateButtonBarLabels();
		}
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
	ae::_Element *CreateButton = ae::Assets.Elements["button_menu_new_create"];
	CreateButton->SetEnabled(false);

	ae::_Element *Name = ae::Assets.Elements["textbox_menu_new_name"];
	Name->SetText("");

	ae::_Element *Check = ae::Assets.Elements["label_menu_new_hardcore_check"];
	Check->Text = "";

	ae::_Element *Label = ae::Assets.Elements["label_menu_new_name"];
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
		ae::Assets.Elements["button_menu_ingame_respawn"]->SetActive(false);

	if(!ShowExitWarning)
		ae::Assets.Elements["label_menu_ingame_exitwarning"]->SetActive(false);

	PlayState.HUD->SetBarState(false);
	PlayState.SendStatus(_Character::STATUS_MENU);
	State = STATE_INGAME;
	FromInGame = true;
	BadGameVersion = false;
}

// Return to play
void _Menu::InitPlay() {
	if(CurrentLayout)
		CurrentLayout->SetActive(false);
	CurrentLayout = nullptr;

	PlayState.HUD->SetBarState(true);
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
	CurrentLayout = ae::Assets.Elements["element_confirm"];
	ae::Assets.Elements["label_confirm_warning"]->Text = "Are you sure?";

	CurrentLayout->SetActive(true);
}

// Exit game and return to character select
void _Menu::ExitGame() {

	// Notify server
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::WORLD_EXIT);
	Packet.WriteBit(0);
	PlayState.Network->SendPacket(Packet);

	// Shutdown game
	PlayState.StopGame();

	// Show character select
	RequestCharacterList();
}

// Browse LAN servers
void _Menu::InitBrowseServers(bool UseConfig, bool ConnectNow) {
	PlayState.Network->Disconnect();

	ChangeLayout("element_menu_browse");
	ae::Audio.PlayMusic(ae::Assets.Music["intro.ogg"]);

	// Get ui elements
	ae::_Element *Host = ae::Assets.Elements["textbox_menu_browse_host"];
	ae::_Element *Message = ae::Assets.Elements["label_menu_browse_message"];
	ae::_Element *Button = ae::Assets.Elements["button_menu_browse_connect"];

	// Set last host
	if(UseConfig)
		Host->SetText(Config.LastHost + ":" + Config.LastPort);

	// Set ui states
	Message->Color = glm::vec4(1.0f);
	Message->Text = "";
	Button->Children.front()->Text = "Connect";
	ValidateConnect();

	// Get server list
	RefreshServers();
	if(ConnectNow)
		ConnectToHost();

	State = STATE_BROWSE;
}

// Init account info screen
void _Menu::InitAccount() {
	ChangeLayout("element_menu_account");

	ae::_Element *Username = ae::Assets.Elements["textbox_menu_account_username"];
	Username->SetText(DefaultUsername);

	ae::_Element *Password = ae::Assets.Elements["textbox_menu_account_password"];
	Password->SetText(DefaultPassword);
	Password->Password = true;

	ae::_Element *Label = ae::Assets.Elements["label_menu_account_message"];
	Label->Color = glm::vec4(1.0f);
	Label->Text = "";

	ae::_Element *Button = ae::Assets.Elements["button_menu_account_login"];
	Button->SetEnabled(true);

	// Set focus
	ae::FocusedElement = Username->Text == "" ? Username : Password;
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
	ae::_Element *Name = ae::Assets.Elements["textbox_menu_new_name"];
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

// Connect to a server
void _Menu::ConnectToHost() {
	ae::_Element *Host = ae::Assets.Elements["textbox_menu_browse_host"];
	if(Host->Text.length() == 0) {
		return;
	}

	// Get IP and port
	std::string IP;
	std::string Port;
	SplitHost(Host->Text, IP, Port);

	// Connect
	PlayState.HostAddress = IP;
	PlayState.ConnectPort = ae::ToNumber<uint16_t>(Port);
	PlayState.Connect(false);

	ae::_Element *Label = ae::Assets.Elements["label_menu_browse_message"];
	Label->Color = glm::vec4(1.0f);
	Label->Text = "Connecting...";

	ae::_Element *Button = ae::Assets.Elements["button_menu_browse_connect"];
	Button->Children.front()->Text = "Cancel";

	ae::FocusedElement = nullptr;
}

// Send character to play
void _Menu::PlayCharacter(size_t Slot) {
	if(!CharacterSlots[Slot].CanPlay)
		return;

	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::CHARACTERS_PLAY);
	Packet.WriteBit(Config.Offline);
	Packet.Write<uint8_t>((uint8_t)Slot);
	PlayState.Network->SendPacket(Packet);

	CharactersState = CHARACTERS_PLAYSENT;

	ae::Audio.StopMusic();
	PlayState.HUD->Reset();
	PlayState.HUD->ResetChat();
}

// Send login info
void _Menu::SendAccountInfo(bool CreateAccount) {
	ae::_Element *Username = ae::Assets.Elements["textbox_menu_account_username"];
	ae::_Element *Password = ae::Assets.Elements["textbox_menu_account_password"];
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

	ae::_Element *Button = ae::Assets.Elements["button_menu_account_login"];
	Button->SetEnabled(false);

	// Send information
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::ACCOUNT_LOGININFO);
	Packet.WriteBit(CreateAccount);
	Packet.WriteString(Username->Text.c_str());
	Packet.WriteString(Password->Text.c_str());
	Packet.Write<uint64_t>(0);
	PlayState.Network->SendPacket(Packet);

	DefaultUsername = Username->Text;

	ae::FocusedElement = nullptr;
}

// Request character list from server
void _Menu::RequestCharacterList() {
	Config.LastUsername = DefaultUsername;
	Config.Save();

	// Request character list
	ae::_Buffer Packet;
	Packet.Write<PacketType>(PacketType::CHARACTERS_REQUEST);
	PlayState.Network->SendPacket(Packet);
}

// Load character slots
void _Menu::LoadCharacterSlots() {
	glm::vec2 Offset(0, 0);
	glm::vec2 Size(180, 180);
	glm::vec2 Padding(58, 116);

	// Clear old children
	ClearCharacterSlots();

	// Iterate over slots
	ae::_Element *CharacterSlotsElement = ae::Assets.Elements["element_menu_character_slots"];
	for(size_t i = 0; i < ACCOUNT_MAX_CHARACTER_SLOTS; i++) {

		// Add button
		ae::_Element *Button = new ae::_Element();
		Button->Name = CharacterButtonPrefix;
		Button->Parent = CharacterSlotsElement;
		Button->BaseOffset = Offset;
		Button->BaseSize = Size;
		Button->Style = ae::Assets.Styles["style_menu_button"];
		Button->DisabledStyle = ae::Assets.Styles["style_menu_button_disabled"];
		Button->HoverStyle = ae::Assets.Styles["style_menu_button_hover"];
		Button->Alignment = ae::LEFT_TOP;
		Button->Index = (int)i;
		Button->Checked = i == PreSelectedSlot ? true : false;
		Button->Clickable = true;
		CharacterSlotsElement->Children.push_back(Button);

		// Add image for portrait
		ae::_Element *Image = new ae::_Element();
		Image->Parent = Button;
		Image->Alignment = ae::CENTER_MIDDLE;
		Image->BaseOffset = glm::vec2(0, 0);
		Button->Children.push_back(Image);

		// Add name label
		ae::_Element *NameLabel = new ae::_Element();
		NameLabel->Parent = Button;
		NameLabel->BaseOffset = glm::vec2(0, 210);
		NameLabel->Alignment = ae::CENTER_BASELINE;
		NameLabel->Font = ae::Assets.Fonts["hud_medium"];
		Button->Children.push_back(NameLabel);

		// Add level label
		ae::_Element *LevelLabel = new ae::_Element();
		LevelLabel->Parent = Button;
		LevelLabel->BaseOffset = glm::vec2(0, 240);
		LevelLabel->Alignment = ae::CENTER_BASELINE;
		LevelLabel->Font = ae::Assets.Fonts["hud_small"];
		Button->Children.push_back(LevelLabel);

		// Add hardcore label
		ae::_Element *HardcoreLabel = new ae::_Element();
		HardcoreLabel->Parent = Button;
		HardcoreLabel->BaseOffset = glm::vec2(0, 264);
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
		CharacterSlots[i].Used = false;
		CharacterSlots[i].CanPlay = true;

		// Update position
		Offset.x += Size.x + Padding.x;
		if(Offset.x > CharacterSlotsElement->BaseSize.x - Size.x) {
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

	glm::vec2 Offset(14, 70);
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
		Button->BaseOffset = Offset;
		Button->BaseSize = Portrait.Texture->Size;
		Button->Alignment = ae::LEFT_TOP;
		Button->Texture = Portrait.Texture;
		Button->HoverStyle = ae::Assets.Styles["style_menu_hover"];
		Button->Index = (int)Portrait.ID;
		Button->Clickable = true;
		PortraitsElement->Children.push_back(Button);

		// Update position
		Offset.x += Portrait.Texture->Size.x + 14;
		if(Offset.x > PortraitsElement->BaseSize.x - Portrait.Texture->Size.x - 14) {
			Offset.y += Portrait.Texture->Size.y + 14;
			Offset.x = 14;
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

	glm::vec2 Offset(14, 70);
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
		Button->BaseOffset = Offset;
		Button->BaseSize = Build.Texture->Size;
		Button->Alignment = ae::LEFT_TOP;
		Button->Texture = Build.Texture;
		Button->HoverStyle = ae::Assets.Styles["style_menu_hover"];
		Button->Index = (int)Build.ID;
		Button->Clickable = true;
		BuildsElement->Children.push_back(Button);

		// Add label
		ae::_Element *Label = new ae::_Element();
		Label->Font = ae::Assets.Fonts["hud_small"];
		Label->Text = Build.Name;
		Label->Color = glm::vec4(1.0f);
		Label->Parent = Button;
		Label->BaseOffset = glm::vec2(0, 122);
		Label->Alignment = ae::CENTER_BASELINE;
		Button->Children.push_back(Label);

		// Update position
		Offset.x += 114;
		if(Offset.x > BuildsElement->BaseSize.x - Build.Texture->Size.x - 14) {
			Offset.y += Build.Texture->Size.y + 32;
			Offset.x = 14;
		}

		i++;
	}

	BuildsElement->CalculateBounds();
}

// Create ui elements for key bindings
void _Menu::LoadKeybindings() {

	// Clear old children
	ae::_Element *KeyBindingsElement = ae::Assets.Elements["element_menu_keybindings_keys"];
	ClearKeybindingElements();

	glm::vec2 StartingPosition(260, 60);
	glm::vec2 Spacing(562, 60);
	glm::vec2 Size(140, 50);

	// Iterate over actions
	size_t i = 0;
	glm::vec2 Offset(StartingPosition);
	for(const auto &Action : KeyBindings) {

		// Add primary key button
		ae::_Element *PrimaryButton = new ae::_Element();
		PrimaryButton->Name = "primary";
		PrimaryButton->Parent = KeyBindingsElement;
		PrimaryButton->BaseOffset = Offset;
		PrimaryButton->BaseSize = Size;
		PrimaryButton->Alignment = ae::LEFT_TOP;
		PrimaryButton->Style = ae::Assets.Styles["style_menu_button"];
		PrimaryButton->DisabledStyle = ae::Assets.Styles["style_menu_button_disabled"];
		PrimaryButton->HoverStyle = ae::Assets.Styles["style_menu_button_hover"];
		PrimaryButton->Index = i;
		PrimaryButton->Clickable = true;
		KeyBindingsElement->Children.push_back(PrimaryButton);

		// Add primary key
		ae::_Element *PrimaryKey = new ae::_Element();
		PrimaryKey->Font = ae::Assets.Fonts["hud_small"];
		PrimaryKey->Text = ae::Actions.GetInputNameForAction(Action, 0);
		PrimaryKey->Parent = PrimaryButton;
		PrimaryKey->BaseOffset = glm::vec2(0, 32);
		PrimaryKey->Alignment = ae::CENTER_BASELINE;
		PrimaryButton->Children.push_back(PrimaryKey);

		// Add secondary key button
		ae::_Element *SecondaryButton = new ae::_Element();
		SecondaryButton->Name = "secondary";
		SecondaryButton->Parent = KeyBindingsElement;
		SecondaryButton->BaseOffset = Offset + glm::vec2(Size.x + 14, 0);
		SecondaryButton->BaseSize = Size;
		SecondaryButton->Alignment = ae::LEFT_TOP;
		SecondaryButton->Style = ae::Assets.Styles["style_menu_button"];
		SecondaryButton->DisabledStyle = ae::Assets.Styles["style_menu_button_disabled"];
		SecondaryButton->HoverStyle = ae::Assets.Styles["style_menu_button_hover"];
		SecondaryButton->Index = i;
		SecondaryButton->Clickable = true;
		KeyBindingsElement->Children.push_back(SecondaryButton);

		// Add secondary key
		ae::_Element *SecondaryKey = new ae::_Element();
		SecondaryKey->Font = ae::Assets.Fonts["hud_small"];
		SecondaryKey->Text = ae::Actions.GetInputNameForAction(Action, 1);
		SecondaryKey->Parent = SecondaryButton;
		SecondaryKey->BaseOffset = glm::vec2(0, 32);
		SecondaryKey->Alignment = ae::CENTER_BASELINE;
		SecondaryButton->Children.push_back(SecondaryKey);

		// Add label
		ae::_Element *Label = new ae::_Element();
		Label->Font = ae::Assets.Fonts["hud_small"];
		Label->Text = KeyBindingNames[i];
		Label->Parent = PrimaryButton;
		Label->BaseOffset = glm::vec2(-168, 32);
		Label->Alignment = ae::CENTER_BASELINE;
		PrimaryButton->Children.push_back(Label);

		// Disable chat rebinding
		if(std::string(KeyBindingNames[i]) == "Chat") {
			PrimaryButton->SetEnabled(false);
			SecondaryButton->SetEnabled(false);
			Label->SetEnabled(true);
		}

		// Add headers
		if(Offset.y == StartingPosition.y) {

			// Add primary
			ae::_Element *PrimaryLabel = new ae::_Element();
			PrimaryLabel->Font = ae::Assets.Fonts["hud_small"];
			PrimaryLabel->Text = "Primary";
			PrimaryLabel->Parent = PrimaryButton;
			PrimaryLabel->BaseOffset = glm::vec2(0, -14);
			PrimaryLabel->Alignment = ae::CENTER_BASELINE;
			PrimaryButton->Children.push_back(PrimaryLabel);

			// Add secondary
			ae::_Element *SecondaryLabel = new ae::_Element();
			SecondaryLabel->Font = ae::Assets.Fonts["hud_small"];
			SecondaryLabel->Text = "Secondary";
			SecondaryLabel->Parent = SecondaryButton;
			SecondaryLabel->BaseOffset = glm::vec2(0, -14);
			SecondaryLabel->Alignment = ae::CENTER_BASELINE;
			SecondaryButton->Children.push_back(SecondaryLabel);
		}

		// Update position
		Offset.y += Spacing.y;
		if(Offset.y > KeyBindingsElement->BaseSize.y - Size.y) {
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
void _Menu::ClearKeybindingElements() {
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
	ae::Assets.Elements["label_menu_options_soundvolume_value"]->Text = Buffer.str();
	Buffer.str("");

	// Set music volume
	Buffer << Config.MusicVolume;
	ae::Assets.Elements["label_menu_options_musicvolume_value"]->Text = Buffer.str();
	Buffer.str("");

	ae::Assets.Elements["button_menu_options_soundvolume"]->SetOffsetPercent(glm::vec2(Config.SoundVolume, 0));
	ae::Assets.Elements["button_menu_options_musicvolume"]->SetOffsetPercent(glm::vec2(Config.MusicVolume, 0));
}

// Update config and audio volumes from options
void _Menu::UpdateVolume() {
	ae::_Element *SoundSlider = ae::Assets.Elements["element_menu_options_soundvolume"];
	ae::_Element *MusicSlider = ae::Assets.Elements["element_menu_options_musicvolume"];
	ae::_Element *SoundVolume = ae::Assets.Elements["label_menu_options_soundvolume_value"];
	ae::_Element *MusicVolume = ae::Assets.Elements["label_menu_options_musicvolume_value"];
	ae::_Element *SoundButton = ae::Assets.Elements["button_menu_options_soundvolume"];
	ae::_Element *MusicButton = ae::Assets.Elements["button_menu_options_musicvolume"];

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
	RebindType = Type;
	RebindAction = KeyBindings[Button->Index];
	NewKeyActionElement->Text = KeyBindingNames[Button->Index];
}

// Clear action on keybinding page
void _Menu::ClearAction(int Action, int Type) {
	for(int i = 0; i < ae::_Input::INPUT_COUNT; i++) {
		if(ae::Actions.GetInputForAction(i, Action, Type) != -1)
			ae::Actions.ClearMappingsForAction(i, Action, Type);
	}
}

// Remap a key/button
void _Menu::RemapInput(int InputType, int Input) {
	ae::Assets.Elements["element_menu_keybindings_newkey"]->SetActive(false);
	if(InputType == ae::_Input::KEYBOARD) {
		if(Input == SDL_SCANCODE_ESCAPE || Input == SDL_SCANCODE_RETURN || Input == SDL_SCANCODE_KP_ENTER)
			return;
	}

	// Remove duplicate keys/buttons
	for(const auto &Action : KeyBindings)
		ae::Actions.ClearMappingForInputAction(InputType, Input, Action);

	// Clear out existing action
	ClearAction(RebindAction, RebindType);

	// Add new binding
	ae::Actions.AddInputMap(RebindType, InputType, Input, RebindAction, 1.0f, -1.0f, false);

	// Update menu labels
	InitKeybindings();
}

// Check new character screen for portrait and name
void _Menu::ValidateCreateCharacter() {
	bool NameValid = false;
	uint32_t PortraitID = GetSelectedIconID(ae::Assets.Elements["element_menu_new_portraits"]);
	uint32_t BuildID = GetSelectedIconID(ae::Assets.Elements["element_menu_new_builds"]);

	// Check name length
	ae::_Element *CreateButton = ae::Assets.Elements["button_menu_new_create"];
	ae::_Element *Name = ae::Assets.Elements["textbox_menu_new_name"];
	if(Name->Text.length() > 0)
		NameValid = true;
	else
		ae::FocusedElement = Name;

	// Enable button
	if(PortraitID != 0 && BuildID != 0 && NameValid) {
		CreateButton->SetEnabled(true);
	}
	else {
		CreateButton->SetEnabled(false);
	}
}

// Update ui button states
void _Menu::UpdateCharacterButtons() {
	ae::_Element *DeleteButton = ae::Assets.Elements["button_menu_characters_delete"];
	ae::_Element *PlayButton = ae::Assets.Elements["button_menu_characters_play"];
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
	ClearKeybindingElements();
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
		case STATE_BROWSE: {
			switch(Action) {
				case Action::MENU_GO:
					ConnectToHost();
				break;
				case Action::MENU_BACK:
					InitTitle(true);
				break;
			}
		} break;
		case STATE_ACCOUNT: {
			switch(Action) {
				case Action::MENU_GO:
					SendAccountInfo();
				break;
				case Action::MENU_BACK:
					InitBrowseServers(true);
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
		case STATE_BROWSE: {
			if(SDL_HasClipboardText() && ae::Input.ModKeyDown(KMOD_CTRL) && KeyEvent.Scancode == SDL_SCANCODE_V) {
				char *PastedText = SDL_GetClipboardText();
				ae::_Element *Host = ae::Assets.Elements["textbox_menu_browse_host"];
				Host->Text = PastedText;
				Host->Text.resize(Host->MaxLength);
				SDL_free(PastedText);
			}
			ValidateConnect();
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
				if(MouseEvent.Pressed && MouseEvent.Button != SDL_BUTTON_LEFT && MouseEvent.Button != SDL_BUTTON_RIGHT)
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

		// Handle right click
		if(MouseEvent.Button == SDL_BUTTON_RIGHT) {
			switch(State) {
				case STATE_KEYBINDINGS: {
					if(Clicked->Name == "primary") {
						ClearAction(KeyBindings[Clicked->Index], 0);
						InitKeybindings();
					}
					else if(Clicked->Name == "secondary") {
						ClearAction(KeyBindings[Clicked->Index], 1);
						InitKeybindings();
					}
				}
				break;
				default:
				break;
			}

			return;
		}

		// Ignore other buttons
		if(MouseEvent.Button != SDL_BUTTON_LEFT)
			return;

		// Handle double clicking
		bool DoubleClick = false;
		if(PreviousClick == Clicked && PreviousClickTimer < MENU_DOUBLECLICK_TIME) {
			PreviousClick = nullptr;
			DoubleClick = true;
		}
		else
			PreviousClick = Clicked;

		// Reset timer
		PreviousClickTimer = 0.0;

		// Handle click
		switch(State) {
			case STATE_TITLE: {
				if(Clicked->Name == "button_menu_title_play") {
					PlayState.Connect(true);
					PlayClickSound();
				}
				else if(Clicked->Name == "button_menu_title_joinserver") {
					InitBrowseServers(true);
					PlayClickSound();
				}
				else if(Clicked->Name == "button_menu_title_options") {
					InitOptions();
					PlayClickSound();
				}
				else if(Clicked->Name == "button_menu_title_exit") {
					Framework.Done = true;
				}
			} break;
			case STATE_CHARACTERS: {
				if(CharactersState == CHARACTERS_NONE) {

					if(Clicked->Name == "button_menu_characters_delete") {
						CharactersState = CHARACTERS_DELETE;
						ae::Assets.Elements["element_menu_characters"]->SetClickable(false);
						ConfirmAction();
						PlayClickSound();
					}
					else if(Clicked->Name == "button_menu_characters_play") {
						size_t SelectedSlot = GetSelectedCharacter();
						if(SelectedSlot < CharacterSlots.size() && CharacterSlots[SelectedSlot].Used) {
							PlayCharacter(SelectedSlot);
						}

						PlayClickSound();
					}
					else if(Clicked->Name == "button_menu_characters_back") {
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
								ae::_Element *Name = ae::Assets.Elements["textbox_menu_new_name"];
								ae::FocusedElement = Name;
								Name->ResetCursor();
								Button->Checked = true;
							}
						}

						ValidateCreateCharacter();
					}
					else if(Clicked->Name == "button_menu_new_hardcore") {
						ae::_Element *Check = ae::Assets.Elements["label_menu_new_hardcore_check"];
						Check->Text = Check->Text == "" ? "X" : "";
					}
					else if(Clicked->Name == "button_menu_new_create") {
						ae::_Element *Check = ae::Assets.Elements["label_menu_new_hardcore_check"];
						CreateCharacter(Check->Text == "X");
						PlayClickSound();
					}
					else if(Clicked->Name == "button_menu_new_cancel") {
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
			case STATE_BROWSE: {
				if(Clicked->Name == "button_menu_browse_connect") {

					// Clicked cancel button
					if(!PlayState.Network->IsDisconnected()) {
						PlayState.Network->Disconnect(true);
						InitBrowseServers(false);
					}
					// Clicked connect button
					else
						ConnectToHost();

					PlayClickSound();
				}
				else if(Clicked->Name == "button_menu_browse_refresh") {
					RefreshServers();
				}
				else if(Clicked->Name == "button_menu_browse_back") {
					InitTitle(true);
					PlayClickSound();
				}
				else if(Clicked->Parent && Clicked->Parent->Name == "element_menu_browse_servers") {
					if(PlayState.Network->IsDisconnected() && (size_t)Clicked->Index < ConnectServers.size()) {

						// Set ip and port
						ae::_Element *Host = ae::Assets.Elements["textbox_menu_browse_host"];
						Host->Text = ConnectServers[Clicked->Index].IP + ":" + std::to_string(ConnectServers[Clicked->Index].Port);
						ValidateConnect();

						// Connect on double click
						if(DoubleClick)
							ConnectToHost();
					}
				}
			} break;
			case STATE_ACCOUNT: {
				if(Clicked->Name == "button_menu_account_login") {
					SendAccountInfo();
					PlayClickSound();
				}
				else if(Clicked->Name == "button_menu_account_create") {
					SendAccountInfo(true);
					PlayClickSound();
				}
				else if(Clicked->Name == "button_menu_account_back") {
					InitBrowseServers(true);
					PlayClickSound();
				}
			} break;
			case STATE_OPTIONS: {
				if(Clicked->Name == "button_menu_options_fullscreen") {
					SetFullscreen(!Config.Fullscreen);
					UpdateOptions();
				}
				else if(Clicked->Name == "button_menu_options_keybindings") {
					InitKeybindings();

					PlayClickSound();
				}
				else if(Clicked->Name == "button_menu_options_back") {
					if(FromInGame)
						InitInGame();
					else
						InitTitle(true);

					PlayClickSound();
				}
			} break;
			case STATE_KEYBINDINGS: {
				if(Clicked->Name == "button_menu_keybindings_default") {
					Config.LoadDefaultInputBindings(false);
					InitKeybindings();

					PlayClickSound();
				}
				else if(Clicked->Name == "button_menu_keybindings_back") {
					InitOptions();
					PlayClickSound();
				}
				else if(Clicked->Name == "primary") {
					ShowNewKey(Clicked, 0);
					PlayClickSound();
				}
				else if(Clicked->Name == "secondary") {
					ShowNewKey(Clicked, 1);
					PlayClickSound();
				}
			} break;
			case STATE_INGAME: {
				if(Clicked->Name == "button_menu_ingame_respawn") {
					ae::_Buffer Packet;
					Packet.Write<PacketType>(PacketType::WORLD_RESPAWN);
					PlayState.Network->SendPacket(Packet);
					InitPlay();
					PlayClickSound();
				}
				else if(Clicked->Name == "button_menu_ingame_resume") {
					InitPlay();
					PlayClickSound();
				}
				else if(Clicked->Name == "button_menu_ingame_options") {
					InitOptions();
					PlayClickSound();
				}
				else if(Clicked->Name == "button_menu_ingame_exit") {
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

		// Reload fonts
		ae::Assets.LoadFonts("tables/fonts.tsv");
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
		case STATE_BROWSE: {
			ae::Assets.Elements["element_menu_browse"]->Render();
			RenderBrowser();
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
		case STATE_BROWSE: {
			ae::_Element *Host = ae::Assets.Elements["textbox_menu_browse_host"];
			std::string IP;
			std::string Port;
			SplitHost(Host->Text, IP, Port);

			// Save connection information
			Config.LastHost = IP;
			Config.LastPort = Port;
			Config.Save();

			// Load account screen
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
		InitBrowseServers(true);

		ae::_Element *Label = ae::Assets.Elements["label_menu_browse_message"];
		Label->Color = ae::Assets.Colors["red"];
		if(BadGameVersion)
			Label->Text = "Wrong game version";
		else
			Label->Text = "Disconnected from server";
	}
}

// Handle packet
void _Menu::HandlePacket(ae::_Buffer &Buffer, PacketType Type) {
	switch(Type) {
		case PacketType::VERSION: {
			std::string Version(Buffer.ReadString());
			int BuildNumber = Buffer.Read<int>();
			BadGameVersion = false;
			if(Version != GAME_VERSION || (BuildNumber > 0 && BuildNumber != BUILD_NUMBER)) {
				BadGameVersion = true;
				PlayState.Network->Disconnect();
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
				int64_t Experience = Buffer.Read<int64_t>();

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
					CharacterSlots[Slot].Image->BaseSize = CharacterSlots[Slot].Image->Texture->Size;

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
			ae::_Element *Label = ae::Assets.Elements["label_menu_new_name"];
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

// Add a game server to the server list
void _Menu::AddConnectServer(_ConnectServer &ConnectServer) {
	ConnectServer.Ping = (SDL_GetPerformanceCounter() - PingTime) / (double)SDL_GetPerformanceFrequency();

	// Check for duplicates
	for(const auto &Server : ConnectServers) {
		if(Server.IP == ConnectServer.IP && Server.Port == ConnectServer.Port)
			return;
	}

	// Add server
	ConnectServers.push_back(ConnectServer);
}

// Set message for account screen
void _Menu::SetAccountMessage(const std::string &Message) {
	ae::_Element *Label = ae::Assets.Elements["label_menu_account_message"];
	Label->Text = Message;
	Label->Color = ae::Assets.Colors["red"];

	ae::_Element *Button = ae::Assets.Elements["button_menu_account_login"];
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

// Render server browser
void _Menu::RenderBrowser() {

	// Get ui elements
	ae::_Element *MainElement = ae::Assets.Elements["element_menu_browse_servers"];
	ae::_Element *FirstElement = ae::Assets.Elements["element_menu_browse_server_0"];
	ae::_Font *Font = ae::Assets.Fonts["hud_small"];

	// Set layout
	float SpacingY = FirstElement->Size.y;
	glm::vec2 DrawPosition = FirstElement->Bounds.Start + glm::vec2(10 * ae::_Element::GetUIScale(), FirstElement->Size.y / 2 + Font->MaxHeight - Font->MaxAbove + Font->MaxBelow);
	glm::vec2 Offset[3] = {
		{ 0, 0 },
		{ 575 * ae::_Element::GetUIScale(), 0 },
		{ 690 * ae::_Element::GetUIScale(), 0 },
		//{ 450 * ae::_Element::GetUIScale(), 0 },
		//{ 575 * ae::_Element::GetUIScale(), 0 },
		//{ 700 * ae::_Element::GetUIScale(), 0 },
	};

	// Draw header
	Font->DrawText("Server", DrawPosition + glm::vec2(0, -SpacingY) + Offset[0], ae::LEFT_BASELINE);
	Font->DrawText("Players", DrawPosition + glm::vec2(0, -SpacingY) + Offset[1], ae::LEFT_BASELINE);
	Font->DrawText("Hardcore", DrawPosition + glm::vec2(0, -SpacingY) + Offset[2], ae::LEFT_BASELINE);
	//Font->DrawText("Ping", DrawPosition + glm::vec2(0, -SpacingY) + Offset[3], ae::LEFT_BASELINE);

	// Draw servers
	size_t Count = 0;
	for(const auto &ConnectServer : ConnectServers) {
		std::string HardcoreText = ConnectServer.Hardcore ? "Yes" : "No";
		Font->DrawText(ConnectServer.IP + ":" + std::to_string(ConnectServer.Port), DrawPosition + Offset[0], ae::LEFT_BASELINE);
		Font->DrawText(std::to_string(ConnectServer.Players) + "/" + std::to_string(ConnectServer.MaxPlayers), DrawPosition + Offset[1], ae::LEFT_BASELINE);
		Font->DrawText(HardcoreText, DrawPosition + Offset[2], ae::LEFT_BASELINE);
		//Font->DrawText(std::to_string((int)(ConnectServer.Ping * 1000)) + "ms", DrawPosition + Offset[3], ae::LEFT_BASELINE);

		DrawPosition.y += SpacingY;

		// Check count
		Count++;
		if(Count >= MainElement->Children.size())
			break;
	};
}

// Cycle focused elements
void _Menu::FocusNextElement() {
	switch(State) {
		case STATE_ACCOUNT: {
			ae::_Element *Username = ae::Assets.Elements["textbox_menu_account_username"];
			ae::_Element *Password = ae::Assets.Elements["textbox_menu_account_password"];

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

// Get list of lan servers
void _Menu::RefreshServers() {
	ConnectServers.clear();

	// Send ping
	ae::_Buffer Packet;
	Packet.Write<PingType>(PingType::SERVER_INFO);
	PlayState.Network->SendPingPacket(Packet, ae::_NetworkAddress(ae::NETWORK_BROADCAST, DEFAULT_NETWORKPINGPORT));

	PingTime = SDL_GetPerformanceCounter();
}

// Set connect button enabled state
void _Menu::ValidateConnect() {
	ae::_Element *Button = ae::Assets.Elements["button_menu_browse_connect"];
	ae::_Element *Host = ae::Assets.Elements["textbox_menu_browse_host"];

	// Enable
	Button->SetEnabled(true);

	// Disable if blank
	if(Host->Text == "")
		Button->SetEnabled(false);
}

// Split host string into ip and port. Return default game port if none
void _Menu::SplitHost(const std::string &Host, std::string &IP, std::string &Port) {
	size_t ColonIndex = Host.find_first_of(':');
	if(ColonIndex != std::string::npos) {
		IP = Host.substr(0, ColonIndex);
		Port = Host.substr(ColonIndex + 1);
	}
	else {
		IP = Host;
		Port = std::to_string(DEFAULT_NETWORKPORT);
	}
}
