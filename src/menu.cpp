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
#include <menu.h>
#include <states/client.h>
#include <states/editor.h>
#include <network/clientnetwork.h>
#include <ui/element.h>
#include <ui/label.h>
#include <ui/button.h>
#include <ui/image.h>
#include <ui/style.h>
#include <ui/textbox.h>
#include <constants.h>
#include <input.h>
#include <actions.h>
#include <graphics.h>
#include <assets.h>
#include <config.h>
#include <framework.h>
#include <buffer.h>
#include <stats.h>
#include <packet.h>
#include <sstream>
#include <SDL_mouse.h>

#include <globals.h>

_Menu Menu;

const std::string InputBoxPrefix = "button_options_input_";
const std::string CharacterButtonPrefix = "button_characters_slot";
const std::string CharacterNamePrefix = "label_menu_characters_slot_name";
const std::string CharacterLevelPrefix = "label_menu_characters_slot_level";
const std::string CharacterImagePrefix = "image_menu_characters_slot";
const std::string NewCharacterPortraitPrefix = "button_newcharacter_portrait";

// Constructor
_Menu::_Menu() {
	State = STATE_NONE;
	CurrentLayout = nullptr;
	CharactersState = CHARACTERS_NONE;
	PreviousClickTimer = 0.0;
}

// Change the current layout
void _Menu::ChangeLayout(const std::string &ElementIdentifier) {
	if(CurrentLayout)
		CurrentLayout->SetVisible(false);

	CurrentLayout = Assets.Elements[ElementIdentifier];
	CurrentLayout->SetVisible(true);
}

// Initialize
void _Menu::InitTitle() {
	Assets.Labels["label_menu_title_version"]->Text = GAME_VERSION;
	Assets.Labels["label_menu_title_version"]->Visible = true;

	ChangeLayout("element_menu_title");

	State = STATE_TITLE;
}

// Init single player
void _Menu::InitCharacters() {
	ChangeLayout("element_menu_characters");

	CharactersState = CHARACTERS_NONE;
	State = STATE_CHARACTERS;
}

// In-game menu
void _Menu::InitInGame() {
	ChangeLayout("element_menu_ingame");

	//OldClientState.SendBusy(true);
	State = STATE_INGAME;
}

// Return to play
void _Menu::InitPlay() {
	if(CurrentLayout)
		CurrentLayout->SetVisible(false);
	CurrentLayout = nullptr;

	//OldClientState.SendBusy(false);
	State = STATE_NONE;
}

// Init new player popup
void _Menu::InitNewCharacter() {
	_Button *CreateButton = Assets.Buttons["button_newcharacter_create"];
	CreateButton->Enabled = false;

	_TextBox *Name = Assets.TextBoxes["textbox_newcharacter_name"];
	Name->Text = "";

	_Label *Label = Assets.Labels["label_menu_newcharacter_name"];
	Label->Text = "Name";
	Label->Color = COLOR_WHITE;

	LoadPortraitButtons();

	FocusedElement = Name;
	Name->ResetCursor();

	CurrentLayout = Assets.Elements["element_menu_new"];
	CurrentLayout->SetVisible(true);

	CharactersState = CHARACTERS_CREATE;
}

// Init connect screen
void _Menu::InitConnect(bool ConnectNow) {
	ClientState.Network->Disconnect();

	ChangeLayout("element_menu_connect");

	_TextBox *Host = Assets.TextBoxes["textbox_connect_host"];
	Host->Text = Config.LastHost;

	_TextBox *Port = Assets.TextBoxes["textbox_connect_port"];
	Port->Text = Config.LastPort;

	_Label *Label = Assets.Labels["label_menu_connect_message"];
	Label->Color = COLOR_WHITE;
	Label->Text = "";

	_Button *Button = Assets.Buttons["button_connect_connect"];
	Button->Enabled = true;

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
	Username->Text = DefaultUsername;

	_TextBox *Password = Assets.TextBoxes["textbox_account_password"];
	Password->Text = DefaultPassword;

	_Label *Label = Assets.Labels["label_menu_account_message"];
	Label->Color = COLOR_WHITE;
	Label->Text = "";

	_Button *Button = Assets.Buttons["button_account_login"];
	Button->Enabled = true;

	// Set focus
	FocusedElement = Username;
	Username->ResetCursor();

	State = STATE_ACCOUNT;
}

