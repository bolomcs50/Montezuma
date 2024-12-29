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

# Optimize for speed and reduce size in Minimum Size Release mode
set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-Wl,--strip-all")

# Use Thin LTO for additional optimizations
include(CheckIPOSupported)
check_ipo_supported(RESULT LTO_SUPPORTED OUTPUT LTO_ERROR)
if(LTO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Use macOS-compatible stripping flags
if (CMAKE_BUILD_TYPE STREQUAL "Release" AND APPLE)
    set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-Wl,-dead_strip")
    set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "-Wl,-dead_strip")
endif()

# Append local modules to the module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_library(montezumaLib OBJECT)
target_sources(montezumaLib
                PRIVATE src/thc.cpp
                        src/hashing.cpp
                        src/book.cpp
                        src/engine.cpp)
target_include_directories(montezumaLib
                PUBLIC ${PROJECT_SOURCE_DIR}/include/thc
                       ${PROJECT_SOURCE_DIR}/include/montezuma)

add_executable(montezuma)
target_sources(montezuma PRIVATE src/main.cpp)
target_link_libraries(montezuma montezumaLib)

### Installing
include(installMontezuma)

### Testing
if (ENABLE_TESTING)
        enable_testing()
        add_subdirectory(test)
endif()