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
#include <assets.h>
#include <ui/style.h>
#include <ui/element.h>
#include <program.h>
#include <font.h>
#include <texture.h>
#include <utils.h>
#include <files.h>
#include <graphics.h>
#include <audio.h>
#include <constants.h>
#include <map>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <limits>
#include <tinyxml2.h>

_Assets Assets;

// Initialize
void _Assets::Init(bool IsServer) {
	LoadTextureDirectory(TEXTURES_BATTLE, IsServer);
	LoadTextureDirectory(TEXTURES_BUFFS, IsServer);
	LoadTextureDirectory(TEXTURES_BUILDS, IsServer);
	LoadTextureDirectory(TEXTURES_EDITOR, IsServer);
	LoadTextureDirectory(TEXTURES_HUD, IsServer);
	LoadTextureDirectory(TEXTURES_HUD_REPEAT, IsServer, true);
	LoadTextureDirectory(TEXTURES_INTERFACE, IsServer);
	LoadTextureDirectory(TEXTURES_ITEMS, IsServer);
	LoadTextureDirectory(TEXTURES_MAP, IsServer);
	LoadTextureDirectory(TEXTURES_MENU, IsServer);
	LoadTextureDirectory(TEXTURES_MONSTERS, IsServer);
	LoadTextureDirectory(TEXTURES_PORTRAITS, IsServer);
	LoadTextureDirectory(TEXTURES_MODELS, IsServer);
	LoadTextureDirectory(TEXTURES_SKILLS, IsServer);
	LoadTextureDirectory(TEXTURES_STATUS, IsServer);
	LoadLayers(ASSETS_LAYERS);
	if(!IsServer) {
		LoadPrograms(ASSETS_PROGRAMS);
		LoadFonts(ASSETS_FONTS);
		LoadColors(ASSETS_COLORS);
		LoadStyles(ASSETS_UI_STYLES);

		LoadUI(ASSETS_UI);
		/*
		LoadElements(ASSETS_UI_ELEMENTS);
		LoadImages(ASSETS_UI_IMAGES);
		LoadButtons(ASSETS_UI_BUTTONS);
		LoadTextBoxes(ASSETS_UI_TEXTBOXES);
		LoadLabels(ASSETS_UI_LABELS);
		ResolveElementParents();
		*/
		//SaveUI(ASSETS_UI);
		LoadSounds(ASSETS_SOUND_PATH);
		LoadMusic(ASSETS_MUSIC_PATH);
	}
}

// Shutdown
void _Assets::Close() {

	for(const auto &Program : Programs)
		delete Program.second;

	for(const auto &Shader : Shaders)
		delete Shader.second;

	for(const auto &Texture : Textures)
		delete Texture.second;

	for(const auto &Font : Fonts)
		delete Font.second;

	for(const auto &Sound : Sounds)
		delete Sound.second;

	for(const auto &Song : Music)
		delete Song.second;

	for(const auto &Style : Styles)
		delete Style.second;

	for(const auto &Element : Elements)
		delete Element.second;

	Fonts.clear();
	Layers.clear();
	Textures.clear();
	Styles.clear();
	Sounds.clear();
	Music.clear();
	Elements.clear();
}

// Loads the fonts
void _Assets::LoadFonts(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read the file
	while(!File.eof() && File.peek() != EOF) {

		// Read strings
		std::string Identifier;
		std::string FontFile;
		std::string ProgramIdentifier;
		std::getline(File, Identifier, '\t');
		std::getline(File, FontFile, '\t');
		std::getline(File, ProgramIdentifier, '\t');

		// Check for duplicates
		if(Fonts[Identifier])
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Identifier);

		// Find program
		if(Programs.find(ProgramIdentifier) == Programs.end())
		   throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find program: " + ProgramIdentifier);

		// Get size
		uint32_t Size;
		File >> Size;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Load font
		Fonts[Identifier] = new _Font(ASSETS_FONTS_PATH + FontFile, Programs[ProgramIdentifier], Size);
	}

	File.close();
}