// Get the selected portrait id
int _Menu::GetSelectedPortraitID() {

	// Check for selected portrait
	_Element *PortraitsElement = Assets.Elements["element_menu_new_portraits"];
	for(auto &Element : PortraitsElement->Children) {
		_Button *Button = (_Button *)Element;
		if(Button->Checked)
			return (intptr_t)Button->UserData;
	}

	return -1;
}

// Get the selected character slot
int _Menu::GetSelectedCharacter() {
	int Index = 0;

	// Check for selected character
	_Element *CharactersElement = Assets.Elements["element_menu_characters"];
	for(auto &Element : CharactersElement->Children) {
		if(Element->Identifier.substr(0, CharacterButtonPrefix.size()) == CharacterButtonPrefix) {
			_Button *Button = (_Button *)Element;
			if(Button->Checked)
				return Index;

			Index++;
		}
	}

	return -1;
}

// Create character
void _Menu::CreateCharacter() {
	_TextBox *Name = Assets.TextBoxes["textbox_newcharacter_name"];
	if(Name->Text.length() == 0)
		return;

	int PortraitID = GetSelectedPortraitID();
	if(PortraitID == -1)
		return;

	// Send information
	_Buffer Packet;
	Packet.Write<char>(Packet::CREATECHARACTER_INFO);
	Packet.WriteString(Name->Text.c_str());
	Packet.Write<int32_t>(PortraitID);
	ClientState.Network->SendPacket(Packet);
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

	std::stringstream Buffer(Port->Text);
	uint16_t PortNumber;
	Buffer >> PortNumber;
	ClientState.HostAddress = Host->Text;
	ClientState.ConnectPort = PortNumber;
	ClientState.Connect(false);

	_Label *Label = Assets.Labels["label_menu_connect_message"];
	Label->Text = "Connecting...";

	_Button *Button = Assets.Buttons["button_connect_connect"];
	Button->Enabled = false;
}

// Send character to play
void _Menu::PlayCharacter(int Slot) {
	_Buffer Packet;
	Packet.Write<char>(Packet::CHARACTERS_PLAY);
	Packet.Write<char>(Slot);
	ClientState.Network->SendPacket(Packet);
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

	Label->Color = COLOR_WHITE;
	Label->Text = "Logging in...";

	_Button *Button = Assets.Buttons["button_account_login"];
	Button->Enabled = false;

	// Send information
	_Buffer Packet;
	Packet.Write<char>(Packet::ACCOUNT_LOGININFO);
	Packet.WriteBit(CreateAccount);
	Packet.WriteString(Username->Text.c_str());
	Packet.WriteString(Password->Text.c_str());
	ClientState.Network->SendPacket(Packet);
}

// Request character list from server
void _Menu::RequestCharacterList() {

	// Request character list
	_Buffer Packet;
	Packet.Write<char>(Packet::CHARACTERS_REQUEST);
	ClientState.Network->SendPacket(Packet);
}

// Load portraits
void _Menu::LoadPortraitButtons() {

	// Clear old children
	_Element *PortraitsElement = Assets.Elements["element_menu_new_portraits"];
	ClearPortraits();

	glm::ivec2 Offset(10, 0);
	size_t i = 0;

	// Iterate over portraits
	for(auto &PortraitIterator : ClientState.Stats->Portraits) {
		_Portrait &Portrait = PortraitIterator.second;

		// Create style
		_Style *Style = new _Style();
		Style->TextureColor = COLOR_WHITE;
		Style->Program = Assets.Programs["ortho_pos_uv"];
		Style->Texture = Portrait.Image;
		Style->UserCreated = true;

		// Add button
		_Button *Button = new _Button();
		Button->Identifier = NewCharacterPortraitPrefix;
		Button->Parent = PortraitsElement;
		Button->Offset = Offset;
		Button->Size = Portrait.Image->Size;
		Button->Alignment = LEFT_TOP;
		Button->Style = Style;
		Button->HoverStyle = Assets.Styles["style_menu_portrait_hover"];
		Button->UserData = (void *)(intptr_t)Portrait.ID;
		Button->UserCreated = true;
		PortraitsElement->Children.push_back(Button);

		// Update position
		Offset.x += Portrait.Image->Size.x + 10;
		if(Offset.x > PortraitsElement->Size.x - Portrait.Image->Size.x - 10) {
			Offset.y += Portrait.Image->Size.y + 10;
			Offset.x = 10;
		}

		i++;
	}

	PortraitsElement->CalculateBounds();
}

