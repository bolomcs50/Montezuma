file(COPY ${CMAKE_SOURCE_DIR}/res DESTINATION ${CMAKE_BINARY_DIR})

include(CTest)

add_test(NAME "Engine Operation" COMMAND "test.sh" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test")
add_test(NAME "Mates in 2" COMMAND "solveMates.sh" ${CMAKE_SOURCE_DIR}/res/MatesIn2.txt WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test")
add_test(NAME "Mates in 3" COMMAND "solveMates.sh" ${CMAKE_SOURCE_DIR}/res/MatesIn3.txt WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test")