// Load render layers
void _Assets::LoadLayers(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read the file
	while(!File.eof() && File.peek() != EOF) {
		std::string Identifier;
		std::getline(File, Identifier, '\t');

		// Get layer
		_Layer Layer;
		File >> Layer.Layer >> Layer.DepthTest >> Layer.DepthMask >> Layer.EditorOnly;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Set layer
		Layers[Identifier] = Layer;
	}

	File.close();
}

// Load shader programs
void _Assets::LoadPrograms(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read the file
	while(!File.eof() && File.peek() != EOF) {
		std::string Identifier;
		std::string VertexPath;
		std::string FragmentPath;
		std::getline(File, Identifier, '\t');
		std::getline(File, VertexPath, '\t');
		std::getline(File, FragmentPath, '\t');

		// Get attrib count
		int Attribs;
		File >> Attribs;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for duplicates
		if(Programs[Identifier])
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Identifier);

		// Load vertex shader
		if(Shaders.find(VertexPath) == Shaders.end())
			Shaders[VertexPath] = new _Shader(VertexPath, GL_VERTEX_SHADER);

		// Load fragment shader
		if(Shaders.find(FragmentPath) == Shaders.end())
			Shaders[FragmentPath] = new _Shader(FragmentPath, GL_FRAGMENT_SHADER);

		// Create program
		Programs[Identifier] = new _Program(Shaders[VertexPath], Shaders[FragmentPath], Attribs);
	}

	File.close();
}

// Load the UI xml file
void _Assets::LoadUI(const std::string &Path) {

	// Load file
	tinyxml2::XMLDocument Document;
	if(Document.LoadFile(Path.c_str()) != tinyxml2::XML_SUCCESS)
		throw std::runtime_error("Error loading: " + Path);

	// Load elements
	tinyxml2::XMLElement *ChildNode = Document.FirstChildElement();
	Graphics.Element = new _Element(ChildNode, nullptr);
	Graphics.Element->Alignment = LEFT_TOP;
	Graphics.Element->Type = _Element::ELEMENT;
	Graphics.Element->Visible = true;
	Graphics.Element->Size = Graphics.CurrentSize;
	Graphics.Element->CalculateBounds();
}

// Loads the color table
void _Assets::LoadColors(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Add default color
	glm::vec4 Color(1.0f);
	Colors[""] = Color;

	// Read table
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier;
		std::getline(File, Identifier, '\t');

		File >> Color.r >> Color.g >> Color.b >> Color.a;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for duplicates
		if(Colors.find(Identifier) != Colors.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Identifier);

		Colors[Identifier] = Color;
	}

	File.close();
}

// Load a directory full of textures
void _Assets::LoadTextureDirectory(const std::string &Path, bool IsServer, bool Repeat, bool MipMaps) {

	// Get files
	_Files Files(TEXTURES_PATH + Path);

	// Load textures
	for(const auto &File : Files.Nodes) {
		std::string Identifier = Path + File;
		if(!Assets.Textures[Identifier])
			Assets.Textures[Identifier] = new _Texture(Identifier, IsServer, Repeat, MipMaps);
	}
}

// Load game audio
void _Assets::LoadSounds(const std::string &Path) {

	// Get files
	_Files Files(Path);

	// Load audio
	for(const auto &File : Files.Nodes) {
		if(!Assets.Sounds[File])
			Assets.Sounds[File] = Audio.LoadSound(Path + File);
	}
}

// Load music files
void _Assets::LoadMusic(const std::string &Path) {

	// Get files
	_Files Files(Path);

	// Load audio
	for(const auto &File : Files.Nodes) {
		if(!Assets.Music[File])
			Assets.Music[File] = Audio.LoadMusic(Path + File);
	}
}