// Check new character screen for portrait and name
void _Menu::ValidateCreateCharacter() {
	bool NameValid = false;

	int PortraitID = GetSelectedPortraitID();

	// Check name length
	_Button *CreateButton = Assets.Buttons["button_newcharacter_create"];
	_TextBox *Name = Assets.TextBoxes["textbox_newcharacter_name"];
	if(Name->Text.length() > 0)
		NameValid = true;
	else
		FocusedElement = Name;

	// Enable button
	if(PortraitID != -1 && NameValid)
		CreateButton->Enabled = true;
	else
		CreateButton->Enabled = false;
}

// Validate characters ui elements
void _Menu::UpdateCharacterButtons() {
	_Button *DeleteButton = Assets.Buttons["button_characters_delete"];
	_Button *PlayButton = Assets.Buttons["button_characters_play"];
	DeleteButton->Enabled = false;
	PlayButton->Enabled = false;

	int SelectedSlot = GetSelectedCharacter();
	if(SelectedSlot != -1 && CharacterSlots[SelectedSlot].Used) {
		DeleteButton->Enabled = true;
		PlayButton->Enabled = true;
	}
}

// Shutdown
void _Menu::Close() {
	ClearPortraits();
}

// Handle actions
void _Menu::HandleAction(int InputType, int Action, int Value) {
	if(State == STATE_NONE)
		return;

	switch(State) {
		case STATE_INGAME:
			switch(Action) {
				case _Actions::MENU:
					Menu.InitPlay();
				break;
			}
		break;
		default:
		break;
	}
}

