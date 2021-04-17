# Kernel elf defaults
cmake_minimum_required(VERSION 3.12)

if (NOT "" STREQUAL ${MIRA_PLATFORM})
  set(MIRA_PLATFORM ${MIRA_PLATFORM})
else()
  message(FATAL_ERROR "MIRA_PLATFORM not set.")
endif()

# Credits: znullptr
set(CMAKE_SYSTEM_NAME FreeBSD)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_SYSTEM_VERSION 9)

#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER) # search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)  # for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Set all of the default CFLAGS
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_AR llvm-ar)

# Set the C standard to c11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Set the C++ standard to c++11
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES "")
set(CMAKE_C_STANDARD_LIBRARIES "")

set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "")
set(CMAKE_CXX_STANDARD_LIBRARIES "")

set(CMAKE_ASM_STANDARD_INCLUDE_DIRECTORIES "")
set(CMAKE_ASM_STANDARD_LIBRARIES "")

set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")

set(CMAKE_CXX_LINK_EXECUTABLE "ld <LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
set(CMAKE_C_LINK_EXECUTABLE "ld <LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

add_link_options(-nostdlib)
add_link_options(-pie)

add_compile_options(-fno-rtti)

# Set the C/C++ compiler flags
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -m64 -O0 -fno-builtin -nodefaultlibs -nostdlib -fcheck-new -ffreestanding -fno-strict-aliasing -fno-exceptions -fno-asynchronous-unwind-tables -Wall -Werror -Wno-unknown-pragmas")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic -m64 -O0 -fno-builtin -nodefaultlibs -nostdlib -fcheck-new -ffreestanding -fno-strict-aliasing -fno-exceptions -fno-asynchronous-unwind-tables -Wall -Werror -Wno-unknown-pragmas")

#add_link_options(-fuse-ld=ld.lld)
# Add the default compiler defines
add_compile_definitions(_KERNEL PS4=1 __BSD_VISIBLE=1 _BSD_SOURCE=1 _STANDALONE=1 MIRA_PLATFORM=${MIRA_PLATFORM} MIRA_UNSUPPORTED_PLATFORMS=1 __LP64__ _M_X64 __amd64__)

# If we are compiling for debug mode enable the flag
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG=1)
endif()
