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
#include <graphics.h>
#include <globals.h>
#include <assets.h>
#include <program.h>
#include <constants.h>
#include <texture.h>
#include <ui/element.h>
#include <SDL.h>
#include <pnglite/pnglite.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

_Graphics Graphics;

// Initializes the graphics system
void _Graphics::Init(const _WindowSettings &WindowSettings) {
	this->WindowSize = WindowSettings.Size;
	this->Anisotropy = 0;
	FramesPerSecond = 0;
	FrameCount = 0;
	FrameRateTimer = 0;
	Context = 0;
	Window = 0;
	Enabled = true;
	DirtyState();

	// Set root element
	Element = new _Element();
	Element->Visible = true;
	Element->Size = WindowSize;
	Element->CalculateBounds();

	// Set video flags
	Uint32 VideoFlags = SDL_WINDOW_OPENGL;
	if(WindowSettings.Fullscreen)
		VideoFlags |= SDL_WINDOW_FULLSCREEN;

	// Set opengl attributes
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

	// Set video mode
	Window = SDL_CreateWindow(WindowSettings.WindowTitle.c_str(), WindowSettings.Position.x, WindowSettings.Position.y, WindowSettings.Size.x, WindowSettings.Size.y, VideoFlags);
	if(Window == nullptr)
		throw std::runtime_error("SDL_CreateWindow failed");

	// Set up opengl context
	Context = SDL_GL_CreateContext(Window);
	if(Context == nullptr)
		throw std::runtime_error("SDL_GL_CreateContext failed");

	InitGLFunctions();

	int MajorVersion, MinorVersion;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &MajorVersion);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &MinorVersion);

	// Set vsync
	SDL_GL_SetSwapInterval(WindowSettings.Vsync);

	// Set up OpenGL
	SetupOpenGL();

	// Setup viewport
	ChangeViewport(WindowSize);
	png_init(0, 0);
}

// Closes the graphics system
void _Graphics::Close() {

	delete Element;

	// Close opengl context
	if(Context) {
		for(int i = 1; i < VBO_COUNT; i++)
			glDeleteBuffers(1, &VertexBuffer[i]);

		SDL_GL_DeleteContext(Context);
		Context = nullptr;
	}

	// Close window
	if(Window) {
		SDL_DestroyWindow(Window);
		Window = nullptr;
	}
}

// Change the viewport
void _Graphics::ChangeViewport(const glm::ivec2 &Size) {
	ViewportSize = Size;

	// Calculate aspect ratio
	AspectRatio = (float)ViewportSize.x / ViewportSize.y;
}

// Change window and viewport size
void _Graphics::ChangeWindowSize(const glm::ivec2 &Size) {

	// Keep viewport difference the same
	glm::ivec2 ViewportDifference = WindowSize - ViewportSize;

	WindowSize = Size;
	ChangeViewport(Size - ViewportDifference);

	// Update shaders
	Ortho = glm::ortho(0.0f, (float)WindowSize.x, (float)WindowSize.y, 0.0f, -1.0f, 1.0f);
	SetStaticUniforms();

	// Update UI elements
	Element->Size = Size;
	Element->CalculateBounds();

	// Update actual window
	SDL_SetWindowSize(Window, Size.x, Size.y);
}

// Toggle fullscreen
void _Graphics::ToggleFullScreen(const glm::ivec2 &WindowSize, const glm::ivec2 &FullscreenSize) {

	if(SDL_GetWindowFlags(Window) & SDL_WINDOW_FULLSCREEN) {
		Graphics.ChangeWindowSize(WindowSize);
	}
	else {
		Graphics.ChangeWindowSize(FullscreenSize);
	}

	if(SDL_SetWindowFullscreen(Window, SDL_GetWindowFlags(Window) ^ SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
		// failed
	}
}

// Sets up OpenGL
void _Graphics::SetupOpenGL() {

	// Anisotropic filtering
	if(SDL_GL_ExtensionSupported("GL_EXT_texture_filter_anisotropic")) {
		GLfloat MaxAnisotropy;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &MaxAnisotropy);

		if(Anisotropy > (int)MaxAnisotropy)
			Anisotropy = (int)MaxAnisotropy;
	}

	// Default state
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnableVertexAttribArray(0);

	// Set ortho matrix
	Ortho = glm::ortho(0.0f, (float)WindowSize.x, (float)WindowSize.y, 0.0f, -1.0f, 1.0f);

	// Build vertex buffers
	BuildVertexBuffers();

	// Clear screen
	ClearScreen();
	Flip(0);
}

