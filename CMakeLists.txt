cmake_minimum_required(VERSION 3.28)
project(RISC_V_Sim)

set(CMAKE_CXX_STANDARD 20)

include_directories(src/include)

add_executable(simulator src/main.cpp)

# Custom unit tests
add_executable(test_buffered unit_tests/test_buffered.cpp)