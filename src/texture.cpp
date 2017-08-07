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
#include <texture.h>
#include <graphics.h>
#include <constants.h>
#include <pnglite/pnglite.h>
#include <stdexcept>

// Load from file
_Texture::_Texture(const std::string &Path, bool IsServer, bool Repeat, bool Mipmaps) {
	if(IsServer) {
		Identifier = Path;
		ID = 0;
		return;
	}

	std::string FullPath = TEXTURES_PATH + Path;

	// Open png file
	png_t Png;
	int Result = png_open_file(&Png, FullPath.c_str());
	if(Result != PNG_NO_ERROR)
		throw std::runtime_error("Error loading png: " + FullPath + " reason: " + png_error_string(Result));

	Identifier = Path;
	Size.x = Png.width;
	Size.y = Png.height;

	// Allocate memory for texture
	unsigned char *TextureData = new unsigned char[Size.x * Size.y * Png.bpp];

	// Load png file
	Result = png_get_data(&Png, TextureData);
	if(Result != PNG_NO_ERROR)
		throw std::runtime_error("Error loading png: " + FullPath + " reason: " + png_error_string(Result));

	// Determine OpenGL format
	GLint ColorFormat;
	switch(Png.bpp) {
		case 1: ColorFormat = GL_LUMINANCE; break;
		case 2: ColorFormat = GL_LUMINANCE_ALPHA; break;
		case 3: ColorFormat = GL_RGB; break;
		case 4: ColorFormat = GL_RGBA; break;
		default:
			throw std::runtime_error("Unsupported bpp: " + std::to_string(Png.bpp));
		break;
	}

	// Create texture and upload to GPU
	glGenTextures(1, &ID);
	glBindTexture(GL_TEXTURE_2D, ID);
	if(Repeat) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if(Mipmaps) {
		if(Graphics.Anisotropy)
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, Graphics.Anisotropy);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	else {
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	// Create texture
	glTexImage2D(GL_TEXTURE_2D, 0, ColorFormat, Size.x, Size.y, 0, ColorFormat, GL_UNSIGNED_BYTE, TextureData);

	// Clean up
	png_close_file(&Png);
	delete[] TextureData;
}

// Initialize from buffer
_Texture::_Texture(unsigned char *Data, const glm::ivec2 &Size, GLint InternalFormat, int Format) :
	Size(Size) {

	// Create texture
	glGenTextures(1, &ID);
	glBindTexture(GL_TEXTURE_2D, ID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Size.x, Size.y, 0, Format, GL_UNSIGNED_BYTE, Data);
}

// Destructor
_Texture::~_Texture() {
	if(ID)
		glDeleteTextures(1, &ID);
}
