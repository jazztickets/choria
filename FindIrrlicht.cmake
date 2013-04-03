#	Alan Witkowski - CMake module to find irrlicht
#
#	Input:
# 		IRRLICHT_ROOT - Environment variable that points to the irrlicht root directory
#
#	Output:
#		IRRLICHT_FOUND - Set to true if irrlicht was found
#		IRRLICHT_INCLUDE_DIRS - Path to irrlicht.h
#		IRRLICHT_LIBRARIES - Contains the list of irrlicht libraries
#		IRRLICHT_VERSION_STRING - Version of irrlicht
#

set(IRRLICHT_FOUND false)

# find include path
find_path(
		IRRLICHT_INCLUDE_DIRS
	NAMES
		irrlicht.h
	HINTS
		ENV IRRLICHT_ROOT
	PATHS
		/usr
		/usr/local
	PATH_SUFFIXES
		include
		irrlicht
)

# find debug library
find_library(
		IRRLICHT_DEBUG_LIBRARY Irrlicht_d Irrlicht
	HINTS
		ENV IRRLICHT_ROOT
	PATHS
		/usr/lib
		/usr/local/lib
	PATH_SUFFIXES
		lib/Win32-visualstudio
)

# find release library
find_library(
		IRRLICHT_RELEASE_LIBRARY Irrlicht
	HINTS
		ENV IRRLICHT_ROOT
	PATHS
		/usr/lib
		/usr/local/lib
	PATH_SUFFIXES
		lib/Win32-visualstudio
)

# get irrlicht version
if(IRRLICHT_INCLUDE_DIRS AND EXISTS "${IRRLICHT_INCLUDE_DIRS}/IrrCompileConfig.h")
	file(STRINGS "${IRRLICHT_INCLUDE_DIRS}/IrrCompileConfig.h" irrlicht_version_str REGEX "^#define[ \t]+IRRLICHT_SDK_VERSION[ \t]+\".+\"")
	string(REGEX REPLACE "^#define[ \t]+IRRLICHT_SDK_VERSION[ \t]+\"([^\"]+)\".*" "\\1" IRRLICHT_VERSION_STRING "${irrlicht_version_str}")
	unset(irrlicht_version_str)
endif()

# combine debug and release
set(IRRLICHT_LIBRARIES
	debug ${IRRLICHT_DEBUG_LIBRARY}
	optimized ${IRRLICHT_RELEASE_LIBRARY}
)

# handle QUIET and REQUIRED
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Irrlicht REQUIRED_VARS IRRLICHT_RELEASE_LIBRARY IRRLICHT_INCLUDE_DIRS VERSION_VAR IRRLICHT_VERSION_STRING)

# advanced variables only show up in gui if show advanced is turned on
mark_as_advanced(IRRLICHT_INCLUDE_DIRS IRRLICHT_LIBRARIES)
