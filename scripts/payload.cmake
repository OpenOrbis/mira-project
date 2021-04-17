# Freestanding payload
cmake_minimum_required(VERSION 3.12)

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
set(CMAKE_OBJCOPY objcopy)

# Set the C standard to c11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED False)

# Set the C++ standard to c++11
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED False)

set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES "")
set(CMAKE_C_STANDARD_LIBRARIES "")

set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "")
set(CMAKE_CXX_STANDARD_LIBRARIES "")

set(CMAKE_ASM_STANDARD_INCLUDE_DIRECTORIES "")
set(CMAKE_ASM_STANDARD_LIBRARIES "")

set(CMAKE_CXX_LINK_EXECUTABLE "ld <LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
set(CMAKE_C_LINK_EXECUTABLE "ld <LINK_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")

# Set the C/C++ compiler flags
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -m64 -std=c++17 -O0 -fno-builtin -nodefaultlibs -nostdlib -fcheck-new -ffreestanding -fno-strict-aliasing -fno-exceptions -fno-asynchronous-unwind-tables -Wall -Werror -Wno-unknown-pragmas")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -m64 -std=c++17 -O0 -fno-builtin -nodefaultlibs -nostdlib -fcheck-new -ffreestanding -fno-strict-aliasing -fno-exceptions -fno-asynchronous-unwind-tables -Wall -Werror -Wno-unknown-pragmas")

#add_link_options(-fuse-ld=ld.lld)
# Add the default compiler defines
add_compile_definitions(_STANDALONE MIRA_PLATFORM=${MIRA_PLATFORM} MIRA_UNSUPPORTED_PLATFORMS=1 __LP64__ _M_X64 __amd64__ __BSD_VISIBLE)
add_link_options(-pie)
add_link_options(-nostdlib)
add_link_options(-gc-sections)
add_link_options(-nmagic)
#add_link_options(-fuse-ld=lld)

# If we are compiling for debug mode enable the flag
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG=1)
endif()

# Cerate .bin automatically
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}.bin POST_BUILD COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}.bin)