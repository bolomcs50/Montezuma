include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME} thc.cpp hashing.cpp book.cpp engine.cpp main.cpp)
add_executable(sandbox thc.cpp hashing.cpp book.cpp engine.cpp sandbox.cpp)