// Initialize
void _Graphics::SetStaticUniforms() {
	SetProgram(Assets.Programs["ortho_pos"]);
	glUniformMatrix4fv(Assets.Programs["ortho_pos"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Ortho));
	SetProgram(Assets.Programs["ortho_pos_uv"]);
	glUniformMatrix4fv(Assets.Programs["ortho_pos_uv"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Ortho));
	SetProgram(Assets.Programs["text"]);
	glUniformMatrix4fv(Assets.Programs["text"]->ViewProjectionTransformID, 1, GL_FALSE, glm::value_ptr(Ortho));
}

// Builds the vertex buffer objects
void _Graphics::BuildVertexBuffers() {
	VertexBuffer[VBO_NONE] = 0;

	// Circle
	{
		float Triangles[GRAPHICS_CIRCLE_VERTICES * 2];

		// Get vertices
		for(int i = 0; i < GRAPHICS_CIRCLE_VERTICES; i++) {
			float Radians = ((float)i / GRAPHICS_CIRCLE_VERTICES) * (MATH_PI * 2);
			Triangles[i * 2] = std::cos(Radians);
			Triangles[i * 2 + 1] = std::sin(Radians);
		}

		VertexBuffer[VBO_CIRCLE] = CreateVBO(Triangles, sizeof(Triangles), GL_STATIC_DRAW);
	}

	// Textured 2D Quad
	{
		// Vertex data for quad
		float Triangles[] = {
			-0.5f,  0.5f,
			 0.5f,  0.5f,
			-0.5f, -0.5f,
			 0.5f, -0.5f,
			 0.0f, 1.0f,
			 1.0f, 1.0f,
			 0.0f, 0.0f,
			 1.0f, 0.0f,
		};

		VertexBuffer[VBO_QUAD] = CreateVBO(Triangles, sizeof(Triangles), GL_STATIC_DRAW);
	}

	// Dynamic vbo for drawing animations
	{

		float Triangles[] = {
			-0.5f, 0.5f,
			 0.5f,  0.5f,
			-0.5f, -0.5f,
			 0.5f, -0.5f,
			 0.0f,  0.0f,
			 0.0f,  0.0f,
			 0.0f,  0.0f,
			 0.0f,  0.0f,
		};

		VertexBuffer[VBO_ATLAS] = CreateVBO(Triangles, sizeof(Triangles), GL_DYNAMIC_DRAW);
	}

	// Cube
	{

		float Triangles[] = {

			// Top
			1.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
			0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,  1.0f,
			1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,
			0.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,

			// Front
			1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
			0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f,
			1.0f,  1.0f,  0.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,
			0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,

			// Left
			0.0f,  1.0f,  1.0f,  1.0f,  0.0f, -1.0f,  0.0f,  0.0f,
			0.0f,  0.0f,  1.0f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
			0.0f,  1.0f,  0.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,
			0.0f,  0.0f,  0.0f,  0.0f,  1.0f, -1.0f,  0.0f,  0.0f,

			// Back
			0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f, -1.0f,  0.0f,
			1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f, -1.0f,  0.0f,
			0.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f, -1.0f,  0.0f,
			1.0f,  0.0f,  0.0f,  0.0f,  1.0f,  0.0f, -1.0f,  0.0f,

			// Right
			1.0f,  0.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,
			1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
			1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,
			1.0f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,

		};

		VertexBuffer[VBO_CUBE] = CreateVBO(Triangles, sizeof(Triangles), GL_STATIC_DRAW);
	}
}

// Create vertex buffer and return id
GLuint _Graphics::CreateVBO(float *Triangles, GLuint Size, GLenum Type) {

	// Create buffer
	GLuint BufferID;
	glGenBuffers(1, &BufferID);
	glBindBuffer(GL_ARRAY_BUFFER, BufferID);
	glBufferData(GL_ARRAY_BUFFER, Size, Triangles, Type);

	return BufferID;
}

