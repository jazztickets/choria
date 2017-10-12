/******************************************************************************
* Copyright (c) 2017 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*******************************************************************************/
#pragma once

// Libraries
#include <glm/vec4.hpp>
#include <glm/vec2.hpp>
#include <unordered_map>
#include <string>

// Forward Declarations
class _Font;
class _Element;
class _Texture;
class _Mesh;
class _Program;
class _Shader;
class _Sound;
class _Music;
struct _Style;

// Animation template struct
struct _AnimationTemplate {
	std::string Identifier;
	const _Texture *Texture;
	int FramesPerLine;
	glm::vec2 TextureScale;
	glm::ivec2 FrameSize;
	int StartFrame;
	int EndFrame;
	int DefaultFrame;
	int RepeatType;
};

struct _Layer {
	_Layer() : Layer(0), DepthTest(0), DepthMask(0), EditorOnly(0) { }
	int Layer;
	int DepthTest;
	int DepthMask;
	int EditorOnly;
};

// Classes
class _Assets {

	public:

		void Init();
		void Close();

		void LoadColors(const std::string &Path);
		void LoadTextureDirectory(const std::string &Path, bool IsServer, bool Repeat=false, bool MipMaps=false);
		void LoadSounds(const std::string &Path);
		void LoadMusic(const std::string &Path);
		void LoadMeshDirectory(const std::string &Path);
		void LoadAnimations(const std::string &Path, bool IsServer);
		void LoadFonts(const std::string &Path);
		void LoadLayers(const std::string &Path);
		void LoadPrograms(const std::string &Path);
		void LoadStyles(const std::string &Path);
		void LoadUI(const std::string &Path);
		void SaveUI(const std::string &Path);

		std::unordered_map<std::string, const _Font *> Fonts;
		std::unordered_map<std::string, _Layer> Layers;
		std::unordered_map<std::string, const _Texture *> Textures;
		std::unordered_map<std::string, const _Mesh *> Meshes;
		std::unordered_map<std::string, _Program *> Programs;
		std::unordered_map<std::string, const _AnimationTemplate *> AnimationTemplates;
		std::unordered_map<std::string, glm::vec4> Colors;
		std::unordered_map<std::string, _Style *> Styles;
		std::unordered_map<std::string, _Sound *> Sounds;
		std::unordered_map<std::string, _Music *> Music;
		std::unordered_map<std::string, _Element *> Elements;

	private:

		std::unordered_map<std::string, const _Shader *> Shaders;
};

extern _Assets Assets;
