set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 3.19.0)

project("Montezuma"
        VERSION 0.1.0
        DESCRIPTION "A simple chess engine"
        HOMEPAGE_URL "https://github.com/bolomcs50/Montezuma"
        LANGUAGES CXX)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Optimize for speed and reduce size in Release mode
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG -march=native")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,--strip-all")

# Use Thin LTO for additional optimizations
include(CheckIPOSupported)
check_ipo_supported(RESULT LTO_SUPPORTED OUTPUT LTO_ERROR)
if(LTO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

add_library(montezumaLib OBJECT)
target_sources(montezumaLib
                PRIVATE src/thc.cpp
                        src/engine.cpp)
target_include_directories(montezumaLib
                PUBLIC ${PROJECT_SOURCE_DIR}/include/thc
                       ${PROJECT_SOURCE_DIR}/include/montezuma)

add_executable(montezuma)
target_sources(montezuma PRIVATE src/main.cpp)
target_link_libraries(montezuma montezumaLib)

# Strip debugging symbols from binaries in Release builds
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(montezuma PRIVATE -s)
endif()

# Use macOS-compatible stripping flags
if (CMAKE_BUILD_TYPE STREQUAL "Release" AND APPLE)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,-dead_strip")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-Wl,-dead_strip")
endif()

### Installing
include(installMontezuma)

### Testing
if (ENABLE_TESTING)
        enable_testing()
        add_subdirectory(test)
endif()
