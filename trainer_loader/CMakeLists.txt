# Set the project to be a C CXX ASM
project(trainer_loader)

# Include all of the OOSDK defaults
include(${CMAKE_SOURCE_DIR}/scripts/payload.cmake)

enable_language(ASM)

# Set the include directories
include_directories(${CMAKE_SOURCE_DIR}/external ${CMAKE_SOURCE_DIR}/external/mira ${CMAKE_SOURCE_DIR}/external/freebsd-headers/include)

add_link_options(-T ${PROJECT_SOURCE_DIR}/src/link.ld)

# Get all of the source files that need to be compiled
file(GLOB_RECURSE PROJ_SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.cc ${PROJECT_SOURCE_DIR}/*.cxx ${PROJECT_SOURCE_DIR}/*.h ${PROJECT_SOURCE_DIR}/*.hpp ${PROJECT_SOURCE_DIR}/*.hxx)
file(GLOB_RECURSE PROJ_S_FILES ${PROJECT_SOURCE_DIR}/*.s ${PROJECT_SOURCE_DIR}/*.S)

# Set all assembler flags
set_source_files_properties(${PROJECT_SOURCE_DIR}/src/__crt0.S PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
set_source_files_properties(${PROJ_S_FILES} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

add_executable(trainer_loader ${PROJECT_SOURCE_DIR}/src/__crt0.S ${PROJ_S_FILES} ${PROJ_SRC_FILES})

get_target_property(LIBA_INCLUDES trainer_loader INCLUDE_DIRECTORIES)

set(LOADER_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/trainer_loader)
add_custom_command(TARGET trainer_loader POST_BUILD COMMAND objcopy -O binary ${LOADER_OUTPUT_PATH} ${LOADER_OUTPUT_PATH}.bin)
