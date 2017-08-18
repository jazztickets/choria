/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2017  Alan Witkowski
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
#include <ae/assets.h>
#include <ae/ui.h>
#include <ae/program.h>
#include <ae/font.h>
#include <ae/texture.h>
#include <ae/files.h>
#include <ae/graphics.h>
#include <ae/audio.h>
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
	LoadTextureDirectory("textures/battle/", IsServer);
	LoadTextureDirectory("textures/buffs/", IsServer);
	LoadTextureDirectory("textures/builds/", IsServer);
	LoadTextureDirectory("textures/editor/", IsServer);
	LoadTextureDirectory("textures/hud/", IsServer);
	LoadTextureDirectory("textures/hud_repeat/", IsServer, true);
	LoadTextureDirectory("textures/interface/", IsServer);
	LoadTextureDirectory("textures/items/", IsServer);
	LoadTextureDirectory("textures/map/", IsServer);
	LoadTextureDirectory("textures/menu/", IsServer);
	LoadTextureDirectory("textures/monsters/", IsServer);
	LoadTextureDirectory("textures/portraits/", IsServer);
	LoadTextureDirectory("textures/models/", IsServer);
	LoadTextureDirectory("textures/skills/", IsServer);
	LoadTextureDirectory("textures/status/", IsServer);
	LoadLayers("tables/layers.tsv");
	if(!IsServer) {
		LoadPrograms("tables/programs.tsv");
		LoadFonts("tables/fonts.tsv");
		LoadColors("tables/colors.tsv");
		LoadStyles("tables/styles.tsv");
		LoadSounds("sounds/");
		LoadMusic("music/");
		LoadUI("tables/ui.xml");
		//SaveUI("tables/ui.xml");
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
		std::string Name;
		std::string FontFile;
		std::string ProgramName;
		std::getline(File, Name, '\t');
		std::getline(File, FontFile, '\t');
		std::getline(File, ProgramName, '\t');

		// Check for duplicates
		if(Fonts[Name])
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		// Find program
		if(Programs.find(ProgramName) == Programs.end())
		   throw std::runtime_error(std::string(__FUNCTION__) + " - Cannot find program: " + ProgramName);

		// Get size
		uint32_t Size;
		File >> Size;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Load font
		Fonts[Name] = new _Font(Name, FontFile, Programs[ProgramName], Size);
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
		std::string Name;
		std::getline(File, Name, '\t');

		// Get layer
		_Layer Layer;
		File >> Layer.Layer >> Layer.DepthTest >> Layer.DepthMask >> Layer.EditorOnly;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Set layer
		Layers[Name] = Layer;
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
		std::string Name;
		std::string VertexPath;
		std::string FragmentPath;
		std::getline(File, Name, '\t');
		std::getline(File, VertexPath, '\t');
		std::getline(File, FragmentPath, '\t');

		// Get attrib count
		int Attribs;
		File >> Attribs;

		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for duplicates
		if(Programs[Name])
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		// Load vertex shader
		if(Shaders.find(VertexPath) == Shaders.end())
			Shaders[VertexPath] = new _Shader(VertexPath, GL_VERTEX_SHADER);

		// Load fragment shader
		if(Shaders.find(FragmentPath) == Shaders.end())
			Shaders[FragmentPath] = new _Shader(FragmentPath, GL_FRAGMENT_SHADER);

		// Create program
		Programs[Name] = new _Program(Name, Shaders[VertexPath], Shaders[FragmentPath], Attribs);
	}

	File.close();
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

		std::string Name;
		std::getline(File, Name, '\t');

		File >> Color.r >> Color.g >> Color.b >> Color.a;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Check for duplicates
		if(Colors.find(Name) != Colors.end())
			throw std::runtime_error(std::string(__FUNCTION__) + " - Duplicate entry: " + Name);

		Colors[Name] = Color;
	}

	File.close();
}

// Load a directory full of textures
void _Assets::LoadTextureDirectory(const std::string &Path, bool IsServer, bool Repeat, bool MipMaps) {

	// Get files
	_Files Files(Path);

	// Load textures
	for(const auto &File : Files.Nodes) {
		std::string Name = Path + File;
		if(!Assets.Textures[Name])
			Assets.Textures[Name] = new _Texture(Name, IsServer, Repeat, MipMaps);
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

		std::string Name;
		std::string BackgroundColorName;
		std::string BorderColorName;
		std::string ProgramName;
		std::string TextureName;
		std::string TextureColorName;
		std::getline(File, Name, '\t');
		std::getline(File, BackgroundColorName, '\t');
		std::getline(File, BorderColorName, '\t');
		std::getline(File, ProgramName, '\t');
		std::getline(File, TextureName, '\t');
		std::getline(File, TextureColorName, '\t');

		// Check for color
		if(BackgroundColorName != "" && Colors.find(BackgroundColorName) == Colors.end())
			throw std::runtime_error("Unable to find color: " + BackgroundColorName + " for style: " + Name);

		// Check for color
		if(BorderColorName != "" && Colors.find(BorderColorName) == Colors.end())
			throw std::runtime_error("Unable to find color: " + BorderColorName + " for style: " + Name);

		// Find program
		if(Programs.find(ProgramName) == Programs.end())
		   throw std::runtime_error("Cannot find program: " + ProgramName + " for style: " + Name);

		// Check for texture
		if(TextureName != "" && Textures.find(TextureName) == Textures.end())
			throw std::runtime_error("Unable to find texture: " + TextureName + " for style: " + Name);

		bool Stretch;
		File >> Stretch;
		File.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

		// Get colors
		glm::vec4 BackgroundColor = Colors[BackgroundColorName];
		glm::vec4 BorderColor = Colors[BorderColorName];
		glm::vec4 TextureColor = Colors[TextureColorName];

		// Get textures
		const _Texture *Texture = Textures[TextureName];

		// Create style
		_Style *Style = new _Style();
		Style->Name = Name;
		Style->HasBackgroundColor = BackgroundColorName != "";
		Style->HasBorderColor = BorderColorName != "";
		Style->BackgroundColor = BackgroundColor;
		Style->BorderColor = BorderColor;
		Style->Program = Programs[ProgramName];
		Style->Texture = Texture;
		Style->TextureColor = TextureColor;
		Style->Stretch = Stretch;

		// Check for duplicates
		if(Styles.find(Name) != Styles.end())
			throw std::runtime_error("Duplicate style Name: " + Name);

		Styles[Name] = Style;
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
	Graphics.Element->Active = true;
	Graphics.Element->Size = Graphics.CurrentSize;
	Graphics.Element->CalculateBounds();
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