// Handle key event
void _Menu::KeyEvent(const _KeyEvent &KeyEvent) {
	if(State == STATE_NONE)
		return;

	switch(State) {
		case STATE_TITLE: {
			if(KeyEvent.Pressed && !KeyEvent.Repeat) {
				if(KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
					Framework.Done = true;
				else if(KeyEvent.Scancode == SDL_SCANCODE_RETURN) {
					ClientState.Connect(true);
				}
			}
		} break;
		case STATE_CHARACTERS: {
			if(CharactersState == CHARACTERS_NONE) {
				if(KeyEvent.Pressed && !KeyEvent.Repeat) {
					if(KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
						ClientState.Network->Disconnect();
					else if(KeyEvent.Scancode == SDL_SCANCODE_RETURN) {
						int SelectedCharacter = GetSelectedCharacter();
						if(SelectedCharacter == -1)
							SelectedCharacter = 0;

						if(CharacterSlots[SelectedCharacter].Used) {
							PlayCharacter(SelectedCharacter);
						}
					}
				}
			}
			else if(CharactersState == CHARACTERS_CREATE){
				ValidateCreateCharacter();

				if(KeyEvent.Pressed) {
					if(KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
						RequestCharacterList();
					else if(KeyEvent.Scancode == SDL_SCANCODE_RETURN)
						CreateCharacter();
				}
			}
		} break;
		case STATE_CONNECT: {
			if(KeyEvent.Pressed && !KeyEvent.Repeat) {
				if(KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
					ClientState.Network->Disconnect();
				else if(KeyEvent.Scancode == SDL_SCANCODE_RETURN)
					ConnectToHost();
				else if(KeyEvent.Scancode == SDL_SCANCODE_TAB)
					FocusNextElement(Input.ModKeyDown(KMOD_SHIFT));
			}
		} break;
		case STATE_ACCOUNT: {
			if(KeyEvent.Pressed && !KeyEvent.Repeat) {
				if(KeyEvent.Scancode == SDL_SCANCODE_ESCAPE)
					InitConnect();
				else if(KeyEvent.Scancode == SDL_SCANCODE_RETURN)
					SendAccountInfo();
				else if(KeyEvent.Scancode == SDL_SCANCODE_TAB)
					FocusNextElement(Input.ModKeyDown(KMOD_SHIFT));
			}
		} break;
		case STATE_INGAME: {
		} break;
		default:
		break;
	}
}

// Handle mouse event
void _Menu::MouseEvent(const _MouseEvent &MouseEvent) {
	if(State == STATE_NONE)
		return;

	if(!CurrentLayout)
		return;

	if(MouseEvent.Button == SDL_BUTTON_LEFT)
		CurrentLayout->HandleInput(MouseEvent.Pressed);

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
				if(Clicked->Identifier == "button_title_singleplayer") {
					ClientState.Connect(true);
				}
				else if(Clicked->Identifier == "button_title_multiplayer") {
					InitConnect();
				}
				else if(Clicked->Identifier == "button_title_mapeditor") {
					Framework.ChangeState(&EditorState);
				}
				else if(Clicked->Identifier == "button_title_exit") {
					Framework.Done = true;
				}
			} break;
			case STATE_CHARACTERS: {
				if(CharactersState == CHARACTERS_NONE) {

					if(Clicked->Identifier == "button_characters_delete") {
						int SelectedSlot = GetSelectedCharacter();
						if(SelectedSlot != -1 && CharacterSlots[SelectedSlot].Used) {
							_Buffer Packet;
							Packet.Write<char>(Packet::CHARACTERS_DELETE);
							Packet.Write<char>(SelectedSlot);
							ClientState.Network->SendPacket(Packet);
						}
					}
					else if(Clicked->Identifier == "button_characters_play") {
						int SelectedSlot = GetSelectedCharacter();
						if(SelectedSlot != -1 && CharacterSlots[SelectedSlot].Used) {
							PlayCharacter(SelectedSlot);
						}
					}
					else if(Clicked->Identifier == "button_characters_back") {
						ClientState.Network->Disconnect();
					}
					else if(Clicked->Identifier.substr(0, CharacterButtonPrefix.size()) == CharacterButtonPrefix) {

						// Deselect slots
						_Element *CharactersElement = Assets.Elements["element_menu_characters"];
						for(auto &Element : CharactersElement->Children) {
							if(Element->Identifier.substr(0, CharacterButtonPrefix.size()) == CharacterButtonPrefix) {
								_Button *Button = (_Button *)Element;
								Button->Checked = false;
							}
						}

						int SelectedSlot = (intptr_t)Clicked->UserData;
						if(CharacterSlots[SelectedSlot].Used) {
							CharacterSlots[SelectedSlot].Button->Checked = true;
						}
						else
							InitNewCharacter();

						UpdateCharacterButtons();

						if(DoubleClick && SelectedSlot != -1) {
							PlayCharacter(SelectedSlot);
						}
					}
				}
				else if(CharactersState == CHARACTERS_CREATE) {
					if(Clicked->Identifier == NewCharacterPortraitPrefix) {
						int SelectedID = (intptr_t)Clicked->UserData;

						// Unselect all portraits and select the clicked element
						for(auto &Element : Clicked->Parent->Children) {
							_Button *Button = (_Button *)Element;
							Button->Checked = false;
							if((intptr_t)Button->UserData == SelectedID) {
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
					}
					else if(Clicked->Identifier == "button_newcharacter_cancel") {
						RequestCharacterList();
					}
				}
			} break;
			case STATE_CONNECT: {
				if(Clicked->Identifier == "button_connect_connect") {
					ConnectToHost();
				}
				else if(Clicked->Identifier == "button_connect_back") {
					InitTitle();
				}
			} break;
			case STATE_ACCOUNT: {
				if(Clicked->Identifier == "button_account_login") {
					SendAccountInfo();
				}
				else if(Clicked->Identifier == "button_account_create") {
					SendAccountInfo(true);
				}
				else if(Clicked->Identifier == "button_account_back") {
					InitConnect();
				}
			} break;
			case STATE_INGAME: {
				if(Clicked->Identifier == "button_ingame_resume") {
					InitPlay();
				}
				else if(Clicked->Identifier == "button_ingame_disconnect") {
					ClientState.Network->Disconnect();
				}
			} break;
			default:
			break;
		}
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
			Assets.Labels["label_menu_title_version"]->Render();
		} break;
		case STATE_CHARACTERS: {
			Assets.Elements["element_menu_characters"]->Render();

			if(CharactersState == CHARACTERS_CREATE) {
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
void _Menu::HandleDisconnect() {
	switch(State) {
		case STATE_NONE:
		case STATE_CHARACTERS: {
			InitTitle();
		} break;
		case STATE_CONNECT: {
			InitConnect();

			_Label *Label = Assets.Labels["label_menu_connect_message"];
			Label->Color = COLOR_RED;
			Label->Text = "Disconnected from server";
		} break;
		default:
		break;
	}
}

// Handle packet
void _Menu::HandlePacket(_Buffer &Buffer, char PacketType) {
	switch(PacketType) {
		case Packet::VERSION: {
			std::string Version(Buffer.ReadString());
			if(Version != GAME_VERSION) {
				throw std::runtime_error("Wrong game version");
			}
		} break;
		case Packet::ACCOUNT_SUCCESS: {
			RequestCharacterList();
		} break;
		case Packet::CHARACTERS_LIST: {

			// Get count
			int CharacterCount = Buffer.Read<char>();

			// Reset character slots
			for(int i = 0; i < SAVE_COUNT; i++) {
				std::stringstream Buffer;

				// Set slot name
				Buffer << CharacterNamePrefix << i;
				CharacterSlots[i].Name = Assets.Labels[Buffer.str()];
				if(!CharacterSlots[i].Name)
					throw std::runtime_error("Can't find label: " + Buffer.str());
				Buffer.str("");

				// Set slot level
				Buffer << CharacterLevelPrefix << i;
				CharacterSlots[i].Level = Assets.Labels[Buffer.str()];
				if(!CharacterSlots[i].Level)
					throw std::runtime_error("Can't find label: " + Buffer.str());
				Buffer.str("");

				// Set image
				Buffer << CharacterImagePrefix << i;
				CharacterSlots[i].Image = Assets.Images[Buffer.str()];
				if(!CharacterSlots[i].Image)
					throw std::runtime_error("Can't find image: " + Buffer.str());
				Buffer.str("");

				// Assign button
				Buffer << CharacterButtonPrefix << i;
				CharacterSlots[i].Button = Assets.Buttons[Buffer.str()];
				CharacterSlots[i].Button->Checked = false;

				// Set state
				CharacterSlots[i].Name->Text = "Empty Slot";
				CharacterSlots[i].Level->Text = "";
				CharacterSlots[i].Image->Texture = nullptr;
				CharacterSlots[i].Image->Clickable = false;
				CharacterSlots[i].Used = false;
			}

			// Get characters
			for(int i = 0; i < CharacterCount; i++) {
				CharacterSlots[i].Name->Text = Buffer.ReadString();
				int32_t PortraitIndex = Buffer.Read<int32_t>();
				int32_t Experience = Buffer.Read<int32_t>();

				std::stringstream Buffer;
				Buffer << "Level " << ClientState.Stats->FindLevel(Experience)->Level;
				CharacterSlots[i].Level->Text = Buffer.str();
				CharacterSlots[i].Used = true;
				const _Texture *PortraitImage = ClientState.Stats->Portraits[PortraitIndex].Image;
				CharacterSlots[i].Image->Texture = PortraitImage;
			}

			// Disable ui buttons
			UpdateCharacterButtons();

			// Set state
			InitCharacters();
		} break;
		case Packet::CREATECHARACTER_SUCCESS: {

			// Close new character screen
			RequestCharacterList();
		} break;
		case Packet::CREATECHARACTER_INUSE: {
			_Label *Label = Assets.Labels["label_menu_newcharacter_name"];
			Label->Text = "Name in use";
			Label->Color = COLOR_RED;
		} break;
		case Packet::ACCOUNT_EXISTS: {
			SetAccountMessage("Account already exists");
		} break;
		case Packet::ACCOUNT_NOTFOUND: {
			SetAccountMessage("Username/password wrong");
		} break;
		case Packet::ACCOUNT_ALREADYLOGGEDIN: {
			SetAccountMessage("Account in use");
		} break;
	}
}

// Set message for account screen
void _Menu::SetAccountMessage(const std::string &Message) {
	_Label *Label = Assets.Labels["label_menu_account_message"];
	Label->Text = Message;
	Label->Color = COLOR_RED;

	_Button *Button = Assets.Buttons["button_account_login"];
	Button->Enabled = true;
}

// Cycle focused elements
void _Menu::FocusNextElement(bool ShiftDown) {
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

// Clear memory used by portraits
void _Menu::ClearPortraits() {
	std::vector<_Element *> &Children = Assets.Elements["element_menu_new_portraits"]->Children;
	for(size_t i = 0; i < Children.size(); i++) {
		delete Children[i]->Style;
		delete Children[i];
	}
	Children.clear();
}
