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
#include <ae/texture.h>
#include <ae/graphics.h>
#include <SDL_image.h>
#include <stdexcept>

// Load from file
_Texture::_Texture(const std::string &Path, bool IsServer, bool Repeat, bool Mipmaps) {
	if(IsServer) {
		Name = Path;
		ID = 0;
		return;
	}

	// Open png file
	SDL_Surface *Image = IMG_Load(Path.c_str());
	if(!Image) {
		throw std::runtime_error("Error loading image: " + Path + " with error: " + IMG_GetError());
	}

	Name = Path;
	Size.x = Image->w;
	Size.y = Image->h;

	// Determine OpenGL format
	GLint ColorFormat;
	switch(Image->format->BitsPerPixel) {
		case 8: ColorFormat = GL_LUMINANCE; break;
		case 16: ColorFormat = GL_LUMINANCE_ALPHA; break;
		case 24: ColorFormat = GL_RGB; break;
		case 32: ColorFormat = GL_RGBA; break;
		default:
			throw std::runtime_error("Unsupported bpp: " + std::to_string(Image->format->BitsPerPixel));
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
		if(Graphics.Anisotropy > 0)
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, Graphics.Anisotropy);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}
	else {
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	// Create texture
	glTexImage2D(GL_TEXTURE_2D, 0, ColorFormat, Size.x, Size.y, 0, (GLenum)ColorFormat, GL_UNSIGNED_BYTE, Image->pixels);

	// Clean up
	SDL_FreeSurface(Image);
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