// Clears the screen
void _Graphics::ClearScreen() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

// Set up modelview matrix
void _Graphics::Setup3D() {
	glViewport(0, WindowSize.y - ViewportSize.y, ViewportSize.x, ViewportSize.y);
	Graphics.SetDepthTest(true);
}

// Sets up the projection matrix for drawing 2D objects
void _Graphics::Setup2D() {

	// Set viewport
	glViewport(0, 0, WindowSize.x, WindowSize.y);
	Graphics.SetDepthTest(false);
}

// Fade the screen
void _Graphics::FadeScreen(float Amount) {
	Graphics.SetProgram(Assets.Programs["ortho_pos"]);
	Graphics.SetVBO(VBO_NONE);

	Graphics.SetColor(glm::vec4(0.0f, 0.0f, 0.0f, Amount));
	DrawRectangle(glm::vec2(0, 0), WindowSize, true);
}

// Draw image centered
void _Graphics::DrawCenteredImage(const glm::ivec2 &Position, const _Texture *Texture, const glm::vec4 &Color) {
	Graphics.SetColor(Color);

	_Bounds Bounds;
	Bounds.Start = Position - Texture->Size / 2;
	Bounds.End = Position + Texture->Size / 2;
	DrawImage(Bounds, Texture, false);
}

// Draw image in screen space
void _Graphics::DrawImage(const _Bounds &Bounds, const _Texture *Texture, bool Stretch) {
	SetTextureID(Texture->ID);

	// Get s and t
	float S, T;
	if(Stretch) {
		S = T = 1;
	}
	else {
		S = (Bounds.End.x - Bounds.Start.x) / (float)(Texture->Size.x);
		T = (Bounds.End.y - Bounds.Start.y) / (float)(Texture->Size.y);
	}

	// Vertex data for quad
	float Vertices[] = {
		(float)Bounds.End.x,   (float)Bounds.Start.y, S,    0.0f,
		(float)Bounds.Start.x, (float)Bounds.Start.y, 0.0f, 0.0f,
		(float)Bounds.End.x,   (float)Bounds.End.y,   S,    T,
		(float)Bounds.Start.x, (float)Bounds.End.y,   0.0f, T,
	};
	if(LastAttribLevel != 2)
		throw std::runtime_error("bad");

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, &Vertices[0]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, &Vertices[2]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Draw image from a texture atlas
void _Graphics::DrawAtlas(const _Bounds &Bounds, const _Texture *Texture, const glm::vec4 &TextureCoords) {
	SetTextureID(Texture->ID);

	// Vertex data for quad
	float Vertices[] = {
		(float)Bounds.End.x,   (float)Bounds.Start.y, TextureCoords[0], TextureCoords[1],
		(float)Bounds.Start.x, (float)Bounds.Start.y, TextureCoords[2], TextureCoords[1],
		(float)Bounds.End.x,   (float)Bounds.End.y,   TextureCoords[0], TextureCoords[3],
		(float)Bounds.Start.x, (float)Bounds.End.y,   TextureCoords[2], TextureCoords[3],
	};
	if(LastAttribLevel != 2)
		throw std::runtime_error("bad");

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, &Vertices[0]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, &Vertices[2]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Draw rectangle in screen space
void _Graphics::DrawRectangle(const _Bounds &Bounds, bool Filled) {
	DrawRectangle(glm::vec2(Bounds.Start.x, Bounds.Start.y), glm::vec2(Bounds.End.x, Bounds.End.y), Filled);
}

// Draw stencil mask
void _Graphics::DrawMask(const _Bounds &Bounds) {
	if(LastAttribLevel != 1)
		throw std::runtime_error(std::string(__FUNCTION__) + " - LastAttribLevel mismatch");

	// Enable stencil
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	glStencilMask(0x01);

	// Write 1 to stencil buffer
	glStencilFunc(GL_ALWAYS, 0x01, 0x01);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

	float Vertices[] = {
		(float)Bounds.Start.x, (float)Bounds.End.y,
		(float)Bounds.End.x,   (float)Bounds.End.y,
		(float)Bounds.Start.x, (float)Bounds.Start.y,
		(float)Bounds.End.x,   (float)Bounds.Start.y,
	};

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, Vertices);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Then draw element only where stencil is 1
	glStencilFunc(GL_EQUAL, 0x01, 0x01);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
}

// Draw 3d sprite
void _Graphics::DrawSprite(const glm::vec3 &Position, const _Texture *Texture, float Rotation, const glm::vec2 Scale) {
	if(LastAttribLevel != 2)
		throw std::runtime_error(std::string(__FUNCTION__) + " - LastAttribLevel mismatch");

	SetTextureID(Texture->ID);

	Rotation = glm::radians(Rotation);

	glm::mat4 ModelTransform;
	ModelTransform = glm::translate(glm::mat4(1.0f), Position);
	if(Rotation != 0.0f)
		ModelTransform = glm::rotate(ModelTransform, Rotation, glm::vec3(0, 0, 1));

	ModelTransform = glm::scale(ModelTransform, glm::vec3(Scale, 0.0f));

	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(ModelTransform));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Draw 3d wall
void _Graphics::DrawCube(const glm::vec3 &Start, const glm::vec3 &Scale, const _Texture *Texture) {
	if(LastAttribLevel != 3)
		throw std::runtime_error(std::string(__FUNCTION__) + " - LastAttribLevel mismatch");

	SetTextureID(Texture->ID);

	glm::mat4 ModelTransform;
	ModelTransform = glm::translate(glm::mat4(1.0f), Start);
	ModelTransform = glm::scale(ModelTransform, Scale);

	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(ModelTransform));

	// Change texture
	glMatrixMode(GL_TEXTURE);

	// Draw top
	glScalef(Scale.x, Scale.y, 1);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glLoadIdentity();

	// Draw front
	glScalef(Scale.x, Scale.z, 1);
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
	glLoadIdentity();

	// Draw left
	glScalef(Scale.y, Scale.z, 1);
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
	glLoadIdentity();

	// Draw back
	glScalef(Scale.x, Scale.z, 1);
	glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
	glLoadIdentity();

	// Draw right
	glScalef(Scale.y, Scale.z, 1);
	glDrawArrays(GL_TRIANGLE_STRIP, 16, 4);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
}

