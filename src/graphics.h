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
#pragma once

// Libraries
#include <opengl.h>
#include <IEventReceiver.h>
#include <IGUIFont.h>
#include <IGUISkin.h>
#include <IGUIStaticText.h>
#include <texture.h>
#include <EDriverTypes.h>
#include <SColor.h>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <SDL_video.h>
#include <SDL_mouse.h>
#include <string>

#include <ITexture.h>

// Forward Declarations
class _Texture;
class _Program;
class _Element;
struct _Bounds;

enum VertexBufferType {
	VBO_NONE,
	VBO_CIRCLE,
	VBO_QUAD,
	VBO_ATLAS,
	VBO_CUBE,
	VBO_COUNT
};

struct _WindowSettings {
	_WindowSettings() : Size(0), Position(0), Fullscreen(false), Vsync(false) { }
	std::string WindowTitle;
	glm::ivec2 Size;
	glm::ivec2 Position;
	bool Fullscreen;
	bool Vsync;
};

// Classes
class _Graphics {

	public:

		enum AlignType {
			ALIGN_LEFT,
			ALIGN_CENTER,
			ALIGN_RIGHT,
		};

		enum FontType {
			FONT_7,
			FONT_8,
			FONT_10,
			FONT_14,
			FONT_18,
			FONT_COUNT,
		};

		enum ImageType {
			IMAGE_EMPTYSLOT,
			IMAGE_SELECTEDSLOT,
			IMAGE_HEALTH,
			IMAGE_HEALTHMAX,
			IMAGE_MANA,
			IMAGE_MANAMAX,
			IMAGE_EXPERIENCE,
			IMAGE_EXPERIENCEMAX,
			IMAGE_BLACK,
			IMAGE_GOLD,
			IMAGE_PVP,
			IMAGE_MENULOGO,
			IMAGE_MENUBLANKSLOT,
			IMAGE_MENUSELECTED,
			IMAGE_WORLDBUSY,
			IMAGE_WORLDTRADE,
			IMAGE_WORLDFIGHTING,
			IMAGE_BATTLESLOTLEFT,
			IMAGE_BATTLESLOTRIGHT,
			IMAGE_BATTLECOINS,
			IMAGE_BATTLEEXPERIENCE,
			IMAGE_BATTLECHEST,
			IMAGE_BATTLETARGET,
			IMAGE_INVENTORY,
			IMAGE_VENDOR,
			IMAGE_TRADE,
			IMAGE_TRADER,
			IMAGE_PLUS,
			IMAGE_MINUS,
			IMAGE_COUNT,
		};

		void Init(const _WindowSettings &WindowSettings);
		void Close();

		// Fonts
		void SetFont(int TType);
		irr::gui::IGUIFont *GetFont(int TType) const { return Fonts[TType]; }

		// Positions
		irr::core::recti GetCenteredRect(int TPositionX, int TPositionY, int TWidth, int THeight);
		irr::core::recti GetRect(int TPositionX, int TPositionY, int TWidth, int THeight);

		// Text
		irr::gui::IGUIStaticText *AddText(const char *TText, int TPositionX, int TPositionY, AlignType TAlignType=ALIGN_LEFT, irr::gui::IGUIElement *TParent=nullptr);
		void RenderText(const char *TText, int TPositionX, int TPositionY, AlignType TAlignType=ALIGN_LEFT, const irr::video::SColor &TColor=irr::video::SColor(255, 255, 255, 255));

		// Images
		void DrawCenteredImage(const _Texture *TTexture, int PositionX, int PositionY, const irr::video::SColor &Color=irr::video::SColor(255, 255, 255, 255));
		void DrawImage(ImageType TType, int TPositionX, int TPositionY, const irr::video::SColor &TColor=irr::video::SColor(255, 255, 255, 255));
		void DrawBar(ImageType TType, int TPositionX, int TPositionY, float TPercent, int TWidth, int THeight);
		void DrawBackground(ImageType TType, int TPositionX, int TPositionY, int TWidth, int THeight, const irr::video::SColor &TColor=irr::video::SColor(255, 255, 255, 255));
		_Texture *GetImage(ImageType TType) { return Images[TType]; }


		void ToggleFullScreen(const glm::ivec2 &WindowSize, const glm::ivec2 &FullscreenSize);
		void ShowCursor(int Type);
		void BuildVertexBuffers();

		void SetStaticUniforms();
		void ChangeViewport(const glm::ivec2 &Size);
		void ChangeWindowSize(const glm::ivec2 &Size);
		void Setup2D();
		void Setup3D();

		void FadeScreen(float Amount);
		void DrawImage(const _Bounds &Bounds, const _Texture *Texture, bool Stretch=false);
		void DrawAtlas(const _Bounds &Bounds, const _Texture *Texture, const glm::vec4 &TextureCoords);
		void DrawRectangle(const _Bounds &Bounds, bool Filled=false);
		void DrawMask(const _Bounds &Bounds);

		void DrawSprite(const glm::vec3 &Position, const _Texture *Texture, float Rotation=0.0f, const glm::vec2 Scale=glm::vec2(1.0f));
		void DrawTile(const glm::vec2 &Start, const glm::vec2 &End, float Z, const _Texture *Texture);
		void DrawCube(const glm::vec3 &Start, const glm::vec3 &Scale, const _Texture *Texture);
		void DrawRectangle(const glm::vec2 &Start, const glm::vec2 &End, bool Filled=false);
		void DrawCircle(const glm::vec3 &Position, float Radius);

		void SetDepthMask(bool Value);
		void EnableStencilTest();
		void DisableStencilTest();
		void ClearScreen();
		void Flip(double FrameTime);

		GLuint CreateVBO(float *Triangles, GLuint Size, GLenum Type);
		void UpdateVBOTextureCoords(int VBO, float *Data);
		void SetVBO(GLuint Type);
		void EnableAttribs(int AttribLevel);

		void SetColor(const glm::vec4 &Color);
		void SetTextureID(GLuint TextureID);
		void SetVertexBufferID(GLuint VertexBufferID);
		void SetProgram(const _Program *Program);
		void SetDepthTest(bool DepthTest);

		void DirtyState();

		// State
		_Element *Element;
		glm::ivec2 WindowSize;
		glm::ivec2 ViewportSize;
		glm::mat4 Ortho;
		float AspectRatio;

		GLfloat Anisotropy;
		GLuint VertexBuffer[VBO_COUNT];

		int FramesPerSecond;

	private:

		void SetupOpenGL();

		// Data structures
		bool Enabled;
		SDL_Window *Window;
		SDL_GLContext Context;

		// State changes
		GLuint LastVertexBufferID;
		GLuint LastTextureID;
		int LastAttribLevel;
		glm::vec4 LastColor;
		const _Program *LastProgram;
		bool LastDepthTest;

		// Benchmarking
		double FrameRateTimer;
		int FrameCount;


		// Graphics state
		irr::video::SColor ClearColor;
		int Width, Height;

		// Theme
		irr::gui::IGUISkin *Skin;

		// Fonts
		irr::gui::IGUIFont *Fonts[FONT_COUNT];
		int CurrentFont;

		// Images
		_Texture *Images[IMAGE_COUNT];

};

extern _Graphics Graphics;
