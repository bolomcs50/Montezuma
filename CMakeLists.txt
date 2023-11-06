cmake_minimum_required(VERSION 3.19.0)

project("Montezuma"
        VERSION 0.1.0
        DESCRIPTION "A simple chess engine"
        HOMEPAGE_URL "https://github.com/bolomcs50/Montezuma"
        LANGUAGES CXX)

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
                       ${PROJECT_SOURCE_DIR}/include/montezuma )

add_executable(montezuma)
target_sources(montezuma PRIVATE src/main.cpp)
target_link_libraries(montezuma montezumaLib)

### Installing
include(installMontezuma)

### Testing
if (ENABLE_TESTING)
        add_subdirectory(test)
endif()