// Draw quad with repeated textures
void _Graphics::DrawTile(const glm::vec2 &Start, const glm::vec2 &End, float Z, const _Texture *Texture) {
	if(LastAttribLevel != 2)
		throw std::runtime_error(std::string(__FUNCTION__) + " - LastAttribLevel mismatch");

	SetTextureID(Texture->ID);
	SetColor(COLOR_WHITE);

	// Get textureID and properties
	float Width = End.x - Start.x;
	float Height = End.y - Start.y;

	glm::mat4 ModelTransform;
	ModelTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, Z));
	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(ModelTransform));

	// Vertex data for quad
	float Vertices[] = {
		Start.x, End.y,   0.0f,  Height,
		End.x,   End.y,   Width, Height,
		Start.x, Start.y, 0.0f,  0.0f,
		End.x,   Start.y, Width, 0.0f,
	};

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, &Vertices[0]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, &Vertices[2]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Draw rectangle
void _Graphics::DrawRectangle(const glm::vec2 &Start, const glm::vec2 &End, bool Filled) {
	if(LastAttribLevel != 1)
		throw std::runtime_error(std::string(__FUNCTION__) + " - LastAttribLevel mismatch");

	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

	if(Filled) {
		float Vertices[] = {
			Start.x, End.y,
			End.x,   End.y,
			Start.x, Start.y,
			End.x,   Start.y,
		};

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, Vertices);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	else {
		float Vertices[] = {
			Start.x, Start.y,
			End.x,   Start.y,
			End.x,   End.y,
			Start.x, End.y,
		};

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, Vertices);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
	}
}