// Loads the styles
void _Assets::LoadStyles(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier;
		std::string BackgroundColorIdentifier;
		std::string BorderColorIdentifier;
		std::string ProgramIdentifier;
		std::string TextureIdentifier;
		std::string TextureColorIdentifier;
		std::getline(File, Identifier, '\t');
		std::getline(File, BackgroundColorIdentifier, '\t');
		std::getline(File, BorderColorIdentifier, '\t');
		std::getline(File, ProgramIdentifier, '\t');
		std::getline(File, TextureIdentifier, '\t');
		std::getline(File, TextureColorIdentifier, '\t');

		// Check for color
		if(BackgroundColorIdentifier != "" && Colors.find(BackgroundColorIdentifier) == Colors.end())
			throw std::runtime_error("Unable to find color: " + BackgroundColorIdentifier + " for style: " + Identifier);

		// Check for color
		if(BorderColorIdentifier != "" && Colors.find(BorderColorIdentifier) == Colors.end())
			throw std::runtime_error("Unable to find color: " + BorderColorIdentifier + " for style: " + Identifier);

		// Find program
		if(Programs.find(ProgramIdentifier) == Programs.end())
		   throw std::runtime_error("Cannot find program: " + ProgramIdentifier);

		bool Stretch;
		File >> Stretch;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Get colors
		glm::vec4 BackgroundColor = Colors[BackgroundColorIdentifier];
		glm::vec4 BorderColor = Colors[BorderColorIdentifier];
		glm::vec4 TextureColor = Colors[TextureColorIdentifier];

		// Get textures
		const _Texture *Texture = Textures[TextureIdentifier];

		// Create style
		_Style *Style = new _Style();
		Style->Identifier = Identifier;
		Style->HasBackgroundColor = BackgroundColorIdentifier != "";
		Style->HasBorderColor = BorderColorIdentifier != "";
		Style->BackgroundColor = BackgroundColor;
		Style->BorderColor = BorderColor;
		Style->Program = Programs[ProgramIdentifier];
		Style->Texture = Texture;
		Style->Atlas = nullptr;
		Style->TextureColor = TextureColor;
		Style->Stretch = Stretch;

		// Check for duplicates
		if(Styles.find(Identifier) != Styles.end())
			throw std::runtime_error("Duplicate style identifier: " + Identifier);

		Styles[Identifier] = Style;
	}

	File.close();
}

// Load elements table
void _Assets::LoadElements(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str());
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		// Read strings
		std::string Identifier;
		std::string ParentIdentifier;
		std::string StyleIdentifier;
		std::getline(File, Identifier, '\t');
		std::getline(File, ParentIdentifier, '\t');
		std::getline(File, StyleIdentifier, '\t');

		// Check for duplicates
		if(Elements.find(Identifier) != Elements.end())
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

		// Read attributes
		glm::vec2 Offset, Size;
		_Alignment Alignment;
		bool MaskOutside;
		bool Clickable;
		intptr_t UserData;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> MaskOutside >> Clickable >> UserData;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for style
		if(StyleIdentifier != "" && Styles.find(StyleIdentifier) == Styles.end())
			throw std::runtime_error("Unable to find style: " + StyleIdentifier + " for element: " + Identifier);

		// Create
		_Element *Element = new _Element();
		Element->Type = _Element::ELEMENT;
		Element->Identifier = Identifier;
		Element->ParentIdentifier = ParentIdentifier;
		Element->Offset = Offset;
		Element->Size = Size;
		Element->Alignment = Alignment;
		Element->Style = Styles[StyleIdentifier];
		Element->MaskOutside = MaskOutside;
		Element->Clickable = Clickable;
		Element->UserData = (void *)UserData;

		// Add to map
		Element->GlobalID = Elements.size();
		Elements[Identifier] = Element;
	}

	File.close();
}

