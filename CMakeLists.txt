cmake_minimum_required(VERSION 2.8)

# define constants
add_definitions(-DGAME_VERSION="0.5.0")
add_definitions("-DGLM_FORCE_RADIANS")
add_definitions("-DHAS_SOCKLEN_T")

# project name
project(choria)

# add extra find modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

# check for -DLUA_VERSION=x.x
if(NOT LUA_VERSION)
	set(LUA_VERSION "5.3")
endif()

# mingw and visual studio
if(WIN32)
	set(EXTRA_LIBS ${EXTRA_LIBS} winmm ws2_32)
endif()

# visual studio only options
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
	add_definitions("-D_CRT_SECURE_NO_WARNINGS")
	add_definitions("-DNOMINMAX")
	set(ZLIB_ROOT $ENV{ZLIB_ROOT})

	# set output path
	set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/)
else()

	# set default build type
	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE Release)
	endif()

	# compiler flags
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -pedantic -std=c++11")

	# set output path
	set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})
endif()

# external libraries
add_definitions("-DTIXML_USE_STL")

# find libraries
find_package(OpenGL REQUIRED)
if(NOT OPENGL_FOUND)
	message(SEND_ERROR "Could not find OpenGL!")
endif()

find_package(SDL2 REQUIRED)
if(NOT SDL2_FOUND)
	message(SEND_ERROR "Could not find SDL2!")
endif()

find_package(Lua ${LUA_VERSION} REQUIRED)
if(NOT LUA_FOUND)
	message(SEND_ERROR "Could not find Lua " ${LUA_VERSION} "!")
endif()

find_package(Freetype REQUIRED)
if(NOT FREETYPE_FOUND)
	message(SEND_ERROR "Could not find Freetype 2!")
endif()

find_package(ZLIB REQUIRED)
if(NOT ZLIB_FOUND)
	message(SEND_ERROR "Could not find zlib!")
endif()

find_package(GLM REQUIRED)
if(NOT GLM_FOUND)
	message(SEND_ERROR "Could not find GLM!")
endif()

find_package(SQLite REQUIRED)
if(NOT SQLITE_FOUND)
	message(SEND_ERROR "Could not find SQLite3!")
endif()

find_package(Threads REQUIRED)
if(NOT Threads_FOUND)
	message(SEND_ERROR "Could not find Threads!")
endif()

# set include directories
include_directories("src")
include_directories(${SDL2_INCLUDE_DIR})
include_directories(${LUA_INCLUDE_DIR})
include_directories(${FREETYPE_INCLUDE_DIRS})
include_directories(${GLM_INCLUDE_DIRS})
include_directories(${SQLITE_INCLUDE_DIR})
include_directories(${ZLIB_INCLUDE_DIRS})

# add source files
file(GLOB SRC_MAIN "src/*.cpp" "src/*.h")
file(GLOB SRC_ENET "src/enet/*.c" "src/enet/*.h")
file(GLOB SRC_INSTANCES "src/instances/*.cpp" "src/instances/*.h")
file(GLOB SRC_NETWORK "src/network/*.cpp" "src/network/*.h")
file(GLOB SRC_OBJECTS "src/objects/*.cpp" "src/objects/*.h")
file(GLOB SRC_STATES "src/states/*.cpp" "src/states/*.h")
file(GLOB SRC_UI "src/ui/*.cpp" "src/ui/*.h")
file(GLOB SRC_PNGLITE "src/pnglite/*.c" "src/pnglite/*.h")
file(GLOB SRC_ZLIB "src/zlib/*.cc" "src/zlib/*.h")

file(GLOB SRC_ALL
	${SRC_MAIN}
	${SRC_STATES}
	${SRC_ENET}
	${SRC_INSTANCES}
	${SRC_NETWORK}
	${SRC_OBJECTS}
	${SRC_UI}
	${SRC_PNGLITE}
	${SRC_ZLIB}
)

# add virtual folders
source_group(main FILES ${SRC_MAIN})
source_group(states FILES ${SRC_STATES})
source_group(instances FILES ${SRC_INSTANCES})
source_group(network FILES ${SRC_NETWORK})
source_group(objects FILES ${SRC_OBJECTS})
source_group(enet FILES ${SRC_ENET})
source_group(pnglite FILES ${SRC_PNGLITE})

# build exe
add_executable(${CMAKE_PROJECT_NAME} ${SRC_ALL} src/resource.rc)

# link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
	${OPENGL_gl_LIBRARY}
	${SDL2_LIBRARY}
	${LUA_LIBRARIES}
	${FREETYPE_LIBRARIES}
	${SQLITE_LIBRARIES}
	${ZLIB_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	${EXTRA_LIBS}
)

# copy dlls to working directory
if(WIN32)
else()

	# linux installation
	install(TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION share/games/${CMAKE_PROJECT_NAME})
	install(DIRECTORY ${PROJECT_SOURCE_DIR}/working/database DESTINATION share/games/${CMAKE_PROJECT_NAME})
	install(DIRECTORY ${PROJECT_SOURCE_DIR}/working/fonts DESTINATION share/games/${CMAKE_PROJECT_NAME})
	install(DIRECTORY ${PROJECT_SOURCE_DIR}/working/maps DESTINATION share/games/${CMAKE_PROJECT_NAME})
	install(DIRECTORY ${PROJECT_SOURCE_DIR}/working/textures DESTINATION share/games/${CMAKE_PROJECT_NAME})
	install(FILES ${PROJECT_SOURCE_DIR}/deployment/${CMAKE_PROJECT_NAME}.png DESTINATION share/pixmaps)
	install(FILES ${PROJECT_SOURCE_DIR}/deployment/${CMAKE_PROJECT_NAME}.desktop DESTINATION share/applications)
	install(FILES ${PROJECT_SOURCE_DIR}/CHANGELOG DESTINATION share/doc/${CMAKE_PROJECT_NAME})
	install(FILES ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION share/doc/${CMAKE_PROJECT_NAME})
	install(FILES ${PROJECT_SOURCE_DIR}/README DESTINATION share/doc/${CMAKE_PROJECT_NAME})

	# generate the script to launch the program
	configure_file(${PROJECT_SOURCE_DIR}/deployment/${CMAKE_PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles)
	install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${CMAKE_PROJECT_NAME} DESTINATION bin)
endif()
