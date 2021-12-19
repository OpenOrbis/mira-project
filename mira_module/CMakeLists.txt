# Check the current CMake version(3.20 Min.)
cmake_minimum_required(VERSION 3.20)

# Set the project to be a C CXX ASM
project(mira_module)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Include all of the OOSDK defaults
include(${CMAKE_SOURCE_DIR}/scripts/oosdk.cmake)

enable_language(ASM)

# Set the include directories
include_directories(${CMAKE_SOURCE_DIR}/external ${CMAKE_SOURCE_DIR}/external/mira ${PROJECT_SOURCE_DIR}/src ${OO_PS4_TOOLCHAIN}/include)

# Get all of the source files that need to be compiled
file(GLOB_RECURSE MIRA_MODULE_SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.cc ${PROJECT_SOURCE_DIR}/*.cxx ${PROJECT_SOURCE_DIR}/*.h ${PROJECT_SOURCE_DIR}/*.hpp ${PROJECT_SOURCE_DIR}/*.hxx)
file(GLOB_RECURSE MIRA_MODULE_S_FILES ${PROJECT_SOURCE_DIR}/*.s ${PROJECT_SOURCE_DIR}/*.S)

# Prebuilt objects
set(PREBUILT_OBJS 
    ${OO_PS4_TOOLCHAIN}/lib/crtlib.o
)

# Set all assembler flags
set_source_files_properties(${MIRA_MODULE_S_FILES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

set_source_files_properties(${PREBUILT_OBJS} PROPERTIES EXTERNAL_OBJECT true GENERATED true)

link_libraries(-lSceLibcInternal -lkernel -lc -lc++)

# This is busted...
add_executable(mira_module ${PREBUILT_OBJS} ${MIRA_MODULE_S_FILES} ${MIRA_MODULE_SRC_FILES})

set(MODULE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/mira_module)

add_custom_command(TARGET mira_module POST_BUILD COMMAND ${OO_PS4_TOOLCHAIN}/bin/linux/create-lib -in=${MODULE_OUTPUT_PATH} -out=${MODULE_OUTPUT_PATH}.oelf --paid 0x3800000000000011)