// Load labels table
void _Assets::LoadLabels(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str());
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier;
		std::string ParentIdentifier;
		std::string FontIdentifier;
		std::string ColorIdentifier;
		std::string Text;
		std::getline(File, Identifier, '\t');
		std::getline(File, ParentIdentifier, '\t');
		std::getline(File, FontIdentifier, '\t');
		std::getline(File, ColorIdentifier, '\t');
		std::getline(File, Text, '\t');

		// Check for duplicates
		if(Elements.find(Identifier) != Elements.end())
			throw std::runtime_error("Duplicate label: " + Identifier);

		// Get font
		const _Font *Font = Fonts[FontIdentifier];
		if(!Font)
			throw std::runtime_error("Unable to find font: " + FontIdentifier);

		// Read attributes
		glm::vec2 Offset, Size;
		_Alignment Alignment;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Create
		_Element *Label = new _Element();
		Label->Type = _Element::LABEL;
		Label->Identifier = Identifier;
		Label->ParentIdentifier = ParentIdentifier;
		Label->Offset = Offset;
		Label->Size = Size;
		Label->Alignment = Alignment;
		Label->Font = Font;
		Label->FontName = FontIdentifier;
		Label->Text = Text;
		Label->Color = Colors[ColorIdentifier];
		Label->ColorName = ColorIdentifier;

		// Add to map
		Label->GlobalID = Elements.size();
		Elements[Identifier] = Label;
	}

	File.close();
}

// Load image table
void _Assets::LoadImages(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str());
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier;
		std::string ParentIdentifier;
		std::string TextureIdentifier;
		std::string ColorIdentifier;
		std::getline(File, Identifier, '\t');
		std::getline(File, ParentIdentifier, '\t');
		std::getline(File, TextureIdentifier, '\t');
		std::getline(File, ColorIdentifier, '\t');

		// Check for duplicates
		if(Elements.find(Identifier) != Elements.end())
			throw std::runtime_error("Duplicate image: " + Identifier);

		// Read attributes
		glm::vec2 Offset, Size;
		_Alignment Alignment;
		int Stretch;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> Stretch;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for texture
		if(TextureIdentifier != "" && Textures.find(TextureIdentifier) == Textures.end())
			throw std::runtime_error("Unable to find texture: " + TextureIdentifier + " for image: " + Identifier);

		// Get texture
		const _Texture *Texture = Textures[TextureIdentifier];

		// Create
		_Element *Image = new _Element();
		Image->Type = _Element::IMAGE;
		Image->Identifier = Identifier;
		Image->ParentIdentifier = ParentIdentifier;
		Image->Offset = Offset;
		Image->Size = Size;
		Image->Alignment = Alignment;
		Image->Texture = Texture;
		Image->Color = Colors[ColorIdentifier];
		Image->ColorName = ColorIdentifier;
		Image->Stretch = Stretch;
		Image->Clickable = false;

		// Add to map
		Image->GlobalID = Elements.size();
		Elements[Identifier] = Image;
	}

	File.close();
}

// Load buttons
void _Assets::LoadButtons(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str());
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read file
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier;
		std::string ParentIdentifier;
		std::string StyleIdentifier;
		std::string DisabledStyleIdentifier;
		std::string HoverStyleIdentifier;
		std::getline(File, Identifier, '\t');
		std::getline(File, ParentIdentifier, '\t');
		std::getline(File, StyleIdentifier, '\t');
		std::getline(File, DisabledStyleIdentifier, '\t');
		std::getline(File, HoverStyleIdentifier, '\t');

		// Check for duplicates
		if(Elements.find(Identifier) != Elements.end())
			throw std::runtime_error("Duplicate button: " + Identifier);

		// Check for style
		if(StyleIdentifier != "" && Styles.find(StyleIdentifier) == Styles.end())
			throw std::runtime_error("Unable to find style: " + StyleIdentifier + " for button: " + Identifier);

		// Check for disabled style
		if(DisabledStyleIdentifier != "" && Styles.find(DisabledStyleIdentifier) == Styles.end())
			throw std::runtime_error("Unable to find style: " + DisabledStyleIdentifier + " for button: " + Identifier);

		// Check for hover style
		if(HoverStyleIdentifier != "" && Styles.find(HoverStyleIdentifier) == Styles.end())
			throw std::runtime_error("Unable to find style: " + HoverStyleIdentifier + " for button: " + Identifier);

		// Read attributes
		glm::vec2 Offset, Size;
		_Alignment Alignment;
		intptr_t UserData;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> UserData;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Create
		_Element *Button = new _Element();
		Button->Type = _Element::BUTTON;
		Button->Identifier = Identifier;
		Button->ParentIdentifier = ParentIdentifier;
		Button->Offset = Offset;
		Button->Size = Size;
		Button->Alignment = Alignment;
		Button->Style = Styles[StyleIdentifier];
		Button->DisabledStyle = Styles[DisabledStyleIdentifier];
		Button->HoverStyle = Styles[HoverStyleIdentifier];
		Button->UserData = (void *)UserData;

		// Add to map
		Button->GlobalID = Elements.size();
		Elements[Identifier] = Button;
	}

	File.close();
}

