cmake_minimum_required(VERSION 3.28)
project(RISC_V_Sim)

set(CMAKE_CXX_STANDARD 20)

include_directories(src/include)

add_executable(simulator src/main.cpp)

# Custom unit tests
add_executable(test_buffered unit_tests/test_buffered.cpp)
add_executable(test_logger unit_tests/test_logger.cpp)
add_executable(test_queue unit_tests/test_queue.cpp)
add_executable(test_decoder unit_tests/test_decoder.cpp)
add_executable(test_bus unit_tests/test_bus.cpp)
add_executable(test_channel unit_tests/test_channel.cpp)
