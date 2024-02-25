find_package(Catch2 3 REQUIRED)

file(COPY ${CMAKE_SOURCE_DIR}/res DESTINATION ${CMAKE_BINARY_DIR})

add_executable(unit_tests)
target_sources(unit_tests PRIVATE test.cpp)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain montezumaLib)

include(CTest)
include(Catch)
catch_discover_tests(unit_tests)
add_test(NAME "Mates in 2" COMMAND "solveMates.sh" ${CMAKE_SOURCE_DIR}/res/MatesIn2.txt WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test")
add_test(NAME "Mates in 3" COMMAND "solveMates.sh" ${CMAKE_SOURCE_DIR}/res/MatesIn3.txt WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test")