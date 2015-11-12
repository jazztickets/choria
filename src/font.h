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
#include <ui/ui.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H

// Forward Declarations
class _Texture;
class _Program;

// Contains glyph info
struct GlyphStruct {
	float Left, Top, Right, Bottom;
	float Width, Height;
	float Advance, OffsetX, OffsetY;
};

struct _TextBounds {
	int Width;
	int AboveBase;
	int BelowBase;
};

// Classes
class _Font {

	public:

		_Font(const _Program *Program);
		_Font(const std::string &FontFile, const _Program *Program, int FontSize=12, int TextureWidth=256);
		~_Font();

		void DrawText(const std::string &Text, glm::vec2 Position, const glm::vec4 &Color=glm::vec4(1.0f), const _Alignment &Alignment=LEFT_BASELINE, float Scale=1.0f) const;
		void GetStringDimensions(const std::string &Text, _TextBounds &TestBounds) const;
		void BreakupString(const std::string &Text, float Width, std::vector<std::string> &Strings) const;
		float GetMaxHeight() const { return MaxHeight; }

	private:

		void CreateFontTexture(std::string SortedCharacters, int TextureWidth);
		void SortCharacters(FT_Face &Face, const std::string &Characters, std::string &SortedCharacters);

		// Glyphs
		GlyphStruct Glyphs[256];
		float MaxHeight;

		// Graphics
		const _Program *Program;
		_Texture *Texture;

		// Freetype
		bool HasKerning;
		FT_Library Library;
		FT_Face Face;
		FT_Int32 LoadFlags;
};
