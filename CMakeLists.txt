cmake_minimum_required(VERSION 3.19.0)

project("Montezuma"
        VERSION 0.1.0
        DESCRIPTION "A simple chess engine"
        HOMEPAGE_URL "https://github.com/bolomcs50/Montezuma"
        LANGUAGES CXX)

add_library(montezumaLib OBJECT)
target_sources(montezumaLib
                PRIVATE src/thc.cpp
                        src/hashing.cpp
                        src/book.cpp
                        src/engine.cpp)
target_include_directories(montezumaLib
                PUBLIC ${PROJECT_SOURCE_DIR}/include/thc
                       ${PROJECT_SOURCE_DIR}/include/montezuma )

add_executable(Montezuma)
target_sources(Montezuma PRIVATE src/main.cpp)
target_link_libraries(Montezuma montezumaLib)


### Testing
if (ENABLE_TESTING)
        add_subdirectory(test)
endif()

### Installing