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
#include <assets.h>
#include <ui/style.h>
#include <ui/element.h>
#include <ui/label.h>
#include <ui/button.h>
#include <ui/image.h>
#include <ui/textbox.h>
#include <program.h>
#include <font.h>
#include <texture.h>
#include <utils.h>
#include <files.h>
#include <graphics.h>
#include <constants.h>
#include <map>
#include <stdexcept>
#include <iostream>
#include <limits>

_Assets Assets;

// Initialize
void _Assets::Init(bool IsServer) {
	LoadTextureDirectory(TEXTURES_BATTLE, IsServer);
	LoadTextureDirectory(TEXTURES_EDITOR, IsServer);
	LoadTextureDirectory(TEXTURES_HUD, IsServer);
	LoadTextureDirectory(TEXTURES_HUD_REPEAT, IsServer, true);
	LoadTextureDirectory(TEXTURES_INTERFACE, IsServer);
	LoadTextureDirectory(TEXTURES_ITEMS, IsServer);
	LoadTextureDirectory(TEXTURES_MAP, IsServer, true);
	LoadTextureDirectory(TEXTURES_MENU, IsServer);
	LoadTextureDirectory(TEXTURES_MONSTERS, IsServer);
	LoadTextureDirectory(TEXTURES_PLAYERS, IsServer);
	LoadTextureDirectory(TEXTURES_PORTRAITS, IsServer);
	LoadTextureDirectory(TEXTURES_SKILLS, IsServer);
	LoadTextureDirectory(TEXTURES_WORLD, IsServer);
	LoadLayers(ASSETS_LAYERS);
	if(!IsServer) {
		LoadPrograms(ASSETS_PROGRAMS);
		LoadFonts(ASSETS_FONTS);
		LoadColors(ASSETS_COLORS);
		LoadStyles(ASSETS_UI_STYLES);
		LoadElements(ASSETS_UI_ELEMENTS);
		LoadImages(ASSETS_UI_IMAGES);
		LoadButtons(ASSETS_UI_BUTTONS);
		LoadTextBoxes(ASSETS_UI_TEXTBOXES);
		LoadLabels(ASSETS_UI_LABELS);

		ResolveElementParents();
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

	for(const auto &Style : Styles)
		delete Style.second;

	for(const auto &Element : AllElements)
		delete Element.second;

	Fonts.clear();
	Layers.clear();
	Textures.clear();
	Styles.clear();

	Elements.clear();
	Labels.clear();
	Images.clear();
	Buttons.clear();
	TextBoxes.clear();
	AllElements.clear();
}

// Loads the strings
void _Assets::LoadStrings(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Ignore the first line
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read the file
	while(!File.eof() && File.peek() != EOF) {

		// Load data
		std::string Identifier = GetTSVText(File);
		std::string Text = GetTSVText(File);

		// Check for duplicates
		if(Strings[Identifier] != "")
			throw std::runtime_error("LoadStringTable - Duplicate entry: " + Identifier);

		Strings[Identifier] = Text;
	}

	File.close();
}

// Loads the fonts
void _Assets::LoadFonts(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Ignore the first line
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read the file
	while(!File.eof() && File.peek() != EOF) {
		std::string Identifier = GetTSVText(File);
		std::string FontFile = GetTSVText(File);
		std::string ProgramIdentifier = GetTSVText(File);

		// Check for duplicates
		if(Fonts[Identifier])
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Identifier);

		// Find program
		if(Programs.find(ProgramIdentifier) == Programs.end())
		   throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find program: " + ProgramIdentifier);

		// Get size
		int Size;
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

	// Ignore the first line
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read the file
	while(!File.eof() && File.peek() != EOF) {
		std::string Identifier = GetTSVText(File);

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

	// Ignore the first line
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Read the file
	while(!File.eof() && File.peek() != EOF) {
		std::string Identifier = GetTSVText(File);
		std::string VertexPath = GetTSVText(File);
		std::string FragmentPath = GetTSVText(File);

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
		Programs[Identifier]= new _Program(Shaders[VertexPath], Shaders[FragmentPath], Attribs);
	}

	File.close();
}

// Loads the color table
void _Assets::LoadColors(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Read the file
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	// Add default color
	glm::vec4 Color(1.0f);
	Colors[""] = Color;

	// Read table
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
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

// Loads the styles
void _Assets::LoadStyles(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str(), std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Read the file
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		std::string BackgroundColorIdentifier = GetTSVText(File);
		std::string BorderColorIdentifier = GetTSVText(File);
		std::string ProgramIdentifier = GetTSVText(File);
		std::string TextureIdentifier = GetTSVText(File);
		std::string TextureColorIdentifier = GetTSVText(File);

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

	// Read the file
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		std::string ParentIdentifier = GetTSVText(File);
		std::string StyleIdentifier = GetTSVText(File);

		// Check for duplicates
		if(Elements.find(Identifier) != Elements.end())
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

		if(AllElements.find(Identifier) != AllElements.end())
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

		// Read attributes
		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		bool MaskOutside;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> MaskOutside;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for style
		if(StyleIdentifier != "" && Styles.find(StyleIdentifier) == Styles.end())
			throw std::runtime_error("Unable to find style: " + StyleIdentifier + " for element: " + Identifier);

		// Create
		_Element *Element = new _Element();
		Element->Identifier = Identifier;
		Element->ParentIdentifier = ParentIdentifier;
		Element->Offset = Offset;
		Element->Size = Size;
		Element->Alignment = Alignment;
		Element->Style = Styles[StyleIdentifier];
		Element->MaskOutside = MaskOutside;

		// Add to map
		Element->GlobalID = AllElements.size();
		Elements[Identifier] = Element;
		AllElements[Identifier] = Element;
	}

	File.close();
}

// Load labels table
void _Assets::LoadLabels(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str());
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Read the file
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		std::string ParentIdentifier = GetTSVText(File);
		std::string FontIdentifier = GetTSVText(File);
		std::string ColorIdentifier = GetTSVText(File);
		std::string Text = GetTSVText(File);

		// Check for duplicates
		if(Labels.find(Identifier) != Labels.end())
			throw std::runtime_error("Duplicate label: " + Identifier);

		if(AllElements.find(Identifier) != AllElements.end())
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

		// Get font
		const _Font *Font = Fonts[FontIdentifier];
		if(!Font)
			throw std::runtime_error("Unable to find font: " + FontIdentifier);

		// Read attributes
		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Create
		_Label *Label = new _Label();
		Label->Identifier = Identifier;
		Label->ParentIdentifier = ParentIdentifier;
		Label->Offset = Offset;
		Label->Size = Size;
		Label->Alignment = Alignment;
		Label->Font = Font;
		Label->Text = Text;
		Label->Color = Colors[ColorIdentifier];

		// Add to map
		Label->GlobalID = AllElements.size();
		Labels[Identifier] = Label;
		AllElements[Identifier] = Label;
	}

	File.close();
}

