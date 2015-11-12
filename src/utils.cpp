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
#include <utils.h>

// Reads in a string that is TSV formatted
std::string GetTSVText(std::ifstream &Stream, bool *EndOfLine) {
	std::string Text;
	char Char;

	while(1) {
		Stream.get(Char);
		if(Char == '\n') {
			if(EndOfLine)
				*EndOfLine = true;
			return Text;
		}
		if(Char == '\t') {
			return Text;
		}
		else {
			Text += Char;
		}
	}

	return Text;
}

// Reads in a string that is TSV formatted
void GetTSVToken(std::ifstream &Stream, std::string &ReturnString, bool *EndOfLine) {
	char Char;

	while(1) {
		Stream.get(Char);
		if(Char == '\n') {
			if(EndOfLine)
				*EndOfLine = true;
			return;
		}
		if(Char == '\t') {
			return;
		}
		else {
			ReturnString += Char;
		}
	}
}

// Loads a file into a string
const char *LoadFileIntoMemory(const char *Path) {

	// Open file
	std::ifstream File(Path, std::ios::binary);
	if(!File)
		return 0;

	// Get file size
	File.seekg(0, std::ios::end);
	std::ifstream::pos_type Size = File.tellg();
	if(!Size)
		return 0;

	File.seekg(0, std::ios::beg);

	// Read data
	char *Data = new char[(size_t)Size + 1];
	File.read(Data, Size);
	File.close();
	Data[(size_t)Size] = 0;

	return Data;
}

// Write a chunk to a stream
void WriteChunk(std::ofstream &File, int Type, const char *Data, size_t Size) {
	File.write((char *)&Type, sizeof(Type));
	File.write((char *)&Size, sizeof(Size));
	File.write(Data, Size);
}

// Generates a random point inside of a circle
glm::vec2 GenerateRandomPointInCircle(float Radius) {
	// TODO fix
	//return glm::vec2(Random.Generate() * 360.0) * Radius * sqrt(Random.Generate());
	return glm::vec2();
}

// Remove extension from a filename
std::string RemoveExtension(const std::string &Path) {
	size_t SuffixPosition = Path.find_last_of(".");
	if(SuffixPosition == std::string::npos)
		return Path;

	return Path.substr(0, SuffixPosition);
}