// Load textboxes
void _Assets::LoadTextBoxes(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str());
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Read the file
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier;
		std::string ParentIdentifier;
		std::string StyleIdentifier;
		std::string FontIdentifier;
		std::getline(File, Identifier, '\t');
		std::getline(File, ParentIdentifier, '\t');
		std::getline(File, StyleIdentifier, '\t');
		std::getline(File, FontIdentifier, '\t');

		// Check for duplicates
		if(Elements.find(Identifier) != Elements.end())
			throw std::runtime_error("Duplicate textbox: " + Identifier);

		// Check for style
		if(StyleIdentifier != "" && Styles.find(StyleIdentifier) == Styles.end())
			throw std::runtime_error("Unable to find style: " + StyleIdentifier + " for textbox: " + Identifier);

		// Get font
		const _Font *Font = Fonts[FontIdentifier];
		if(!Font)
			throw std::runtime_error("Unable to find font: " + FontIdentifier);

		// Get style
		_Style *Style = Styles[StyleIdentifier];

		// Read attributes
		glm::vec2 Offset, Size;
		_Alignment Alignment;
		size_t MaxLength;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> MaxLength;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Create
		_Element *TextBox = new _Element();
		TextBox->Type = _Element::TEXTBOX;
		TextBox->Identifier = Identifier;
		TextBox->ParentIdentifier = ParentIdentifier;
		TextBox->Offset = Offset;
		TextBox->Size = Size;
		TextBox->Alignment = Alignment;
		TextBox->Style = Style;
		TextBox->Font = Font;
		TextBox->FontName = FontIdentifier;
		TextBox->MaxLength = MaxLength;

		// Add to map
		TextBox->GlobalID = Elements.size();
		Elements[Identifier] = TextBox;
	}

	File.close();
}

// Turn ParentIdentifier into Parent pointers
void _Assets::ResolveElementParents() {

	// Sort elements by global id
	std::map<size_t, _Element *> SortedElements;
	for(const auto &Iterator : Elements) {
		_Element *Element = Iterator.second;

		SortedElements[Element->GlobalID] = Element;
	}

	// Iterate through sorted elements
	for(const auto &Iterator : SortedElements) {
		_Element *Element = Iterator.second;

		// Set parent pointer
		if(Element->ParentIdentifier != "") {
			if(Elements.find(Element->ParentIdentifier) == Elements.end())
				throw std::runtime_error("Cannot find parent element: " + Element->ParentIdentifier);

			Element->Parent = Elements[Element->ParentIdentifier];
		}
		else
			Element->Parent = Graphics.Element;

		Element->Parent->Children.push_back(Element);
		Element->CalculateBounds();
	}
}

// Save UI to xml
void _Assets::SaveUI(const std::string &Path) {

	// Create doc
	tinyxml2::XMLDocument Document;
	Document.InsertEndChild(Document.NewDeclaration());

	// Serialize root ui element
	Graphics.Element->SerializeElement(Document, nullptr);

	// Write file
	Document.SaveFile(Path.c_str());
}
