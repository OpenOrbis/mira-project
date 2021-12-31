# Default cmake version
cmake_minimum_required(VERSION 3.20)

# Name of the project
project(example_trainer)

set(CMAKE_VERBOSE_MAKEFILE OFF)

# NOTE: By default this line is disabled
# You should compile this project by using
# mkdir build && cd build
# cmake -DCMAKE_TOOLCHAIN_FILE=$OO_PS4_TOOLCHAIN/oosdk.cmake ..
# cmake --build .
#
# Include all of the OO_PS4_TOOLCHAIN defaults
include (${CMAKE_SOURCE_DIR}/scripts/oosdk.cmake)

# Enable assembly files
enable_language(ASM)

# Not needed for library examples
# Set the SCE authentication id
#set(PROJECT_AUTH_ID 0x3800000000000011)

# Include the default source directory, add additional include paths here
include_directories(${PROJECT_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/external)

# Linked libraries
link_libraries(-lc -lkernel -lc++)

# Get all source location
file(GLOB_RECURSE PROJECT_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.cc ${PROJECT_SOURCE_DIR}/*.cxx ${PROJECT_SOURCE_DIR}/*.h ${PROJECT_SOURCE_DIR}/*.hpp ${PROJECT_SOURCE_DIR}/*.hxx)
file(GLOB_RECURSE PROJECT_S_FILES ${PROJECT_SOURCE_DIR}/*.s ${PROJECT_SOURCE_DIR}/*.S)

# Set all assembler flags
set_source_files_properties(${PROJECT_S_FILES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

# Prebuilt objects
set(PREBUILT_OBJS 
    ${OO_PS4_TOOLCHAIN}/lib/crtlib.o
)

# Add our project
# NOTE: libraries use crtlib.o, executables use crt1.o
add_executable(${PROJECT_NAME} ${PROJECT_S_FILES} ${PREBUILT_OBJS} ${PROJECT_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

# Create the fself
set(MODULE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME})
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${OO_PS4_TOOLCHAIN}/bin/linux/create-fself -in=${MODULE_OUTPUT_PATH} -out=${MODULE_OUTPUT_PATH}.oelf --lib ${MODULE_OUTPUT_PATH}.prx)
