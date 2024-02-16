find_package(Catch2 3 REQUIRED)

file(COPY ${CMAKE_SOURCE_DIR}/res DESTINATION ${CMAKE_BINARY_DIR})

add_executable(unit_tests)
target_sources(unit_tests PRIVATE test.cpp)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain montezumaLib)

add_executable(mates_tests)
target_sources(mates_tests PRIVATE
        solveMates.cpp
        ${CMAKE_SOURCE_DIR}/res/MatesIn2.txt
        ${CMAKE_SOURCE_DIR}/res/MatesIn3.txt)
target_link_libraries(mates_tests PRIVATE Catch2::Catch2WithMain montezumaLib)

include(CTest)
include(Catch)
catch_discover_tests(unit_tests)
catch_discover_tests(mates_tests)