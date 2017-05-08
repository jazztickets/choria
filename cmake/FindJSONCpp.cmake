#	CMake module to find jsoncpp
#	Alan Witkowski
#
#	Input:
# 		JSONCPP_ROOT          - Environment variable that points to the root directory
#
#	Output:
#		JSONCPP_FOUND         - Set to true if found
#		JSONCPP_INCLUDE_DIR   - Path to json.h
#		JSONCPP_LIBRARIES     - Contains the list of libraries
#

set(JSONCPP_FOUND false)

# find include path
find_path(
		JSONCPP_INCLUDE_DIR
	NAMES
		json/json.h
	HINTS
		ENV JSONCPP_ROOT
	PATHS
		/usr
		/usr/local
	PATH_SUFFIXES
		include
		include/json
		include/jsoncpp
		json
)

# find debug library
find_library(
		JSONCPP_DEBUG_LIBRARY libjsoncpp_d libjsoncpp jsoncpp
	HINTS
		ENV JSONCPP_ROOT
	PATHS
		/usr/lib
		/usr/local/lib
	PATH_SUFFIXES
		lib/
)

# find release library
find_library(
		JSONCPP_RELEASE_LIBRARY libjsoncpp jsoncpp
	HINTS
		ENV JSONCPP_ROOT
	PATHS
		/usr/lib
		/usr/local/lib
	PATH_SUFFIXES
		lib/
)

# combine debug and release
set(JSONCPP_LIBRARIES
	debug ${JSONCPP_DEBUG_LIBRARY}
	optimized ${JSONCPP_RELEASE_LIBRARY}
)

# handle QUIET and REQUIRED
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JSONCpp DEFAULT_MSG JSONCPP_LIBRARIES JSONCPP_INCLUDE_DIR)

# advanced variables only show up in gui if show advanced is turned on
mark_as_advanced(JSONCPP_INCLUDE_DIR JSONCPP_LIBRARIES)
