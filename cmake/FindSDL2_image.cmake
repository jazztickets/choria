#	Alan Witkowski - CMake module to find SDL2_image
#
#	Input:
#       SDL2_IMAGE_ROOT - Environment variable that points to the SDL2_image root directory
#
#	Output:
#       SDL2_IMAGE_FOUND - Set to true if SDL2_image was found
#       SDL2_IMAGE_INCLUDE_DIR - Path to SDL2_image.h
#       SDL2_IMAGE_LIBRARIES - Contains the list of SDL2_image libraries
#

set(SDL2_IMAGE_FOUND false)

# find include path
find_path(
		SDL2_IMAGE_INCLUDE_DIR
	NAMES
		SDL_image.h
	HINTS
		ENV SDL2_IMAGE_ROOT
	PATHS
		/usr
		/usr/local
	PATH_SUFFIXES
		include
		SDL2
)

# find library
find_library(
		SDL2_IMAGE_LIBRARIES SDL2_image
	HINTS
		ENV SDL2_IMAGE_ROOT
	PATHS
		/usr/lib
		/usr/local/lib
)

# handle QUIET and REQUIRED
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2_image REQUIRED_VARS SDL2_IMAGE_LIBRARIES SDL2_IMAGE_INCLUDE_DIR)

# advanced variables only show up in gui if show advanced is turned on
mark_as_advanced(SDL2_IMAGE_INCLUDE_DIR SDL2_IMAGE_LIBRARIES)
