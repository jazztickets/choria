cmake_minimum_required(VERSION 2.8)

# project name
project(choria)

# set default build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

if(WIN32)

else()
	add_definitions("-DHAS_SOCKLEN_T")
endif()

# external libraries
add_definitions("-DTIXML_USE_STL")

# add source files
file(GLOB SRC_MAIN src/*.cpp src/*.h)
file(GLOB SRC_ENET src/enet/*.c src/enet/*.h)
file(GLOB SRC_ENGINE src/engine/*.cpp src/engine/*.h)
file(GLOB SRC_INSTANCES src/instances/*.cpp src/instances/*.h)
file(GLOB SRC_NETWORK src/network/*.cpp src/network/*.h)
file(GLOB SRC_OBJECTS src/objects/*.cpp src/objects/*.h)

file(GLOB SRC_ALL ${SRC_MAIN} ${SRC_ENET} ${SRC_ENGINE} ${SRC_INSTANCES} ${SRC_NETWORK} ${SRC_OBJECTS})

# add virtual folders
source_group(main FILES ${SRC_MAIN})
source_group(enet FILES ${SRC_ENET})
source_group(engine FILES ${SRC_ENGINE})
source_group(instances FILES ${SRC_INSTANCES})
source_group(network FILES ${SRC_NETWORK})
source_group(objects FILES ${SRC_OBJECTS})

# set include directories
include_directories("src")

# output path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

# build exe
add_executable(${CMAKE_PROJECT_NAME} ${SRC_ALL})
target_link_libraries(${CMAKE_PROJECT_NAME} Irrlicht sqlite3 tinyxml)
