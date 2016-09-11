#	Alan Witkowski - CMake module to find SDL2_mixer
#
#	Input:
#       SDL2_ROOT - Environment variable that points to the sdl2 root directory
#
#	Output:
#       SDL2_MIXER_FOUND - Set to true if SDL2_mixer was found
#       SDL2_MIXER_INCLUDE_DIRS - Path to SDL2_mixer.h
#       SDL2_MIXER_LIBRARIES - Contains the list of SDL2_mixer libraries
#

set(SDL2_MIXER_FOUND false)

# find include path
find_path(
		SDL2_MIXER_INCLUDE_DIRS
	NAMES
		SDL_mixer.h
	HINTS
		ENV SDL2_MIXER_ROOT
	PATHS
		/usr
		/usr/local
	PATH_SUFFIXES
		include
		SDL2
)

# find library
find_library(
		SDL2_MIXER_LIBRARIES SDL2_mixer
	HINTS
		ENV SDL2_MIXER_ROOT
	PATHS
		/usr/lib
		/usr/local/lib
	#PATH_SUFFIXES
)

# handle QUIET and REQUIRED
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2_mixer REQUIRED_VARS SDL2_MIXER_LIBRARIES SDL2_MIXER_INCLUDE_DIRS)

# advanced variables only show up in gui if show advanced is turned on
mark_as_advanced(SDL2_MIXER_INCLUDE_DIRS SDL2_MIXER_LIBRARIES)
