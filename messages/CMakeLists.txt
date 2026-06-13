cmake_minimum_required(VERSION 3.10)
project(messages_tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)

# Fetch GTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(test_received_packet tests/test_received_packet.cpp)
target_link_libraries(test_received_packet GTest::gtest_main GTest::gtest)
target_include_directories(test_received_packet PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

include(GoogleTest)
gtest_discover_tests(test_received_packet)

add_executable(test_packet_to_send tests/test_packet_to_send.cpp)
target_link_libraries(test_packet_to_send GTest::gtest_main GTest::gtest)
target_include_directories(test_packet_to_send PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
gtest_discover_tests(test_packet_to_send)

add_executable(test_buffer_utils tests/test_buffer_utils.cpp)
target_link_libraries(test_buffer_utils GTest::gtest_main GTest::gtest)
target_include_directories(test_buffer_utils PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
gtest_discover_tests(test_buffer_utils)

add_executable(test_start_device tests/test_start_device.cpp)
target_link_libraries(test_start_device GTest::gtest_main GTest::gtest)
target_include_directories(test_start_device PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
gtest_discover_tests(test_start_device)


