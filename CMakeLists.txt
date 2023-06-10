cmake_minimum_required(VERSION 3.16.3)

project("Montezuma"
        VERSION 0.1.0
        DESCRIPTION "A simple chess engine"
        LANGUAGES CXX)

add_library(montezuma)
target_sources(montezuma PRIVATE src/thc.cpp src/hashing.cpp src/book.cpp src/engine.cpp)
include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(Montezuma)
target_sources(Montezuma PRIVATE src/main.cpp)
target_link_libraries(Montezuma montezuma)


### Testing
find_package(Catch2 3 REQUIRED)

add_executable(unit_tests)
target_sources(unit_tests PRIVATE test/test.cpp)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain montezuma)

add_executable(mates_tests)
target_sources(mates_tests PRIVATE
        test/solveMates.cpp
        res/MatesIn2.txt
        res/MatesIn3.txt)
target_link_libraries(mates_tests PRIVATE Catch2::Catch2WithMain montezuma)


include(CTest)
include(Catch)
catch_discover_tests(unit_tests)
catch_discover_tests(mates_tests)

# Copy montezuma to engine directory
add_custom_command(
        TARGET Montezuma POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${PROJECT_BINARY_DIR}/Montezuma
                /home/michele/engines/)