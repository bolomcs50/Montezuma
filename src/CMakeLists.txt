include_directories(${PROJECT_SOURCE_DIR}/include)
#add_executable(Demo thc.cpp demo.cpp)
add_executable(${PROJECT_NAME} thc.cpp engine.cpp main.cpp)
add_executable(Tester thc.cpp engine.cpp tester.cpp)
add_executable(ResReader resultReader.cpp)