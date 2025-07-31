cmake_minimum_required(VERSION 3.28)
project(RISC_V_Sim)

set(CMAKE_CXX_STANDARD 20)

include_directories(src/include)

# Collect all implementation files
file(GLOB IMPL_SOURCES src/impl/*.cpp)

# add_executable(simulator src/main.cpp ${IMPL_SOURCES})
add_executable(code src/main.cpp ${IMPL_SOURCES})
set_target_properties(code PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
add_executable(redirected_sim src/redirected_main.cpp ${IMPL_SOURCES})

# Custom unit tests
#add_executable(test_buffered unit_tests/test_buffered.cpp)
#add_executable(test_logger unit_tests/test_logger.cpp)
#add_executable(test_queue unit_tests/test_queue.cpp)
#add_executable(test_decoder unit_tests/test_decoder.cpp ${IMPL_SOURCES})
#add_executable(test_bus unit_tests/test_bus.cpp)
#add_executable(test_channel unit_tests/test_channel.cpp)
#add_executable(test_vector unit_tests/test_vector.cpp)
#add_executable(test_alu_rs unit_tests/test_alu_rs.cpp ${IMPL_SOURCES})
#add_executable(test_lsb unit_tests/test_lsb.cpp ${IMPL_SOURCES})
