cmake_minimum_required(VERSION 3.10)

# Set the project to be a C CXX ASM
project(kernel)

# Set verbosity
set(CMAKE_VERBOSE_MAKEFILE ON)

set(MIRA_PLATFORM MIRA_PLATFORM_ORBIS_BSD_672)

# Include all of the OOSDK defaults
include(${CMAKE_SOURCE_DIR}/scripts/kernel.cmake)

#enable_language(C)
#enable_language(CXX)
enable_language(ASM)

# Set the include directories
include_directories(${CMAKE_SOURCE_DIR}/external ${CMAKE_SOURCE_DIR}/external/mira src)

# Additional include directories
include_directories("${CMAKE_SOURCE_DIR}/external" "${CMAKE_SOURCE_DIR}/external/freebsd-headers/include" "${PROJECT_SOURCE_DIR}/src")

# Get all of the source files that need to be compiled
file(GLOB_RECURSE MIRA_MODULE_SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.cc ${PROJECT_SOURCE_DIR}/*.cxx ${PROJECT_SOURCE_DIR}/*.h ${PROJECT_SOURCE_DIR}/*.hpp ${PROJECT_SOURCE_DIR}/*.hxx)
file(GLOB_RECURSE MIRA_MODULE_S_FILES ${PROJECT_SOURCE_DIR}/*.s ${PROJECT_SOURCE_DIR}/*.S)

# Set all assembler flags
set_source_files_properties(${MIRA_MODULE_S_FILES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

#message("SrcFiles")
#message("${MIRA_MODULE_SRC_FILES}")

#message("SFiles")
#message("${MIRA_MODULE_S_FILES}")

# Set all assembler flags
set_source_files_properties(${MIRA_MODULE_S_FILES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

# Set the linker options for this project
add_link_options(-Wl,-entry="mira_entry")
add_link_options(-Wl,-T ${PROJECT_SOURCE_DIR}/src/link.ld)
add_link_options(-v)

# Check to see if the trainer_loader payload has been build already
set(TRAINER_LOADER_BIN ${CMAKE_BINARY_DIR}/trainer_loader/libtrainer_loader.so)
if (EXISTS ${TRAINER_LOADER_BIN})
    message("trainer loader found")
    add_compile_definitions(TRAINER_LOADER_BIN_PATH="${TRAINER_LOADER_BIN}")
else()
    message(FATAL_ERROR "could not find built trainer_loader.")
endif()

add_executable(kernel ${MIRA_MODULE_S_FILES} ${MIRA_MODULE_SRC_FILES})