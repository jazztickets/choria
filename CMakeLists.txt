cmake_minimum_required(VERSION 2.8)

# define constants
add_definitions(-DGAME_VERSION="0.9.0")
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
endif()

# set default build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-result -Wno-unused-parameter -pedantic -std=c++11 -msse -mfpmath=sse")

# set output path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

# use legacy gl lib
set(OpenGL_GL_PREFERENCE LEGACY)

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
file(GLOB SRC_STATES "src/states/*.cpp" "src/states/*.h")
file(GLOB SRC_AE "ext/ae/ae/*.cpp" "ext/ae/ae/*.h")
file(GLOB SRC_ENET "ext/enet/*.c" "ext/enet/*.h")
file(GLOB SRC_PATH "ext/path/*.cpp" "ext/path/*.h")
file(GLOB SRC_ZLIB "ext/zlib/*.cc" "ext/zlib/*.h")

# combine source files
file(GLOB SRC_ALL
	${SRC_MAIN}
	${SRC_AE}
	${SRC_ENET}
	${SRC_OBJECTS}
	${SRC_COMPONENTS}
	${SRC_PATH}
	${SRC_STATES}
	${SRC_ZLIB}
	deployment/resource.rc
)

# build binary
add_executable(${CMAKE_PROJECT_NAME} ${SRC_ALL})

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
	${OPENGL_gl_LIBRARY}
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
	install(DIRECTORY ${PROJECT_SOURCE_DIR}/working/ DESTINATION share/games/${CMAKE_PROJECT_NAME} PATTERN "${PROJECT_SOURCE_DIR}/working/*" )
	install(FILES ${PROJECT_SOURCE_DIR}/deployment/${CMAKE_PROJECT_NAME}.png DESTINATION share/pixmaps)
	install(FILES ${PROJECT_SOURCE_DIR}/deployment/${CMAKE_PROJECT_NAME}.desktop DESTINATION share/applications)
	install(FILES ${PROJECT_SOURCE_DIR}/CHANGELOG DESTINATION share/doc/${CMAKE_PROJECT_NAME})
	install(FILES ${PROJECT_SOURCE_DIR}/LICENSE DESTINATION share/doc/${CMAKE_PROJECT_NAME})
	install(FILES ${PROJECT_SOURCE_DIR}/README DESTINATION share/doc/${CMAKE_PROJECT_NAME})

	# generate the script to launch the program
	configure_file(${PROJECT_SOURCE_DIR}/deployment/${CMAKE_PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles)
	install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${CMAKE_PROJECT_NAME} DESTINATION bin)
endif()
