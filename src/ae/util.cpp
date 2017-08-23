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
#include <ae/util.h>
#include <sys/stat.h>
#include <fstream>
#include <regex>

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

// Remove extension from a filename
std::string RemoveExtension(const std::string &Path) {
	size_t SuffixPosition = Path.find_last_of(".");
	if(SuffixPosition == std::string::npos)
		return Path;

	return Path.substr(0, SuffixPosition);
}

// Trim whitespace from string
std::string TrimString(const std::string &String) {
	std::regex Regex("^[ \t]+|[ \t]+$");
	return std::regex_replace(String, Regex, "");
}

// Create directory
int MakeDirectory(const std::string &Path) {
#ifdef _WIN32
	return mkdir(Path.c_str());
#else
	return mkdir(Path.c_str(), 0755);
#endif
}
