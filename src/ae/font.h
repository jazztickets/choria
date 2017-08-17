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
#pragma once

// Libraries
#include <ae/ui.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <ft2build.h>
#include <string>
#include <list>
#include FT_FREETYPE_H

// Forward Declarations
class _Texture;
class _Program;

// Contains glyph info
struct _Glyph {
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

		_Font(const std::string &ID, const std::string &FontFile, const _Program *Program, uint32_t FontSize=12, uint32_t TextureWidth=256);
		~_Font();

		float DrawText(const std::string &Text, glm::vec2 Position, const _Alignment &Alignment=LEFT_BASELINE, const glm::vec4 &Color=glm::vec4(1.0f), float Scale=1.0f) const;
		void DrawTextFormatted(const std::string &Text, glm::vec2 Position, const _Alignment &Alignment=LEFT_BASELINE) const;
		void GetStringDimensions(const std::string &Text, _TextBounds &TestBounds, bool UseFormatting=false) const;
		void BreakupString(const std::string &Text, float Width, std::list<std::string> &Strings, bool UseFormatting=false) const;

		// Attributes
		std::string ID;
		float MaxHeight;
		float MaxAbove;
		float MaxBelow;

	private:

		void CreateFontTexture(std::string SortedCharacters, uint32_t TextureWidth);
		void SortCharacters(FT_Face &Face, const std::string &Characters, std::string &SortedCharacters);
		void DrawGlyph(glm::vec2 &Position, char Char, float Scale) const;
		void AdjustPosition(const std::string &Text, glm::vec2 &Position, bool UseFormatting, const _Alignment &Alignment, float Scale) const;

		// Glyphs
		_Glyph Glyphs[256];

		// Graphics
		const _Program *Program;
		_Texture *Texture;

		// Freetype
		bool HasKerning;
		FT_Library Library;
		FT_Face Face;
		FT_Int32 LoadFlags;
};
