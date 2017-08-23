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
#include <ae/files.h>

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
