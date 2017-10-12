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
#include <ae/opengl.h>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <SDL_video.h>
#include <SDL_mouse.h>
#include <string>

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

enum CursorType {
	CURSOR_MAIN,
	CURSOR_CROSS,
	CURSOR_COUNT,
};

// Classes
class _Graphics {

	public:

		void Init(const _WindowSettings &WindowSettings);
		void Close();

		bool SetFullscreen(bool Fullscreen);
		void ShowCursor(int Type);
		void BuildVertexBuffers();

		void SetStaticUniforms();
		void ChangeViewport(const glm::ivec2 &Size);
		void ChangeWindowSize(const glm::ivec2 &Size);
		void Setup2D();
		void Setup3D();

		void DrawCenteredImage(const glm::ivec2 &Position, const _Texture *Texture, const glm::vec4 &Color=glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		void DrawImage(const _Bounds &Bounds, const _Texture *Texture, bool Stretch=false);
		void DrawAtlas(const _Bounds &Bounds, const _Texture *Texture, const glm::vec4 &TextureCoords);
		void DrawSprite(const glm::vec3 &Position, const _Texture *Texture, float Rotation=0.0f, const glm::vec2 Scale=glm::vec2(1.0f));
		void DrawCube(const glm::vec3 &Start, const glm::vec3 &Scale, const _Texture *Texture);
		void DrawRectangle(const _Bounds &Bounds, bool Filled=false);
		void DrawRectangle(const glm::vec2 &Start, const glm::vec2 &End, bool Filled=false);
		void DrawMask(const _Bounds &Bounds);
		void DrawCircle(const glm::vec3 &Position, float Radius);

		void SetDepthMask(bool Value);
		void SetScissor(const _Bounds &Bounds);
		void EnableStencilTest();
		void DisableStencilTest();
		void EnableScissorTest();
		void DisableScissorTest();

		void FadeScreen(float Amount);
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
		glm::ivec2 CurrentSize;
		glm::ivec2 ViewportSize;
		glm::mat4 Ortho;
		float AspectRatio;

		GLfloat Anisotropy;
		GLuint VertexBuffer[VBO_COUNT];

		int FramesPerSecond;

	private:

		void SetupOpenGL();

		// Attributes
		GLuint CircleVertices;

		// Data structures
		bool Enabled;
		SDL_Window *Window;
		SDL_GLContext Context;
		SDL_Cursor *Cursors[CURSOR_COUNT];

		// State
		glm::ivec2 WindowSize;
		glm::ivec2 FullscreenSize;

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
};

extern _Graphics Graphics;
