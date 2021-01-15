cmake_minimum_required(VERSION 2.8.12)

# define constants
add_definitions(-DGAME_VERSION="0.6.0")
add_definitions("-DGLM_FORCE_RADIANS")
add_definitions("-DGLM_ENABLE_EXPERIMENTAL")
add_definitions("-DHAS_SOCKLEN_T")

# project name
project(choria)

# add extra find modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

# mingw
if(WIN32)
	set(EXTRA_LIBS ${EXTRA_LIBS} winmm ws2_32)

	# disable console window
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-subsystem,windows")
endif()

# enable editor
set("DISABLE_EDITOR=${DISABLE_EDITOR}")
if(DISABLE_EDITOR)
	add_definitions("-DENABLE_EDITOR=0")
else()
	add_definitions("-DENABLE_EDITOR=1")
endif()

# set default build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-result -Wno-switch -Wno-unused-parameter -pedantic -std=c++17 -msse -mfpmath=sse")

# Set gl preference
if(NOT DEFINED OpenGL_GL_PREFERENCE)
	set(OpenGL_GL_PREFERENCE GLVND)
endif()

# find libraries
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Vorbis REQUIRED)
find_package(Ogg REQUIRED)
find_package(Lua 5.2 REQUIRED)
find_package(Freetype REQUIRED)
find_package(ZLIB REQUIRED)
find_package(GLM REQUIRED)
find_package(SQLite REQUIRED)
find_package(Threads REQUIRED)
find_package(JSONCpp REQUIRED)
find_package(TinyXML2 REQUIRED)

# set include directories
include_directories("src")
include_directories("ext")
include_directories("ext/ae")
include_directories(${SDL2_INCLUDE_DIR})
include_directories(${SDL2_IMAGE_INCLUDE_DIR})
include_directories(${OPENAL_INCLUDE_DIR})
include_directories(${VORBIS_INCLUDE_DIR})
include_directories(${OGG_INCLUDE_DIR})
include_directories(${LUA_INCLUDE_DIR})
include_directories(${FREETYPE_INCLUDE_DIRS})
include_directories(${GLM_INCLUDE_DIRS})
include_directories(${SQLITE_INCLUDE_DIR})
include_directories(${ZLIB_INCLUDE_DIRS})
include_directories(${JSONCPP_INCLUDE_DIR})
include_directories(${TINYXML2_INCLUDE_DIR})

# add source files
file(GLOB SRC_MAIN "src/*.cpp" "src/*.h")
file(GLOB SRC_OBJECTS "src/objects/*.cpp" "src/objects/*.h")
file(GLOB SRC_COMPONENTS "src/objects/components/*.cpp" "src/objects/components/*.h")
file(GLOB SRC_HUD "src/hud/*.cpp" "src/hud/*.h")
file(GLOB SRC_STATES "src/states/*.cpp" "src/states/*.h")
file(GLOB SRC_AE "ext/ae/ae/*.cpp" "ext/ae/ae/*.h")
file(GLOB SRC_ENET "ext/enet/*.c" "ext/enet/*.h")
file(GLOB SRC_PATH "ext/path/*.cpp" "ext/path/*.h")
file(GLOB SRC_ZLIB "ext/zlib/*.cc" "ext/zlib/*.h")

# combine source files
file(GLOB SRC_ALL
	${SRC_MAIN}
	${SRC_OBJECTS}
	${SRC_COMPONENTS}
	${SRC_HUD}
	${SRC_STATES}
	${SRC_AE}
	${SRC_ENET}
	${SRC_PATH}
	${SRC_ZLIB}
	deployment/resource.rc
)

# build binary
add_executable(${CMAKE_PROJECT_NAME} ${SRC_ALL})

# set output path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/working/)

# set debug options
set(STRIP_LUA "-s")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(STRIP_LUA "")
	set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${CMAKE_PROJECT_NAME}_debug)
endif()

# add command to build data
if(EXISTS "${PROJECT_SOURCE_DIR}/assets/source/process.sh")

	# pack data
	add_custom_command(
		OUTPUT "${PROJECT_SOURCE_DIR}/working/data/sounds"
		COMMAND "${PROJECT_SOURCE_DIR}/assets/source/process.sh"
		WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/assets/source/"
	)
	add_custom_target(data ALL DEPENDS "${PROJECT_SOURCE_DIR}/working/data/sounds")

	# compile scripts
	file(GLOB SOURCE_SCRIPTS "${PROJECT_SOURCE_DIR}/assets/source/scripts/*.lua")
	file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/working/scripts/")
	foreach(SCRIPT_PATH ${SOURCE_SCRIPTS})
		get_filename_component(FILENAME "${SCRIPT_PATH}" NAME)
		set(SCRIPT_OUTPUT "${PROJECT_SOURCE_DIR}/working/scripts/${FILENAME}")
		list(APPEND DATA_SCRIPTS "${SCRIPT_OUTPUT}")
		add_custom_command(
			OUTPUT "${SCRIPT_OUTPUT}"
			COMMAND luac "${STRIP_LUA}" -o "${SCRIPT_OUTPUT}" "${SCRIPT_PATH}"
			DEPENDS "${SCRIPT_PATH}"
		)
	endforeach()
	add_custom_target(data_scripts ALL DEPENDS "${DATA_SCRIPTS}")
endif()

# add custom target to generate version.h
add_custom_target(
	version
	${CMAKE_COMMAND}
	-D SRC=${PROJECT_SOURCE_DIR}/cmake/version.h.in
	-D DST=${PROJECT_SOURCE_DIR}/src/version.h
	-P ${PROJECT_SOURCE_DIR}/cmake/version.cmake
)
add_dependencies(${CMAKE_PROJECT_NAME} version)

# link libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
	${OPENGL_LIBRARIES}
	${SDL2_LIBRARY}
	${SDL2_IMAGE_LIBRARIES}
	${OPENAL_LIBRARY}
	${VORBIS_LIBRARIES}
	${OGG_LIBRARIES}
	${LUA_LIBRARIES}
	${FREETYPE_LIBRARIES}
	${SQLITE_LIBRARIES}
	${ZLIB_LIBRARIES}
	${JSONCPP_LIBRARIES}
	${TINYXML2_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
	${EXTRA_LIBS}
)

# install
if(WIN32)
else()

	# linux installation
	install(TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION share/games/${CMAKE_PROJECT_NAME})
	install(DIRECTORY ${PROJECT_SOURCE_DIR}/working/ DESTINATION share/games/${CMAKE_PROJECT_NAME} PATTERN "working/choria" EXCLUDE PATTERN "working/*")
	install(FILES ${PROJECT_SOURCE_DIR}/deployment/${CMAKE_PROJECT_NAME}.png DESTINATION share/pixmaps)
	install(FILES ${PROJECT_SOURCE_DIR}/deployment/${CMAKE_PROJECT_NAME}.desktop DESTINATION share/applications)
	install(FILES ${PROJECT_SOURCE_DIR}/CHANGELOG DESTINATION share/doc/${CMAKE_PROJECT_NAME})
	install(FILES ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION share/doc/${CMAKE_PROJECT_NAME})
	install(FILES ${PROJECT_SOURCE_DIR}/README DESTINATION share/doc/${CMAKE_PROJECT_NAME})

	# generate the script to launch the program
	configure_file(${PROJECT_SOURCE_DIR}/deployment/${CMAKE_PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles)
	install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${CMAKE_PROJECT_NAME} DESTINATION bin)
endif()
