/******************************************************************************
* choria - https://github.com/jazztickets/choria
* Copyright (C) 2016  Alan Witkowski
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
#include <string>
#include <sstream>

const char *LoadFileIntoMemory(const char *Path);
std::string RemoveExtension(const std::string &Path);
std::string TrimString(const std::string &String);

template <class T>
inline T ToNumber(const std::string &String) {
	T Number = 0;

	std::stringstream Buffer(String);
	Buffer >> Number;

	return Number;
}

inline float Round(float Number) { return (int)(Number * 10) / 10.0f; }