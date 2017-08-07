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
#include <files.h>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <sys/stat.h>
	#include <dirent.h>
#endif

// Get a list of files in a directory
_Files::_Files(const std::string &Path) : Path(Path) {

	#ifdef _WIN32

		// Get file handle
		WIN32_FIND_DATA FindFileData;
		HANDLE FindHandle = FindFirstFile((Path + "*").c_str(), &FindFileData);
		if(FindHandle == INVALID_HANDLE_VALUE) {
			return;
		}

		// Add first value
		if(!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			Nodes.push_back(FindFileData.cFileName);

		// Get the other files
		while(FindNextFile(FindHandle, &FindFileData)) {
			if(!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				Nodes.push_back(FindFileData.cFileName);
		}

		// Close
		FindClose(FindHandle);
	#else

		DIR *Directory;
		struct dirent *Entry;
		Directory = opendir(Path.c_str());
		if(Directory) {
			while((Entry = readdir(Directory)) != nullptr) {
				if(Entry->d_type == DT_REG) {
					Nodes.push_back(Entry->d_name);
				}
			}

			closedir(Directory);
		}

	#endif
}
