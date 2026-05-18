cmake_minimum_required(VERSION 3.10)
project(utility_tests)

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

add_executable(test_receiving_buffer test_receiving_buffer.cpp)
target_link_libraries(test_receiving_buffer GTest::gtest_main GTest::gtest utility)

include(GoogleTest)
gtest_discover_tests(test_receiving_buffer)
