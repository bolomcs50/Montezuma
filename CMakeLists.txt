cmake_minimum_required(VERSION 3.16.3)

project("Montezuma"
        VERSION 0.1.0
        DESCRIPTION "A simple chess engine"
        LANGUAGES CXX)


add_executable(Montezuma)
target_sources(Montezuma PRIVATE src/thc.cpp src/hashing.cpp src/book.cpp src/engine.cpp src/main.cpp)
include_directories(${PROJECT_SOURCE_DIR}/include)

### Testing
enable_testing()

add_executable(dummy_test)
target_sources(dummy_test PRIVATE test/test.cpp src/engine.cpp src/thc.cpp src/hashing.cpp src/book.cpp)

add_test(NAME example_test COMMAND dummy_test)

# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)