// Load image table
void _Assets::LoadImages(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str());
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Read the file
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		std::string ParentIdentifier = GetTSVText(File);
		std::string TextureIdentifier = GetTSVText(File);
		std::string ColorIdentifier = GetTSVText(File);

		// Check for duplicates
		if(Images.find(Identifier) != Images.end())
			throw std::runtime_error("Duplicate image: " + Identifier);

		if(AllElements.find(Identifier) != AllElements.end())
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

		// Read attributes
		glm::ivec2 Offset, Size;
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
		_Image *Image = new _Image();
		Image->Identifier = Identifier;
		Image->ParentIdentifier = ParentIdentifier;
		Image->Offset = Offset;
		Image->Size = Size;
		Image->Alignment = Alignment;
		Image->Texture = Texture;
		Image->Color = Colors[ColorIdentifier];
		Image->Stretch = Stretch;

		// Add to map
		Image->GlobalID = AllElements.size();
		Images[Identifier] = Image;
		AllElements[Identifier] = Image;
	}

	File.close();
}

// Load buttons
void _Assets::LoadButtons(const std::string &Path) {

	// Load file
	std::ifstream File(Path.c_str());
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Read the file
	File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	while(!File.eof() && File.peek() != EOF) {

		std::string Identifier = GetTSVText(File);
		std::string ParentIdentifier = GetTSVText(File);
		std::string StyleIdentifier = GetTSVText(File);
		std::string DisabledStyleIdentifier = GetTSVText(File);
		std::string HoverStyleIdentifier = GetTSVText(File);

		// Check for duplicates
		if(Buttons.find(Identifier) != Buttons.end())
			throw std::runtime_error("Duplicate button: " + Identifier);

		if(AllElements.find(Identifier) != AllElements.end())
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

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
		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		intptr_t UserData;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> UserData;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Create
		_Button *Button = new _Button();
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
		Button->GlobalID = AllElements.size();
		Buttons[Identifier] = Button;
		AllElements[Identifier] = Button;
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

		std::string Identifier = GetTSVText(File);
		std::string ParentIdentifier = GetTSVText(File);
		std::string StyleIdentifier = GetTSVText(File);
		std::string FontIdentifier = GetTSVText(File);

		// Check for duplicates
		if(TextBoxes.find(Identifier) != TextBoxes.end())
			throw std::runtime_error("Duplicate textbox: " + Identifier);

		if(AllElements.find(Identifier) != AllElements.end())
			throw std::runtime_error("Duplicate element identifier: " + Identifier);

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
		glm::ivec2 Offset, Size;
		_Alignment Alignment;
		int MaxLength;
		File >> Offset.x >> Offset.y >> Size.x >> Size.y >> Alignment.Horizontal >> Alignment.Vertical >> MaxLength;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Create
		_TextBox *TextBox = new _TextBox();
		TextBox->Identifier = Identifier;
		TextBox->ParentIdentifier = ParentIdentifier;
		TextBox->Offset = Offset;
		TextBox->Size = Size;
		TextBox->Alignment = Alignment;
		TextBox->Style = Style;
		TextBox->Font = Font;
		TextBox->MaxLength = MaxLength;

		// Add to map
		TextBox->GlobalID = AllElements.size();
		TextBoxes[Identifier] = TextBox;
		AllElements[Identifier] = TextBox;
	}

	File.close();
}

// Turn ParentIdentifier into Parent pointers
void _Assets::ResolveElementParents() {

	// Sort elements by global id
	std::map<size_t, _Element *> SortedElements;
	for(const auto &Iterator : AllElements) {
		_Element *Element = Iterator.second;

		SortedElements[Element->GlobalID] = Element;
	}

	// Iterate through sorted elements
	for(const auto &Iterator : SortedElements) {
		_Element *Element = Iterator.second;

		// Set parent pointer
		if(Element->ParentIdentifier != "") {
			if(AllElements.find(Element->ParentIdentifier) == AllElements.end())
				throw std::runtime_error("Cannot find parent element: " + Element->ParentIdentifier);

			Element->Parent = AllElements[Element->ParentIdentifier];
		}
		else
			Element->Parent = Graphics.Element;

		Element->Parent->Children.push_back(Element);
		Element->CalculateBounds();
	}
}
