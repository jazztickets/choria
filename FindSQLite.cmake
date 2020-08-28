#	CMake module to find sqlite3
#	Alan Witkowski
#
#	Input:
# 		SQLITE_ROOT          - Environment variable that points to the sqlite3 root directory
#
#	Output:
#		SQLITE_FOUND         - Set to true if sqlite3 was found
#		SQLITE_INCLUDE_DIR   - Path to sqlite3.h
#		SQLITE_LIBRARIES     - Contains the list of sqlite3 libraries
#

set(SQLITE_FOUND false)

# find include path
find_path(
		SQLITE_INCLUDE_DIR
	NAMES
		sqlite3.h
	HINTS
		ENV SQLITE_ROOT
		ENV SQLITEDIR
	PATHS
		/usr
		/usr/local
	PATH_SUFFIXES
		include
		include/sqlite3
		sqlite3
)

# find library
find_library(
		SQLITE_LIBRARY sqlite3 libsqlite3
	HINTS
		ENV SQLITE_ROOT
		ENV SQLITEDIR
	PATHS
		/usr/lib
		/usr/local/lib
	PATH_SUFFIXES
		lib/
)

# set libraries var
set(SQLITE_LIBRARIES ${SQLITE_LIBRARY})

# handle QUIET and REQUIRED
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SQLite DEFAULT_MSG SQLITE_LIBRARIES SQLITE_INCLUDE_DIR)

# advanced variables only show up in gui if show advanced is turned on
mark_as_advanced(SQLITE_INCLUDE_DIR SQLITE_LIBRARIES)
