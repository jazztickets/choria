#	CMake module to find TinyXML-2
#	Alan Witkowski
#
#	Input:
# 		TINYXML2_ROOT          - Environment variable that points to the root directory
#
#	Output:
#		TINYXML2_FOUND         - Set to true if found
#		TINYXML2_INCLUDE_DIR   - Path to tinyxml2.h
#		TINYXML2_LIBRARIES     - Contains the list of libraries
#

set(TINYXML2_FOUND false)

# find include path
find_path(
		TINYXML2_INCLUDE_DIR
	NAMES
		tinyxml2.h
	HINTS
		ENV TINYXML2_ROOT
	PATHS
		/usr
		/usr/local
	PATH_SUFFIXES
		include
		include/tinyxml
		tinyxml
)

# find debug library
find_library(
		TINYXML2_DEBUG_LIBRARY libtinyxml2_d libtinyxml2 tinyxml2
	HINTS
		ENV TINYXML2_ROOT
	PATHS
		/usr/lib
		/usr/local/lib
	PATH_SUFFIXES
		lib/
)

# find release library
find_library(
		TINYXML2_RELEASE_LIBRARY libtinyxml2 tinyxml2
	HINTS
		ENV TINYXML2_ROOT
	PATHS
		/usr/lib
		/usr/local/lib
	PATH_SUFFIXES
		lib/
)

# combine debug and release
set(TINYXML2_LIBRARIES
	debug ${TINYXML2_DEBUG_LIBRARY}
	optimized ${TINYXML2_RELEASE_LIBRARY}
)

# handle QUIET and REQUIRED
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TinyXML2 DEFAULT_MSG TINYXML2_LIBRARIES TINYXML2_INCLUDE_DIR)

# advanced variables only show up in gui if show advanced is turned on
mark_as_advanced(TINYXML2_INCLUDE_DIR TINYXML2_LIBRARIES)