// Draw circle
void _Graphics::DrawCircle(const glm::vec3 &Position, float Radius) {
	if(LastAttribLevel != 1)
		throw std::runtime_error(std::string(__FUNCTION__) + " - LastAttribLevel mismatch");

	glm::mat4 ModelTransform;
	ModelTransform = glm::translate(glm::mat4(1.0f), Position);
	ModelTransform = glm::scale(ModelTransform, glm::vec3(Radius, Radius, 0.0f));
	glUniformMatrix4fv(LastProgram->ModelTransformID, 1, GL_FALSE, glm::value_ptr(ModelTransform));

	glDrawArrays(GL_LINE_LOOP, 0, GRAPHICS_CIRCLE_VERTICES);
}

// Draws the frame
void _Graphics::Flip(double FrameTime) {
	if(!Enabled)
		return;

	// Swap buffers
	SDL_GL_SwapWindow(Window);

	// Clear screen
	ClearScreen();

	// Update frame counter
	FrameCount++;
	FrameRateTimer += FrameTime;
	if(FrameRateTimer >= 1.0) {
		FramesPerSecond = FrameCount;
		FrameCount = 0;
		FrameRateTimer -= 1.0;
	}
}

// Update texture coordinates for a vbo
void _Graphics::UpdateVBOTextureCoords(int VBO, float *Data) {
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[VBO]);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 8, sizeof(float) * 8, (GLvoid *)(&Data[0]));
}

// Enable state for VBO
void _Graphics::SetVBO(GLuint VBO) {
	if(LastVertexBufferID == VBO)
		return;

	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer[VBO]);

	switch(VBO) {
		case VBO_CUBE:
			EnableAttribs(3);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, 0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid *)(sizeof(glm::vec3)));
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid *)(sizeof(glm::vec3) + sizeof(glm::vec2)));
		break;
		case VBO_QUAD:
		case VBO_ATLAS:
			EnableAttribs(2);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (GLvoid *)(sizeof(float) * 8));
		break;
		case VBO_CIRCLE:
			EnableAttribs(1);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
		break;
	}

	LastVertexBufferID = VBO;
}

// Enable vertex attrib arrays
void _Graphics::EnableAttribs(int AttribLevel) {
	if(AttribLevel == LastAttribLevel)
		return;

	if(AttribLevel < LastAttribLevel) {
		switch(LastAttribLevel) {
			case 1:
			break;
			case 2:
				glDisableVertexAttribArray(1);
			break;
			case 3:
				glDisableVertexAttribArray(1);
				glDisableVertexAttribArray(2);
			break;
		}
	}

	switch(AttribLevel) {
		case 1:
		break;
		case 2:
			glEnableVertexAttribArray(1);
		break;
		case 3:
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
		break;
	}

	LastAttribLevel = AttribLevel;
}

// Set opengl color
void _Graphics::SetColor(const glm::vec4 &Color) {
	if(Color == LastColor)
		return;

	glColor4f(Color.r, Color.g, Color.b, Color.a);
	LastColor = Color;
}

// Set texture id
void _Graphics::SetTextureID(GLuint TextureID) {
	if(TextureID == LastTextureID)
		return;

	glBindTexture(GL_TEXTURE_2D, TextureID);
	LastTextureID = TextureID;
}

// Set vertex buffer id
void _Graphics::SetVertexBufferID(GLuint VertexBufferID) {
	if(VertexBufferID == LastVertexBufferID)
		return;

	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);
	LastVertexBufferID = VertexBufferID;
}

// Enable a program
void _Graphics::SetProgram(const _Program *Program) {
	if(Program == LastProgram)
		return;

	EnableAttribs(Program->Attribs);
	Program->Use();
	LastProgram = Program;
}

// Enable/disable depth test
void _Graphics::SetDepthTest(bool DepthTest) {
	if(DepthTest == LastDepthTest)
		return;

	if(DepthTest)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	LastDepthTest = DepthTest;
}

// Resets all the last used variables
void _Graphics::DirtyState() {
	LastVertexBufferID = -1;
	LastTextureID = -1;
	LastAttribLevel = -1;
	LastColor = glm::vec4(-1, -1, -1, -1);
	LastProgram = nullptr;
	LastDepthTest = true;
}

void _Graphics::SetDepthMask(bool Value) { glDepthMask(Value); }
void _Graphics::EnableStencilTest() { glEnable(GL_STENCIL_TEST); }
void _Graphics::DisableStencilTest() { glDisable(GL_STENCIL_TEST); }
